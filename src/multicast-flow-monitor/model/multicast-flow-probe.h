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

#ifndef MULTICAST_FLOW_PROBE_H
#define MULTICAST_FLOW_PROBE_H

#include <map>
#include <vector>

#include "ns3/object.h"
#include "ns3/multicast-flow-classifier.h"
#include "ns3/nstime.h"

namespace ns3 {

class MulticastFlowMonitor;

/// The MulticastFlowProbe class is responsible for listening for packet events
/// in a specific point of the simulated space, report those events to
/// the global MulticastFlowMonitor, and collect its own flow statistics
/// regarding only the packets that pass through that probe.
class MulticastFlowProbe : public Object
{
private:
  /// Defined and not implemented to avoid misuse
  MulticastFlowProbe (MulticastFlowProbe const &);
  /// Defined and not implemented to avoid misuse
  /// \returns
  MulticastFlowProbe& operator= (MulticastFlowProbe const &);

protected:
  /// Constructor
  /// \param multicastFlowMonitor the multicastFlowMonitor this probe is associated with
  MulticastFlowProbe (Ptr<MulticastFlowMonitor> multicastFlowMonitor);
  virtual void DoDispose (void);

public:
  virtual ~MulticastFlowProbe ();

  /// Register this type.
  /// \return The TypeId.
  static TypeId GetTypeId (void);

  /// Structure to hold the statistics of a flow
  struct MulticastFlowStats
  {
    /// packetsDropped[reasonCode] => number of dropped packets
    std::map<uint32_t, std::vector<uint32_t> > packetsDropped;
    /// bytesDropped[reasonCode] => number of dropped bytes
    std::map<uint32_t, std::vector<uint64_t> > bytesDropped;
    /// divide by 'packets' to get the average delay from the
    /// first (entry) probe up to this one (partial delay)
    std::map<uint32_t, Time> delayFromFirstProbeSum;
    /// Number of bytes seen of this flow
    std::map<uint32_t, uint64_t> bytes;
    /// Number of packets seen of this flow
    std::map<uint32_t, uint32_t> packets;
  };

  /// Container to map MulticastFlowId -> MulticastFlowStats
  typedef std::map<MulticastFlowId, MulticastFlowStats> Stats;

  /// Add a packet data to the flow stats
  /// \param flowId the flow Identifier
  /// \param packetSize the packet size
  /// \param delayFromFirstProbe packet delay
  /// \param nodeId the node seeing these stats
  void AddPacketStats (MulticastFlowId flowId, uint32_t packetSize, Time delayFromFirstProbe, uint32_t nodeId);
  /// Add a packet drop data to the flow stats
  /// \param flowId the flow Identifier
  /// \param packetSize the packet size
  /// \param reasonCode reason code for the drop
  /// \param nodeId the node seeing these stats
  void AddPacketDropStats (MulticastFlowId flowId, uint32_t packetSize, uint32_t reasonCode, uint32_t nodeId);

  /// Get the partial flow statistics stored in this probe.  With this
  /// information you can, for example, find out what is the delay
  /// from the first probe to this one.
  /// \returns the partial flow statistics
  Stats GetStats () const;

protected:
  Ptr<MulticastFlowMonitor> m_multicastFlowMonitor; //!< the MulticastFlowMonitor instance
  Stats m_stats; //!< The flow stats

};


} // namespace ns3

#endif /* MULTICAST_FLOW_PROBE_H */
