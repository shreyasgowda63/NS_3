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

#include "dhcp6-client.h"

#include "dhcp6-duid.h"

#include "ns3/address-utils.h"
#include "ns3/icmpv6-l4-protocol.h"
#include "ns3/ipv6-interface.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/ipv6-packet-info-tag.h"
#include "ns3/ipv6.h"
#include "ns3/log.h"
#include "ns3/loopback-net-device.h"
#include "ns3/mac48-address.h"
#include "ns3/net-device-container.h"
#include "ns3/object.h"
#include "ns3/pointer.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/string.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/traced-value.h"
#include "ns3/trickle-timer.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Dhcp6Client");

TypeId
Dhcp6Client::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::Dhcp6Client")
            .SetParent<Application>()
            .AddConstructor<Dhcp6Client>()
            .SetGroupName("InternetApps")
            .AddAttribute("Transactions",
                          "A value to be used as the transaction ID.",
                          StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1000000.0]"),
                          MakePointerAccessor(&Dhcp6Client::m_transactionId),
                          MakePointerChecker<RandomVariableStream>())
            .AddAttribute("SolicitJitter",
                          "The jitter in ms that a node waits before sending any solicitation. By "
                          "default, the model will wait for a duration in ms defined by a uniform "
                          "random-variable between 0 and SolicitJitter",
                          StringValue("ns3::UniformRandomVariable[Min=0.0|Max=10.0]"),
                          MakePointerAccessor(&Dhcp6Client::m_solicitJitter),
                          MakePointerChecker<RandomVariableStream>())
            .AddAttribute("IaidValue",
                          "The identifier for a new IA created by a client.",
                          StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1000000.0]"),
                          MakePointerAccessor(&Dhcp6Client::m_iaidStream),
                          MakePointerChecker<RandomVariableStream>())
            .AddTraceSource("NewLease",
                            "The client has obtained a lease",
                            MakeTraceSourceAccessor(&Dhcp6Client::m_newLease),
                            "ns3::Ipv6Address::TracedCallback")
            .AddAttribute("RenewTime",
                          "Time after which client should renew. 1000 seconds by default in Linux",
                          TimeValue(Seconds(1000)),
                          MakeTimeAccessor(&Dhcp6Client::InterfaceConfig::renew),
                          MakeTimeChecker())
            .AddAttribute("RebindTime",
                          "Time after which client should rebind. 2000 seconds by default in Linux",
                          TimeValue(Seconds(2000)),
                          MakeTimeAccessor(&Dhcp6Client::InterfaceConfig::rebind),
                          MakeTimeChecker())
            .AddAttribute(
                "PreferredLifetime",
                "The preferred lifetime of the leased address. 3000 seconds by default in Linux",
                TimeValue(Seconds(3000)),
                MakeTimeAccessor(&Dhcp6Client::InterfaceConfig::prefLifetime),
                MakeTimeChecker())
            .AddAttribute("ValidLifetime",
                          "Time after which client should release the address. 4000 seconds by "
                          "default in Linux",
                          TimeValue(Seconds(4000)),
                          MakeTimeAccessor(&Dhcp6Client::InterfaceConfig::validLifetime),
                          MakeTimeChecker())
            .AddAttribute("SolicitInterval",
                          "Time after which the client resends the Solicit. ",
                          TimeValue(Seconds(100)),
                          MakeTimeAccessor(&Dhcp6Client::InterfaceConfig::solicitInterval),
                          MakeTimeChecker());
    return tid;
}

Dhcp6Client::Dhcp6Client()
{
    NS_LOG_FUNCTION(this);
}

void
Dhcp6Client::DoDispose()
{
    NS_LOG_FUNCTION(this);

    m_interfaces.clear();

    Application::DoDispose();
}

int64_t
Dhcp6Client::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_solicitJitter->SetStream(stream);
    m_transactionId->SetStream(stream + 1);
    m_iaidStream->SetStream(stream + 2);
    return 3;
}

// void
// Dhcp6Client::SetDhcp6ClientNetDevice(Ptr<NetDevice> netDevice)
// {
//     // m_device = netDevice;
//     // void
// }

void
Dhcp6Client::SetDhcp6ClientNetDevice(NetDeviceContainer netDevices)
{
    for (auto itr = netDevices.Begin(); itr != netDevices.End(); itr++)
    {
        m_interfaces[*itr] = nullptr;
    }
}

