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

#include "ns3/address-utils.h"
#include "ns3/icmpv6-l4-protocol.h"
#include "ns3/ipv6-interface.h"
#include "ns3/ipv6-packet-info-tag.h"
#include "ns3/ipv6.h"
#include "ns3/log.h"
#include "ns3/loopback-net-device.h"
#include "ns3/mac48-address.h"
#include "ns3/object.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/string.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/traced-value.h"

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
            .SetGroupName("Internet-Apps")
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
            .AddTraceSource("NewLease",
                            "Get a new lease",
                            MakeTraceSourceAccessor(&Dhcp6Client::m_newLease),
                            "ns3::Ipv6Address::TracedCallback")
            .AddAttribute("RenewTime",
                          "Time after which client should renew. 1000 seconds by "
                          "default, set to 10 seconds here.",
                          TimeValue(Seconds(10)),
                          MakeTimeAccessor(&Dhcp6Client::m_renew),
                          MakeTimeChecker())
            .AddAttribute("RebindTime",
                          "Time after which client should rebind. 2000 seconds by "
                          "default, set to 20 seconds here.",
                          TimeValue(Seconds(20)),
                          MakeTimeAccessor(&Dhcp6Client::m_rebind),
                          MakeTimeChecker())
            .AddAttribute("PreferredLifetime",
                          "The preferred lifetime of the leased address. 3000 "
                          "seconds by default, set to 30 seconds here.",
                          TimeValue(Seconds(30)),
                          MakeTimeAccessor(&Dhcp6Client::m_prefLifetime),
                          MakeTimeChecker())
            .AddAttribute("ValidLifetime",
                          "Time after which client should release the address. "
                          "4000 seconds by default, set to 40 seconds here.",
                          TimeValue(Seconds(40)),
                          MakeTimeAccessor(&Dhcp6Client::m_validLifetime),
                          MakeTimeChecker())
            .AddAttribute("SolicitInterval",
                          "Time after which the client resends the Solicit. ",
                          TimeValue(Seconds(5)),
                          MakeTimeAccessor(&Dhcp6Client::m_solicitInterval),
                          MakeTimeChecker());
    return tid;
}

Dhcp6Client::Dhcp6Client()
{
    NS_LOG_FUNCTION(this);

    m_solicitEvent = EventId();
}

void
Dhcp6Client::DoDispose()
{
    NS_LOG_FUNCTION(this);

    m_device = nullptr;

    m_solicitEvent.Cancel();
    m_renewEvent.Cancel();
    m_rebindEvent.Cancel();

    for (auto itr : m_releaseEvent)
    {
        itr.Cancel();
    }

    Application::DoDispose();
}

int64_t
Dhcp6Client::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_solicitJitter->SetStream(stream);
    m_transactionId->SetStream(stream + 1);
    return 2;
}

void
Dhcp6Client::SetDhcp6ClientNetDevice(Ptr<NetDevice> netDevice)
{
    m_device = netDevice;
}

void
Dhcp6Client::ValidateAdvertise(Dhcp6Header header)
{
    NS_LOG_INFO(this << header);

    Ptr<Packet> packet = Create<Packet>();
    uint32_t receivedTransactId = header.GetTransactId();
    NS_ASSERT_MSG(receivedTransactId == m_clientTransactId, "Transaction ID mismatch.");

    uint16_t clientHwType = header.GetClientIdentifier().GetHardwareType();
    Address clientHwAddr = header.GetClientIdentifier().GetLinkLayerAddress();

    NS_ASSERT_MSG(clientHwType == m_clientIdentifier.GetHardwareType(),
                  "Client DUID hardware type mismatch.");
    NS_ASSERT_MSG(clientHwAddr == m_clientIdentifier.GetLinkLayerAddress(),
                  "Client DUID link layer address mismatch.");

    uint16_t serverHwType = header.GetServerIdentifier().GetHardwareType();
    Address serverHwAddr = header.GetServerIdentifier().GetLinkLayerAddress();
    m_serverIdentifier.SetHardwareType(serverHwType);
    m_serverIdentifier.SetLinkLayerAddress(serverHwAddr);
}

