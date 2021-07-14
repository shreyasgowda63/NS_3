/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
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
 * Based on
 *      NS-2 AODV model developed by the CMU/MONARCH group and optimized and
 *      tuned by Samir Das and Mahesh Marina, University of Cincinnati;
 *
 *      AODV-UU implementation by Erik Nordström of Uppsala University
 *      http://core.it.uu.se/core/index.php/AODV-UU
 *
 * Authors: Elena Buchatskaia <borovkovaes@iitp.ru>
 *          Pavel Boyko <boyko@iitp.ru>
 *          Nitya Chandra <nityachandra6@gmail.com>
 */
#define NS_LOG_APPEND_CONTEXT                                   \
  if (m_rProtocol->m_ipv4) { std::clog << "[node " << m_rProtocol->m_ipv4->GetObject<Node> ()->GetId () << "] "; }

#include "aodv-ipv4-routing-protocol.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/random-variable-stream.h"
#include "ns3/inet-socket-address.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/udp-header.h"
#include "ns3/wifi-net-device.h"
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/wifi-mac-queue-item.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include <algorithm>
#include <limits>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AodvIpv4RoutingProtocol");

namespace aodv {
NS_OBJECT_ENSURE_REGISTERED (Ipv4RoutingProtocol);

/// UDP Port for AODV control traffic
const uint32_t Ipv4RoutingProtocol::AODV_PORT = 654;

// /**
// * \ingroup aodv
// * \brief Tag used by AODV implementation
// */
//-----------------------------------------------------------------------------
Ipv4RoutingProtocol::Ipv4RoutingProtocol ()
{
}

TypeId
Ipv4RoutingProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::aodv::Ipv4RoutingProtocol")
    .SetParent<ns3::Ipv4RoutingProtocol> ()
    .SetGroupName ("Aodv")
    .AddConstructor<Ipv4RoutingProtocol> ()
  ;
  return tid;
}

Ipv4RoutingProtocol::~Ipv4RoutingProtocol ()
{
}

int64_t
Ipv4RoutingProtocol::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_rProtocol->m_uniformRandomVariable->SetStream (stream);
  return 1;
}

void
Ipv4RoutingProtocol::DoDispose ()
{
  ns3::Ipv4RoutingProtocol::DoDispose ();
  m_rProtocol=0;
}

void
Ipv4RoutingProtocol::PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
  *stream->GetStream () << "Node: " << m_rProtocol->m_ipv4->GetObject<Node> ()->GetId ()
                        << "; Time: " << Now ().As (unit)
                        << ", Local time: " << m_rProtocol->m_ipv4->GetObject<Node> ()->GetLocalTime ().As (unit)
                        << ", AODV Routing table" << std::endl;

  m_rProtocol->m_routingTable.Print (stream, unit);
  *stream->GetStream () << std::endl;
}

Ptr<Ipv4Route>
Ipv4RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header,
                              Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{
  NS_LOG_FUNCTION (this << header << (oif ? oif->GetIfIndex () : 0));
  if (!p)
    {
      NS_LOG_DEBUG ("Packet is == 0");
      return LoopbackRoute (header, oif); // later
    }
  if (m_rProtocol->m_socketAddresses.empty ())
    {
      sockerr = Socket::ERROR_NOROUTETOHOST;
      NS_LOG_LOGIC ("No aodv interfaces");
      Ptr<Ipv4Route> route;
      return route;
    }
  sockerr = Socket::ERROR_NOTERROR;
  Ptr<Ipv4Route> route;
  Ipv4Address dst = header.GetDestination ();
  RoutingTableEntry rt;
  if (m_rProtocol->m_routingTable.LookupValidRoute (dst, rt))
    {
      route = rt.GetRoute ();
      NS_ASSERT (route != 0);
      NS_LOG_DEBUG ("Exist route to " << route->GetDestination () << " from interface " << route->GetSource ());
      if (oif != 0 && route->GetOutputDevice () != oif)
        {
          NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
          sockerr = Socket::ERROR_NOROUTETOHOST;
          return Ptr<Ipv4Route> ();
        }
      m_rProtocol->UpdateRouteLifeTime (dst, m_rProtocol->m_activeRouteTimeout);
      m_rProtocol->UpdateRouteLifeTime (route->GetGateway (), m_rProtocol->m_activeRouteTimeout);
      return route;
    }

  // Valid route not found, in this case we return loopback.
  // Actual route request will be deferred until packet will be fully formed,
  // routed to loopback, received from loopback and passed to RouteInput (see below)
  uint32_t iif = (oif ? m_rProtocol->m_ipv4->GetInterfaceForDevice (oif) : -1);
  DeferredRouteOutputTag tag (iif);
  NS_LOG_DEBUG ("Valid Route not found");
  if (!p->PeekPacketTag (tag))
    {
      p->AddPacketTag (tag);
    }
  return LoopbackRoute (header, oif);
}

