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

#ifndef MULTICAST_FLOW_MONITOR_HELPER_H
#define MULTICAST_FLOW_MONITOR_HELPER_H

#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/multicast-flow-monitor.h"
#include "ns3/multicast-flow-classifier.h"
#include <string>

namespace ns3 {

class AttributeValue;
class Ipv4MulticastFlowClassifier;

/*
* \ingroup multicast-flow-monitor
* \brief Helper to enable IP multicast flow monitoring on a set of Nodes
*/

class MulticastFlowMonitorHelper
{
public:
  MulticastFlowMonitorHelper ();
  ~MulticastFlowMonitorHelper ();

  /**
   * \brief Set an attribute for the to-be-created MulticastFlowMonitor object
   * \param n1 attribute name
   * \param v1 attribute value
   */
  void SetMulticastMonitorAttribute (std::string n1, const AttributeValue &v1);

  /**
   * \brief Enable multicast flow monitoring on a set of nodes
   * \param nodes A NodeContainer holding the set of nodes to work with.
   * \param addressGroups An std::map object mapping multicast address group and group id.
   * \returns a pointer to the MulticastFlowMonitor object
   */
  Ptr<MulticastFlowMonitor> Install (NodeContainer nodes, std::map<Ipv4Address, std::vector<uint32_t> > addressGroups);

  /**
   * \brief Enable multicast flow monitoring on a single node
   * \param node A Ptr<Node> to the node on which to enable Multicast flow monitoring.
   * \param addressGroups An std::map object mapping multicast address group and group id.
   * \returns a pointer to the MulticastFlowMonitor
   */
  Ptr<MulticastFlowMonitor> Install (Ptr<Node> node, std::map<Ipv4Address, std::vector<uint32_t> > addressGroups);

  /**
   * \brief Enable flow monitoring on all nodes
   * \param addressGroups An std::map object mapping multicast address group and group id.
   * \returns a pointer to the MulticastFlowMonitor object
   */
  Ptr<MulticastFlowMonitor> InstallAll (std::map<Ipv4Address, std::vector<uint32_t> > addressGroups);

  /**
  * \brief Retrieve the FlowMonitor object created by the Install* methods
  * \returns a pointer to the FlowMonitor object
  */
  Ptr<MulticastFlowMonitor> GetMonitor ();

  /**
   * \brief Retrieve the FlowClassifier object for IPv4 created by the Install* methods
   * \returns a pointer to the FlowClassifier object
   */
  Ptr<MulticastFlowClassifier> GetClassifier ();

private:
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  MulticastFlowMonitorHelper (const MulticastFlowMonitorHelper&);
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  MulticastFlowMonitorHelper& operator= (const MulticastFlowMonitorHelper&);

  ObjectFactory m_monitorFactory;                         //!< Object factory
  Ptr<MulticastFlowMonitor> m_multicastFlowMonitor;       //!< the MulticastFlowMonitor object
  Ptr<MulticastFlowClassifier> m_multicastFlowClassifier; //!< the MulticastFlowClassifier object for multicast (IPv4 only)
};

}

#endif /* MULTICAST_FLOW_MONITOR_HELPER_H */

