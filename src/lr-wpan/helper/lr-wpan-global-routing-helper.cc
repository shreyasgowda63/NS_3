/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Institute of Operating Systems and Computer Networks, TU Braunschweig
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
 * Author: Philip Hönnecke <p.hoennecke@tu-braunschweig.de>
 */

#include "lr-wpan-global-routing-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LrWpanGlobalRoutingHelper");

NS_OBJECT_ENSURE_REGISTERED (LrWpanGlobalRoutingHelper);

TypeId
LrWpanGlobalRoutingHelper::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::LrWpanGlobalRoutingHelper")
          .SetParent<Object> ()
          .SetGroupName ("LrWpan")
          .AddConstructor<LrWpanGlobalRoutingHelper> ()
          .AddAttribute ("SendDiscoveryTime",
                         "Time to wait before sending the first discovery packet.",
                         TimeValue (Seconds (0.5)),
                         MakeTimeAccessor (&LrWpanGlobalRoutingHelper::GetSendDiscoveryTime,
                                           &LrWpanGlobalRoutingHelper::SetSendDiscoveryTime),
                         MakeTimeChecker (Seconds (0.0)))
          .AddAttribute ("PacketOffsetTime",
                         "The time to wait until sending the next"
                         "discovery packet (from next node).",
                         TimeValue (Seconds (0.01)),
                         MakeTimeAccessor (&LrWpanGlobalRoutingHelper::GetPacketOffsetTime,
                                           &LrWpanGlobalRoutingHelper::SetPacketOffsetTime),
                         MakeTimeChecker (Seconds (0.0001)))
          .AddAttribute ("WaitTime",
                         "Time to wait between transmitting the first "
                         "discovery packet and calculating the routes.",
                         TimeValue (Seconds (0.5)),
                         MakeTimeAccessor (&LrWpanGlobalRoutingHelper::GetWaitTime,
                                           &LrWpanGlobalRoutingHelper::SetWaitTime),
                         MakeTimeChecker ())
          .AddAttribute ("CreateDirectRoutes",
                         "Whether to create direct routes (destination = gateway).",
                         BooleanValue (false),
                         MakeBooleanAccessor (&LrWpanGlobalRoutingHelper::GetCreateDirectRoutes,
                                              &LrWpanGlobalRoutingHelper::SetCreateDirectRoutes),
                         MakeBooleanChecker ());
  return tid;
}

LrWpanGlobalRoutingHelper::LrWpanGlobalRoutingHelper ()
{
  NS_LOG_FUNCTION (this);
}

LrWpanGlobalRoutingHelper::~LrWpanGlobalRoutingHelper ()
{
  NS_LOG_FUNCTION (this);
}

void LrWpanGlobalRoutingHelper::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  if (!m_calcRoutesEvent.IsExpired ())
    {
      NS_ABORT_MSG ("LrWpanGlobalRoutingHelper: Destroying this object before the calculation is "
                    "completed is not allowed! Member functions of this are still to be used!");
    }
}

void
LrWpanGlobalRoutingHelper::SetRoutingCalcCompleteCallback (RoutingCalcCompleteCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_callback = cb;
}

NetDeviceContainer
LrWpanGlobalRoutingHelper::Install (NetDeviceContainer lrWpanDevices, uint16_t id)
{
  NS_LOG_FUNCTION (this << &lrWpanDevices << id);

  NetDeviceContainer container;
  for (NetDeviceContainer::Iterator it = lrWpanDevices.Begin (); it != lrWpanDevices.End (); it++)
    {
      Ptr<LrWpanNetDevice> dev = (*it)->GetObject<LrWpanNetDevice> ();
      NS_ASSERT_MSG (dev != 0, "LrWpanGlobalRoutingHelper: Can't install for non-LrWpanNetDevice!");
      container.Add (Install (dev, id));
    }
  return container;
}

