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

#ifndef IPV4_MULTICAST_FLOW_PROBE_H
#define IPV4_MULTICAST_FLOW_PROBE_H

#include "ns3/multicast-flow-probe.h"
#include "ns3/ipv4-multicast-flow-classifier.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-address.h"
#include "ns3/queue-item.h"

namespace ns3 {

class MulticastFlowMonitor;
class Node;

/// \ingroup multicast-flow-monitor
/// \brief Class that monitors flows at the IPv4 layer of a Node
///
/// For each node in the simulation, one instance of the class
/// Ipv4MulticastFlowProbe is created to monitor that node.  Ipv4MulticastFlowProbe
/// accomplishes this by connecting callbacks to trace sources in the
/// Ipv4L3Protocol interface of the node.
class Ipv4MulticastFlowProbe : public MulticastFlowProbe
{

public:
  /// \brief Constructor
  /// \param monitor the MulticastFlowMonitor this probe is associated with
  /// \param classifier the Ipv4MulticastFlowClassifier this probe is associated with
  /// \param node the Node this probe is associated with
  /// \param addressGroups Map of address and nodes
  Ipv4MulticastFlowProbe (Ptr<MulticastFlowMonitor> monitor,
                          Ptr<Ipv4MulticastFlowClassifier> classifier,
                          Ptr<Node> node, std::map<Ipv4Address, std::vector<uint32_t> > addressGroups);
  virtual ~Ipv4MulticastFlowProbe ();

  /// Register this type.
  /// \return The TypeId.
  static TypeId GetTypeId (void);

  /// \brief enumeration of possible reasons why a packet may be dropped
  enum DropReason
  {
    DROP_TTL_EXPIRED = 1,       /**< Packet TTL has expired */
    DROP_NO_ROUTE,       /**< No route to host */
    DROP_BAD_CHECKSUM,       /**< Bad checksum */
    DROP_INTERFACE_DOWN,       /**< Interface is down so can not send packet */
    DROP_ROUTE_ERROR,       /**< Route error */
    DROP_FRAGMENT_TIMEOUT,     /**< Fragment timeout exceeded */
    DROP_DUPLICATE,      /**< Duplicate packet received */

    /// Packet dropped due to queue overflow.  Note: only works for
    /// NetDevices that provide a TxQueue attribute of type Queue
    /// with a Drop trace source.  It currently works with Csma and
    /// PointToPoint devices, but not with WiFi or WiMax.
    DROP_QUEUE,

    /// Packet dropped by the queue disc
    DROP_QUEUE_DISC,

    DROP_INVALID_REASON,     /**< Fallback reason (no known reason) */
  };

protected:

  virtual void DoDispose (void);

private:
  /// Log a packet being sent
  /// \param ipHeader IP header
  /// \param ipPayload IP payload
  /// \param interface outgoing interface
  void SendOutgoingLogger (std::string nodeId,
                           const Ipv4Header &ipHeader,
                           Ptr<const Packet> ipPayload,
                           uint32_t interface);
  /// Log a packet being forwarded
  /// \param ipHeader IP header
  /// \param ipPayload IP payload
  /// \param interface incoming interface
  void ForwardLogger (std::string nodeId, const Ipv4Header &ipHeader, Ptr<const Packet> ipPayload, uint32_t interface);
  /// Log a packet being received by the destination
  /// \param ipHeader IP header
  /// \param ipPayload IP payload
  /// \param interface incoming interface
  void ForwardUpLogger (std::string nodeId, const Ipv4Header &ipHeader, Ptr<const Packet> ipPayload, uint32_t interface);
  /// Log a packet being dropped
  /// \param ipHeader IP header
  /// \param ipPayload IP payload
  /// \param reason drop reason
  /// \param ipv4 pointer to the IP object dropping the packet
  /// \param ifIndex interface index
  void DropLogger (std::string nodeId, const Ipv4Header &ipHeader, Ptr<const Packet> ipPayload,
                   Ipv4L3Protocol::DropReason reason, Ptr<Ipv4> ipv4, uint32_t ifIndex);
  /// Log a packet being dropped by a queue
  /// \param ipPayload IP payload
  void QueueDropLogger (std::string context, Ptr<const Packet> ipPayload);
  /// Log a packet being dropped by a queue disc
  /// \param item queue disc item
  void QueueDiscDropLogger (std::string context, Ptr<const QueueDiscItem> item);

  uint32_t GetNodeFromContext (std::string context, std::string type);

  Ptr<Ipv4MulticastFlowClassifier> m_classifier;   //!< the Ipv4MulticastFlowClassifier this probe is associated with
  Ptr<Ipv4L3Protocol> m_ipv4;   //!< the Ipv4L3Protocol this probe is bound to
  std::map<Ipv4Address, std::vector<uint32_t> > m_flowGroupNodes; //!< The destination group nodes associated with this flow
};


} // namespace ns3

#endif /* IPV4_MULTICAST_FLOW_PROBE_H */
