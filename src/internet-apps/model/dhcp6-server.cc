/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "dhcp6-server.h"

#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/ipv6-packet-info-tag.h"
#include "ns3/ipv6.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Dhcp6Server");

TypeId
Dhcp6Server::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Dhcp6Server")
                            .SetParent<Application>()
                            .AddConstructor<Dhcp6Server>()
                            .SetGroupName("Internet-Apps")
                            .AddAttribute("RenewTime",
                                          "Time after which client should renew.",
                                          TimeValue(Seconds(10)),
                                          MakeTimeAccessor(&Dhcp6Server::m_renew),
                                          MakeTimeChecker())
                            .AddAttribute("RebindTime",
                                          "Time after which client should rebind.",
                                          TimeValue(Seconds(16)),
                                          MakeTimeAccessor(&Dhcp6Server::m_rebind),
                                          MakeTimeChecker())
                            .AddAttribute("PreferredLifetime",
                                          "The preferred lifetime of the leased address.",
                                          TimeValue(Seconds(18)),
                                          MakeTimeAccessor(&Dhcp6Server::m_prefLifetime),
                                          MakeTimeChecker())
                            .AddAttribute("ValidLifetime",
                                          "Time after which client should release the address.",
                                          TimeValue(Seconds(20)),
                                          MakeTimeAccessor(&Dhcp6Server::m_validLifetime),
                                          MakeTimeChecker());

    return tid;
}

Dhcp6Server::Dhcp6Server()
{
    NS_LOG_FUNCTION(this);
}

void
Dhcp6Server::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Application::DoDispose();
}

void
Dhcp6Server::ProcessSolicit(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress client)
{
    NS_LOG_INFO(this << iDev << header << client);

    IdentifierOption clientId = header.GetClientIdentifier();
    Address duid = clientId.GetLinkLayerAddress();

    std::vector<bool> headerOptions = header.GetOptionList();

    if (headerOptions[Dhcp6Header::OPTION_IA_NA])
    {
        uint32_t iaid = header.GetIanaOptions().front().GetIaid();
        m_iaBindings[duid] = std::make_pair(Dhcp6Header::OPTION_IA_NA, iaid);
    }
}