void
Ipv4RoutingProtocol::DeferredRouteOutput (Ptr<const Packet> p, const Ipv4Header & header,
                                      UnicastForwardCallback ucb, ErrorCallback ecb)
{
  NS_LOG_FUNCTION (this << p << header);
  NS_ASSERT (p != 0 && p != Ptr<Packet> ());

  QueueEntry newEntry (p, header, ucb, ecb);
  bool result = m_rProtocol->m_queue.Enqueue (newEntry);
  if (result)
    {
      NS_LOG_LOGIC ("Add packet " << p->GetUid () << " to queue. Protocol " << (uint16_t) header.GetProtocol ());
      RoutingTableEntry rt;
      bool result = m_rProtocol->m_routingTable.LookupRoute (header.GetDestination (), rt);
      if (!result || ((rt.GetFlag () != IN_SEARCH) && result))
        {
          NS_LOG_LOGIC ("Send new RREQ for outbound packet to " << header.GetDestination ());
          m_rProtocol->SendRequest (header.GetDestination ());
        }
    }
}

bool
Ipv4RoutingProtocol::RouteInput (Ptr<const Packet> p, const Ipv4Header &header,
                             Ptr<const NetDevice> idev, UnicastForwardCallback ucb,
                             MulticastForwardCallback mcb, LocalDeliverCallback lcb, ErrorCallback ecb)
{
  NS_LOG_FUNCTION (this << p->GetUid () << header.GetDestination () << idev->GetAddress ());
  if (m_rProtocol->m_socketAddresses.empty ())
    {
      NS_LOG_LOGIC ("No aodv interfaces");
      return false;
    }
  NS_ASSERT (m_rProtocol->m_ipv4 != 0);
  NS_ASSERT (p != 0);
  // Check if input device supports IP
  NS_ASSERT (m_rProtocol->m_ipv4->GetInterfaceForDevice (idev) >= 0);
  int32_t iif = m_rProtocol->m_ipv4->GetInterfaceForDevice (idev);

  Ipv4Address dst = header.GetDestination ();
  Ipv4Address origin = header.GetSource ();

  // Deferred route request
  if (idev == m_rProtocol->m_lo)
    {
      DeferredRouteOutputTag tag;
      if (p->PeekPacketTag (tag))
        {
          DeferredRouteOutput (p, header, ucb, ecb);
          return true;
        }
    }

  // Duplicate of own packet
  if (IsMyOwnAddress (origin))
    {
      return true;
    }

  // AODV is not a multicast routing protocol
  if (dst.IsMulticast ())
    {
      return false;
    }

  // Broadcast local delivery/forwarding
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_rProtocol->m_socketAddresses.begin (); j != m_rProtocol->m_socketAddresses.end (); ++j)
    {
      Ipv4InterfaceAddress iface = j->second;
      if (m_rProtocol->m_ipv4->GetInterfaceForAddress (iface.GetLocal ()) == iif)
        {
          if (dst == iface.GetBroadcast () || dst.IsBroadcast ())
            {
              if (m_rProtocol->m_dpd.IsDuplicate (p, header))
                {
                  NS_LOG_DEBUG ("Duplicated packet " << p->GetUid () << " from " << origin << ". Drop.");
                  return true;
                }
              m_rProtocol->UpdateRouteLifeTime (origin, m_rProtocol->m_activeRouteTimeout);
              Ptr<Packet> packet = p->Copy ();
              if (lcb.IsNull () == false)
                {
                  NS_LOG_LOGIC ("Broadcast local delivery to " << iface.GetLocal ());
                  lcb (p, header, iif);
                  // Fall through to additional processing
                }
              else
                {
                  NS_LOG_ERROR ("Unable to deliver packet locally due to null callback " << p->GetUid () << " from " << origin);
                  ecb (p, header, Socket::ERROR_NOROUTETOHOST);
                }
              if (!m_rProtocol->m_enableBroadcast)
                {
                  return true;
                }
              if (header.GetProtocol () == UdpL4Protocol::PROT_NUMBER)
                {
                  UdpHeader udpHeader;
                  p->PeekHeader (udpHeader);
                  if (udpHeader.GetDestinationPort () == AODV_PORT)
                    {
                      // AODV packets sent in broadcast are already managed
                      return true;
                    }
                }
              if (header.GetTtl () > 1)
                {
                  NS_LOG_LOGIC ("Forward broadcast. TTL " << (uint16_t) header.GetTtl ());
                  RoutingTableEntry toBroadcast;
                  if (m_rProtocol->m_routingTable.LookupRoute (dst, toBroadcast))
                    {
                      Ptr<Ipv4Route> route = toBroadcast.GetRoute ();
                      ucb (route, packet, header);
                    }
                  else
                    {
                      NS_LOG_DEBUG ("No route to forward broadcast. Drop packet " << p->GetUid ());
                    }
                }
              else
                {
                  NS_LOG_DEBUG ("TTL exceeded. Drop packet " << p->GetUid ());
                }
              return true;
            }
        }
    }

  // Unicast local delivery
  if (m_rProtocol->m_ipv4->IsDestinationAddress (dst, iif))
    {
      m_rProtocol->UpdateRouteLifeTime (origin, m_rProtocol->m_activeRouteTimeout);
      RoutingTableEntry toOrigin;
      if (m_rProtocol->m_routingTable.LookupValidRoute (origin, toOrigin))
        {
          m_rProtocol->UpdateRouteLifeTime (toOrigin.GetNextHop (), m_rProtocol->m_activeRouteTimeout);
          m_rProtocol->m_nb.Update (toOrigin.GetNextHop (), m_rProtocol->m_activeRouteTimeout);
        }
      if (lcb.IsNull () == false)
        {
          NS_LOG_LOGIC ("Unicast local delivery to " << dst);
          lcb (p, header, iif);
        }
      else
        {
          NS_LOG_ERROR ("Unable to deliver packet locally due to null callback " << p->GetUid () << " from " << origin);
          ecb (p, header, Socket::ERROR_NOROUTETOHOST);
        }
      return true;
    }

  // Check if input device supports IP forwarding
  if (m_rProtocol->m_ipv4->IsForwarding (iif) == false)
    {
      NS_LOG_LOGIC ("Forwarding disabled for this interface");
      ecb (p, header, Socket::ERROR_NOROUTETOHOST);
      return true;
    }

  // Forwarding
  return Forwarding (p, header, ucb, ecb);
}

