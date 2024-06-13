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
                                          MakeTimeChecker())
                            .AddAttribute("AddressPool",
                                          "Pool of addresses to provide on request.",
                                          Ipv6AddressValue(),
                                          MakeIpv6AddressAccessor(&Dhcp6Server::m_addressPool),
                                          MakeIpv6AddressChecker())
                            .AddAttribute("Prefix",
                                          "Prefix of the pool of addresses.",
                                          Ipv6PrefixValue(),
                                          MakeIpv6PrefixAccessor(&Dhcp6Server::m_prefix),
                                          MakeIpv6PrefixChecker())
                            .AddAttribute("MinAddress",
                                          "First address in the pool",
                                          Ipv6PrefixValue(),
                                          MakeIpv6AddressAccessor(&Dhcp6Server::m_minAddress),
                                          MakeIpv6AddressChecker())
                            .AddAttribute("MaxAddress",
                                          "Final address in the pool.",
                                          Ipv6PrefixValue(),
                                          MakeIpv6AddressAccessor(&Dhcp6Server::m_maxAddress),
                                          MakeIpv6AddressChecker());
    return tid;
}

Dhcp6Server::Dhcp6Server()
{
    NS_LOG_FUNCTION(this);
}

void
Dhcp6Server::AddAddressPool(Ipv6Address pool,
                            Ipv6Prefix prefix,
                            Ipv6Address minAddress,
                            Ipv6Address maxAddress)
{
    NS_LOG_FUNCTION(this << pool << prefix << minAddress << maxAddress);

    auto itr = availablePools.find(pool);

    if (itr != availablePools.end())
    {
        NS_LOG_INFO("Address pool already exists.");
    }
    else
    {
        availablePools[pool] = std::make_pair(minAddress, maxAddress);
        availablePrefixes[pool] = prefix;
    }
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
    for (auto itr = availablePools.begin(); itr != availablePools.end(); itr++)
    {
        Ipv6Address pool = itr->first;
        Ipv6Prefix prefix = availablePrefixes[pool];
        Ipv6Address minAddress = itr->second.first;
        Ipv6Address maxAddress = itr->second.second;

        // TODO: Retrieve all existing IA_NA options.
        advertiseHeader.AddIanaOption(m_iaidCount, m_renew.GetSeconds(), m_rebind.GetSeconds());
        advertiseHeader.AddAddress(m_iaidCount,
                                   pool,
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
        // SendReply(iDev, header, senderAddr);
    }
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

    // Add the address pool to the DHCPv6 server.
    // TODO: Add multiple pools.
    AddAddressPool(m_addressPool, m_prefix, m_minAddress, m_maxAddress);

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_recvSocket = Socket::CreateSocket(node, tid);
    m_sendSocket = Socket::CreateSocket(node, tid);

    Inet6SocketAddress local =
        Inet6SocketAddress(Ipv6Address::GetAllNodesMulticast(), Dhcp6Server::PORT);
    m_recvSocket->Bind(local);
    m_recvSocket->SetRecvPktInfo(true);
    m_recvSocket->SetRecvCallback(MakeCallback(&Dhcp6Server::NetHandler, this));

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

} // namespace ns3