Ptr<LrWpanGlobalRoutingDevice>
LrWpanGlobalRoutingHelper::Install (Ptr<LrWpanNetDevice> netDevice, uint16_t id)
{
  NS_LOG_FUNCTION (this << netDevice << id);

  Ptr<LrWpanGlobalRoutingDevice> dev = CreateObject<LrWpanGlobalRoutingDevice> (id);
  netDevice->GetNode ()->AddDevice (dev);
  dev->SetDevice (netDevice);
  dev->SetTransmissionReceivedCallback (
      MakeCallback (&LrWpanGlobalRoutingHelper::TransmissionReceived, this));
  Simulator::Schedule (m_sendDiscoveryTime, &LrWpanGlobalRoutingDevice::SendDiscoveryTransmission,
                       dev);
  NS_LOG_LOGIC ("Scheduled with m_sendDiscoveryTime = " << m_sendDiscoveryTime);

  // Create new entry in m_neighborMaps for id if not existent yet.
  // Also schedule new event for when to calculate routes
  if (m_neighborMaps.count (id) == 0)
    {
      m_neighborMaps[id];
      m_netDevices[id];
      m_calcRoutesEvent = Simulator::Schedule (
          m_sendDiscoveryTime + m_waitTime, &LrWpanGlobalRoutingHelper::CalculateRoutes, this, id);
    }
  m_netDevices[id].Add (dev);
  m_sendDiscoveryTime += m_packetOffsetTime;
  return dev;
}

void
LrWpanGlobalRoutingHelper::TransmissionReceived (Ptr<LrWpanGlobalRoutingDevice> device,
                                                 const Address &sender, uint16_t id)
{
  NS_LOG_FUNCTION (this << device << sender << id);

  NS_ASSERT (m_neighborMaps.count (id) > 0);

  Address devAddr = device->GetAddress ();

  NS_LOG_INFO ("LrWpanGlobalRoutingHelper: New neighbor in id " << id << " for " << devAddr
                                                                << " is " << sender);
  m_neighborMaps.at (id)[devAddr].push_back (sender);
}

void
LrWpanGlobalRoutingHelper::PrintNeighbors (uint16_t id)
{
  NS_LOG_FUNCTION (this << id);

  NS_ASSERT (m_neighborMaps.count (id) > 0);

  NS_ASSERT_MSG (m_neighborMaps.count (id) > 0,
                 "LrWpanGlobalRoutingHelper: Map with this id not found!");

  NeighborsMap &nm = m_neighborMaps.at (id);

  for (NeighborsMap::iterator it = nm.begin (); it != nm.end (); it++)
    {
      NS_LOG_UNCOND ("Incoming neighbors of " << it->first);
      for (std::vector<Address>::iterator neighbor = it->second.begin ();
           neighbor != it->second.end (); neighbor++)
        {
          NS_LOG_UNCOND ("\t" << *neighbor);
        }
    }
}

void
LrWpanGlobalRoutingHelper::SetSendDiscoveryTime (Time t)
{
  NS_LOG_FUNCTION (this << t);
  m_sendDiscoveryTime = t;
}
Time
LrWpanGlobalRoutingHelper::GetSendDiscoveryTime () const
{
  NS_LOG_FUNCTION (this);
  return m_sendDiscoveryTime;
}
void
LrWpanGlobalRoutingHelper::SetPacketOffsetTime (Time t)
{
  NS_LOG_FUNCTION (this << t);
  m_packetOffsetTime = t;
}
Time
LrWpanGlobalRoutingHelper::GetPacketOffsetTime () const
{
  NS_LOG_FUNCTION (this);
  return m_packetOffsetTime;
}
void
LrWpanGlobalRoutingHelper::SetWaitTime (Time t)
{
  NS_LOG_FUNCTION (this << t);
  m_waitTime = t;
}
Time
LrWpanGlobalRoutingHelper::GetWaitTime () const
{
  NS_LOG_FUNCTION (this);
  return m_waitTime;
}
void
LrWpanGlobalRoutingHelper::SetCreateDirectRoutes (bool create)
{
  NS_LOG_FUNCTION (this << create);
  m_createDirectRoutes = create;
}
bool
LrWpanGlobalRoutingHelper::GetCreateDirectRoutes () const
{
  NS_LOG_FUNCTION (this);
  return m_createDirectRoutes;
}