bool
Ipv4RoutingProtocol::Forwarding (Ptr<const Packet> p, const Ipv4Header & header,
                             UnicastForwardCallback ucb, ErrorCallback ecb)
{
  NS_LOG_FUNCTION (this);
  Ipv4Address dst = header.GetDestination ();
  Ipv4Address origin = header.GetSource ();
  m_rProtocol->m_routingTable.Purge ();
  RoutingTableEntry toDst;
  if (m_rProtocol->m_routingTable.LookupRoute (dst, toDst))
    {
      if (toDst.GetFlag () == VALID)
        {
          Ptr<Ipv4Route> route = toDst.GetRoute ();
          NS_LOG_LOGIC (route->GetSource () << " forwarding to " << dst << " from " << origin << " packet " << p->GetUid ());

          /*
           *  Each time a route is used to forward a data packet, its Active Route
           *  Lifetime field of the source, destination and the next hop on the
           *  path to the destination is updated to be no less than the current
           *  time plus ActiveRouteTimeout.
           */
          m_rProtocol->UpdateRouteLifeTime (origin, m_rProtocol->m_activeRouteTimeout);
          m_rProtocol->UpdateRouteLifeTime (dst, m_rProtocol->m_activeRouteTimeout);
          m_rProtocol->UpdateRouteLifeTime (route->GetGateway (), m_rProtocol->m_activeRouteTimeout);
          /*
           *  Since the route between each originator and destination pair is expected to be symmetric, the
           *  Active Route Lifetime for the previous hop, along the reverse path back to the IP source, is also updated
           *  to be no less than the current time plus ActiveRouteTimeout
           */
          RoutingTableEntry toOrigin;
          m_rProtocol->m_routingTable.LookupRoute (origin, toOrigin);
          m_rProtocol->UpdateRouteLifeTime (toOrigin.GetNextHop (), m_rProtocol->m_activeRouteTimeout);

          m_rProtocol->m_nb.Update (route->GetGateway (), m_rProtocol->m_activeRouteTimeout);
          m_rProtocol->m_nb.Update (toOrigin.GetNextHop (), m_rProtocol->m_activeRouteTimeout);

          ucb (route, p, header);
          return true;
        }
      else
        {
          if (toDst.GetValidSeqNo ())
            {
              m_rProtocol->SendRerrWhenNoRouteToForward (dst, toDst.GetSeqNo (), origin);
              NS_LOG_DEBUG ("Drop packet " << p->GetUid () << " because no route to forward it.");
              return false;
            }
        }
    }
  NS_LOG_LOGIC ("route not found to " << dst << ". Send RERR message.");
  NS_LOG_DEBUG ("Drop packet " << p->GetUid () << " because no route to forward it.");
  m_rProtocol->SendRerrWhenNoRouteToForward (dst, 0, origin);
  return false;
}