bool
Dhcp6Client::ValidateAdvertise(Dhcp6Header header, Ptr<NetDevice> iDev)
{
    Ptr<Packet> packet = Create<Packet>();
    uint32_t clientTransactId = m_interfaces[iDev]->transactId;
    uint32_t receivedTransactId = header.GetTransactId();

    if (clientTransactId != receivedTransactId)
    {
        return false;
    }

    Duid clientDuid = header.GetClientIdentifier().GetDuid();
    NS_ASSERT_MSG(clientDuid == m_clientDuid, "Client DUID mismatch.");

    m_serverDuid = header.GetServerIdentifier().GetDuid();
    return true;
}

void
Dhcp6Client::SendRequest(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress server)
{
    NS_LOG_FUNCTION(this << iDev << header << server);

    Ptr<Packet> packet = Create<Packet>();
    Dhcp6Header requestHeader;
    requestHeader.ResetOptions();
    requestHeader.SetMessageType(Dhcp6Header::REQUEST);

    // TODO: Use min, max for GetValue
    Ptr<InterfaceConfig> dhcpInterface = m_interfaces[iDev];
    dhcpInterface->transactId = static_cast<uint32_t>(m_transactionId->GetValue());
    requestHeader.SetTransactId(dhcpInterface->transactId);

    // Add Client Identifier Option.
    requestHeader.AddClientIdentifier(m_clientDuid);

    // Add Server Identifier Option, copied from the received header.
    Duid serverDuid = header.GetServerIdentifier().GetDuid();
    requestHeader.AddServerIdentifier(serverDuid);

    // Add Elapsed Time Option.
    uint32_t actualElapsedTime =
        (Simulator::Now() - dhcpInterface->msgStartTime).GetMilliSeconds() / 10;
    uint16_t elapsed = actualElapsedTime > 65535 ? 65535 : actualElapsedTime;
    requestHeader.AddElapsedTime(elapsed);

    // Add IA_NA option.
    // Request all addresses from the Advertise message.
    std::vector<IaOptions> ianaOptionsList = header.GetIanaOptions();

    for (const auto& iaOpt : ianaOptionsList)
    {
        // Iterate through the offered addresses.
        // Current approach: Try to accept all offers.
        for (const auto& iaAddrOpt : iaOpt.m_iaAddressOption)
        {
            requestHeader.AddIanaOption(iaOpt.GetIaid(), iaOpt.GetT1(), iaOpt.GetT2());
            requestHeader.AddAddress(iaOpt.GetIaid(),
                                     iaAddrOpt.GetIaAddress(),
                                     iaAddrOpt.GetPreferredLifetime(),
                                     iaAddrOpt.GetValidLifetime());
        }
    }

    // Add Option Request.
    header.AddOptionRequest(Dhcp6Header::OPTION_SOL_MAX_RT);

    packet->AddHeader(requestHeader);

    // TODO: Handle server unicast option.

    // Send the request message.
    dhcpInterface->state = WAIT_REPLY;
    if (dhcpInterface->socket->SendTo(
            packet,
            0,
            Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(), Dhcp6Header::SERVER_PORT)) >= 0)
    {
        NS_LOG_INFO("DHCPv6 client: Request sent.");
    }
    else
    {
        NS_LOG_INFO("DHCPv6 client: Error while sending Request.");
    }
}

Dhcp6Client::InterfaceConfig::InterfaceConfig()
{
    solicitInterval = Seconds(100);
    renew = Seconds(1000);
    rebind = Seconds(2000);
    prefLifetime = Seconds(3000);
    validLifetime = Seconds(4000);
}

void
Dhcp6Client::InterfaceConfig::AcceptedAddress(const Ipv6Address& offeredAddress)
{
    NS_LOG_INFO("Accepting " << offeredAddress);
    nAcceptedAddresses += 1;

    // Notify the new lease.
    client->m_newLease(offeredAddress);
}

void
Dhcp6Client::InterfaceConfig::AddDeclinedAddress(const Ipv6Address& offeredAddress)
{
    declinedAddresses.emplace_back(offeredAddress);

    if (declinedAddresses.size() + nAcceptedAddresses == nOfferedAddresses)
    {
        DeclineOffer();
    }
}