void
LrWpanGlobalRoutingHelper::CalculateRoutes (uint16_t id)
{
  NS_LOG_FUNCTION (this << id);

  /*
   * This method (and LrWpanGlobalRoutingHelper::BFS) could be improved
   * with better data structures.
   * Additionally, the route calculation could be improved by getting
   * data about neighbors (direct routes) as well as routes as part of
   * bigger ones.
   */

  NS_ASSERT (m_neighborMaps.count (id) > 0);

  // Print found neighbors if LogLevel::LOG_DEBUG is enabled
  if (LogComponent::GetComponentList ()
          ->at ("LrWpanGlobalRoutingHelper")
          ->IsEnabled (LogLevel::LOG_INFO))
    {
      PrintNeighbors (id);
    }

  // Create directed graph

  // Create nodes
  std::map<Address, Node *> graph;
  uint32_t idCounter = 0;
  for (NetDeviceContainer::Iterator it = m_netDevices.at (id).Begin ();
       it != m_netDevices.at (id).End (); it++)
    {
      Node *n = new Node (idCounter++, LrWpanRoute::ConvertAddress ((*it)->GetAddress ()),
                          (*it)->GetObject<LrWpanStaticRoutingDevice> ());
      NS_ASSERT (n->dev != nullptr);
      graph[n->addr] = n;
    }

  // Add edges
  // Iterate over receivers of discovery packets
  for (NeighborsMap::iterator receiver = m_neighborMaps[id].begin ();
       receiver != m_neighborMaps[id].end (); receiver++)
    {
      Address rxAddr = LrWpanRoute::ConvertAddress (receiver->first);
      // Find receiver's Node
      Node *rxNode = nullptr;
      // graph.at(rxAddr) somehow isn't working
      for (std::map<Address, Node *>::iterator it = graph.begin (); it != graph.end (); it++)
        {
          if (it->first == rxAddr)
            {
              rxNode = it->second;
              break;
            }
        }
      NS_ASSERT (rxNode != nullptr);

      // Iterate over their transmitters
      for (std::vector<Address>::iterator transmitter = receiver->second.begin ();
           transmitter != receiver->second.end (); transmitter++)
        {
          Address txAddr = LrWpanRoute::ConvertAddress (*transmitter);
          // Find transmitter's Node
          Node *txNode = nullptr;
          // graph.at(txAddr) somehow isn't working
          for (std::map<Address, Node *>::iterator it = graph.begin (); it != graph.end (); it++)
            {
              if (it->first == txAddr)
                {
                  txNode = it->second;
                  break;
                }
            }
          NS_ASSERT (txNode != nullptr);
          txNode->neighbors.push_back (rxNode);
        }
    }

  // Do breadth-first search for each node.
  // Since we are walking backwards (instead of knowing where we can send to, we know where we can receive from)
  // we will first loop over the destinations.
  // This is only relevant for non-bidirectional links
  for (std::map<Address, Node *>::iterator destIt = graph.begin (); destIt != graph.end ();
       destIt++)
    {
      Node *dest = destIt->second;
      // Modify the Node::next values for all reachable nodes
      dest->next = nullptr;
      BFS (graph, dest);

      for (std::map<Address, Node *>::iterator srcIt = graph.begin (); srcIt != graph.end ();
           srcIt++)
        {
          Node *src = srcIt->second;
          // Skip src == dest
          if (src == dest)
            {
              continue;
            }

          // Skip direct routes
          if (!m_createDirectRoutes)
            {
              bool skip = false;
              // Check if this is direct route
              for (Node *neighbor : dest->neighbors)
                {
                  if (neighbor == src)
                    {
                      skip = true;
                      break;
                    }
                }
              if (skip)
                {
                  continue;
                }
            }

          // Find the gateway
          if (src->next == nullptr)
            { // No gateway found -> No route found
              NS_LOG_DEBUG ("LrWpanGlobalRoutingHelper: Couldn't find a route from "
                            << src->addr << " to " << dest->addr);
            }
          else
            { // Route found
              Ptr<LrWpanRoute> route = CreateObject<LrWpanRoute> ();
              route->SetSource (src->addr);
              route->SetGateway (src->next->addr);
              route->SetDestination (dest->addr);
              src->dev->AddStaticRoute (route);
              NS_LOG_DEBUG ("LrWpanGlobalRoutingHelper: Route from "
                            << src->addr << " to " << dest->addr << " via " << src->next->addr);
            }
        }
    }

  // Delete nodes
  for (std::map<Address, Node *>::iterator it = graph.begin (); it != graph.end (); it++)
    {
      delete it->second;
    }

  m_callback (id);
}

void
LrWpanGlobalRoutingHelper::BFS (std::map<Address, Node *> &graph, Node *dest)
{
  std::queue<Node *> q;
  std::vector<uint32_t> discovered (graph.size ());

  // Add destination (start) vertex to queue
  // This is because we will go "backwards"
  q.push (dest);
  discovered[dest->id] = true;

  Node *v = nullptr;
  while (!q.empty ())
    {
      v = q.front ();
      q.pop ();
      for (Node *neighbor : v->neighbors)
        {
          if (!discovered[neighbor->id])
            {
              discovered[neighbor->id] = true;
              neighbor->next = v;

              q.push (neighbor);
            }
        }
    }
}

} // namespace ns3