void
Ipv4RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_ASSERT (ipv4 != 0);
  NS_ASSERT (m_rProtocol->m_ipv4 == 0);

  m_rProtocol->m_ipv4 = ipv4;

  // Create lo route. It is asserted that the only one interface up for now is loopback
  NS_ASSERT (m_rProtocol->m_ipv4->GetNInterfaces () == 1 && m_rProtocol->m_ipv4->GetAddress (0, 0).GetLocal () == Ipv4Address ("127.0.0.1"));
  m_rProtocol->m_lo = m_rProtocol->m_ipv4->GetNetDevice (0);
  NS_ASSERT (m_rProtocol->m_lo != 0);
  // Remember lo route
  RoutingTableEntry rt (/*device=*/ m_rProtocol->m_lo, /*dst=*/ Ipv4Address::GetLoopback (), /*know seqno=*/ true, /*seqno=*/ 0,
                                    /*iface=*/ Ipv4InterfaceAddress (Ipv4Address::GetLoopback (), Ipv4Mask ("255.0.0.0")),
                                    /*hops=*/ 1, /*next hop=*/ Ipv4Address::GetLoopback (),
                                    /*lifetime=*/ Simulator::GetMaximumSimulationTime ());
  m_rProtocol->m_routingTable.AddRoute (rt);

  Simulator::ScheduleNow (&RoutingProtocol::Start, m_rProtocol);
}