void
Dhcp6Client::InterfaceConfig::DeclineOffer()
{
    if (declinedAddresses.empty())
    {
        return;
    }

    // Cancel all scheduled Release, Renew, Rebind events.
    renewEvent.Cancel();
    rebindEvent.Cancel();
    for (auto itr : releaseEvent)
    {
        itr.Cancel();
    }

    Dhcp6Header declineHeader;
    Ptr<Packet> packet = Create<Packet>();

    // Remove address associations.
    for (const auto& offer : declinedAddresses)
    {
        uint32_t iaid = client->m_iaidMap[offer];

        // IA_NA option, IA address option
        declineHeader.AddIanaOption(iaid, renew.GetSeconds(), rebind.GetSeconds());
        declineHeader.AddAddress(iaid,
                                 offer,
                                 prefLifetime.GetSeconds(),
                                 validLifetime.GetSeconds());
    }

    transactId = static_cast<uint32_t>(client->m_transactionId->GetValue());
    declineHeader.SetTransactId(transactId);
    declineHeader.SetMessageType(Dhcp6Header::DECLINE);

    // Add client identifier option
    declineHeader.AddClientIdentifier(client->m_clientDuid);

    // Add server identifier option
    declineHeader.AddServerIdentifier(client->m_serverDuid);

    msgStartTime = Simulator::Now();
    declineHeader.AddElapsedTime(0);

    packet->AddHeader(declineHeader);
    if ((socket->SendTo(packet,
                        0,
                        Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(),
                                           Dhcp6Header::SERVER_PORT))) >= 0)
    {
        NS_LOG_INFO("DHCPv6 client: Decline sent");
    }
    else
    {
        NS_LOG_INFO("DHCPv6 client: Error while sending Decline");
    }

    state = WAIT_REPLY_AFTER_DECLINE;
}

void
Dhcp6Client::CheckLeaseStatus(Ptr<NetDevice> iDev,
                              Dhcp6Header header,
                              Inet6SocketAddress server) const
{
    // Read Status Code option.
    uint16_t statusCode = header.GetStatusCodeOption().GetStatusCode();

    if (statusCode == 0)
    {
        NS_LOG_INFO("DHCPv6 client: Server bindings updated successfully.");
    }
    else
    {
        NS_LOG_INFO("DHCPv6 client: Server bindings update failed.");
    }
}

void
Dhcp6Client::ProcessReply(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress server)
{
    NS_LOG_FUNCTION(this << iDev << header << server);

    Ptr<Ipv6> ipv6 = GetNode()->GetObject<Ipv6>();
    int32_t ifIndex = ipv6->GetInterfaceForDevice(iDev);

    Ptr<InterfaceConfig> dhcpInterface = m_interfaces[iDev];

    // Read IA_NA options.
    std::vector<IaOptions> ianaOptionsList = header.GetIanaOptions();

    dhcpInterface->declinedAddresses.clear();

    Time earliestRebind{Time::Max()};
    Time earliestRenew{Time::Max()};
    std::vector<uint32_t> iaidList;

    for (const auto& iaOpt : ianaOptionsList)
    {
        // Iterate through the offered addresses.
        // Current approach: Try to accept all offers.
        for (const auto& iaAddrOpt : iaOpt.m_iaAddressOption)
        {
            Ipv6Address offeredAddress = iaAddrOpt.GetIaAddress();

            // TODO: In Linux, all leased addresses seem to be /128. Double-check this.
            Ipv6InterfaceAddress addr(offeredAddress, 128);
            ipv6->AddAddress(ifIndex, addr);
            ipv6->SetUp(ifIndex);

            // Set the preferred and valid lifetimes.
            dhcpInterface->prefLifetime = Seconds(iaAddrOpt.GetPreferredLifetime());
            dhcpInterface->validLifetime = Seconds(iaAddrOpt.GetValidLifetime());

            // Add the IPv6 address - IAID association.
            m_iaidMap[offeredAddress] = iaOpt.GetIaid();

            // TODO: Check whether Release event happens for each address.
            dhcpInterface->releaseEvent.emplace_back(
                Simulator::Schedule(dhcpInterface->validLifetime,
                                    &Dhcp6Client::SendRelease,
                                    this,
                                    offeredAddress));

            dhcpInterface->nOfferedAddresses += 1;
        }

        earliestRenew = std::min(earliestRenew, Seconds(iaOpt.GetT1()));
        earliestRebind = std::min(earliestRebind, Seconds(iaOpt.GetT2()));
        iaidList.emplace_back(iaOpt.GetIaid());
    }

    // The renew and rebind events are scheduled for the earliest time across
    // all IA_NA options. RFC 8415, Section 18.2.4.
    dhcpInterface->renew = earliestRenew;
    dhcpInterface->renewEvent =
        Simulator::Schedule(dhcpInterface->renew, &Dhcp6Client::SendRenew, this, dhcpInterface);

    // Set the rebind timer and schedule the event.
    dhcpInterface->rebind = earliestRebind;
    dhcpInterface->rebindEvent =
        Simulator::Schedule(dhcpInterface->rebind, &Dhcp6Client::SendRebind, this, dhcpInterface);

    int32_t interfaceId = ipv6->GetInterfaceForDevice(iDev);
    Ptr<Icmpv6L4Protocol> icmpv6 = DynamicCast<Icmpv6L4Protocol>(
        ipv6->GetProtocol(Icmpv6L4Protocol::GetStaticProtocolNumber(), interfaceId));

    // If DAD fails, the offer is declined.
    icmpv6->TraceConnectWithoutContext(
        "DadFailure",
        MakeCallback(&Dhcp6Client::InterfaceConfig::AddDeclinedAddress, dhcpInterface));

    icmpv6->TraceConnectWithoutContext(
        "DadSuccess",
        MakeCallback(&Dhcp6Client::InterfaceConfig::AcceptedAddress, dhcpInterface));
}

