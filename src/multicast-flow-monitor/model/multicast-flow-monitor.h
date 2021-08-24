/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) YEAR COPYRIGHTHOLDER
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
 * Author: Caleb Bowers <caleb.bowers@nrl.navy.mil>
 */
#ifndef MULTICAST_FLOW_MONITOR_H
#define MULTICAST_FLOW_MONITOR_H

#include <vector>
#include <map>

#include "ns3/ptr.h"
#include "ns3/object.h"
#include "ns3/multicast-flow-probe.h"
#include "ns3/multicast-flow-classifier.h"
#include "ns3/histogram.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"

namespace ns3 {

/**
 * \defgroup multicast-flow-monitor Multicast Flow Monitor
 * \brief  Collect and store multicast performance data from a simulation
 */

/**
 * \ingroup multicast-flow-monitor
 * \brief An object that monitors and reports back packet flows observed during a simulation
 * generating multicast traffic
 *
 * The MulticastFlowMonitor class is responsible for coordinating efforts
 * regarding probes, and collects end-to-end flow statistics for multicast traffic.
 *
 */
class MulticastFlowMonitor : public Object
{
public:

  /// \brief Structure that represents the measured metrics for an indiviudal multicast flow
  struct MulticastFlowStats
  {
    /// Contains the absolute time when the first packet in the flow
    /// was transmitted, i.e. the time when the flow transmission
    /// starts
    Time     timeFirstTxPacket;

    /// Contains the absolute time when the first packet in the flow
    /// was received by an end group node, i.e. the time when the flow
    /// reception starts
    std::map<uint32_t, Time>     timeFirstRxPacket;

    /// Contains the absolute time when the last packet in the flow
    /// was transmitted, i.e. the time when the flow transmission
    /// ends
    Time     timeLastTxPacket;

    /// Contains the absolute time when the last packet in the flow
    /// was received, i.e. the time when the flow reception ends
    std::map<uint32_t, Time>     timeLastRxPacket;

    /// Contains the sum of all end-to-end delays for all received
    /// packets of the flow.
    std::map<uint32_t, Time>     delaySum;             // delayCount == rxPackets

    /// Contains the sum of all end-to-end delay jitter (delay
    /// variation) values for all received packets of the flow.  Here
    /// we define _jitter_ of a packet as the delay variation
    /// relatively to the last packet of the stream,
    /// i.e. \f$Jitter\left\{P_N\right\} = \left|Delay\left\{P_N\right\} - Delay\left\{P_{N-1}\right\}\right|\f$.
    /// This definition is in accordance with the Type-P-One-way-ipdv
    /// as defined in IETF \RFC{3393}.
    std::map<uint32_t, Time>     jitterSum;             // jitterCount == rxPackets - 1

    /// Contains the last measured delay of a packet
    /// It is stored to measure the packet's Jitter
    std::map<uint32_t, std::map<uint32_t, Time> >     packetDelay;

    /// Contains the number of hops a packet incurred during delivery
    std::map<uint32_t, std::map<uint32_t, uint32_t> > numHops;

    /// The last delay a packet had for the flow
    std::map<uint32_t, Time> lastDelay;

    /// Total number of transmitted bytes for the flow
    uint64_t txBytes;
    /// Total number of received bytes for the flow
    std::map<uint32_t, uint64_t> rxBytes;
    /// Total number of transmitted packets for the flow
    uint32_t txPackets;
    /// Total number of received packets for the flow
    std::map<uint32_t, uint32_t> rxPackets;

    /// Total number of packets that are assumed to be lost,
    /// i.e. those that were transmitted but have not been reportedly
    /// received or forwarded for a long time.  By default, packets
    /// missing for a period of over 10 seconds are assumed to be
    /// lost, although this value can be easily configured in runtime
    std::map<uint32_t, uint32_t> lostPackets;

    /// For each node, map whether it lost this packet id, e.g., nodeLostPackets[node1][packet1] = False ==> packet was received. 
    /// Used to account for lost packets in group destination nodes
    std::map<uint32_t, std::map<uint32_t, bool> > nodeLostPackets;

    /// duplicates dropped for each Node
    std::map<uint32_t, uint32_t> dupsDropped;

    /// Contains the number of times a packet has been reportedly
    /// forwarded, summed for all received packets in the flow
    std::map<uint32_t, uint32_t> timesForwarded;


    /// This attribute also tracks the number of lost packets and
    /// bytes, but discriminates the losses by a _reason code_.  This
    /// reason code is usually an enumeration defined by the concrete
    /// FlowProbe class, and for each reason code there may be a
    /// vector entry indexed by that code and whose value is the
    /// number of packets or bytes lost due to this reason.  For
    /// instance, in the Ipv4FlowProbe case the following reasons are
    /// currently defined: DROP_NO_ROUTE (no IPv4 route found for a
    /// packet), DROP_TTL_EXPIRE (a packet was dropped due to an IPv4
    /// TTL field decremented and reaching zero), and
    /// DROP_BAD_CHECKSUM (a packet had bad IPv4 header checksum and
    /// had to be dropped).
    std::map<uint32_t, std::vector<uint32_t> > packetsDropped;            // packetsDropped[reasonCode] => number of dropped packets