void
Ipv4RoutingProtocol::NotifyInterfaceUp (uint32_t i)
{
  NS_LOG_FUNCTION (this << m_rProtocol->m_ipv4->GetAddress (i, 0).GetLocal ());
  Ptr<Ipv4L3Protocol> l3 = m_rProtocol->m_ipv4->GetObject<Ipv4L3Protocol> ();
  if (l3->GetNAddresses (i) > 1)
    {
      NS_LOG_WARN ("AODV does not work with more then one address per each interface.");
    }
  Ipv4InterfaceAddress iface = l3->GetAddress (i, 0);
  if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
    {
      return;
    }

  // Create a socket to listen only on this interface
  Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                             UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&Ipv4RoutingProtocol::RecvAodv, this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (iface.GetLocal (), AODV_PORT));
  socket->SetAllowBroadcast (true);
  socket->SetIpRecvTtl (true);
  m_rProtocol->m_socketAddresses.insert (std::make_pair (socket, iface));

  // create also a subnet broadcast socket
  socket = Socket::CreateSocket (GetObject<Node> (),
                                 UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&Ipv4RoutingProtocol::RecvAodv, this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (iface.GetBroadcast (), AODV_PORT));
  socket->SetAllowBroadcast (true);
  socket->SetIpRecvTtl (true);
  m_rProtocol->m_socketSubnetBroadcastAddresses.insert (std::make_pair (socket, iface));

  // Add local broadcast record to the routing table
  Ptr<NetDevice> dev = m_rProtocol->m_ipv4->GetNetDevice (m_rProtocol->m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
  RoutingTableEntry rt (/*device=*/ dev, /*dst=*/ iface.GetBroadcast (), /*know seqno=*/ true, /*seqno=*/ 0, /*iface=*/ iface,
                                    /*hops=*/ 1, /*next hop=*/ iface.GetBroadcast (), /*lifetime=*/ Simulator::GetMaximumSimulationTime ());
  m_rProtocol->m_routingTable.AddRoute (rt);

  if (l3->GetInterface (i)->GetArpCache ())
    {
      m_rProtocol->m_nb.AddArpCache (l3->GetInterface (i)->GetArpCache ());
    }

  // Allow neighbor manager use this interface for layer 2 feedback if possible
  Ptr<WifiNetDevice> wifi = dev->GetObject<WifiNetDevice> ();
  if (wifi == 0)
    {
      return;
    }
  Ptr<WifiMac> mac = wifi->GetMac ();
  if (mac == 0)
    {
      return;
    }

  mac->TraceConnectWithoutContext ("DroppedMpdu", MakeCallback (&Ipv4RoutingProtocol::NotifyTxError, this));
}

void
Ipv4RoutingProtocol::NotifyTxError (WifiMacDropReason reason, Ptr<const WifiMacQueueItem> mpdu)
{
  m_rProtocol->m_nb.GetTxErrorCallback ()(mpdu->GetHeader ());
}

void
Ipv4RoutingProtocol::NotifyInterfaceDown (uint32_t i)
{
  NS_LOG_FUNCTION (this << m_rProtocol->m_ipv4->GetAddress (i, 0).GetLocal ());

  // Disable layer 2 link state monitoring (if possible)
  Ptr<Ipv4L3Protocol> l3 = m_rProtocol->m_ipv4->GetObject<Ipv4L3Protocol> ();
  Ptr<NetDevice> dev = l3->GetNetDevice (i);
  Ptr<WifiNetDevice> wifi = dev->GetObject<WifiNetDevice> ();
  if (wifi != 0)
    {
      Ptr<WifiMac> mac = wifi->GetMac ()->GetObject<AdhocWifiMac> ();
      if (mac != 0)
        {
          mac->TraceDisconnectWithoutContext ("DroppedMpdu",
                                              MakeCallback (&Ipv4RoutingProtocol::NotifyTxError, this));
          m_rProtocol->m_nb.DelArpCache (l3->GetInterface (i)->GetArpCache ());
        }
    }

  // Close socket
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (m_rProtocol->m_ipv4->GetAddress (i, 0));
  NS_ASSERT (socket);
  socket->Close ();
  m_rProtocol->m_socketAddresses.erase (socket);

  // Close socket
  socket = FindSubnetBroadcastSocketWithInterfaceAddress (m_rProtocol->m_ipv4->GetAddress (i, 0));
  NS_ASSERT (socket);
  socket->Close ();
  m_rProtocol->m_socketSubnetBroadcastAddresses.erase (socket);

  if (m_rProtocol->m_socketAddresses.empty ())
    {
      NS_LOG_LOGIC ("No aodv interfaces");
      m_rProtocol->m_htimer.Cancel ();
      m_rProtocol->m_nb.Clear ();
      m_rProtocol->m_routingTable.Clear ();
      return;
    }
  m_rProtocol->m_routingTable.DeleteAllRoutesFromInterface (m_rProtocol->m_ipv4->GetAddress (i, 0));
}

