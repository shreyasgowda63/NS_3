/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006,2007 INRIA
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
 * Author: Mathieu Lacage, <mathieu.lacage@sophia.inria.fr>
 */

#include "yans-wifi-channel-si.h"

#include "wifi-utils.h"
#include "yans-wifi-phy.h"

#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/simulator.h"
//#include "ns3/spatial-index.h"
#include "ns3/kdtree-index.h"
#include "ns3/wifi-net-device.h"

namespace ns3 {

//using YansWifiChannel::PhyList;

NS_LOG_COMPONENT_DEFINE ("YansWifiChannelSpatialIndex");

NS_OBJECT_ENSURE_REGISTERED (YansWifiChannelSpatialIndex);

TypeId
YansWifiChannelSpatialIndex::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::YansWifiChannelSpatialIndex")
          .SetParent<YansWifiChannel> ()
          .SetGroupName ("Wifi")
          .AddConstructor<YansWifiChannelSpatialIndex> ()
          .AddAttribute ("ReceiveClipRange", "Range at which to clip reception event scheduling",
                         DoubleValue (0),
                         MakeDoubleAccessor (&YansWifiChannelSpatialIndex::m_receive_clip_range),
                         MakeDoubleChecker<double> ())
          .AddAttribute (
              "EnableSpatialIndexing",
              "If true, enable spatial indexing for faster wireless simulations.",
              BooleanValue (false), // TODO later may want to change default to true
              MakeBooleanAccessor (&YansWifiChannelSpatialIndex::m_spatialIndexingEnabled),
              MakeBooleanChecker ());
  return tid;
}

YansWifiChannelSpatialIndex::YansWifiChannelSpatialIndex ()
{
  NS_LOG_FUNCTION (this);
  m_spatialIndex = new KDTreeSpatialIndexing ();
}

YansWifiChannelSpatialIndex::~YansWifiChannelSpatialIndex ()
{
  NS_LOG_FUNCTION (this);
}

const YansWifiChannel::PhyList
YansWifiChannelSpatialIndex ::getPhyList (const Ptr<YansWifiPhy> sender)
{

  if (m_spatialIndexingEnabled)
  {
    std::vector<Ptr<YansWifiPhy>> phyList;
    // passing a pointer to the sendingNode was supposed to optimize by not sending that node back
    // in the phys, however, it may actually take longer now.  TODO consider reverting.
    std::vector<Ptr<const Node>> nodes = m_spatialIndex->getNodesInRange (
        m_receive_clip_range, sender->GetMobility()->GetPosition (), sender->GetDevice ()->GetNode ());
    GetPhysForNodes (nodes, phyList); // todo, can we optimize this to avoid copying?
    return phyList;
  }
  return m_phyList;
}

void
YansWifiChannelSpatialIndex::Add (Ptr<YansWifiPhy> phy)
{
  NS_LOG_FUNCTION (this << phy);
  auto m = phy->GetMobility ();
  auto n = m->GetObject<Node>(); //TODO: convert spatial indexing to objects?
  m_spatialIndex->add (n, m->GetPosition ());
  m_phyList.push_back (phy);
}

void
YansWifiChannelSpatialIndex::GetPhysForNodes (std::vector<Ptr<const Node>> &nodes,
                                              std::vector<Ptr<YansWifiPhy>> &phys)
{
  for (unsigned int i = 0; i < nodes.size (); i++)
    {
      for (unsigned int j = 0; j < nodes[i]->GetNDevices (); j++)
        {
          Ptr<WifiNetDevice> nd = nodes[i]->GetDevice (j)->GetObject<WifiNetDevice> ();
          if (nd != 0)
            {
              Ptr<YansWifiPhy> phy = nd->GetPhy ()->GetObject<YansWifiPhy> ();
              if (phy != 0)
                {
                  phys.push_back (phy);
                }
            }
        }
    }
}

} // namespace ns3
