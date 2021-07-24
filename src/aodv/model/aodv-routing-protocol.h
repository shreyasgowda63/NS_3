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
 * Author: Nitya Chandra <nityachandra6@gmail.com>
 *          
 */
#ifndef AODVROUTINGPROTOCOL_H
#define AODVROUTINGPROTOCOL_H

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


namespace ns3{

namespace aodv{

class Ipv4RoutingProtocol;
/**
 * \ingroup aodv
 *
 * \brief AODV routing protocol
 */
class RoutingProtocol : public Object
{
  friend class Ipv4RoutingProtocol;

public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
 
  ///constructor
  RoutingProtocol();
  virtual ~RoutingProtocol();
  void DoDispose ();

    // Handle protocol parameters
  /**
   * Get maximum queue time
   * \returns the maximum queue time
   */
  Time GetMaxQueueTime () const
  {
    return m_maxQueueTime;
  }
  /**
   * Set the maximum queue time
   * \param t the maximum queue time
   */
  void SetMaxQueueTime (Time t);
  /**
   * Get the maximum queue length
   * \returns the maximum queue length
   */
  uint32_t GetMaxQueueLen () const
  {
    return m_maxQueueLen;
  }
  /**
   * Set the maximum queue length
   * \param len the maximum queue length
   */
  void SetMaxQueueLen (uint32_t len);
  /**
   * Get destination only flag
   * \returns the destination only flag
   */
  bool GetDestinationOnlyFlag () const
  {
    return m_destinationOnly;
  }
  /**
   * Set destination only flag
   * \param f the destination only flag
   */
  void SetDestinationOnlyFlag (bool f)
  {
    m_destinationOnly = f;
  }
    /**
   * Get gratuitous reply flag
   * \returns the gratuitous reply flag
   */
  bool GetGratuitousReplyFlag () const
  {
    return m_gratuitousReply;
  }
  /**
   * Set gratuitous reply flag
   * \param f the gratuitous reply flag
   */
  void SetGratuitousReplyFlag (bool f)
  {
    m_gratuitousReply = f;
  }
    /**
   * Set hello enable
   * \param f the hello enable flag
   */
  void SetHelloEnable (bool f)
  {
    m_enableHello = f;
  }
  /**
   * Get hello enable flag
   * \returns the enable hello flag
   */
  bool GetHelloEnable () const
  {
    return m_enableHello;
  }
  /**
   * Set broadcast enable flag
   * \param f enable broadcast flag
   */
  void SetBroadcastEnable (bool f)
  {
    m_enableBroadcast = f;
  }
  /**
   * Get broadcast enable flag
   * \returns the broadcast enable flag
   */
  bool GetBroadcastEnable () const
  {
    return m_enableBroadcast;
  }

  /** 
   * Set Protocol, used by AodvHelper to pass a poiner to an object 
   * \param ipv4_agent assign a pointer of aodv::Ipv4RoutingProtocol to RoutingProtocol to hook functions 
  */
  void SetIPv4AODVProtocol(Ptr<Ipv4RoutingProtocol> ipv4_agent);
protected:
  virtual void DoInitialize (void);
private:
  // Protocol parameters.
  uint32_t m_rreqRetries;             ///< Maximum number of retransmissions of RREQ with TTL = NetDiameter to discover a route
  uint16_t m_ttlStart;                ///< Initial TTL value for RREQ.
  uint16_t m_ttlIncrement;            ///< TTL increment for each attempt using the expanding ring search for RREQ dissemination.
  uint16_t m_ttlThreshold;            ///< Maximum TTL value for expanding ring search, TTL = NetDiameter is used beyond this value.
  uint16_t m_timeoutBuffer;           ///< Provide a buffer for the timeout.
  uint16_t m_rreqRateLimit;           ///< Maximum number of RREQ per second.
  uint16_t m_rerrRateLimit;           ///< Maximum number of REER per second.
  Time m_activeRouteTimeout;          ///< Period of time during which the route is considered to be valid.
  uint32_t m_netDiameter;             ///< Net diameter measures the maximum possible number of hops between two nodes in the network
  /**
   *  NodeTraversalTime is a conservative estimate of the average one hop traversal time for packets
   *  and should include queuing delays, interrupt processing times and transfer times.
   */
  Time m_nodeTraversalTime;
  Time m_netTraversalTime;             ///< Estimate of the average net traversal time.
  Time m_pathDiscoveryTime;            ///< Estimate of maximum time needed to find route in network.
  Time m_myRouteTimeout;               ///< Value of lifetime field in RREP generating by this node.
  /**
   * Every HelloInterval the node checks whether it has sent a broadcast  within the last HelloInterval.
   * If it has not, it MAY broadcast a  Hello message
   */
  Time m_helloInterval;
  uint32_t m_allowedHelloLoss;         ///< Number of hello messages which may be loss for valid link
  /**
   * DeletePeriod is intended to provide an upper bound on the time for which an upstream node A
   * can have a neighbor B as an active next hop for destination D, while B has invalidated the route to D.
   */
  Time m_deletePeriod;
  Time m_nextHopWait;                  ///< Period of our waiting for the neighbour's RREP_ACK
  Time m_blackListTimeout;             ///< Time for which the node is put into the blacklist
  uint32_t m_maxQueueLen;              ///< The maximum number of packets that we allow a routing protocol to buffer.
  Time m_maxQueueTime;                ///< The maximum period of time that a routing protocol is allowed to buffer a packet for.
  bool m_destinationOnly;              ///< Indicates only the destination may respond to this RREQ.  
  bool m_gratuitousReply;              ///< Indicates whether a gratuitous RREP should be unicast to the node originated route discovery.
  bool m_enableHello;                  ///< Indicates whether a hello messages enable
  bool m_enableBroadcast;              ///< Indicates whether a a broadcast data packets forwarding enable
  