void
Ipv4RoutingProtocol::NotifyAddAddress (uint32_t i, Ipv4InterfaceAddress address)
{
  NS_LOG_FUNCTION (this << " interface " << i << " address " << address);
  Ptr<Ipv4L3Protocol> l3 = m_rProtocol->m_ipv4->GetObject<Ipv4L3Protocol> ();
  if (!l3->IsUp (i))
    {
      return;
    }
  if (l3->GetNAddresses (i) == 1)
    {
      Ipv4InterfaceAddress iface = l3->GetAddress (i, 0);
      Ptr<Socket> socket = FindSocketWithInterfaceAddress (iface);
      if (!socket)
        {
          if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
            {
              return;
            }
          // Create a socket to listen only on this interface
          Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                                     UdpSocketFactory::GetTypeId ());
          NS_ASSERT (socket != 0);
          socket->SetRecvCallback (MakeCallback (&Ipv4RoutingProtocol::RecvAodv,this));
          socket->BindToNetDevice (l3->GetNetDevice (i));
          socket->Bind (InetSocketAddress (iface.GetLocal (), AODV_PORT));
          socket->SetAllowBroadcast (true);
          m_rProtocol->m_socketAddresses.insert (std::make_pair (socket, iface));

          // create also a subnet directed broadcast socket
          socket = Socket::CreateSocket (GetObject<Node> (),
                                         UdpSocketFactory::GetTypeId ());
          NS_ASSERT (socket != 0);
          socket->SetRecvCallback (MakeCallback (&Ipv4RoutingProtocol::RecvAodv, this));
          socket->BindToNetDevice (l3->GetNetDevice (i));
          socket->Bind (InetSocketAddress (iface.GetBroadcast (), AODV_PORT));
          socket->SetAllowBroadcast (true);
          socket->SetIpRecvTtl (true);
          m_rProtocol->m_socketSubnetBroadcastAddresses.insert (std::make_pair (socket, iface));

          // Add local broadcast record to the routing table
          Ptr<NetDevice> dev = m_rProtocol->m_ipv4->GetNetDevice (
              m_rProtocol->m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
          RoutingTableEntry rt (/*device=*/ dev, /*dst=*/ iface.GetBroadcast (), /*know seqno=*/ true,
                                            /*seqno=*/ 0, /*iface=*/ iface, /*hops=*/ 1,
                                            /*next hop=*/ iface.GetBroadcast (), /*lifetime=*/ Simulator::GetMaximumSimulationTime ());
          m_rProtocol->m_routingTable.AddRoute (rt);
        }
    }
  else
    {
      NS_LOG_LOGIC ("AODV does not work with more then one address per each interface. Ignore added address");
    }
}

void
Ipv4RoutingProtocol::NotifyRemoveAddress (uint32_t i, Ipv4InterfaceAddress address)
{
  NS_LOG_FUNCTION (this);
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (address);
  if (socket)
    {
      m_rProtocol->m_routingTable.DeleteAllRoutesFromInterface (address);
      socket->Close ();
      m_rProtocol->m_socketAddresses.erase (socket);

      Ptr<Socket> unicastSocket = FindSubnetBroadcastSocketWithInterfaceAddress (address);
      if (unicastSocket)
        {
          unicastSocket->Close ();
          m_rProtocol->m_socketAddresses.erase (unicastSocket);
        }

      Ptr<Ipv4L3Protocol> l3 = m_rProtocol->m_ipv4->GetObject<Ipv4L3Protocol> ();
      if (l3->GetNAddresses (i))
        {
          Ipv4InterfaceAddress iface = l3->GetAddress (i, 0);
          // Create a socket to listen only on this interface
          Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                                     UdpSocketFactory::GetTypeId ());
          NS_ASSERT (socket != 0);
          socket->SetRecvCallback (MakeCallback (&Ipv4RoutingProtocol::RecvAodv, this));
          // Bind to any IP address so that broadcasts can be received
          socket->BindToNetDevice (l3->GetNetDevice (i));
          socket->Bind (InetSocketAddress (iface.GetLocal (), AODV_PORT));
          socket->SetAllowBroadcast (true);
          socket->SetIpRecvTtl (true);
          m_rProtocol->m_socketAddresses.insert (std::make_pair (socket, iface));

          // create also a unicast socket
          socket = Socket::CreateSocket (GetObject<Node> (),
                                         UdpSocketFactory::GetTypeId ());
          NS_ASSERT (socket != 0);
          socket->SetRecvCallback (MakeCallback (&Ipv4RoutingProtocol::RecvAodv, this));
          socket->BindToNetDevice (l3->GetNetDevice (i));
          socket->Bind (InetSocketAddress (iface.GetBroadcast (), AODV_PORT));
          socket->SetAllowBroadcast (true);
          socket->SetIpRecvTtl (true);
          m_rProtocol->m_socketSubnetBroadcastAddresses.insert (std::make_pair (socket, iface));

          // Add local broadcast record to the routing table
          Ptr<NetDevice> dev = m_rProtocol->m_ipv4->GetNetDevice (m_rProtocol->m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
          RoutingTableEntry rt (/*device=*/ dev, /*dst=*/ iface.GetBroadcast (), /*know seqno=*/ true, /*seqno=*/ 0, /*iface=*/ iface,
                                            /*hops=*/ 1, /*next hop=*/ iface.GetBroadcast (), /*lifetime=*/ Simulator::GetMaximumSimulationTime ());
          m_rProtocol->m_routingTable.AddRoute (rt);
        }
      if (m_rProtocol->m_socketAddresses.empty ())
        {
          NS_LOG_LOGIC ("No aodv interfaces");
          m_rProtocol->m_htimer.Cancel ();
          m_rProtocol->m_nb.Clear ();
          m_rProtocol->m_routingTable.Clear ();
          return;
        }
    }
  else
    {
      NS_LOG_LOGIC ("Remove address not participating in AODV operation");
    }
}