void
Dhcp6Client::SendRenew(Ptr<InterfaceConfig> dhcpInterface)
{
    NS_LOG_FUNCTION(this);

    Dhcp6Header header;
    Ptr<Packet> packet = Create<Packet>();

    dhcpInterface->transactId = static_cast<uint32_t>(m_transactionId->GetValue());

    header.SetTransactId(dhcpInterface->transactId);
    header.SetMessageType(Dhcp6Header::RENEW);

    // Add client identifier option
    header.AddClientIdentifier(m_clientDuid);

    // Add server identifier option
    header.AddServerIdentifier(m_serverDuid);

    dhcpInterface->msgStartTime = Simulator::Now();
    header.AddElapsedTime(0);

    // Add IA_NA options.
    for (const auto& iaidRenew : dhcpInterface->iaids)
    {
        header.AddIanaOption(iaidRenew,
                             dhcpInterface->renew.GetSeconds(),
                             dhcpInterface->rebind.GetSeconds());

        // Iterate through the IPv6Address - IAID map, and add all addresses
        // that match the IAID to be renewed.
        for (const auto& itr : m_iaidMap)
        {
            Ipv6Address address = itr.first;
            uint32_t iaid = itr.second;
            if (iaid == iaidRenew)
            {
                header.AddAddress(iaidRenew,
                                  address,
                                  dhcpInterface->prefLifetime.GetSeconds(),
                                  dhcpInterface->validLifetime.GetSeconds());
            }
        }
    }

    // Add Option Request option.
    header.AddOptionRequest(Dhcp6Header::OPTION_SOL_MAX_RT);

    packet->AddHeader(header);
    if ((dhcpInterface->socket->SendTo(packet,
                                       0,
                                       Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(),
                                                          Dhcp6Header::SERVER_PORT))) >= 0)
    {
        NS_LOG_INFO("DHCPv6 client: Renew sent");
    }
    else
    {
        NS_LOG_INFO("DHCPv6 client: Error while sending Renew");
    }

    dhcpInterface->state = WAIT_REPLY;
}