    /// This attribute also tracks the number of lost bytes.  See also
    /// comment in attribute packetsDropped.
    std::map<uint32_t, std::vector<uint64_t> > bytesDropped;            // bytesDropped[reasonCode] => number of dropped bytes
    // Histogram flowInterruptionsHistogram; //!< histogram of durations of flow interruptions

    // For a given group, these are the node ids
    std::vector<uint32_t> groupNodeIds;             // destinations

    /// This group and their node ids, signal if all group members received packet
    std::map<uint32_t, std::map<uint32_t, bool> > groupDelivered;            // need flow, node, and packet id for unique id

    /// This group and their node ids, signal if all group members dropped packet
    std::map<uint32_t, std::map<uint32_t, bool> > groupDropped;            // need flow, node, and packet id for unique id
  };
  // --- basic methods ---
  /**
  * \brief Get the type ID.
  * \return the object TypeId
  */
  static TypeId GetTypeId ();
  virtual TypeId GetInstanceTypeId () const;
  MulticastFlowMonitor ();

  /// Add a MulticastFlowClassifier to be used by the flow monitor.
  /// \param classifier the MulticastFlowClassifier
  void AddMulticastFlowClassifier (Ptr<MulticastFlowClassifier> classifier);

  /// Set the time, counting from the current time, from which to start monitoring flows.
  /// This method overwrites any previous calls to Start()
  /// \param time delta time to start
  void Start (const Time &time);
  /// Set the time, counting from the current time, from which to stop monitoring flows.
  /// This method overwrites any previous calls to Stop()
  /// \param time delta time to stop
  void Stop (const Time &time);
  /// Begin monitoring flows *right now*
  void StartRightNow ();
  /// End monitoring flows *right now*
  void StopRightNow ();

  // --- methods to be used by the MulticastFlowMonitorProbe's only ---
  /// Register a new FlowProbe that will begin monitoring and report
  /// events to this monitor.  This method is normally only used by
  /// MulticastFlowProbe implementations.
  /// \param probe the probe to add
  void AddProbe (Ptr<MulticastFlowProbe> probe);

  /// MulticastFlowProbe implementations are supposed to call this method to
  /// report that a new packet was transmitted (but keep in mind the
  /// distinction between a new packet entering the system and a
  /// packet that is already known and is only being forwarded).
  /// \param probe the reporting probe
  /// \param flowId flow identification
  /// \param packetId Packet ID
  /// \param packetSize packet size 
  /// \param txNodeId the transmitting node (need to know src node)
  /// \param ttl the current ttl on the packet. Need to know hop count
  /// \param groupNodeIds to know destination nodes
  void ReportFirstTx (Ptr<MulticastFlowProbe> probe, MulticastFlowId flowId, MulticastFlowPacketId packetId, uint32_t packetSize, uint32_t txNodeId, uint32_t ttl, std::vector<uint32_t> groupNodeIds);

  /// MulticastFlowProbe implementations are supposed to call this method to
  /// report that a known packet is being forwarded.
  /// \param probe the reporting probe
  /// \param flowId flow identification
  /// \param packetId Packet ID
  /// \param packetSize packet size
  /// \param nodeId forwarding node. Need for duplicate detection, hop count, loss, etc.
  void ReportForwarding (Ptr<MulticastFlowProbe> probe, MulticastFlowId flowId, MulticastFlowPacketId packetId, uint32_t packetSize, uint32_t nodeId);

  /// MulticastFlowProbe implementations are supposed to call this method to
  /// report that a known packet is being received.
  /// \param probe the reporting probe
  /// \param flowId flow identification
  /// \param packetId Packet ID
  /// \param packetSize packet size
  /// \param ttl for hop count
  void ReportRx (Ptr<MulticastFlowProbe> probe, MulticastFlowId flowId, MulticastFlowPacketId packetId, uint32_t packetSize, uint32_t nodeId, uint32_t ttl);

  /// MulticastFlowProbe implementations are supposed to call this method to
  /// report that a known packet is being dropped due to some reason.
  /// \param probe the reporting probe
  /// \param flowId flow identification
  /// \param packetId Packet ID
  /// \param packetSize packet size
  /// \param reasonCode drop reason code
  /// \param nodeId Node dropping packet.
  void ReportDrop (Ptr<MulticastFlowProbe> probe, MulticastFlowId flowId, MulticastFlowPacketId packetId,
                   uint32_t packetSize, uint32_t reasonCode, uint32_t nodeId);

