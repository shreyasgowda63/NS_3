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

#include "dhcp6-client.h"

#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/ipv6-packet-info-tag.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv6.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Dhcp6Client");

TypeId
Dhcp6Client::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Dhcp6Client")
                            .SetParent<Application>()
                            .AddConstructor<Dhcp6Client>()
                            .SetGroupName("Internet-Apps");
    return tid;
}

Dhcp6Client::Dhcp6Client()
{
    NS_LOG_FUNCTION(this);
    m_firstBoot = true;

    m_solicitEvent = EventId();
    m_solicitInterval = Seconds(5);

    m_renew = Time(Seconds(10));
    m_rebind = Time(Seconds(20));
    m_prefLifetime = Time(Seconds(30));
    m_validLifetime = Time(Seconds(40));

    m_ianaIds = 0;
}

void
Dhcp6Client::DoDispose()
{
    NS_LOG_FUNCTION(this);

    m_solicitEvent.Cancel();
    m_renewEvent.Cancel();
    m_rebindEvent.Cancel();

    Application::DoDispose();
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

    m_clientTransactId = 456;
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
    // Current approach: Use the first available IA Address option in the
    // advertise message.
    std::list<IaOptions> ianaOptionsList = header.GetIanaOptions();
    IaOptions iaOpt = ianaOptionsList.front();
    IaAddressOption iaAddrOpt = iaOpt.m_iaAddressOption.front();

    requestHeader.AddIanaOption(iaOpt.GetIaid(), iaOpt.GetT1(), iaOpt.GetT2());
    requestHeader.AddAddress(iaOpt.GetIaid(),
                             iaAddrOpt.GetIaAddress(),
                             iaAddrOpt.GetPreferredLifetime(),
                             iaAddrOpt.GetValidLifetime());

    // Add Option Request.
    header.AddOptionRequest(Dhcp6Header::OPTION_SOL_MAX_RT);

    packet->AddHeader(requestHeader);

    // TODO: Handle server unicast option.

    // Send the request message.
    m_state = WAIT_REPLY;
    if (m_socket->SendTo(packet,
                         0,
                         Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(), DHCP_PEER_PORT)) >=
        0)
    {
        NS_LOG_INFO("DHCPv6 Request sent.");
    }
    else
    {
        NS_LOG_INFO("Error while sending DHCPv6 Request.");
    }
}

void
Dhcp6Client::AcceptReply(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress server)
{
    NS_LOG_INFO(this << iDev << header << server);
    m_state = RENEW;

    Ptr<Ipv6> ipv6 = GetNode()->GetObject<Ipv6>();
    int32_t ifIndex = ipv6->GetInterfaceForDevice(m_device);

    // Read IA_NA option.
    // Current approach: Use the first available IA Address option in the
    // advertise message.
    std::list<IaOptions> ianaOptionsList = header.GetIanaOptions();
    IaOptions iaOpt = ianaOptionsList.front();
    IaAddressOption iaAddrOpt = iaOpt.m_iaAddressOption.front();

    Ipv6Address offeredAddress = iaAddrOpt.GetIaAddress();

    NS_LOG_INFO("Offered address: " << offeredAddress);

    // TODO: In Linux, all leased addresses seem to be /128. Double-check this.
    ipv6->AddAddress(ifIndex, Ipv6InterfaceAddress(offeredAddress, 128));
    ipv6->SetUp(ifIndex);

    // Add the IPv6 address - IAID association.
    m_iaidMap[offeredAddress] = iaOpt.GetIaid();

    // Set the renew timer.
    m_renew = Time(Seconds(iaOpt.GetT1()));
    m_renewEvent = Simulator::Schedule(m_renew, &Dhcp6Client::SendRenew, this, offeredAddress);

    m_rebind = Time(Seconds(iaOpt.GetT2()));
    m_rebindEvent = Simulator::Schedule(m_rebind, &Dhcp6Client::SendRebind, this, offeredAddress);

    // Set the preferred and valid lifetimes.
    m_prefLifetime = Time(Seconds(iaAddrOpt.GetPreferredLifetime()));
    m_validLifetime = Time(Seconds(iaAddrOpt.GetValidLifetime()));

    m_releaseEvent =
        Simulator::Schedule(m_validLifetime, &Dhcp6Client::SendRelease, this, offeredAddress);
}

void
Dhcp6Client::SendRenew(Ipv6Address address)
{
    NS_LOG_FUNCTION(this);

    Dhcp6Header header;
    Ptr<Packet> packet;
    packet = Create<Packet>();

    m_clientTransactId = 789;
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

    // IA_NA option, IA address option
    uint32_t iaid = m_iaidMap[address];
    header.AddIanaOption(iaid, m_renew.GetSeconds(), m_rebind.GetSeconds());
    header.AddAddress(iaid, address, 40, 60);

    // Add Option Request option.
    header.AddOptionRequest(Dhcp6Header::OPTION_SOL_MAX_RT);

    packet->AddHeader(header);
    if ((m_socket->SendTo(
            packet,
            0,
            Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(), DHCP_PEER_PORT))) >= 0)
    {
        NS_LOG_INFO("DHCP Renew sent");
    }
    else
    {
        NS_LOG_INFO("Error while sending DHCP Renew");
    }

    m_state = WAIT_REPLY;
}