void
Dhcp6Client::SendRebind(Ptr<InterfaceConfig> dhcpInterface)
{
    NS_LOG_FUNCTION(this);

    Dhcp6Header header;
    Ptr<Packet> packet = Create<Packet>();

    dhcpInterface->transactId = static_cast<uint32_t>(m_transactionId->GetValue());

    header.SetTransactId(dhcpInterface->transactId);
    header.SetMessageType(Dhcp6Header::REBIND);

    // Add client identifier option
    header.AddClientIdentifier(m_clientDuid);

    dhcpInterface->msgStartTime = Simulator::Now();
    header.AddElapsedTime(0);

    // Add IA_NA options.
    for (const auto& iaid : dhcpInterface->iaids)
    {
        header.AddIanaOption(iaid,
                             dhcpInterface->renew.GetSeconds(),
                             dhcpInterface->rebind.GetSeconds());
    }

    // Add Option Request option.
    header.AddOptionRequest(Dhcp6Header::OPTION_SOL_MAX_RT);

    packet->AddHeader(header);
    if ((dhcpInterface->socket->SendTo(packet,
                                       0,
                                       Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(),
                                                          Dhcp6Header::SERVER_PORT))) >= 0)
    {
        NS_LOG_INFO("DHCPv6 client: Rebind sent.");
    }
    else
    {
        NS_LOG_INFO("DHCPv6 client: Error while sending Rebind");
    }

    dhcpInterface->state = WAIT_REPLY;
}

void
Dhcp6Client::SendRelease(Ipv6Address address)
{
    NS_LOG_FUNCTION(this);

    Ptr<Ipv6> ipv6 = GetNode()->GetObject<Ipv6>();

    Dhcp6Header header;
    Ptr<Packet> packet = Create<Packet>();

    for (const auto& itr : m_interfaces)
    {
        Ptr<NetDevice> device = itr.first;
        Ptr<InterfaceConfig> dhcpInterface = itr.second;

        uint32_t ifIndex = ipv6->GetInterfaceForDevice(device);
        dhcpInterface->transactId = static_cast<uint32_t>(m_transactionId->GetValue());
        bool removed = ipv6->RemoveAddress(ifIndex, address);

        if (!removed)
        {
            continue;
        }

        header.SetTransactId(dhcpInterface->transactId);
        header.SetMessageType(Dhcp6Header::RELEASE);

        // Add client identifier option
        header.AddClientIdentifier(m_clientDuid);

        // Add server identifier option
        header.AddServerIdentifier(m_serverDuid);

        dhcpInterface->msgStartTime = Simulator::Now();
        header.AddElapsedTime(0);

        // IA_NA option, IA address option
        uint32_t iaid = m_iaidMap[address];
        header.AddIanaOption(iaid,
                             dhcpInterface->renew.GetSeconds(),
                             dhcpInterface->rebind.GetSeconds());
        header.AddAddress(iaid,
                          address,
                          dhcpInterface->prefLifetime.GetSeconds(),
                          dhcpInterface->validLifetime.GetSeconds());

        packet->AddHeader(header);
        if ((dhcpInterface->socket->SendTo(packet,
                                           0,
                                           Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(),
                                                              Dhcp6Header::SERVER_PORT))) >= 0)
        {
            NS_LOG_INFO("DHCPv6 client: Release sent.");
        }
        else
        {
            NS_LOG_INFO("DHCPv6 client: Error while sending Release");
        }

        dhcpInterface->state = WAIT_REPLY_AFTER_RELEASE;
    }
}

void
Dhcp6Client::NetHandler(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    Address from;
    Ptr<Packet> packet = socket->RecvFrom(from);
    Dhcp6Header header;

    Inet6SocketAddress senderAddr = Inet6SocketAddress::ConvertFrom(from);

    Ipv6PacketInfoTag interfaceInfo;
    NS_ASSERT_MSG(packet->RemovePacketTag(interfaceInfo),
                  "No incoming interface on DHCPv6 message.");

    uint32_t incomingIf = interfaceInfo.GetRecvIf();
    Ptr<NetDevice> iDev = GetNode()->GetDevice(incomingIf);
    Ptr<InterfaceConfig> dhcpInterface = m_interfaces[iDev];

    if (packet->RemoveHeader(header) == 0)
    {
        return;
    }
    if (dhcpInterface->state == WAIT_ADVERTISE && header.GetMessageType() == Dhcp6Header::ADVERTISE)
    {
        NS_LOG_INFO("DHCPv6 client: Received Advertise.");
        dhcpInterface->solicitTimer.Stop();
        bool check = ValidateAdvertise(header, iDev);
        if (check)
        {
            SendRequest(iDev, header, senderAddr);
        }
    }
    if (dhcpInterface->state == WAIT_REPLY && header.GetMessageType() == Dhcp6Header::REPLY)
    {
        NS_LOG_INFO("DHCPv6 client: Received Reply.");

        dhcpInterface->renewEvent.Cancel();
        dhcpInterface->rebindEvent.Cancel();
        for (auto itr : dhcpInterface->releaseEvent)
        {
            itr.Cancel();
        }

        ProcessReply(iDev, header, senderAddr);
    }
    if ((dhcpInterface->state == WAIT_REPLY_AFTER_DECLINE ||
         dhcpInterface->state == WAIT_REPLY_AFTER_RELEASE) &&
        (header.GetMessageType() == Dhcp6Header::REPLY))
    {
        NS_LOG_INFO("DHCPv6 client: Received Reply.");
        CheckLeaseStatus(iDev, header, senderAddr);
    }
}

