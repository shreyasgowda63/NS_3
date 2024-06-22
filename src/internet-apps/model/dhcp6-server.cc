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
                            .AddAttribute("LeaseTime",
                                          "Preferred lifetime for which address will be leased.",
                                          TimeValue(Seconds(30)),
                                          MakeTimeAccessor(&Dhcp6Server::m_prefLifetime),
                                          MakeTimeChecker())
                            .AddAttribute("RenewTime",
                                          "Time after which client should renew.",
                                          TimeValue(Seconds(10)),
                                          MakeTimeAccessor(&Dhcp6Server::m_renew),
                                          MakeTimeChecker())
                            .AddAttribute("RebindTime",
                                          "Time after which client should rebind.",
                                          TimeValue(Seconds(20)),
                                          MakeTimeAccessor(&Dhcp6Server::m_rebind),
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
    auto linkLayer = iDev->GetAddress();
    advertiseHeader.AddServerIdentifier(1234, linkLayer);

    // Add IA_NA option.
    // Available address pools and IA information is sent in this option.
    for (auto itr = m_subnets.begin(); itr != m_subnets.end(); itr++)
    {
        LeaseInfo subnet = *itr;
        Ipv6Address pool = subnet.GetAddressPool();
        Ipv6Prefix prefix = subnet.GetPrefix();
        Ipv6Address minAddress = subnet.GetMinAddress();
        Ipv6Address maxAddress = subnet.GetMaxAddress();
        uint32_t numAddresses = subnet.GetNumAddresses();

        // Obtain the address to be included in the message.
        // TODO: Check declined addresses, expired addresses and then leased
        // addresses to find the next available address.

        // Allocate a new address.
        uint8_t minAddrBuf[16];
        minAddress.GetBytes(minAddrBuf);

        uint8_t offeredAddrBuf[16];
        uint8_t addition[16];
        // convert m_numAddresses to byte array.
        // offeredAddrBuf = minAddressBuf | addition

        uint8_t byteCount = 0;
        for (uint8_t bits = 0; bits <= 120; bits += 8)
        {
            addition[byteCount] = (numAddresses << bits) & 0xff;
            byteCount += 1;
        }

        for (uint8_t i = 0; i < 16; i++)
        {
            offeredAddrBuf[i] = minAddrBuf[i] | addition[i];
        }

        Ipv6Address offeredAddr(offeredAddrBuf);
        numAddresses += 1;

        // TODO: Retrieve all existing IA_NA options.
        advertiseHeader.AddIanaOption(m_iaidCount, m_renew.GetSeconds(), m_rebind.GetSeconds());
        advertiseHeader.AddAddress(m_iaidCount,
                                   offeredAddr,
                                   m_prefLifetime.GetSeconds(),
                                   m_validLifetime.GetSeconds());
    }

    packet->AddHeader(advertiseHeader);
    // TODO: Add OPTION_REQUEST Option.
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
    auto linkLayer = iDev->GetAddress();
    replyHeader.AddServerIdentifier(1, linkLayer);

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
        // uint32_t numAddresses = subnet.GetNumAddresses();

        Ipv6Address requestedAddr = iaAddrOpt.GetIaAddress();
        // TODO: Check that requestedAddr is within [min, max] range.
        // TODO: Check that requestedAddr is not in declinedAddresses.

        // TODO: Retrieve all existing IA_NA options.
        replyHeader.AddIanaOption(iaOpt.GetIaid(), iaOpt.GetT1(), iaOpt.GetT2());
        replyHeader.AddAddress(iaOpt.GetIaid(),
                               iaAddrOpt.GetIaAddress(),
                               iaAddrOpt.GetPreferredLifetime(),
                               iaAddrOpt.GetValidLifetime());
    }

    packet->AddHeader(replyHeader);
    // TODO: Add OPTION_REQUEST Option.
    // Send the advertise message.
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
        SendAdvertise(iDev, header, senderAddr);
    }
    if (header.GetMessageType() == Dhcp6Header::REQUEST)
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
