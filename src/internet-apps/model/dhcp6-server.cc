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
Dhcp6Server::SetDhcp6ServerNetDevice(Ptr<NetDevice> netDevice)
{
    m_device = netDevice;
}

void
Dhcp6Server::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Application::DoDispose();
}

void
Dhcp6Server::NetHandler(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    Dhcp6Header header;
    Ptr<Packet> packet = nullptr;
    Address from;
    packet = m_socket->RecvFrom(from);

    // InetSocketAddress senderAddr = InetSocketAddress::ConvertFrom(from);

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
        // SendAdvertise(iDev, header, senderAddr);
    }
    if (header.GetMessageType() == Dhcp6Header::REQUEST)
    {
        // SendReply(iDev, header, senderAddr);
    }
}

void
Dhcp6Server::StartApplication()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Starting DHCPv6 server.");
    // TODO: Check that address ranges are valid.

    if (m_socket)
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
    m_socket = Socket::CreateSocket(node, tid);

    Inet6SocketAddress local =
        Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(), Dhcp6Server::PORT);
    m_socket->Bind(local);
    m_socket->SetRecvPktInfo(true);
    m_socket->SetRecvCallback(MakeCallback(&Dhcp6Server::NetHandler, this));
}

void
Dhcp6Server::StopApplication()
{
    NS_LOG_FUNCTION(this);

    if (m_socket)
    {
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }

    m_leasedAddresses.clear();
    m_declinedAddresses.clear();
}

} // namespace ns3
