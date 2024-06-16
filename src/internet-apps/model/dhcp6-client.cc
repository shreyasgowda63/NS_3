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
}

void
Dhcp6Client::DoDispose()
{
    NS_LOG_FUNCTION(this);
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

    requestHeader.AddElapsedTime(0);

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

    packet->AddHeader(requestHeader);
    // TODO: Add OPTION_REQUEST Option.
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
        ValidateAdvertise(header);
        SendRequest(iDev, header, senderAddr);
    }
    if (m_state == WAIT_REPLY && header.GetMessageType() == Dhcp6Header::REPLY)
    {
        NS_LOG_INFO("Received Reply.");
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
            NS_LOG_INFO("addr index" << addrIndex);
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
    header.AddElapsedTime(0);
    header.AddClientIdentifier(m_clientIdentifier.GetHardwareType(),
                               m_clientIdentifier.GetLinkLayerAddress());
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
}

void
Dhcp6Client::StopApplication()
{
    NS_LOG_FUNCTION(this);

    if (m_socket)
    {
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
}

} // namespace ns3