void
Dhcp6Client::SendRequest(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress server)
{
    NS_LOG_INFO(this << iDev << header << server);

    Ptr<Packet> packet = Create<Packet>();
    Dhcp6Header requestHeader;
    requestHeader.ResetOptions();
    requestHeader.SetMessageType(Dhcp6Header::REQUEST);

    // TODO: Use min, max for GetValue
    m_clientTransactId = (uint32_t)(m_transactionId->GetValue());
    requestHeader.SetTransactId(m_clientTransactId);

    // Add Client Identifier Option.
    requestHeader.AddClientIdentifier(m_clientIdentifier.GetHardwareType(),
                                      m_clientIdentifier.GetLinkLayerAddress());

    // Add Server Identifier Option, copied from the received header.
    uint16_t serverHardwareType = header.GetServerIdentifier().GetHardwareType();
    Address serverAddress = header.GetServerIdentifier().GetLinkLayerAddress();
    requestHeader.AddServerIdentifier(serverHardwareType, serverAddress);

    // Add Elapsed Time Option.
    uint16_t now = (uint16_t)Simulator::Now().GetMilliSeconds() / 10; // expressed in 0.01 seconds
    uint16_t elapsed = now - (uint16_t)m_msgStartTime.GetMilliSeconds() / 10;
    requestHeader.AddElapsedTime(elapsed);

    // Add IA_NA option.
    // Request all addresses from the Advertise message.
    std::list<IaOptions> ianaOptionsList = header.GetIanaOptions();

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
    m_state = WAIT_REPLY;
    if (m_socket->SendTo(
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

void
Dhcp6Client::AcceptedAddress(const Ipv6Address& offeredAddress)
{
    NS_LOG_INFO("Accepting " << offeredAddress);
    m_acceptedAddresses += 1;

    // Notify the new lease.
    m_newLease(offeredAddress);

    if (m_declinedAddresses.size() + m_acceptedAddresses == m_offeredAddresses)
    {
        DeclineOffer();
    }
}

void
Dhcp6Client::AddDeclinedAddress(const Ipv6Address& offeredAddress)
{
    m_declinedAddresses.push_back(offeredAddress);

    if (m_declinedAddresses.size() + m_acceptedAddresses == m_offeredAddresses)
    {
        DeclineOffer();
    }
}

void
Dhcp6Client::DeclineOffer()
{
    if (m_declinedAddresses.empty())
    {
        return;
    }

    // Cancel all scheduled Release, Renew, Rebind events.
    m_renewEvent.Cancel();
    m_rebindEvent.Cancel();
    for (auto itr : m_releaseEvent)
    {
        itr.Cancel();
    }

    Dhcp6Header declineHeader;
    Ptr<Packet> packet;
    packet = Create<Packet>();

    // Remove address associations.
    for (uint32_t i = 0; i < m_declinedAddresses.size(); i++)
    {
        Ipv6Address offer = m_declinedAddresses[i];
        uint32_t iaid = m_iaidMap[offer];

        // IA_NA option, IA address option
        declineHeader.AddIanaOption(iaid, m_renew.GetSeconds(), m_rebind.GetSeconds());
        declineHeader.AddAddress(iaid,
                                 offer,
                                 m_prefLifetime.GetSeconds(),
                                 m_validLifetime.GetSeconds());
    }

    m_clientTransactId = (uint32_t)(m_transactionId->GetValue());
    declineHeader.SetTransactId(m_clientTransactId);
    declineHeader.SetMessageType(Dhcp6Header::DECLINE);

    // Add client identifier option
    declineHeader.AddClientIdentifier(m_clientIdentifier.GetHardwareType(),
                                      m_clientIdentifier.GetLinkLayerAddress());

    // Add server identifier option
    declineHeader.AddServerIdentifier(m_serverIdentifier.GetHardwareType(),
                                      m_serverIdentifier.GetLinkLayerAddress());

    Time m_msgStartTime = Simulator::Now();
    declineHeader.AddElapsedTime(0);

    packet->AddHeader(declineHeader);
    if ((m_socket->SendTo(packet,
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

    m_state = WAIT_REPLY_AFTER_DECLINE;
}

void
Dhcp6Client::CheckLeaseStatus(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress server)
{
    NS_LOG_INFO(this << iDev << header << server);

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
    NS_LOG_INFO(this << iDev << header << server);

    Ptr<Ipv6> ipv6 = GetNode()->GetObject<Ipv6>();
    int32_t ifIndex = ipv6->GetInterfaceForDevice(m_device);

    // Read IA_NA options.
    std::list<IaOptions> ianaOptionsList = header.GetIanaOptions();

    m_declinedAddresses.clear();
    m_addressDadComplete = false;

    Time earliestRebind = Time(Seconds(1000000));
    Time earliestRenew = Time(Seconds(1000000));
    std::vector<uint32_t> iaidList;

    for (const auto& iaOpt : ianaOptionsList)
    {
        // Iterate through the offered addresses.
        // Current approach: Try to accept all offers.
        for (const auto& iaAddrOpt : iaOpt.m_iaAddressOption)
        {
            Ipv6Address offeredAddress = iaAddrOpt.GetIaAddress();
            NS_LOG_INFO("Offered address: " << offeredAddress);

            // TODO: In Linux, all leased addresses seem to be /128. Double-check this.
            Ipv6InterfaceAddress addr(offeredAddress, 128);
            ipv6->AddAddress(ifIndex, addr);
            ipv6->SetUp(ifIndex);

            // Set the preferred and valid lifetimes.
            m_prefLifetime = Time(Seconds(iaAddrOpt.GetPreferredLifetime()));
            m_validLifetime = Time(Seconds(iaAddrOpt.GetValidLifetime()));

            // Add the IPv6 address - IAID association.
            m_iaidMap[offeredAddress] = iaOpt.GetIaid();

            // TODO: Check whether Release event happens for each address.
            m_releaseEvent.push_back(Simulator::Schedule(m_validLifetime,
                                                         &Dhcp6Client::SendRelease,
                                                         this,
                                                         offeredAddress));

            m_offeredAddresses += 1;
        }

        earliestRenew = std::min(earliestRenew, Time(Seconds(iaOpt.GetT1())));
        earliestRebind = std::min(earliestRebind, Time(Seconds(iaOpt.GetT2())));
        iaidList.push_back(iaOpt.GetIaid());
    }

    // The renew and rebind events are scheduled for the earliest time across
    // all IA_NA options. RFC 8415, Section 18.2.4.
    m_renew = earliestRenew;
    m_renewEvent = Simulator::Schedule(m_renew, &Dhcp6Client::SendRenew, this, iaidList);

    // Set the rebind timer and schedule the event.
    m_rebind = earliestRebind;
    m_rebindEvent = Simulator::Schedule(m_rebind, &Dhcp6Client::SendRebind, this, iaidList);

    int32_t interfaceId = ipv6->GetInterfaceForDevice(m_device);
    Ptr<Icmpv6L4Protocol> icmpv6 = DynamicCast<Icmpv6L4Protocol>(
        ipv6->GetProtocol(Icmpv6L4Protocol::GetStaticProtocolNumber(), interfaceId));

    // If DAD fails, the offer is declined.
    icmpv6->TraceConnectWithoutContext("DadFailure",
                                       MakeCallback(&Dhcp6Client::AddDeclinedAddress, this));

    icmpv6->TraceConnectWithoutContext("DadSuccess",
                                       MakeCallback(&Dhcp6Client::AcceptedAddress, this));
}

void
Dhcp6Client::SendRenew(std::vector<uint32_t> iaidList)
{
    NS_LOG_FUNCTION(this);

    Dhcp6Header header;
    Ptr<Packet> packet;
    packet = Create<Packet>();

    m_clientTransactId = (uint32_t)(m_transactionId->GetValue());

    header.SetTransactId(m_clientTransactId);
    header.SetMessageType(Dhcp6Header::RENEW);

    // Add client identifier option
    header.AddClientIdentifier(m_clientIdentifier.GetHardwareType(),
                               m_clientIdentifier.GetLinkLayerAddress());

    // Add server identifier option
    header.AddServerIdentifier(m_serverIdentifier.GetHardwareType(),
                               m_serverIdentifier.GetLinkLayerAddress());

    Time m_msgStartTime = Simulator::Now();
    header.AddElapsedTime(0);

    // Add IA_NA options.
    for (uint32_t i = 0; i < iaidList.size(); i++)
    {
        header.AddIanaOption(iaidList[i], m_renew.GetSeconds(), m_rebind.GetSeconds());

        // Iterate through the IPv6Address - IAID map, and add all addresses
        // that match the IAID to be renewed.
        for (const auto& itr : m_iaidMap)
        {
            Ipv6Address address = itr.first;
            uint32_t iaid = itr.second;
            if (iaid == iaidList[i])
            {
                header.AddAddress(iaidList[i],
                                  address,
                                  m_prefLifetime.GetSeconds(),
                                  m_validLifetime.GetSeconds());
            }
        }
    }

    // Add Option Request option.
    header.AddOptionRequest(Dhcp6Header::OPTION_SOL_MAX_RT);

    packet->AddHeader(header);
    if ((m_socket->SendTo(packet,
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

    m_state = WAIT_REPLY;
}

void
Dhcp6Client::SendRebind(std::vector<uint32_t> iaidList)
{
    NS_LOG_FUNCTION(this);

    Dhcp6Header header;
    Ptr<Packet> packet;
    packet = Create<Packet>();

    m_clientTransactId = (uint32_t)(m_transactionId->GetValue());

    header.SetTransactId(m_clientTransactId);
    header.SetMessageType(Dhcp6Header::REBIND);

    // Add client identifier option
    header.AddClientIdentifier(m_clientIdentifier.GetHardwareType(),
                               m_clientIdentifier.GetLinkLayerAddress());

    Time m_msgStartTime = Simulator::Now();
    header.AddElapsedTime(0);

    // Add IA_NA options.
    for (uint32_t i = 0; i < iaidList.size(); i++)
    {
        header.AddIanaOption(iaidList[i], m_renew.GetSeconds(), m_rebind.GetSeconds());
    }

    // Add Option Request option.
    header.AddOptionRequest(Dhcp6Header::OPTION_SOL_MAX_RT);

    packet->AddHeader(header);
    if ((m_socket->SendTo(packet,
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

    m_state = WAIT_REPLY;
}

void
Dhcp6Client::SendRelease(Ipv6Address address)
{
    NS_LOG_FUNCTION(this);

    Ptr<Ipv6> ipv6 = GetNode()->GetObject<Ipv6>();
    int32_t ifIndex = ipv6->GetInterfaceForDevice(m_device);
    ipv6->RemoveAddress(ifIndex, address);

    Dhcp6Header header;
    Ptr<Packet> packet;
    packet = Create<Packet>();

    m_clientTransactId = (uint32_t)(m_transactionId->GetValue());

    header.SetTransactId(m_clientTransactId);
    header.SetMessageType(Dhcp6Header::RELEASE);

    // Add client identifier option
    header.AddClientIdentifier(m_clientIdentifier.GetHardwareType(),
                               m_clientIdentifier.GetLinkLayerAddress());

    // Add server identifier option
    header.AddServerIdentifier(m_serverIdentifier.GetHardwareType(),
                               m_serverIdentifier.GetLinkLayerAddress());

    Time m_msgStartTime = Simulator::Now();
    header.AddElapsedTime(0);

    // IA_NA option, IA address option
    uint32_t iaid = m_iaidMap[address];
    header.AddIanaOption(iaid, m_renew.GetSeconds(), m_rebind.GetSeconds());
    header.AddAddress(iaid, address, m_prefLifetime.GetSeconds(), m_validLifetime.GetSeconds());

    packet->AddHeader(header);
    if ((m_socket->SendTo(packet,
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

    m_state = WAIT_REPLY_AFTER_RELEASE;
}

void
Dhcp6Client::NetHandler(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    Address from;
    Ptr<Packet> packet = m_socket->RecvFrom(from);
    Dhcp6Header header;

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
    if (m_state == WAIT_ADVERTISE && header.GetMessageType() == Dhcp6Header::ADVERTISE)
    {
        NS_LOG_INFO("DHCPv6 client: Received Advertise.");
        m_solicitEvent.Cancel();
        ValidateAdvertise(header);
        SendRequest(iDev, header, senderAddr);
    }
    if (m_state == WAIT_REPLY && header.GetMessageType() == Dhcp6Header::REPLY)
    {
        NS_LOG_INFO("DHCPv6 client: Received Reply.");

        m_renewEvent.Cancel();
        m_rebindEvent.Cancel();
        for (auto itr : m_releaseEvent)
        {
            itr.Cancel();
        }

        ProcessReply(iDev, header, senderAddr);
    }
    if ((m_state == WAIT_REPLY_AFTER_DECLINE || m_state == WAIT_REPLY_AFTER_RELEASE) &&
        (header.GetMessageType() == Dhcp6Header::REPLY))
    {
        NS_LOG_INFO("DHCPv6 client: Received Reply.");
        CheckLeaseStatus(iDev, header, senderAddr);
    }
}

std::vector<uint32_t>
Dhcp6Client::GetIaids()
{
    return m_iaNaIds;
}

void
Dhcp6Client::LinkStateHandler()
{
    NS_LOG_FUNCTION(this);

    if (m_device->IsLinkUp())
    {
        NS_LOG_INFO("DHCPv6 client: Link up at " << Simulator::Now().As(Time::S));
        m_socket->SetRecvCallback(MakeCallback(&Dhcp6Client::NetHandler, this));
        StartApplication();
    }
    else
    {
        m_solicitEvent.Cancel();
        m_renewEvent.Cancel();
        m_rebindEvent.Cancel();
        for (auto itr : m_releaseEvent)
        {
            itr.Cancel();
        }

        // Stop receiving on the socket.
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        NS_LOG_INFO("DHCPv6 client: Link down at " << Simulator::Now().As(Time::S));
    }
}

IdentifierOption
Dhcp6Client::GetDuid()
{
    return m_clientIdentifier;
}

void
Dhcp6Client::StartApplication()
{
    NS_LOG_FUNCTION(this);

    Ptr<Node> node = m_device->GetNode();
    NS_ASSERT_MSG(node, "Dhcp6Client::StartApplication: cannot get the node from the device.");

    Ptr<Ipv6> ipv6 = node->GetObject<Ipv6>();
    NS_ASSERT_MSG(ipv6, "Dhcp6Client::StartApplication: node does not have IPv6.");

    uint32_t interface = ipv6->GetInterfaceForDevice(m_device);
    NS_ASSERT_MSG(interface >= 0,
                  "Dhcp6Client::StartApplication: device is not connected to IPv6.");

    if (!m_socket)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(node, tid);
        NS_ASSERT_MSG(m_socket, "Dhcp6Client::StartApplication: can not create socket.");

        Ipv6Address linkLocal;
        for (uint32_t addrIndex = 0; addrIndex < ipv6->GetNAddresses(interface); addrIndex++)
        {
            Ipv6InterfaceAddress ifaceAddr = ipv6->GetAddress(interface, addrIndex);
            Ipv6Address addr = ifaceAddr.GetAddress();
            if (addr.IsLinkLocal())
            {
                linkLocal = addr;
                break;
            }
        }

        m_socket->Bind(Inet6SocketAddress(linkLocal, Dhcp6Header::CLIENT_PORT));
        m_socket->BindToNetDevice(m_device);
        m_socket->Bind6();
        m_socket->SetRecvPktInfo(true);
        m_socket->SetRecvCallback(MakeCallback(&Dhcp6Client::NetHandler, this));
        m_device->AddLinkChangeCallback(MakeCallback(&Dhcp6Client::LinkStateHandler, this));
    }

    uint32_t nApplications = node->GetNApplications();
    bool validDuid = false;

    for (uint32_t i = 0; i < nApplications; i++)
    {
        Ptr<Dhcp6Client> client = DynamicCast<Dhcp6Client>(node->GetApplication(i));
        if (client)
        {
            Address clientLinkLayer = client->GetDuid().GetLinkLayerAddress();

            if (!clientLinkLayer.IsInvalid())
            {
                validDuid = true;
                m_clientIdentifier.SetLinkLayerAddress(clientLinkLayer);
                break;
            }
        }
    }

    if (!validDuid)
    {
        uint32_t nInterfaces = node->GetNDevices();

        uint32_t maxAddressLength = 0;
        Address duidAddress;

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
                    duidAddress = address;
                }
            }
        }

        if (duidAddress.IsInvalid())
        {
            NS_ABORT_MSG("DHCPv6 client: No suitable NetDevice found for DUID, aborting.");
        }

        // Consider the link-layer address of the first NetDevice in the list.
        m_clientIdentifier.SetLinkLayerAddress(duidAddress);
    }

    // Create IAs to be used, and assign IAIDs.
    // Note: Each interface may have multiple IA_NAs, but here we use only one.

    if (m_iaNaIds.empty())
    {
        // IAID should be unique in the client. Get all existing IAIDs to ensure uniqueness.
        std::vector<uint32_t> existingIaNaIds;
        for (uint32_t i = 0; i < nApplications; i++)
        {
            Ptr<Dhcp6Client> client = DynamicCast<Dhcp6Client>(node->GetApplication(i));

            if (client != this)
            {
                std::vector<uint32_t> iaidList = client->GetIaids();

                existingIaNaIds.insert(existingIaNaIds.end(), iaidList.begin(), iaidList.end());
                NS_LOG_INFO("Existing IAIDs: " << iaidList.size());
            }
        }

        // Create a new IAID for the client.
        Ptr<RandomVariableStream> iaidStream = CreateObject<UniformRandomVariable>();
        iaidStream->SetAttribute("Min", DoubleValue(0.0));
        iaidStream->SetAttribute("Max", DoubleValue(100000.0));
        while (true)
        {
            uint32_t iaid = iaidStream->GetInteger();
            if (std::find(existingIaNaIds.begin(), existingIaNaIds.end(), iaid) ==
                existingIaNaIds.end())
            {
                m_iaNaIds.push_back(iaid);
                break;
            }
        }

        NS_LOG_INFO("IAID Count " << m_iaNaIds.size());
        for (auto item : m_iaNaIds)
        {
            NS_LOG_INFO("IAID: " << item);
        }
    }

    // Introduce a random delay before sending the Solicit message.
    Simulator::Schedule(Time(MilliSeconds(m_solicitJitter->GetValue())), &Dhcp6Client::Boot, this);
}

void
Dhcp6Client::Boot()
{
    Dhcp6Header header;
    Ptr<Packet> packet;
    packet = Create<Packet>();

    // Retrieve link layer address of the device.
    m_clientTransactId = (uint32_t)(m_transactionId->GetValue());

    header.SetTransactId(m_clientTransactId);
    header.SetMessageType(Dhcp6Header::SOLICIT);

    // Store start time of the message exchange.
    Time m_msgStartTime = Simulator::Now();

    header.AddElapsedTime(0);
    header.AddClientIdentifier(m_clientIdentifier.GetHardwareType(),
                               m_clientIdentifier.GetLinkLayerAddress());
    header.AddOptionRequest(Dhcp6Header::OPTION_SOL_MAX_RT);

    // Add IA_NA option.

    for (auto iaid : m_iaNaIds)
    {
        header.AddIanaOption(iaid, m_renew.GetSeconds(), m_rebind.GetSeconds());
    }

    packet->AddHeader(header);

    if ((m_socket->SendTo(packet,
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

    m_state = WAIT_ADVERTISE;
    m_solicitEvent = Simulator::Schedule(m_solicitInterval, &Dhcp6Client::Boot, this);
}

void
Dhcp6Client::StopApplication()
{
    NS_LOG_FUNCTION(this);

    m_solicitEvent.Cancel();
    m_renewEvent.Cancel();
    m_rebindEvent.Cancel();

    for (auto itr : m_releaseEvent)
    {
        itr.Cancel();
    }

    if (m_socket)
    {
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
}

} // namespace ns3
