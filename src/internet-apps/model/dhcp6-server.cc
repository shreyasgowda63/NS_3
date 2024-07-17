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
#include "ns3/loopback-net-device.h"
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
                                          "Time after which client should renew. 1000 seconds by "
                                          "default, set to 10 seconds here.",
                                          TimeValue(Seconds(10)),
                                          MakeTimeAccessor(&Dhcp6Server::m_renew),
                                          MakeTimeChecker())
                            .AddAttribute("RebindTime",
                                          "Time after which client should rebind. 2000 seconds by "
                                          "default, set to 16 seconds here.",
                                          TimeValue(Seconds(16)),
                                          MakeTimeAccessor(&Dhcp6Server::m_rebind),
                                          MakeTimeChecker())
                            .AddAttribute("PreferredLifetime",
                                          "The preferred lifetime of the leased address. 3000 "
                                          "seconds by default, set to 18 seconds here.",
                                          TimeValue(Seconds(18)),
                                          MakeTimeAccessor(&Dhcp6Server::m_prefLifetime),
                                          MakeTimeChecker())
                            .AddAttribute("ValidLifetime",
                                          "Time after which client should release the address. "
                                          "4000 seconds by default, set to 20 seconds here.",
                                          TimeValue(Seconds(20)),
                                          MakeTimeAccessor(&Dhcp6Server::m_validLifetime),
                                          MakeTimeChecker());

    return tid;
}