void
Dhcp6Server::SendAdvertise(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress client)
{
    NS_LOG_INFO(this << iDev << header << client);

    Ptr<Packet> packet = Create<Packet>();
    Dhcp6Header advertiseHeader;
    advertiseHeader.ResetOptions();
    advertiseHeader.SetMessageType(Dhcp6Header::ADVERTISE);
    advertiseHeader.SetTransactId(header.GetTransactId());

    // Add Client Identifier Option, copied from the received header.
    uint16_t clientHardwareType = header.GetClientIdentifier().GetHardwareType();
    Address clientAddress = header.GetClientIdentifier().GetLinkLayerAddress();
    advertiseHeader.AddClientIdentifier(clientHardwareType, clientAddress);

    // Add Server Identifier Option.
    advertiseHeader.AddServerIdentifier(m_serverIdentifier.GetHardwareType(),
                                        m_serverIdentifier.GetLinkLayerAddress());

    // Add IA_NA option.
    // Available address pools and IA information is sent in this option.
    uint32_t iaid = m_iaBindings[clientAddress].second;
    for (auto itr = m_subnets.begin(); itr != m_subnets.end(); itr++)
    {
        LeaseInfo subnet = *itr;
        Ipv6Address pool = subnet.GetAddressPool();
        Ipv6Prefix prefix = subnet.GetPrefix();
        Ipv6Address minAddress = subnet.GetMinAddress();
        Ipv6Address maxAddress = subnet.GetMaxAddress();

        /*
         * Find the next available address. Checks the expired address map.
         * If there are no expired addresses, it advertises a new address.
         */
        if (!subnet.m_expiredAddresses.empty())
        {
            auto itr = subnet.m_expiredAddresses.begin();
            Ipv6Address nextAddress = itr->second;
            subnet.m_expiredAddresses.erase(itr->first);

            advertiseHeader.AddIanaOption(iaid, m_renew.GetSeconds(), m_rebind.GetSeconds());
            advertiseHeader.AddAddress(iaid,
                                       nextAddress,
                                       m_prefLifetime.GetSeconds(),
                                       m_validLifetime.GetSeconds());
            continue;
        }
        else
        {
            // Allocate a new address.
            uint8_t minAddrBuf[16];
            minAddress.GetBytes(minAddrBuf);

            // Get the latest leased address.
            uint8_t lastLeasedAddrBuf[16];
            uint8_t offeredAddrBuf[16];
            if (!subnet.m_leasedAddresses.empty())
            {
                auto itr = subnet.m_leasedAddresses.rbegin();
                Ipv6Address lastLeasedAddress = (itr->second).first;

                lastLeasedAddress.GetBytes(lastLeasedAddrBuf);
                memcpy(offeredAddrBuf, lastLeasedAddrBuf, 16);

                bool addedOne = false;
                for (uint8_t i = 15; !addedOne && i >= 0; i--)
                {
                    for (int j = 0; j < 8; j++)
                    {
                        uint8_t bit = (offeredAddrBuf[i] << j);
                        if (bit == 0)
                        {
                            offeredAddrBuf[i] = offeredAddrBuf[i] | (1 << j);
                            addedOne = true;
                            break;
                        }
                        offeredAddrBuf[i] = offeredAddrBuf[i] & ~(1 << j);
                    }
                }
            }
            else
            {
                memcpy(offeredAddrBuf, minAddrBuf, 16);
            }

            Ipv6Address offeredAddr(offeredAddrBuf);
            NS_LOG_INFO("Offered address: " << offeredAddr);

            // TODO: Retrieve all existing IA_NA options.
            advertiseHeader.AddIanaOption(iaid, m_renew.GetSeconds(), m_rebind.GetSeconds());
            advertiseHeader.AddAddress(iaid,
                                       offeredAddr,
                                       m_prefLifetime.GetSeconds(),
                                       m_validLifetime.GetSeconds());
        }
    }

    std::vector<bool> headerOptions = header.GetOptionList();
    if (headerOptions[Dhcp6Header::OPTION_ORO])
    {
        std::list<uint16_t> requestedOptions = header.GetOptionRequest().GetRequestedOptions();
        advertiseHeader.HandleOptionRequest(requestedOptions);
    }

    packet->AddHeader(advertiseHeader);

    // Send the advertise message.
    if (m_sendSocket->SendTo(packet, 0, client) >= 0)
    {
        NS_LOG_INFO("DHCPv6 Advertise sent.");
    }
    else
    {
        NS_LOG_INFO("Error while sending DHCPv6 Advertise.");
    }
}