  /// MulticastFlowProbe implementations are supposed to call this method to
  /// report that a known packet is being dropped due to some reason.
  /// \param probe the reporting probe
  /// \param flowId flow identification
  /// \param packetId Packet ID
  /// \param packetSize packet size
  /// \param reasonCode drop reason code
  /// \param nodeId Node dropping duplicate packet.
  void ReportDupDrop (Ptr<MulticastFlowProbe> probe, MulticastFlowId flowId, MulticastFlowPacketId packetId, uint32_t packetSize,
                      uint32_t nodeId);

  /// Check right now for packets that appear to be lost
  void CheckForLostPackets ();

  /// Check right now for packets that appear to be lost, considering
  /// packets as lost if not seen in the network for a time larger
  /// than maxDelay
  /// \param maxDelay the max delay for a packet
  void CheckForLostPackets (Time maxDelay);

  // --- methods to get the results ---

  /// Container: MulticastFlowId, MulticastFlowStats
  typedef std::map<MulticastFlowId, MulticastFlowStats> MulticastFlowStatsContainer;
  /// Container Iterator: MulticastFlowId, MulticastFlowStats
  typedef std::map<MulticastFlowId, MulticastFlowStats>::iterator MulticastFlowStatsContainerI;
  /// Container Const Iterator: MulticastFlowId, MulticastFlowStats
  typedef std::map<MulticastFlowId, MulticastFlowStats>::const_iterator MulticastlowStatsContainerCI;
  /// Container: MulticastFlowProbe
  typedef std::vector< Ptr<MulticastFlowProbe> > MulticastFlowProbeContainer;
  /// Container Iterator: MulticastFlowProbe
  typedef std::vector< Ptr<MulticastFlowProbe> >::iterator MulticastFlowProbeContainerI;
  /// Container Const Iterator: MulticastFlowProbe
  typedef std::vector< Ptr<MulticastFlowProbe> >::const_iterator MulticastFlowProbeContainerCI;

  /// Retrieve all collected the flow statistics.  Note, if the
  /// MulticastFlowMonitor has not stopped monitoring yet, you should call
  /// CheckForLostPackets() to make sure all possibly lost packets are
  /// accounted for.
  /// \returns the flows statistics
  const MulticastFlowStatsContainer& GetMulticastFlowStats () const;

  /// Get a list of all MulticastFlowProbe's associated with this FlowMonitor
  /// \returns a list of all the probes
  const MulticastFlowProbeContainer& GetAllMulticastProbes () const;

protected:

  virtual void NotifyConstructionCompleted ();
  virtual void DoDispose (void);

private:

  // Structure to represent a single tracked packet data
  struct TrackedPacket
  {
    Time firstSeenTime;             //!< absolute time when the packet was first seen by a probe
    uint32_t initial_ttl;           //!< Initial TTL to determine hop count
    Time lastSeenTime;             //!< absolute time when the packet was last seen by a probe
    uint32_t nodesSeen;             //!< How many hops already
    uint32_t timesForwarded;             //!< number of times the packet was reportedly forwarded
  };

  /// MulticastFlowId --> MulticastFlowStats
  MulticastFlowStatsContainer m_multicastFlowStats;

  /// (MulticastFlowId,MulticastPacketId) --> TrackedPacket
  typedef std::map< std::pair<MulticastFlowId, MulticastFlowPacketId>, TrackedPacket> TrackedPacketMap;
  TrackedPacketMap m_trackedPackets;       //!< Tracked packets
  Time m_maxPerHopDelay;       //!< Minimum per-hop delay
  MulticastFlowProbeContainer m_multicastFlowProbes;       //!< all the MulticastFlowProbes

  // note: this is needed only for serialization and destruction
  std::list<Ptr<MulticastFlowClassifier> > m_mcastClassifiers;       //!< the MulticastFlowClassifiers

  EventId m_startEvent;           //!< Start event
  EventId m_stopEvent;            //!< Stop event
  bool m_enabled;                 //!< MulticastFlowMon is enabled

  /// Get the stats for a given flow
  /// \param flowId the MulticastFlow identification
  /// \returns the stats of the flow
  MulticastFlowStats& GetStatsForMulticastFlow (MulticastFlowId flowId);

  /// Get the stats for a given flow
  /// \param flowId the MulticastFlow identification
  /// \returns the stats of the flow
  MulticastFlowStats& GetStatsForMulticastFlow (MulticastFlowId flowId, std::vector<uint32_t> groupNodeIds);

  /// Periodic function to check for lost packets and prune statistics
  void PeriodicCheckForLostPackets ();
};

} // namespace ns3

#endif /* MULTICAST_FLOW_MONITOR_H */