    /// IP protocol
  Ptr<Ipv4> m_ipv4;
  /// Raw unicast socket per each IP interface, map socket -> iface address (IP + mask)
  std::map< Ptr<Socket>, Ipv4InterfaceAddress > m_socketAddresses;
  /// Raw subnet directed broadcast socket per each IP interface, map socket -> iface address (IP + mask)
  std::map< Ptr<Socket>, Ipv4InterfaceAddress > m_socketSubnetBroadcastAddresses;
  /// Loopback device used to defer RREQ until packet will be fully formed
  Ptr<NetDevice> m_lo;


  /// Routing table
  RoutingTable m_routingTable;
  /// A "drop-front" queue used by the routing layer to buffer packets to which it does not have a route.
  RequestQueue m_queue;
    /// Broadcast ID
  uint32_t m_requestId;
  /// Request sequence number
  uint32_t m_seqNo;
  /// Handle duplicated RREQ
  IdCache m_rreqIdCache;
  /// Handle duplicated broadcast/multicast packets
  DuplicatePacketDetection m_dpd;
  /// Handle neighbors
  Neighbors m_nb;
  /// Number of RREQs used for RREQ rate control
  uint16_t m_rreqCount;
  /// Number of RERRs used for RERR rate control
  uint16_t m_rerrCount;

  Ptr<Ipv4RoutingProtocol> m_ipver4;  //!< Pointer to the IPv4 specific AdovroutingProtocol part
  /// Start protocol operation
  void Start ();
    /**
   * Repeated attempts by a source node at route discovery for a single destination
   * use the expanding ring search technique.
   * \param dst the destination IP address
   */
  void ScheduleRreqRetry (Ipv4Address dst);
  /**
   * Set lifetime field in routing table entry to the maximum of existing lifetime and lt, if the entry exists
   * \param addr - destination address
   * \param lt - proposed time for lifetime field in routing table entry for destination with address addr.
   * \return true if route to destination address addr exist
   */
  bool UpdateRouteLifeTime (Ipv4Address addr, Time lt);
    /**
   * Update neighbor record.
   * \param receiver is supposed to be my interface
   * \param sender is supposed to be IP address of my neighbor.
   */
  void UpdateRouteToNeighbor (Ipv4Address sender, Ipv4Address receiver);
  /**
   * Process hello message
   * 
   * \param rrepHeader RREP message header
   * \param receiverIfaceAddr receiver interface IP address
   */
  void ProcessHello (RrepHeader const & rrepHeader, Ipv4Address receiverIfaceAddr);
    /// Send hello
  void SendHello ();
    /// Schedule next send of hello message
  void HelloTimerExpire ();
    ///\name Receive control packets
  //\{
    /**
   * Receive RREQ
   * \param p packet
   * \param receiver receiver address
   * \param src sender address
   */
  void RecvRequest (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src);
    /**
   * Receive RREP
   * \param p packet
   * \param my destination address
   * \param src sender address
   */
  void RecvReply (Ptr<Packet> p, Ipv4Address my, Ipv4Address src);
    /**
   * Receive RREP_ACK
   * \param neighbor neighbor address
   */
  void RecvReplyAck (Ipv4Address neighbor);
    /**
   * Receive RERR
   * \param p packet
   * \param src sender address
   */
  /// Receive  from node with address src
  void RecvError (Ptr<Packet> p, Ipv4Address src);
  //\}
    ///\name Send
  //\{
    /** Send RREQ
   * \param dst destination address
   */
  void SendRequest (Ipv4Address dst);  
    /** Initiate RERR
   * \param nextHop next hop address
   */
  void SendRerrWhenBreaksLinkToNextHop (Ipv4Address nextHop);
  /** Send RREP
   * \param rreqHeader route request header
   * \param toOrigin routing table entry to originator
   */
  void SendReply (RreqHeader const & rreqHeader, RoutingTableEntry const & toOrigin);
    /** Send RREP by intermediate node
   * \param toDst routing table entry to destination
   * \param toOrigin routing table entry to originator
   * \param gratRep indicates whether a gratuitous RREP should be unicast to destination
   */
  void SendReplyByIntermediateNode (RoutingTableEntry & toDst, RoutingTableEntry & toOrigin, bool gratRep);
    /** Send RREP_ACK
   * \param neighbor neighbor address
   */
  void SendReplyAck (Ipv4Address neighbor);
    /** Forward RERR
   * \param packet packet
   * \param precursors list of addresses of the visited nodes
   */
  void SendRerrMessage (Ptr<Packet> packet,  std::vector<Ipv4Address> precursors);
    /**
   * Send RERR message when no route to forward input packet. Unicast if there is reverse route to originating node, broadcast otherwise.
   * \param dst - destination node IP address
   * \param dstSeqNo - destination node sequence number
   * \param origin - originating node IP address
   */
  void SendRerrWhenNoRouteToForward (Ipv4Address dst, uint32_t dstSeqNo, Ipv4Address origin);
  /// @}
    /**
   * Mark link to neighbor node as unidirectional for blacklistTimeout
   *
   * \param neighbor the IP address of the neightbor node
   * \param blacklistTimeout the black list timeout time
   */
  private:
  /// Hello timer
  Timer m_htimer;
    /**
   * Mark link to neighbor node as unidirectional for blacklistTimeout
   *
   * \param neighbor the IP address of the neightbor node
   * \param blacklistTimeout the black list timeout time
   */