void
Dhcp6Server::SendReply(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress client)
{
    NS_LOG_INFO(this << iDev << header << client);

    Ptr<Packet> packet = Create<Packet>();
    Dhcp6Header replyHeader;
    replyHeader.ResetOptions();
    replyHeader.SetMessageType(Dhcp6Header::REPLY);
    replyHeader.SetTransactId(header.GetTransactId());

    // Add Client Identifier Option, copied from the received header.
    uint16_t clientHardwareType = header.GetClientIdentifier().GetHardwareType();
    Address clientAddress = header.GetClientIdentifier().GetLinkLayerAddress();
    replyHeader.AddClientIdentifier(clientHardwareType, clientAddress);

    // Add Server Identifier Option.
    replyHeader.AddServerIdentifier(m_serverIdentifier.GetHardwareType(),
                                    m_serverIdentifier.GetLinkLayerAddress());

    // Add IA_NA option.
    // Retrieve requested IA Option from client header.
    std::list<IaOptions> ianaOptionsList = header.GetIanaOptions();
    IaOptions iaOpt = ianaOptionsList.front();
    IaAddressOption iaAddrOpt = iaOpt.m_iaAddressOption.front();

    for (auto itr = m_subnets.begin(); itr != m_subnets.end(); itr++)
    {
        LeaseInfo subnet = *itr;
        Ipv6Address pool = subnet.GetAddressPool();
        Ipv6Prefix prefix = subnet.GetPrefix();
        Ipv6Address minAddress = subnet.GetMinAddress();
        Ipv6Address maxAddress = subnet.GetMaxAddress();

        Ipv6Address requestedAddr = iaAddrOpt.GetIaAddress();

        if (subnet.m_declinedAddresses.find(requestedAddr) != subnet.m_declinedAddresses.end())
        {
            NS_LOG_INFO("Requested address is declined.");
            return;
        }

        // Check whether this subnet matches the requested address.
        if (prefix.IsMatch(requestedAddr, pool))
        {
            uint8_t minBuf[16];
            uint8_t maxBuf[16];
            uint8_t requestedBuf[16];
            minAddress.GetBytes(minBuf);
            maxAddress.GetBytes(maxBuf);
            requestedAddr.GetBytes(requestedBuf);

            if (memcmp(requestedBuf, minBuf, 16) < 0 || memcmp(requestedBuf, maxBuf, 16) > 0)
            {
                NS_LOG_INFO("Requested address is not in the range of the subnet.");
                return;
            }

            // TODO: Retrieve all existing IA_NA options.
            replyHeader.AddIanaOption(iaOpt.GetIaid(), iaOpt.GetT1(), iaOpt.GetT2());
            replyHeader.AddAddress(iaOpt.GetIaid(),
                                   iaAddrOpt.GetIaAddress(),
                                   iaAddrOpt.GetPreferredLifetime(),
                                   iaAddrOpt.GetValidLifetime());

            subnet.m_leasedAddresses[clientAddress] =
                std::make_pair(requestedAddr, m_prefLifetime.GetSeconds());
            break;
        }
    }

    std::vector<bool> headerOptions = header.GetOptionList();
    if (headerOptions[Dhcp6Header::OPTION_ORO])
    {
        std::list<uint16_t> requestedOptions = header.GetOptionRequest().GetRequestedOptions();
        replyHeader.HandleOptionRequest(requestedOptions);
    }

    packet->AddHeader(replyHeader);

    // Send the Reply message.
    if (m_sendSocket->SendTo(packet, 0, client) >= 0)
    {
        NS_LOG_INFO("DHCPv6 Reply sent.");
    }
    else
    {
        NS_LOG_INFO("Error while sending DHCPv6 Reply.");
    }
}

void
Dhcp6Server::SetDhcp6ServerNetDevice(Ptr<NetDevice> netDevice)
{
    m_device = netDevice;
}

void
Dhcp6Server::NetHandler(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    Dhcp6Header header;
    Ptr<Packet> packet = nullptr;
    Address from;
    packet = m_recvSocket->RecvFrom(from);

    Inet6SocketAddress senderAddr = Inet6SocketAddress::ConvertFrom(from);

    Ipv6PacketInfoTag interfaceInfo;
    if (!packet->RemovePacketTag(interfaceInfo))
    {
        NS_ABORT_MSG("No incoming interface on DHCPv6 message, aborting.");
    }

    uint32_t incomingIf = interfaceInfo.GetRecvIf();
    Ptr<NetDevice> iDev = GetNode()->GetDevice(incomingIf);

    if (packet->RemoveHeader(header) == 0)
    {
        return;
    }
    if (header.GetMessageType() == Dhcp6Header::SOLICIT)
    {
        NS_LOG_INFO("Received Solicit");
        ProcessSolicit(iDev, header, senderAddr);
        SendAdvertise(iDev, header, senderAddr);
    }
    if ((header.GetMessageType() == Dhcp6Header::REQUEST) ||
        (header.GetMessageType() == Dhcp6Header::RENEW))
    {
        SendReply(iDev, header, senderAddr);
    }
}

