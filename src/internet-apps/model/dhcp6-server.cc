/*
 * Copyright (c) 2024 NITK Surathkal
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
 * Author: Kavya Bhat <kavyabhat@gmail.com>
 *
 */

#include "dhcp6-server.h"

#include "dhcp6-duid.h"

#include "ns3/address-utils.h"
#include "ns3/ipv6-interface.h"
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
namespace internetApplications
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

    Duid clientDuid = header.GetClientIdentifier().GetDuid();
    std::map<Dhcp6Header::OptionType, bool> headerOptions = header.GetOptionList();

    // Add each IA in the header to the IA bindings.
    if (headerOptions.find(Dhcp6Header::OPTION_IA_NA) != headerOptions.end())
    {
        std::vector<IaOptions> iaOpt = header.GetIanaOptions();
        for (const auto& itr : iaOpt)
        {
            uint32_t iaid = itr.GetIaid();
            m_iaBindings.insert({clientDuid, std::make_pair(Dhcp6Header::OPTION_IA_NA, iaid)});
        }
    }
}

void
Dhcp6Server::SendAdvertise(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress client)
{
    NS_LOG_INFO(this << iDev << header << client);

    // Options included according to RFC 8415 Section 18.3.9

    Ptr<Packet> packet = Create<Packet>();
    Dhcp6Header advertiseHeader;
    advertiseHeader.ResetOptions();
    advertiseHeader.SetMessageType(Dhcp6Header::ADVERTISE);
    advertiseHeader.SetTransactId(header.GetTransactId());

    // Add Client Identifier Option, copied from the received header.
    Duid clientDuid = header.GetClientIdentifier().GetDuid();
    advertiseHeader.AddClientIdentifier(clientDuid);

    // Add Server Identifier Option.
    advertiseHeader.AddServerIdentifier(m_serverDuid);

    // Find all requested IAIDs for this client.
    std::vector<IaOptions> ianaOptionsList = header.GetIanaOptions();
    std::vector<uint32_t> requestedIa(ianaOptionsList.size());
    for (const auto& iaOpt : ianaOptionsList)
    {
        requestedIa.emplace_back(iaOpt.GetIaid());
    }

    // Add IA_NA option.
    // Available address pools and IA information is sent in this option.
    for (auto& subnet : m_subnets)
    {
        Ipv6Address pool = subnet.GetAddressPool();
        Ipv6Prefix prefix = subnet.GetPrefix();
        Ipv6Address minAddress = subnet.GetMinAddress();
        Ipv6Address maxAddress = subnet.GetMaxAddress();

        /*
         * Find the next available address. Checks the expired address map.
         * If there are no expired addresses, it advertises a new address.
         */

        uint8_t offeredAddrBuf[16];

        bool foundAddress = false;
        if (!subnet.m_expiredAddresses.empty())
        {
            Ipv6Address nextAddress;

            for (auto itr = subnet.m_expiredAddresses.begin();
                 itr != subnet.m_expiredAddresses.end();)
            {
                if (itr->second.first == clientDuid)
                {
                    nextAddress = itr->second.second;
                    nextAddress.GetBytes(offeredAddrBuf);
                    itr = subnet.m_expiredAddresses.erase(itr);
                    foundAddress = true;
                    break;
                }
                itr++;
            }

            /*
             Prevent Expired Addresses from building up.
             We set a maximum limit of 30 expired addresses, after which the
             oldest expired address is removed and offered to a client.
             */
            if (!foundAddress && subnet.m_expiredAddresses.size() > 30)
            {
                auto firstExpiredAddress = subnet.m_expiredAddresses.begin();
                nextAddress = firstExpiredAddress->second.second;
                nextAddress.GetBytes(offeredAddrBuf);
                subnet.m_expiredAddresses.erase(firstExpiredAddress);
                foundAddress = true;
            }
        }

        if (!foundAddress)
        {
            // Allocate a new address.
            uint8_t minAddrBuf[16];
            minAddress.GetBytes(minAddrBuf);

            // Get the latest leased address.
            uint8_t lastLeasedAddrBuf[16];

            if (!subnet.m_leasedAddresses.empty())
            {
                // Obtain the highest address that has been offered.
                subnet.m_maxOfferedAddress.GetBytes(lastLeasedAddrBuf);
                memcpy(offeredAddrBuf, lastLeasedAddrBuf, 16);

                // Increment the address by adding 1. Bitwise addition is used.
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
            subnet.m_maxOfferedAddress = offer;

            /*
            Optimistic assumption that the address will be leased to this client.
            This is to prevent multiple clients from receiving the same address.
            */
            subnet.m_leasedAddresses.insert(
                {clientDuid, std::make_pair(offer, Time(Seconds(m_prefLifetime.GetSeconds())))});
        }

        Ipv6Address offeredAddr(offeredAddrBuf);
        NS_LOG_INFO("Offered address: " << offeredAddr);

        for (const auto& iaid : requestedIa)
        {
            // Add the IA_NA option and IA Address option.
            advertiseHeader.AddIanaOption(iaid, m_renew.GetSeconds(), m_rebind.GetSeconds());
            advertiseHeader.AddAddress(iaid,
                                       offeredAddr,
                                       m_prefLifetime.GetSeconds(),
                                       m_validLifetime.GetSeconds());
        }
    }

    std::map<Dhcp6Header::OptionType, bool> headerOptions = header.GetOptionList();
    if (headerOptions.find(Dhcp6Header::OPTION_ORO) != headerOptions.end())
    {
        std::vector<uint16_t> requestedOptions = header.GetOptionRequest().GetRequestedOptions();
        advertiseHeader.HandleOptionRequest(requestedOptions);
    }

    packet->AddHeader(advertiseHeader);

    // Find the socket corresponding to the NetDevice.
    Ptr<Socket> sendSocket = m_sendSockets[iDev];

    // Send the advertise message.
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

    // Options included according to RFC 8415 Section 18.3.10

    Ptr<Packet> packet = Create<Packet>();
    Dhcp6Header replyHeader;
    replyHeader.ResetOptions();
    replyHeader.SetMessageType(Dhcp6Header::REPLY);
    replyHeader.SetTransactId(header.GetTransactId());

    // Add Client Identifier Option, copied from the received header.
    Duid clientDuid = header.GetClientIdentifier().GetDuid();
    replyHeader.AddClientIdentifier(clientDuid);

    // Add Server Identifier Option.
    replyHeader.AddServerIdentifier(m_serverDuid);

    // Add IA_NA option.
    // Retrieve requested IA Option from client header.
    std::vector<IaOptions> ianaOptionsList = header.GetIanaOptions();

    for (auto& iaOpt : ianaOptionsList)
    {
        // Iterate through the offered addresses.
        // Current approach: Try to accept all offers.
        std::vector<IaAddressOption> iaAddrOptList = iaOpt.m_iaAddressOption;
        for (auto& addrItr : iaAddrOptList)
        {
            Ipv6Address requestedAddr = addrItr.GetIaAddress();

            for (auto& subnet : m_subnets)
            {
                Ipv6Address pool = subnet.GetAddressPool();
                Ipv6Prefix prefix = subnet.GetPrefix();
                Ipv6Address minAddress = subnet.GetMinAddress();
                Ipv6Address maxAddress = subnet.GetMaxAddress();

                // Check if the requested address has been declined earlier.
                // In this case, it cannot be leased.
                if (subnet.m_declinedAddresses.find(requestedAddr) !=
                    subnet.m_declinedAddresses.end())
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

                    // Add the IA_NA option and IA Address option.
                    replyHeader.AddIanaOption(iaOpt.GetIaid(), iaOpt.GetT1(), iaOpt.GetT2());
                    replyHeader.AddAddress(iaOpt.GetIaid(),
                                           requestedAddr,
                                           m_prefLifetime.GetSeconds(),
                                           m_validLifetime.GetSeconds());

                    // Update the lease time of the newly leased addresses.
                    // Find all the leases for this client.
                    auto range = subnet.m_leasedAddresses.equal_range(clientDuid);

                    // Create a new multimap to store the updated lifetimes.
                    std::multimap<Duid, std::pair<Ipv6Address, Time>> updatedLifetimes;
                    for (auto it = range.first; it != range.second; it++)
                    {
                        Ipv6Address clientLease = it->second.first;
                        std::pair<Ipv6Address, Time> clientLeaseTime = {
                            clientLease,
                            Time(Seconds(m_prefLifetime.GetSeconds()))};

                        // Add the DUID + Ipv6Address / LeaseTime to the map.
                        updatedLifetimes.insert({clientDuid, clientLeaseTime});
                    }

                    // Remove all the old leases for this client.
                    // This is done to prevent multiple entries for the same lease.
                    subnet.m_leasedAddresses.erase(range.first->first);

                    // Add the updated leases to the subnet.
                    for (auto& itr : updatedLifetimes)
                    {
                        subnet.m_leasedAddresses.insert({itr.first, itr.second});
                    }
                    break;
                }
            }
        }
    }

    std::map<Dhcp6Header::OptionType, bool> headerOptions = header.GetOptionList();

    // Check if the client has requested any options.
    if (headerOptions.find(Dhcp6Header::OPTION_ORO) != headerOptions.end())
    {
        std::vector<uint16_t> requestedOptions = header.GetOptionRequest().GetRequestedOptions();
        replyHeader.HandleOptionRequest(requestedOptions);
    }

    packet->AddHeader(replyHeader);

    // Find the socket corresponding to the NetDevice.
    Ptr<Socket> sendSocket = m_sendSockets[iDev];

    // Send the Reply message.
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

    // Options included according to RFC 8415 Section 18.3.4, 18.3.5

    Ptr<Packet> packet = Create<Packet>();
    Dhcp6Header replyHeader;
    replyHeader.ResetOptions();
    replyHeader.SetMessageType(Dhcp6Header::REPLY);
    replyHeader.SetTransactId(header.GetTransactId());

    // Add Client Identifier Option, copied from the received header.
    Duid clientDuid = header.GetClientIdentifier().GetDuid();
    replyHeader.AddClientIdentifier(clientDuid);

    // Add Server Identifier Option.
    replyHeader.AddServerIdentifier(m_serverDuid);

    // Add IA_NA option.
    // Retrieve IA_NAs from client header.
    std::vector<IaOptions> ianaOptionsList = header.GetIanaOptions();
    for (auto& iaOpt : ianaOptionsList)
    {
        std::vector<IaAddressOption> iaAddrOptList = iaOpt.m_iaAddressOption;

        // Add the IA_NA option.
        replyHeader.AddIanaOption(iaOpt.GetIaid(), iaOpt.GetT1(), iaOpt.GetT2());

        for (const auto& addrItr : iaAddrOptList)
        {
            // Find the lease address which is to be renewed or rebound.
            Ipv6Address clientLease = addrItr.GetIaAddress();

            // Update the lifetime for the address.
            // Iterate through the subnet list to find the subnet that the
            // address belongs to.
            for (auto& subnet : m_subnets)
            {
                Ipv6Prefix prefix = subnet.GetPrefix();
                Ipv6Address pool = subnet.GetAddressPool();

                // Check if the prefix of the lease matches that of the pool.
                if (prefix.IsMatch(clientLease, pool))
                {
                    // Find all the leases for this client.
                    auto range = subnet.m_leasedAddresses.equal_range(clientDuid);
                    for (auto itr = range.first; itr != range.second; itr++)
                    {
                        // Check if the IPv6 address matches the client lease.
                        if (itr->second.first == clientLease)
                        {
                            NS_LOG_INFO("Renewing address: " << itr->second.first);
                            std::pair<Ipv6Address, Time> clientLeaseTime = {
                                clientLease,
                                Time(Seconds(m_prefLifetime.GetSeconds()))};

                            // Remove the old lease information.
                            subnet.m_leasedAddresses.erase(itr);

                            // Add the new lease information (with updated time)
                            subnet.m_leasedAddresses.insert({clientDuid, clientLeaseTime});

                            // Add the IA Address option.
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

    std::map<Dhcp6Header::OptionType, bool> headerOptions = header.GetOptionList();
    if (headerOptions.find(Dhcp6Header::OPTION_ORO) != headerOptions.end())
    {
        std::vector<uint16_t> requestedOptions = header.GetOptionRequest().GetRequestedOptions();
        replyHeader.HandleOptionRequest(requestedOptions);
    }

    packet->AddHeader(replyHeader);

    // Find the socket corresponding to the NetDevice.
    Ptr<Socket> sendSocket = m_sendSockets[iDev];

    // Send the Reply message.
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
    // Invoked in case a Decline or Release message is received.
    NS_LOG_INFO(this << iDev << header << client);

    // Options included in accordance with RFC 8415, Section 18.3.7, 18.3.8

    Ptr<Packet> packet = Create<Packet>();
    Dhcp6Header replyHeader;
    replyHeader.ResetOptions();
    replyHeader.SetMessageType(Dhcp6Header::REPLY);
    replyHeader.SetTransactId(header.GetTransactId());

    // Add Client Identifier Option, copied from the received header.
    Duid clientDuid = header.GetClientIdentifier().GetDuid();
    replyHeader.AddClientIdentifier(clientDuid);

    // Add Server Identifier Option.
    replyHeader.AddServerIdentifier(m_serverDuid);

    // Add Status code option.
    replyHeader.AddStatusCode(Dhcp6Header::Success, "Address declined.");

    // Add the declined or expired address to the subnet information.
    std::vector<IaOptions> ianaOptionsList = header.GetIanaOptions();
    for (const auto& iaOpt : ianaOptionsList)
    {
        std::vector<IaAddressOption> iaAddrOptList = iaOpt.m_iaAddressOption;

        for (const auto& addrItr : iaAddrOptList)
        {
            Ipv6Address address = addrItr.GetIaAddress();
            if (header.GetMessageType() == Dhcp6Header::DECLINE)
            {
                // Find the subnet that this address belongs to.
                for (auto& subnet : m_subnets)
                {
                    // Find the client that the address currently belongs to.

                    for (auto itr = subnet.m_leasedAddresses.begin();
                         itr != subnet.m_leasedAddresses.end();)
                    {
                        Ipv6Address leaseAddr = itr->second.first;
                        if (leaseAddr == address)
                        {
                            itr = subnet.m_leasedAddresses.erase(itr);
                            subnet.m_declinedAddresses[address] = clientDuid;
                            continue;
                        }
                        itr++;
                    }
                }
            }
            else if (header.GetMessageType() == Dhcp6Header::RELEASE)
            {
                // Find the subnet that this address belongs to.
                for (auto& subnet : m_subnets)
                {
                    // Find the client that the address currently belongs to.

                    for (auto itr = subnet.m_leasedAddresses.begin();
                         itr != subnet.m_leasedAddresses.end();)
                    {
                        Duid duid = itr->first;
                        Ipv6Address leaseAddr = itr->second.first;
                        Time expiredTime = itr->second.second;
                        if (leaseAddr == address)
                        {
                            itr = subnet.m_leasedAddresses.erase(itr);
                            std::pair<Duid, Ipv6Address> expiredLease = {duid, leaseAddr};
                            subnet.m_expiredAddresses.insert({expiredTime, expiredLease});
                            continue;
                        }
                        itr++;
                    }
                }
            }
        }
    }

    packet->AddHeader(replyHeader);

    // Find the socket corresponding to the NetDevice.
    Ptr<Socket> sendSocket = m_sendSockets[iDev];

    // Send the Reply message.
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
Dhcp6Server::SetDhcp6ServerNetDevice(NetDeviceContainer netDevices)
{
    for (auto itr = netDevices.Begin(); itr != netDevices.End(); itr++)
    {
        m_sendSockets[*itr] = nullptr;
    }
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
    NS_ASSERT_MSG(packet->RemovePacketTag(interfaceInfo),
                  "No incoming interface on DHCPv6 message.");

    uint32_t incomingIf = interfaceInfo.GetRecvIf();
    Ptr<NetDevice> iDev = GetNode()->GetDevice(incomingIf);

    if (packet->RemoveHeader(header) == 0)
    {
        return;
    }

    // Initialize the DUID before responding to the client.
    Ptr<Node> node = (m_sendSockets.begin()->first)->GetNode();
    m_serverDuid.Initialize(node);

    if (header.GetMessageType() == Dhcp6Header::SOLICIT)
    {
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
    m_subnets.emplace_back(newSubnet);
}

void
Dhcp6Server::StartApplication()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Starting DHCPv6 server.");

    if (m_recvSocket)
    {
        NS_LOG_INFO("DHCPv6 daemon is not meant to be started repeatedly.");
        return;
    }

    Ptr<Node> node = (m_sendSockets.begin()->first)->GetNode();
    Ptr<Ipv6> ipv6 = node->GetObject<Ipv6>();
    Ptr<Ipv6L3Protocol> ipv6l3 = node->GetObject<Ipv6L3Protocol>();

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_recvSocket = Socket::CreateSocket(node, tid);

    Inet6SocketAddress local =
        Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(), Dhcp6Header::SERVER_PORT);
    m_recvSocket->Bind(local);
    m_recvSocket->SetRecvPktInfo(true);
    m_recvSocket->SetRecvCallback(MakeCallback(&Dhcp6Server::NetHandler, this));

    for (auto itr = m_sendSockets.begin(); itr != m_sendSockets.end(); itr++)
    {
        Ptr<NetDevice> device = itr->first;
        uint32_t ifIndex = ipv6->GetInterfaceForDevice(device);

        NS_ASSERT_MSG(ifIndex >= 0,
                      "Dhcp6Server::StartApplication: device is not connected to IPv6.");

        Ipv6Address linkLocal = ipv6l3->GetInterface(ifIndex)->GetLinkLocalAddress().GetAddress();
        Ptr<Socket> socket;
        socket = Socket::CreateSocket(node, tid);
        socket->Bind(Inet6SocketAddress(linkLocal, Dhcp6Header::SERVER_PORT));
        socket->BindToNetDevice(device);
        m_sendSockets[device] = socket;
    }

    m_leaseCleanupEvent = Simulator::Schedule(m_leaseCleanup, &Dhcp6Server::CleanLeases, this);
}

void
Dhcp6Server::StopApplication()
{
    NS_LOG_FUNCTION(this);

    m_recvSocket = nullptr;

    m_subnets.clear();
    m_leaseCleanupEvent.Cancel();
}

void
Dhcp6Server::CleanLeases()
{
    NS_LOG_FUNCTION(this);

    for (auto& subnet : m_subnets)
    {
        for (auto itr = subnet.m_leasedAddresses.begin(); itr != subnet.m_leasedAddresses.end();)
        {
            Duid duid = itr->first;
            Ipv6Address address = itr->second.first;
            Time leaseTime = itr->second.second;

            if (Simulator::Now() >= leaseTime)
            {
                std::pair<Duid, Ipv6Address> expiredLease = {duid, address};
                subnet.m_expiredAddresses.insert({leaseTime, expiredLease});
                itr = subnet.m_leasedAddresses.erase(itr);
                continue;
            }
            itr++;
        }
    }

    m_leaseCleanupEvent = Simulator::Schedule(m_leaseCleanup, &Dhcp6Server::CleanLeases, this);
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
} // namespace internetApplications
} // namespace ns3