bool
Ipv4RoutingProtocol::IsMyOwnAddress (Ipv4Address src)
{
  NS_LOG_FUNCTION (this << src);
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_rProtocol->m_socketAddresses.begin (); j != m_rProtocol->m_socketAddresses.end (); ++j)
    {
      Ipv4InterfaceAddress iface = j->second;
      if (src == iface.GetLocal ())
        {
          return true;
        }
    }
  return false;
}

Ptr<Ipv4Route>
Ipv4RoutingProtocol::LoopbackRoute (const Ipv4Header & hdr, Ptr<NetDevice> oif) const
{
  NS_LOG_FUNCTION (this << hdr);
  NS_ASSERT (m_rProtocol->m_lo != 0);
  Ptr<Ipv4Route> rt = Create<Ipv4Route> ();
  rt->SetDestination (hdr.GetDestination ());
  //
  // Source address selection here is tricky.  The loopback route is
  // returned when AODV does not have a route; this causes the packet
  // to be looped back and handled (cached) in RouteInput() method
  // while a route is found. However, connection-oriented protocols
  // like TCP need to create an endpoint four-tuple (src, src port,
  // dst, dst port) and create a pseudo-header for checksumming.  So,
  // AODV needs to guess correctly what the eventual source address
  // will be.
  //
  // For single interface, single address nodes, this is not a problem.
  // When there are possibly multiple outgoing interfaces, the policy
  // implemented here is to pick the first available AODV interface.
  // If RouteOutput() caller specified an outgoing interface, that
  // further constrains the selection of source address
  //
  std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_rProtocol->m_socketAddresses.begin ();
  if (oif)
    {
      // Iterate to find an address on the oif device
      for (j = m_rProtocol->m_socketAddresses.begin (); j != m_rProtocol->m_socketAddresses.end (); ++j)
        {
          Ipv4Address addr = j->second.GetLocal ();
          int32_t interface = m_rProtocol->m_ipv4->GetInterfaceForAddress (addr);
          if (oif == m_rProtocol->m_ipv4->GetNetDevice (static_cast<uint32_t> (interface)))
            {
              rt->SetSource (addr);
              break;
            }
        }
    }
  else
    {
      rt->SetSource (j->second.GetLocal ());
    }
  NS_ASSERT_MSG (rt->GetSource () != Ipv4Address (), "Valid AODV source address not found");
  rt->SetGateway (Ipv4Address ("127.0.0.1"));
  rt->SetOutputDevice (m_rProtocol->m_lo);
  return rt;
}

void
Ipv4RoutingProtocol::SendTo (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination)
{
  socket->SendTo (packet, 0, InetSocketAddress (destination, AODV_PORT));
}