void
Dhcp6Server::AddSubnet(Ipv6Address addressPool,
                       Ipv6Prefix prefix,
                       Ipv6Address minAddress,
                       Ipv6Address maxAddress)
{
    NS_LOG_FUNCTION(this << addressPool << prefix << minAddress << maxAddress);
    LeaseInfo newSubnet(addressPool, prefix, minAddress, maxAddress);
    m_subnets.push_back(newSubnet);
}

void
Dhcp6Server::StartApplication()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Starting DHCPv6 server.");
    // TODO: Check that address ranges are valid.
    // TODO: Set all address pools and IA_NA / IA_TA option information.

    if (m_recvSocket && m_sendSocket)
    {
        NS_ABORT_MSG("DHCPv6 daemon is not meant to be started repeatedly.");
    }

    Ptr<Node> node = m_device->GetNode();
    Ptr<Ipv6> ipv6 = node->GetObject<Ipv6>();
    uint32_t ifIndex = ipv6->GetInterfaceForDevice(m_device);

    if (ifIndex < 0)
    {
        NS_ABORT_MSG("DHCPv6 daemon must have a link-local address.");
    }

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_recvSocket = Socket::CreateSocket(node, tid);
    m_sendSocket = Socket::CreateSocket(node, tid);

    Inet6SocketAddress local =
        Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(), Dhcp6Server::PORT);
    m_recvSocket->Bind(local);
    m_recvSocket->SetRecvPktInfo(true);
    m_recvSocket->SetRecvCallback(MakeCallback(&Dhcp6Server::NetHandler, this));

    Ipv6Address linkLocal;
    for (uint32_t addrIndex = 0; addrIndex < ipv6->GetNAddresses(ifIndex); addrIndex++)
    {
        Ipv6InterfaceAddress ifaceAddr = ipv6->GetAddress(ifIndex, addrIndex);
        Ipv6Address addr = ifaceAddr.GetAddress();
        if (addr.IsLinkLocal())
        {
            linkLocal = addr;
            break;
        }
    }

    m_serverIdentifier.SetHardwareType(1);
    m_serverIdentifier.SetLinkLayerAddress(m_device->GetAddress());

    m_sendSocket->Bind(Inet6SocketAddress(linkLocal, Dhcp6Server::PORT));
    m_sendSocket->BindToNetDevice(m_device);
}

void
Dhcp6Server::StopApplication()
{
    NS_LOG_FUNCTION(this);

    if (m_recvSocket)
    {
        m_recvSocket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }

    m_leasedAddresses.clear();
    m_declinedAddresses.clear();
}

LeaseInfo::LeaseInfo(Ipv6Address addressPool,
                     Ipv6Prefix prefix,
                     Ipv6Address minAddress,
                     Ipv6Address maxAddress)
{
    m_addressPool = addressPool;
    m_prefix = prefix;
    m_minAddress = minAddress;
    m_maxAddress = maxAddress;
    m_numAddresses = 0;
}

Ipv6Address
LeaseInfo::GetAddressPool()
{
    NS_LOG_FUNCTION(this);
    return m_addressPool;
}

Ipv6Prefix
LeaseInfo::GetPrefix()
{
    NS_LOG_FUNCTION(this);
    return m_prefix;
}

Ipv6Address
LeaseInfo::GetMinAddress()
{
    NS_LOG_FUNCTION(this);
    return m_minAddress;
}

Ipv6Address
LeaseInfo::GetMaxAddress()
{
    NS_LOG_FUNCTION(this);
    return m_maxAddress;
}

uint32_t
LeaseInfo::GetNumAddresses()
{
    NS_LOG_FUNCTION(this);
    return m_numAddresses;
}

} // namespace ns3
