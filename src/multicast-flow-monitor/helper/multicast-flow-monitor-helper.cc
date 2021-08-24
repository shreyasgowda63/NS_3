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

#include "multicast-flow-monitor-helper.h"

#include "ns3/multicast-flow-monitor.h"
#include "ns3/ipv4-multicast-flow-classifier.h"
#include "ns3/ipv4-multicast-flow-probe.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/node.h"
#include "ns3/node-list.h"

namespace ns3 {

MulticastFlowMonitorHelper::MulticastFlowMonitorHelper ()
{
  m_monitorFactory.SetTypeId ("ns3::MulticastFlowMonitor");
}

MulticastFlowMonitorHelper::~MulticastFlowMonitorHelper ()
{
  if (m_multicastFlowMonitor)
    {
      m_multicastFlowMonitor->Dispose ();
      m_multicastFlowMonitor = 0;
      m_multicastFlowClassifier = 0;
    }
}

void
MulticastFlowMonitorHelper::SetMulticastMonitorAttribute (std::string n1, const AttributeValue &v1)
{
  m_monitorFactory.Set (n1, v1);
}

Ptr<MulticastFlowMonitor>
MulticastFlowMonitorHelper::GetMonitor ()
{
  if (!m_multicastFlowMonitor)
    {
      m_multicastFlowMonitor = m_monitorFactory.Create<MulticastFlowMonitor> ();
      m_multicastFlowClassifier = Create<Ipv4MulticastFlowClassifier> ();
      m_multicastFlowMonitor->AddMulticastFlowClassifier (m_multicastFlowClassifier);
    }
  return m_multicastFlowMonitor;
}

Ptr<MulticastFlowClassifier>
MulticastFlowMonitorHelper::GetClassifier ()
{
  if (!m_multicastFlowClassifier)
    {
      m_multicastFlowClassifier = Create<Ipv4MulticastFlowClassifier> ();
    }
  return m_multicastFlowClassifier;
}

Ptr<MulticastFlowMonitor>
MulticastFlowMonitorHelper::Install (Ptr<Node> node, std::map<Ipv4Address, std::vector<uint32_t> > addressGroups)
{
  Ptr<MulticastFlowMonitor> monitor = GetMonitor ();
  Ptr<MulticastFlowClassifier> classifier = GetClassifier ();
  Ptr<Ipv4L3Protocol> ipv4 = node->GetObject<Ipv4L3Protocol> ();
  if (ipv4)
    {
      Ptr<Ipv4MulticastFlowProbe> probe = Create<Ipv4MulticastFlowProbe> (monitor,
                                                                          DynamicCast<Ipv4MulticastFlowClassifier> (classifier),
                                                                          node,
                                                                          addressGroups);
    }
  return m_multicastFlowMonitor;
}

Ptr<MulticastFlowMonitor>
MulticastFlowMonitorHelper::Install (NodeContainer nodes, std::map<Ipv4Address, std::vector<uint32_t> > addressGroups)
{
  for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i)
    {
      Ptr<Node> node = *i;
      if (node->GetObject<Ipv4L3Protocol> ())
        {
          Install (node, addressGroups);
        }
    }
  return m_multicastFlowMonitor;
}

Ptr<MulticastFlowMonitor>
MulticastFlowMonitorHelper::InstallAll (std::map<Ipv4Address, std::vector<uint32_t> > addressGroups)
{
  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i)
    {
      Ptr<Node> node = *i;
      if (node->GetObject<Ipv4L3Protocol> ())
        {
          Install (node, addressGroups);
        }
    }
  return m_multicastFlowMonitor;
}


} // namespace ns3