void
Ipv4RoutingProtocol::RecvAodv (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Address sourceAddress;
  Ptr<Packet> packet = socket->RecvFrom (sourceAddress);
  InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
  Ipv4Address sender = inetSourceAddr.GetIpv4 ();
  Ipv4Address receiver;

  if (m_rProtocol->m_socketAddresses.find (socket) != m_rProtocol->m_socketAddresses.end ())
    {
      receiver = m_rProtocol->m_socketAddresses[socket].GetLocal ();
    }
  else if (m_rProtocol->m_socketSubnetBroadcastAddresses.find (socket) != m_rProtocol->m_socketSubnetBroadcastAddresses.end ())
    {
      receiver = m_rProtocol->m_socketSubnetBroadcastAddresses[socket].GetLocal ();
    }
  else
    {
      NS_ASSERT_MSG (false, "Received a packet from an unknown socket");
    }
  NS_LOG_DEBUG ("AODV node " << this << " received a AODV packet from " << sender << " to " << receiver);

  m_rProtocol->UpdateRouteToNeighbor (sender, receiver);
  TypeHeader tHeader (AODVTYPE_RREQ);
  packet->RemoveHeader (tHeader);
  if (!tHeader.IsValid ())
    {
      NS_LOG_DEBUG ("AODV message " << packet->GetUid () << " with unknown type received: " << tHeader.Get () << ". Drop");
      return; // drop
    }
  switch (tHeader.Get ())
    {
    case AODVTYPE_RREQ:
      {
        m_rProtocol->RecvRequest (packet, receiver, sender);
        break;
      }
    case AODVTYPE_RREP:
      {
        m_rProtocol->RecvReply (packet, receiver, sender);
        break;
      }
    case AODVTYPE_RERR:
      {
        m_rProtocol->RecvError (packet, sender);
        break;
      }
    case AODVTYPE_RREP_ACK:
      {
        m_rProtocol->RecvReplyAck (sender);
        break;
      }
    }
}

void
Ipv4RoutingProtocol::SendPacketFromQueue (Ipv4Address dst, Ptr<Ipv4Route> route)
{
  NS_LOG_FUNCTION (this);
  QueueEntry queueEntry;
  while (m_rProtocol->m_queue.Dequeue (dst, queueEntry))
    {
      DeferredRouteOutputTag tag;
      Ptr<Packet> p = ConstCast<Packet> (queueEntry.GetPacket ());
      if (p->RemovePacketTag (tag)
          && tag.GetInterface () != -1
          && tag.GetInterface () != m_rProtocol->m_ipv4->GetInterfaceForDevice (route->GetOutputDevice ()))
        {
          NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
          return;
        }
      UnicastForwardCallback ucb = queueEntry.GetUnicastForwardCallback ();
      Ipv4Header header = queueEntry.GetIpv4Header ();
      header.SetSource (route->GetSource ());
      header.SetTtl (header.GetTtl () + 1); // compensate extra TTL decrement by fake loopback routing
      ucb (route, p, header);
    }
}

Ptr<Socket>
Ipv4RoutingProtocol::FindSocketWithInterfaceAddress (Ipv4InterfaceAddress addr ) const
{
  NS_LOG_FUNCTION (this << addr);
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_rProtocol->m_socketAddresses.begin (); j != m_rProtocol->m_socketAddresses.end (); ++j)
    {
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      if (iface == addr)
        {
          return socket;
        }
    }
  Ptr<Socket> socket;
  return socket;
}

Ptr<Socket>
Ipv4RoutingProtocol::FindSubnetBroadcastSocketWithInterfaceAddress (Ipv4InterfaceAddress addr ) const
{
  NS_LOG_FUNCTION (this << addr);
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_rProtocol->m_socketSubnetBroadcastAddresses.begin (); j != m_rProtocol->m_socketSubnetBroadcastAddresses.end (); ++j)
    {
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      if (iface == addr)
        {
          return socket;
        }
    }
  Ptr<Socket> socket;
  return socket;
}

void
Ipv4RoutingProtocol::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  ns3::Ipv4RoutingProtocol::DoInitialize ();
}

void
Ipv4RoutingProtocol::SetProtocol(Ptr<RoutingProtocol> agent2)
{
  m_rProtocol = agent2;
}

} //namespace aodv
} //namespace ns3