Dhcp6Server::Dhcp6Server()
{
    NS_LOG_FUNCTION(this);
    m_leaseCleanup = Time(Seconds(10.0));
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
        std::list<IaOptions> iaOpt = header.GetIanaOptions();
        for (auto itr = iaOpt.begin(); itr != iaOpt.end(); itr++)
        {
            uint32_t iaid = itr->GetIaid();
            m_iaBindings.insert({duid, std::make_pair(Dhcp6Header::OPTION_IA_NA, iaid)});
        }
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

    // Find all IAIDs for this client.
    auto range = m_iaBindings.equal_range(clientAddress);

    // Add IA_NA option.
    // Available address pools and IA information is sent in this option.
    for (auto subnet = m_subnets.begin(); subnet != m_subnets.end(); subnet++)
    {
        Ipv6Address pool = subnet->GetAddressPool();
        Ipv6Prefix prefix = subnet->GetPrefix();
        Ipv6Address minAddress = subnet->GetMinAddress();
        Ipv6Address maxAddress = subnet->GetMaxAddress();

        /*
         * Find the next available address. Checks the expired address map.
         * If there are no expired addresses, it advertises a new address.
         */

        uint8_t offeredAddrBuf[16];
        if (!subnet->m_expiredAddresses.empty())
        {
            auto firstExpiredAddress = subnet->m_expiredAddresses.begin();
            Ipv6Address nextAddress = firstExpiredAddress->second;
            subnet->m_expiredAddresses.erase(firstExpiredAddress);

            nextAddress.GetBytes(offeredAddrBuf);
        }
        else
        {
            // Allocate a new address.
            uint8_t minAddrBuf[16];
            minAddress.GetBytes(minAddrBuf);

            // Get the latest leased address.
            uint8_t lastLeasedAddrBuf[16];

            if (!subnet->m_leasedAddresses.empty())
            {
                subnet->m_maxOfferedAddress.GetBytes(lastLeasedAddrBuf);
                memcpy(offeredAddrBuf, lastLeasedAddrBuf, 16);

                bool addedOne = false;
                for (uint8_t i = 15; !addedOne && i >= 0; i--)
                {
                    for (int j = 0; j < 8; j++)
                    {
                        uint8_t bit = (offeredAddrBuf[i] & (1 << j)) >> j;
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

            Ipv6Address offer(offeredAddrBuf);
            subnet->m_maxOfferedAddress = offer;

            /*
            Optimistic assumption that the address will be leased to this client.
            This is to prevent multiple clients from receiving the same address.
            */
            subnet->m_leasedAddresses.insert(
                {clientAddress, std::make_pair(offer, Time(Seconds(m_prefLifetime.GetSeconds())))});
        }

        Ipv6Address offeredAddr(offeredAddrBuf);
        NS_LOG_INFO("Offered address: " << offeredAddr);

        // Add the IA option for each IAID listed in the header.
        for (auto iaBinding = range.first; iaBinding != range.second; iaBinding++)
        {
            // Pair: IA type + IAID.
            std::pair<uint8_t, uint32_t> iaInfo = iaBinding->second;
            uint32_t iaid = iaInfo.second;
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
    Ptr<Socket> sendSocket = m_sendSockets[iDev];
    if (sendSocket->SendTo(packet, 0, client) >= 0)
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

    for (auto ianaOpt = ianaOptionsList.begin(); ianaOpt != ianaOptionsList.end(); ianaOpt++)
    {
        IaOptions iaOpt = *ianaOpt;

        // Iterate through the offered addresses.
        // Current approach: Try to accept all offers.
        std::list<IaAddressOption> iaAddrOptList = iaOpt.m_iaAddressOption;
        for (auto addrItr = iaAddrOptList.begin(); addrItr != iaAddrOptList.end(); addrItr++)
        {
            Ipv6Address requestedAddr = addrItr->GetIaAddress();

            for (auto subnet = m_subnets.begin(); subnet != m_subnets.end(); subnet++)
            {
                Ipv6Address pool = subnet->GetAddressPool();
                Ipv6Prefix prefix = subnet->GetPrefix();
                Ipv6Address minAddress = subnet->GetMinAddress();
                Ipv6Address maxAddress = subnet->GetMaxAddress();

                if (subnet->m_declinedAddresses.find(requestedAddr) !=
                    subnet->m_declinedAddresses.end())
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

                    if (memcmp(requestedBuf, minBuf, 16) < 0 ||
                        memcmp(requestedBuf, maxBuf, 16) > 0)
                    {
                        NS_LOG_INFO("Requested address is not in the range of the subnet.");
                        return;
                    }

                    replyHeader.AddIanaOption(iaOpt.GetIaid(), iaOpt.GetT1(), iaOpt.GetT2());
                    replyHeader.AddAddress(iaOpt.GetIaid(),
                                           requestedAddr,
                                           m_prefLifetime.GetSeconds(),
                                           m_validLifetime.GetSeconds());

                    auto range = subnet->m_leasedAddresses.equal_range(clientAddress);
                    std::multimap<Address, std::pair<Ipv6Address, Time>> updatedLifetimes;
                    for (auto it = range.first; it != range.second; it++)
                    {
                        Ipv6Address clientLease = it->second.first;
                        std::pair<Ipv6Address, Time> clientLeaseTime = {
                            clientLease,
                            Time(Seconds(m_prefLifetime.GetSeconds()))};
                        updatedLifetimes.insert({clientAddress, clientLeaseTime});
                    }

                    subnet->m_leasedAddresses.erase(range.first->first);

                    for (auto itr = updatedLifetimes.begin(); itr != updatedLifetimes.end(); itr++)
                    {
                        subnet->m_leasedAddresses.insert({itr->first, itr->second});
                    }
                    break;
                }
            }
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
    Ptr<Socket> sendSocket = m_sendSockets[iDev];
    if (sendSocket->SendTo(packet, 0, client) >= 0)
    {
        NS_LOG_INFO("DHCPv6 Reply sent.");
    }
    else
    {
        NS_LOG_INFO("Error while sending DHCPv6 Reply.");
    }
}

void
Dhcp6Server::RenewRebindLeases(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress client)
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
    // Retrieve IA_NAs from client header.
    std::list<IaOptions> ianaOptionsList = header.GetIanaOptions();
    for (auto ianaOpt = ianaOptionsList.begin(); ianaOpt != ianaOptionsList.end(); ianaOpt++)
    {
        IaOptions iaOpt = *ianaOpt;
        std::list<IaAddressOption> iaAddrOptList = iaOpt.m_iaAddressOption;

        replyHeader.AddIanaOption(iaOpt.GetIaid(), iaOpt.GetT1(), iaOpt.GetT2());

        for (auto addrItr = iaAddrOptList.begin(); addrItr != iaAddrOptList.end(); addrItr++)
        {
            Ipv6Address clientLease = addrItr->GetIaAddress();

            replyHeader.AddAddress(iaOpt.GetIaid(),
                                   clientLease,
                                   m_prefLifetime.GetSeconds(),
                                   m_validLifetime.GetSeconds());

            for (auto subnet = m_subnets.begin(); subnet != m_subnets.end(); subnet++)
            {
                Ipv6Prefix prefix = subnet->GetPrefix();
                Ipv6Address pool = subnet->GetAddressPool();

                if (prefix.IsMatch(clientLease, pool))
                {
                    auto range = subnet->m_leasedAddresses.equal_range(clientAddress);
                    std::multimap<Address, std::pair<Ipv6Address, Time>> newLifetimes;
                    for (auto itr = range.first; itr != range.second; itr++)
                    {
                        if (itr->second.first == clientLease)
                        {
                            NS_LOG_INFO("Renewing address: " << itr->second.first);
                            std::pair<Ipv6Address, Time> clientLeaseTime = {
                                clientLease,
                                Time(Seconds(m_prefLifetime.GetSeconds()))};
                            subnet->m_leasedAddresses.erase(itr);
                            subnet->m_leasedAddresses.insert({clientAddress, clientLeaseTime});
                            replyHeader.AddAddress(iaOpt.GetIaid(),
                                                   clientLease,
                                                   m_prefLifetime.GetSeconds(),
                                                   m_validLifetime.GetSeconds());
                            break;
                        }
                    }
                }
            }
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
    Ptr<Socket> sendSocket = m_sendSockets[iDev];
    if (sendSocket->SendTo(packet, 0, client) >= 0)
    {
        NS_LOG_INFO("DHCPv6 Reply sent.");
    }
    else
    {
        NS_LOG_INFO("Error while sending DHCPv6 Reply.");
    }
}

void
Dhcp6Server::UpdateBindings(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress client)
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

    // Add Status code option.
    replyHeader.AddStatusCode(Dhcp6Header::Success, "Address declined.");

    // Add the declined or expired address to the subnet information.
    std::list<IaOptions> ianaOptionsList = header.GetIanaOptions();
    for (auto ianaOpt = ianaOptionsList.begin(); ianaOpt != ianaOptionsList.end(); ianaOpt++)
    {
        IaOptions iaOpt = *ianaOpt;
        std::list<IaAddressOption> iaAddrOptList = iaOpt.m_iaAddressOption;

        for (auto addrItr = iaAddrOptList.begin(); addrItr != iaAddrOptList.end(); addrItr++)
        {
            Ipv6Address address = addrItr->GetIaAddress();
            if (header.GetMessageType() == Dhcp6Header::DECLINE)
            {
                // Find the subnet that this address belongs to.
                for (auto subnet = m_subnets.begin(); subnet != m_subnets.end(); subnet++)
                {
                    // Find the client that the address currently belongs to.
                    auto range = subnet->m_leasedAddresses.equal_range(clientAddress);
                    std::vector<Ipv6Address> declinedAddrs;
                    for (auto itr = range.first; itr != range.second; itr++)
                    {
                        Ipv6Address leaseAddr = itr->second.first;
                        if (leaseAddr == address)
                        {
                            declinedAddrs.push_back(leaseAddr);
                            subnet->m_declinedAddresses[address] = clientAddress;
                        }
                    }

                    for (auto itr = declinedAddrs.begin(); itr != declinedAddrs.end(); itr++)
                    {
                        // Remove declined address from the leased address map.
                        for (auto lease = subnet->m_leasedAddresses.begin();
                             lease != subnet->m_leasedAddresses.end();
                             lease++)
                        {
                            Ipv6Address address = lease->second.first;

                            if (address == *itr)
                            {
                                subnet->m_leasedAddresses.erase(lease);
                                break;
                            }
                        }
                    }
                }
            }
            else if (header.GetMessageType() == Dhcp6Header::RELEASE)
            {
                // Find the subnet that this address belongs to.
                for (auto subnet = m_subnets.begin(); subnet != m_subnets.end(); subnet++)
                {
                    // Find the client that the address currently belongs to.
                    auto range = subnet->m_leasedAddresses.equal_range(clientAddress);
                    std::vector<Ipv6Address> expiredAddrs;
                    for (auto itr = range.first; itr != range.second; itr++)
                    {
                        Ipv6Address leaseAddr = itr->second.first;
                        Time expiredTime = itr->second.second;
                        if (leaseAddr == address)
                        {
                            expiredAddrs.push_back(leaseAddr);
                            subnet->m_expiredAddresses.insert({expiredTime, leaseAddr});
                        }
                    }

                    for (auto itr = expiredAddrs.begin(); itr != expiredAddrs.end(); itr++)
                    {
                        // Remove expired address from the leased address map.
                        for (auto lease = subnet->m_leasedAddresses.begin();
                             lease != subnet->m_leasedAddresses.end();
                             lease++)
                        {
                            Ipv6Address address = lease->second.first;
                            if (address == *itr)
                            {
                                subnet->m_leasedAddresses.erase(lease);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    packet->AddHeader(replyHeader);

    // Send the Reply message.
    Ptr<Socket> sendSocket = m_sendSockets[iDev];
    if (sendSocket->SendTo(packet, 0, client) >= 0)
    {
        NS_LOG_INFO("DHCPv6 Reply sent.");
    }
    else
    {
        NS_LOG_INFO("Error while sending DHCPv6 Reply.");
    }
}

void
Dhcp6Server::SetDhcp6ServerNetDevice(std::vector<Ptr<NetDevice>> netDevices)
{
    m_devices = netDevices;
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
    if (header.GetMessageType() == Dhcp6Header::REQUEST)
    {
        SendReply(iDev, header, senderAddr);
    }
    if ((header.GetMessageType() == Dhcp6Header::RENEW) ||
        (header.GetMessageType() == Dhcp6Header::REBIND))
    {
        RenewRebindLeases(iDev, header, senderAddr);
    }
    if ((header.GetMessageType() == Dhcp6Header::RELEASE) ||
        (header.GetMessageType() == Dhcp6Header::DECLINE))
    {
        UpdateBindings(iDev, header, senderAddr);
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

    if (m_recvSocket)
    {
        NS_ABORT_MSG("DHCPv6 daemon is not meant to be started repeatedly.");
    }

    Ptr<Node> node = m_devices[0]->GetNode();
    Ptr<Ipv6> ipv6 = node->GetObject<Ipv6>();

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_recvSocket = Socket::CreateSocket(node, tid);

    Inet6SocketAddress local =
        Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(), Dhcp6Server::PORT);
    m_recvSocket->Bind(local);
    m_recvSocket->SetRecvPktInfo(true);
    m_recvSocket->SetRecvCallback(MakeCallback(&Dhcp6Server::NetHandler, this));

    for (auto itr = m_devices.begin(); itr != m_devices.end(); itr++)
    {
        Ptr<NetDevice> device = *itr;
        uint32_t ifIndex = ipv6->GetInterfaceForDevice(device);

        if (ifIndex < 0)
        {
            NS_ABORT_MSG("DHCPv6 daemon must have a link-local address.");
        }

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

        Ptr<Socket> socket;
        socket = Socket::CreateSocket(node, tid);
        socket->Bind(Inet6SocketAddress(linkLocal, Dhcp6Server::PORT));
        socket->BindToNetDevice(device);

        m_sendSockets[device] = socket;
    }

    uint32_t nInterfaces = node->GetNDevices();
    std::vector<Ptr<NetDevice>> possibleDuidDevices;

    uint32_t maxAddressLength = 0;
    for (uint32_t i = 0; i < nInterfaces; i++)
    {
        Ptr<NetDevice> device = node->GetDevice(i);

        // Discard the loopback device.
        if (DynamicCast<LoopbackNetDevice>(node->GetDevice(i)))
        {
            continue;
        }

        // Check if the NetDevice is up.
        if (device->IsLinkUp())
        {
            Address address = device->GetAddress();
            if (address.GetLength() > maxAddressLength)
            {
                maxAddressLength = address.GetLength();
            }
            possibleDuidDevices.push_back(device);
        }
    }

    std::vector<Ptr<NetDevice>> longestDuidDevices;
    for (uint32_t i = 0; i < possibleDuidDevices.size(); i++)
    {
        if (possibleDuidDevices[i]->GetAddress().GetLength() == maxAddressLength)
        {
            longestDuidDevices.push_back(possibleDuidDevices[i]);
        }
    }

    if (longestDuidDevices.empty())
    {
        NS_ABORT_MSG("No suitable NetDevice found for DUID, aborting.");
    }

    // Consider the link-layer address of the first NetDevice in the list.
    m_serverIdentifier.SetHardwareType(1);
    m_serverIdentifier.SetLinkLayerAddress(longestDuidDevices[0]->GetAddress());

    m_leaseCleanupEvent = Simulator::Schedule(m_leaseCleanup, &Dhcp6Server::CleanLeases, this);
}

void
Dhcp6Server::StopApplication()
{
    NS_LOG_FUNCTION(this);

    if (m_recvSocket)
    {
        m_recvSocket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }

    m_subnets.clear();
    m_leaseCleanupEvent.Cancel();
}

void
Dhcp6Server::CleanLeases()
{
    NS_LOG_FUNCTION(this);

    for (auto sub = m_subnets.begin(); sub != m_subnets.end(); sub++)
    {
        LeaseInfo subnet = *sub;

        std::vector<Ipv6Address> expiredAddrs;
        for (auto itr = subnet.m_leasedAddresses.begin(); itr != subnet.m_leasedAddresses.end();
             itr++)
        {
            Ipv6Address address = itr->second.first;
            Time leaseTime = itr->second.second;

            if (Simulator::Now() >= leaseTime)
            {
                subnet.m_expiredAddresses.insert({leaseTime, address});
                expiredAddrs.push_back(itr->second.first);
            }
        }

        for (auto itr = expiredAddrs.begin(); itr != expiredAddrs.end(); itr++)
        {
            for (auto it = subnet.m_leasedAddresses.begin(); it != subnet.m_leasedAddresses.end();
                 it++)
            {
                Ipv6Address address = it->second.first;

                if (address == *itr)
                {
                    subnet.m_leasedAddresses.erase(it);
                    break;
                }
            }
        }
    }

    m_leaseCleanupEvent = Simulator::Schedule(m_leaseCleanup, &Dhcp6Server::CleanLeases, this);
}

size_t
AddressHash::operator()(const Address& x) const
{
    uint8_t buffer[20];
    uint8_t addrLen = x.GetLength();
    x.CopyTo(buffer);

    std::string s(buffer, buffer + addrLen);
    return std::hash<std::string>{}(s);
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
