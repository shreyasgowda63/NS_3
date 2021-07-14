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
#ifndef AODV_IPV4_ROUTINGPROTOCOL_H
#define AODV_IPV4_ROUTINGPROTOCOL_H

#include "aodv-routing-protocol.h"
#include "aodv-rtable.h"
#include "aodv-rqueue.h"
#include "aodv-packet.h"
#include "aodv-neighbor.h"
#include "aodv-dpd.h"
#include "ns3/node.h"
#include "ns3/random-variable-stream.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include <map>

namespace ns3 {

class WifiMacQueueItem;
enum WifiMacDropReason : uint8_t;  // opaque enum declaration

namespace aodv {
/**
 * \ingroup aodv
 *
 * \brief AODV Ipv4 routing protocol
 */
class Ipv4RoutingProtocol : public ns3::Ipv4RoutingProtocol
{
friend class RoutingProtocol;
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  static const uint32_t AODV_PORT;
  /// constructor
  Ipv4RoutingProtocol ();
  ~Ipv4RoutingProtocol ();
  void DoDispose ();
  Ptr<RoutingProtocol> m_rProtocol;
  // Set Protocol, used by AodvHelper to pass a poiner to an object(sets m_rProtocol, to be specific)
  void SetProtocol(Ptr<RoutingProtocol> agent2);
  // Inherited from Ipv4RoutingProtocol
  Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
  bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                   UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                   LocalDeliverCallback lcb, ErrorCallback ecb);
  void NotifyInterfaceUp (uint32_t interface);
  void NotifyInterfaceDown (uint32_t interface);
  void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
  void SetIpv4 (Ptr<Ipv4> ipv4);
  void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;
  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model.  Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams (int64_t stream);
protected:
  virtual void DoInitialize (void);
private:
  /**
   * Notify that an MPDU was dropped.
   *
   * \param reason the reason why the MPDU was dropped
   * \param mpdu the dropped MPDU
   */
  void NotifyTxError (WifiMacDropReason reason, Ptr<const WifiMacQueueItem> mpdu);

// private:
  /// Start protocol operation
  /**
   * Queue packet and send route request
   *
   * \param p the packet to route
   * \param header the IP header
   * \param ucb the UnicastForwardCallback function
   * \param ecb the ErrorCallback function
   */ 
  void DeferredRouteOutput (Ptr<const Packet> p, const Ipv4Header & header, UnicastForwardCallback ucb, ErrorCallback ecb);
  /**
   * If route exists and is valid, forward packet.
   *
   * \param p the packet to route
   * \param header the IP header
   * \param ucb the UnicastForwardCallback function
   * \param ecb the ErrorCallback function
   * \returns true if forwarded
   */ 
  bool Forwarding (Ptr<const Packet> p, const Ipv4Header & header, UnicastForwardCallback ucb, ErrorCallback ecb);
  /**
   * Test whether the provided address is assigned to an interface on this node
   * \param src the source IP address
   * \returns true if the IP address is the node's IP address
   */
  bool IsMyOwnAddress (Ipv4Address src);
  /**
   * Find unicast socket with local interface address iface
   *
   * \param iface the interface
   * \returns the socket associated with the interface
   */
  Ptr<Socket> FindSocketWithInterfaceAddress (Ipv4InterfaceAddress iface) const;
  /**
   * Find subnet directed broadcast socket with local interface address iface
   *
   * \param iface the interface
   * \returns the socket associated with the interface
   */
  Ptr<Socket> FindSubnetBroadcastSocketWithInterfaceAddress (Ipv4InterfaceAddress iface) const;
  /**
   * Create loopback route for given header
   *
   * \param header the IP header
   * \param oif the output interface net device
   * \returns the route
   */
  Ptr<Ipv4Route> LoopbackRoute (const Ipv4Header & header, Ptr<NetDevice> oif) const;

  ///\name Receive control packets
  // \{
  /**
   * Receive and process control packet
   * \param socket input socket
   */
  void RecvAodv (Ptr<Socket> socket);
  //\}

  ///\name Send
  //\{
  /** Forward packet from route request queue
   * \param dst destination address
   * \param route route to use
   */
  void SendPacketFromQueue (Ipv4Address dst, Ptr<Ipv4Route> route);
  /// @}
public:
  /**
   * Send packet to destination scoket
   * \param socket - destination node socket
   * \param packet - packet to send
   * \param destination - destination node IP address
   */
  void SendTo (Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination);
};

} //namespace aodv
} //namespace ns3

#endif /* AODV_IPV4_ROUTINGPROTOCOL_H */