void
Dhcp6Client::SendRebind(Ipv6Address address)
{
    NS_LOG_FUNCTION(this);

    Dhcp6Header header;
    Ptr<Packet> packet;
    packet = Create<Packet>();

    m_clientTransactId = 789;
    header.SetTransactId(m_clientTransactId);
    header.SetMessageType(Dhcp6Header::REBIND);

    // Add client identifier option
    header.AddClientIdentifier(m_clientIdentifier.GetHardwareType(),
                               m_clientIdentifier.GetLinkLayerAddress());

    Time m_msgStartTime = Simulator::Now();
    header.AddElapsedTime(0);

    // IA_NA option, IA address option
    uint32_t iaid = m_iaidMap[address];
    header.AddIanaOption(iaid, m_renew.GetSeconds(), m_rebind.GetSeconds());
    header.AddAddress(iaid, address, 40, 60);

    // Add Option Request option.
    header.AddOptionRequest(Dhcp6Header::OPTION_SOL_MAX_RT);

    packet->AddHeader(header);
    if ((m_socket->SendTo(
            packet,
            0,
            Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(), DHCP_PEER_PORT))) >= 0)
    {
        NS_LOG_INFO("DHCP Rebind sent");
    }
    else
    {
        NS_LOG_INFO("Error while sending DHCP Rebind");
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

    m_clientTransactId = 789;
    header.SetTransactId(m_clientTransactId);
    header.SetMessageType(Dhcp6Header::REBIND);

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
    header.AddAddress(iaid, address, 40, 60);

    packet->AddHeader(header);
    if ((m_socket->SendTo(
            packet,
            0,
            Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(), DHCP_PEER_PORT))) >= 0)
    {
        NS_LOG_INFO("DHCP Release sent");
    }
    else
    {
        NS_LOG_INFO("Error while sending DHCP Release");
    }

    m_state = WAIT_REPLY;
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
        NS_LOG_INFO("Received Advertise.");
        m_solicitEvent.Cancel();
        ValidateAdvertise(header);
        SendRequest(iDev, header, senderAddr);
    }
    if (m_state == WAIT_REPLY && header.GetMessageType() == Dhcp6Header::REPLY)
    {
        NS_LOG_INFO("Received Reply.");
        m_renewEvent.Cancel();
        m_rebindEvent.Cancel();
        AcceptReply(iDev, header, senderAddr);
    }
}

void
Dhcp6Client::LinkStateHandler()
{
    NS_LOG_FUNCTION(this);

    if (m_device->IsLinkUp())
    {
        NS_LOG_INFO("Link up at " << Simulator::Now().As(Time::S));
        m_socket->SetRecvCallback(MakeCallback(&Dhcp6Client::NetHandler, this));
        StartApplication();
    }
    else
    {
        m_solicitEvent.Cancel();
        m_renewEvent.Cancel();
        m_rebindEvent.Cancel();
        NS_LOG_INFO("Link down at " << Simulator::Now().As(Time::S));
    }
}

void
Dhcp6Client::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (m_socket)
    {
        NS_ABORT_MSG("DHCPv6 daemon is not meant to be started repeatedly.");
    }

    Ptr<Node> node = m_device->GetNode();
    Ptr<Ipv6> ipv6 = node->GetObject<Ipv6>();
    uint32_t interface = ipv6->GetInterfaceForDevice(m_device);

    if (interface < 0)
    {
        NS_ABORT_MSG("DHCPv6 daemon must have a link-local address.");
    }

    if (!m_socket)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(node, tid);

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

        m_socket->Bind(Inet6SocketAddress(linkLocal, DHCP_CLIENT_PORT));
        m_socket->BindToNetDevice(m_device);
        m_socket->Bind6();
        m_socket->SetRecvPktInfo(true);
    }
    m_socket->SetRecvCallback(MakeCallback(&Dhcp6Client::NetHandler, this));

    if (m_firstBoot)
    {
        m_device->AddLinkChangeCallback(MakeCallback(&Dhcp6Client::LinkStateHandler, this));
        m_firstBoot = false;
    }

    m_clientIdentifier.SetHardwareType(1);
    m_clientIdentifier.SetLinkLayerAddress(m_device->GetAddress());

    Boot();
}

void
Dhcp6Client::Boot()
{
    Dhcp6Header header;
    Ptr<Packet> packet;
    packet = Create<Packet>();

    // Retrieve link layer address of the device.
    m_clientTransactId = 123;
    header.SetTransactId(m_clientTransactId);
    header.SetMessageType(Dhcp6Header::SOLICIT);

    // Store start time of the message exchange.
    Time m_msgStartTime = Simulator::Now();

    header.AddElapsedTime(0);
    header.AddClientIdentifier(m_clientIdentifier.GetHardwareType(),
                               m_clientIdentifier.GetLinkLayerAddress());
    header.AddOptionRequest(Dhcp6Header::OPTION_SOL_MAX_RT);

    // Add IA_NA option.
    m_ianaIds += 1;
    header.AddIanaOption(m_ianaIds, m_renew.GetSeconds(), m_rebind.GetSeconds());

    packet->AddHeader(header);

    if ((m_socket->SendTo(
            packet,
            0,
            Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(), DHCP_PEER_PORT))) >= 0)
    {
        NS_LOG_INFO("DHCP Solicit sent");
    }
    else
    {
        NS_LOG_INFO("Error while sending DHCP Solicit");
    }

    m_state = WAIT_ADVERTISE;
    m_solicitEvent = Simulator::Schedule(m_solicitInterval, &Dhcp6Client::Boot, this);
}

void
Dhcp6Client::StopApplication()
{
    NS_LOG_FUNCTION(this);

    m_solicitEvent.Cancel();

    if (m_socket)
    {
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
}

} // namespace ns3