  void AckTimerExpire (Ipv4Address neighbor, Time blacklistTimeout);
  /// RREQ rate limit timer
  Timer m_rreqRateLimitTimer;
  /// Reset RREQ count and schedule RREQ rate limit timer with delay 1 sec.
  void RreqRateLimitTimerExpire ();
  /// Reset RERR count and schedule RERR rate limit timer with delay 1 sec.
  void RerrRateLimitTimerExpire ();
  /// RERR rate limit timer
  Timer m_rerrRateLimitTimer;
    /**
   * Handle route discovery process
   * \param dst the destination IP address
   */
  void RouteRequestTimerExpire (Ipv4Address dst);

  private:
  /// Map IP address + RREQ timer.
  std::map<Ipv4Address, Timer> m_addressReqTimer;
  /// Provides uniform random variables.
  Ptr<UniformRandomVariable> m_uniformRandomVariable;
  /// Keep track of the last bcast time
  Time m_lastBcastTime;
};


/**
* \ingroup aodv
* \brief Tag used by AODV implementation
*/
class DeferredRouteOutputTag : public Tag
{

public:
  /**
   * \brief Constructor
   * \param o the output interface
   */
  DeferredRouteOutputTag (int32_t o = -1) : Tag (),
                                            m_oif (o)
  {
  }

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId ()
  {
    static TypeId tid = TypeId ("ns3::aodv::DeferredRouteOutputTag")
      .SetParent<Tag> ()
      .SetGroupName ("Aodv")
      .AddConstructor<DeferredRouteOutputTag> ()
    ;
    return tid;
  }

  TypeId  GetInstanceTypeId () const
  {
    return GetTypeId ();
  }

  /**
   * \brief Get the output interface
   * \return the output interface
   */
  int32_t GetInterface () const
  {
    return m_oif;
  }

  /**
   * \brief Set the output interface
   * \param oif the output interface
   */
  void SetInterface (int32_t oif)
  {
    m_oif = oif;
  }

  uint32_t GetSerializedSize () const
  {
    return sizeof(int32_t);
  }

  void  Serialize (TagBuffer i) const
  {
    i.WriteU32 (m_oif);
  }

  void  Deserialize (TagBuffer i)
  {
    m_oif = i.ReadU32 ();
  }

  void  Print (std::ostream &os) const
  {
    os << "DeferredRouteOutputTag: output interface = " << m_oif;
  }

private:
  /// Positive if output device is fixed in RouteOutput
  int32_t m_oif;
};

NS_OBJECT_ENSURE_REGISTERED (DeferredRouteOutputTag);

} //namespace aodv
} //namespace ns3

#endif /*AODVROUTINGPROTOCOL_H*/