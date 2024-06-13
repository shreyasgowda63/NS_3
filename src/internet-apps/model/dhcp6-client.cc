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
Dhcp6Client::NetHandler(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    Address from;
    Ptr<Packet> packet = m_socket->RecvFrom(from);
    Dhcp6Header header;
    if (packet->RemoveHeader(header) == 0)
    {
        return;
    }
    if (m_state == WAIT_ADVERTISE && header.GetMessageType() == Dhcp6Header::ADVERTISE)
    {
        NS_LOG_INFO("Received Advertise");
    }
    if (m_state == WAIT_REPLY && header.GetMessageType() == Dhcp6Header::REPLY)
    {
        // AcceptReply(header, from);
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
        m_socket->BindToNetDevice(m_device);
        m_socket->Bind6();
    }
    m_socket->SetRecvCallback(MakeCallback(&Dhcp6Client::NetHandler, this));

    if (m_firstBoot)
    {
        m_device->AddLinkChangeCallback(MakeCallback(&Dhcp6Client::LinkStateHandler, this));
        m_firstBoot = false;
    }
    Boot();
}

void
Dhcp6Client::Boot()
{
    Dhcp6Header header;
    Ptr<Packet> packet;
    packet = Create<Packet>();

    // Retrieve link layer address of the device.
    auto linkLayer = m_device->GetAddress();
    header.SetTransactId(123);
    header.SetMessageType(Dhcp6Header::SOLICIT);
    header.AddElapsedTime(0);
    header.AddClientIdentifier(1234, linkLayer);
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