void
Dhcp6Client::LinkStateHandler(Ptr<NetDevice> device)
{
    NS_LOG_FUNCTION(this);
    Ptr<InterfaceConfig> dhcpInterface = m_interfaces[device];
    if (device->IsLinkUp())
    {
        NS_LOG_INFO("DHCPv6 client: Link up at " << Simulator::Now().As(Time::S));
        dhcpInterface->socket->SetRecvCallback(MakeCallback(&Dhcp6Client::NetHandler, this));
        StartApplication();
    }
    else
    {
        dhcpInterface->solicitTimer.Stop();
        dhcpInterface->renewEvent.Cancel();
        dhcpInterface->rebindEvent.Cancel();
        for (auto itr : dhcpInterface->releaseEvent)
        {
            itr.Cancel();
        }

        // Stop receiving on the socket.
        dhcpInterface->socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        NS_LOG_INFO("DHCPv6 client: Link down at " << Simulator::Now().As(Time::S));
    }
}

Duid
Dhcp6Client::GetSelfDuid()
{
    return m_clientDuid;
}

void
Dhcp6Client::StartApplication()
{
    NS_LOG_FUNCTION(this);

    Ptr<NetDevice> netDevice = m_interfaces.begin()->first;
    Ptr<Node> node = netDevice->GetNode();
    NS_ASSERT_MSG(node, "Dhcp6Client::StartApplication: cannot get the node from the device.");

    Ptr<Ipv6> ipv6 = node->GetObject<Ipv6>();
    NS_ASSERT_MSG(ipv6, "Dhcp6Client::StartApplication: node does not have IPv6.");

    Ptr<Ipv6L3Protocol> ipv6l3 = node->GetObject<Ipv6L3Protocol>();

    std::vector<uint32_t> existingIaNaIds;
    for (auto itr = m_interfaces.begin(); itr != m_interfaces.end(); itr++)
    {
        Ptr<NetDevice> device = itr->first;
        Ptr<InterfaceConfig> dhcpInterface = new InterfaceConfig();
        dhcpInterface->client = this;
        dhcpInterface->device = device;

        uint32_t ifIndex = ipv6->GetInterfaceForDevice(device);

        NS_ASSERT_MSG(ifIndex >= 0,
                      "Dhcp6Server::StartApplication: device is not connected to IPv6.");

        Ipv6Address linkLocal = ipv6l3->GetInterface(ifIndex)->GetLinkLocalAddress().GetAddress();
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");

        Ptr<Socket> socket = Socket::CreateSocket(node, tid);
        socket->Bind(Inet6SocketAddress(linkLocal, Dhcp6Header::CLIENT_PORT));
        socket->BindToNetDevice(device);
        socket->SetRecvPktInfo(true);
        socket->SetRecvCallback(MakeCallback(&Dhcp6Client::NetHandler, this));

        dhcpInterface->socket = socket;

        // Add an IAID to the client interface.
        // Note: There may be multiple IAIDs per interface. We use only one.
        while (true)
        {
            uint32_t iaid = m_iaidStream->GetInteger();
            if (std::find(existingIaNaIds.begin(), existingIaNaIds.end(), iaid) ==
                existingIaNaIds.end())
            {
                dhcpInterface->iaids.push_back(iaid);
                existingIaNaIds.emplace_back(iaid);
                break;
            }
        }

        // int32_t interfaceId = ipv6->GetInterfaceForDevice(m_device);
        Ptr<Icmpv6L4Protocol> icmpv6 = DynamicCast<Icmpv6L4Protocol>(
            ipv6->GetProtocol(Icmpv6L4Protocol::GetStaticProtocolNumber(), ifIndex));

        // If the RA message contains an M flag, the client starts sending Solicits.
        icmpv6->TraceConnectWithoutContext("StartDhcpv6",
                                           MakeCallback(&Dhcp6Client::ReceiveMflag, this));

        device->AddLinkChangeCallback(MakeCallback(&Dhcp6Client::LinkStateHandler, this, device));
        m_interfaces[device] = dhcpInterface;
    }
}

void
Dhcp6Client::ReceiveMflag(uint32_t recvInterface)
{
    NS_LOG_FUNCTION(this);
    for (const auto& itr : m_interfaces)
    {
        Ptr<NetDevice> device = itr.first;
        Ptr<InterfaceConfig> dhcpInterface = itr.second;

        Ptr<Node> node = device->GetNode();
        Ptr<Ipv6> ipv6 = node->GetObject<Ipv6>();
        Ptr<Ipv6L3Protocol> ipv6l3 = node->GetObject<Ipv6L3Protocol>();
        uint32_t interface = ipv6->GetInterfaceForDevice(device);

        // Check that RA was received on this interface.
        if (interface == recvInterface)
        {
            // Introduce a random delay before sending the Solicit message.
            Simulator::Schedule(Time(MilliSeconds(m_solicitJitter->GetValue())),
                                &Dhcp6Client::Boot,
                                this,
                                device);

            NS_LOG_INFO("time " << dhcpInterface->solicitInterval.GetSeconds());
            uint32_t minInterval = dhcpInterface->solicitInterval.GetSeconds() / 2;
            dhcpInterface->solicitTimer = TrickleTimer(Seconds(minInterval), 4, 1);
            dhcpInterface->solicitTimer.SetFunction(&Dhcp6Client::Boot, this);
            dhcpInterface->solicitTimer.Enable();
            break;
        }
    }
}

void
Dhcp6Client::Boot(Ptr<NetDevice> device)
{
    Ptr<InterfaceConfig> dhcpInterface = m_interfaces[device];

    Ptr<Node> node = device->GetNode();
    uint32_t nApplications = node->GetNApplications();
    bool validDuid = false;

    for (uint32_t i = 0; i < nApplications; i++)
    {
        Ptr<Dhcp6Client> client = DynamicCast<Dhcp6Client>(node->GetApplication(i));
        if (client)
        {
            Duid clientDuid = client->GetSelfDuid();
            if (!clientDuid.IsInvalid())
            {
                validDuid = true;
                m_clientDuid = clientDuid;
                break;
            }
        }
    }

    if (!validDuid)
    {
        m_clientDuid.Initialize(node);
    }

    Dhcp6Header header;
    Ptr<Packet> packet = Create<Packet>();

    // Create a unique transaction ID.
    dhcpInterface->transactId = static_cast<uint32_t>(m_transactionId->GetValue());

    header.SetTransactId(dhcpInterface->transactId);
    header.SetMessageType(Dhcp6Header::SOLICIT);

    // Store start time of the message exchange.
    dhcpInterface->msgStartTime = Simulator::Now();

    header.AddElapsedTime(0);
    header.AddClientIdentifier(m_clientDuid);
    header.AddOptionRequest(Dhcp6Header::OPTION_SOL_MAX_RT);

    // Add IA_NA option.

    for (auto iaid : dhcpInterface->iaids)
    {
        header.AddIanaOption(iaid,
                             dhcpInterface->renew.GetSeconds(),
                             dhcpInterface->rebind.GetSeconds());
    }

    packet->AddHeader(header);

    if ((dhcpInterface->socket->SendTo(packet,
                                       0,
                                       Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(),
                                                          Dhcp6Header::SERVER_PORT))) >= 0)
    {
        NS_LOG_INFO("DHCPv6 client: Solicit sent");
    }
    else
    {
        NS_LOG_INFO("DHCPv6 client: Error while sending Solicit");
    }

    dhcpInterface->state = WAIT_ADVERTISE;
}

void
Dhcp6Client::StopApplication()
{
    NS_LOG_FUNCTION(this);

    m_interfaces.clear();
}

// void
// Dhcp6Client::Print(std::ostream& os) const
// {
//     os << "(state=" << GetState() << ")";
// }

// std::ostream&
// operator<<(std::ostream& os, const Dhcp6Client& h)
// {
//     h.Print(os);
//     return os;
// }

} // namespace ns3
