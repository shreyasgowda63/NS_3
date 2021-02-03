/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2020 National Technology & Engineering Solutions of Sandia,
 * LLC (NTESS). Under the terms of Contract DE-NA0003525 with NTESS,
 * the U.S. Government retains certain rights in this software.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 *
 */

#include "single-model-spectrum-channel-si.h"

#include <ns3/angles.h>
#include <ns3/antenna-model.h>
#include <ns3/boolean.h>
#include <ns3/double.h>
#include <ns3/kdtree-index.h>
#include <ns3/log.h>
#include <ns3/mobility-model.h>
#include <ns3/net-device.h>
#include <ns3/node.h>
#include <ns3/object.h>
#include <ns3/packet-burst.h>
#include <ns3/packet.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/simulator.h>
#include <ns3/spectrum-phy.h>
#include <ns3/spectrum-propagation-loss-model.h>

#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SingleModelSpectrumChannelSpatialIndex");

NS_OBJECT_ENSURE_REGISTERED (SingleModelSpectrumChannelSpatialIndex);

SingleModelSpectrumChannelSpatialIndex::SingleModelSpectrumChannelSpatialIndex ()
{
  NS_LOG_FUNCTION (this);
  m_spatialIndex = new KDTreeSpatialIndexing ();
}

TypeId
SingleModelSpectrumChannelSpatialIndex::GetTypeId (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  static TypeId tid =
      TypeId ("ns3::SingleModelSpectrumChannelSpatialIndex")
          .SetParent<SingleModelSpectrumChannel> ()
          .SetGroupName ("Spectrum")
          .AddConstructor<SingleModelSpectrumChannelSpatialIndex> ()
          .AddAttribute (
              "ReceiveClipRange", "Range at which to clip reception event scheduling",
              DoubleValue (0),
              MakeDoubleAccessor (&SingleModelSpectrumChannelSpatialIndex::m_receive_clip_range),
              MakeDoubleChecker<double> ())
          .AddAttribute ("EnableSpatialIndexing",
                         "If true, enable spatial indexing for faster wireless simulations.",
                         BooleanValue (false), // TODO later may want to change default to true
                         MakeBooleanAccessor (
                             &SingleModelSpectrumChannelSpatialIndex::m_spatialIndexingEnabled),
                         MakeBooleanChecker ());
  return tid;
}

void
SingleModelSpectrumChannelSpatialIndex::AddRx (Ptr<SpectrumPhy> phy)
{
  NS_LOG_FUNCTION (this << phy);
  SingleModelSpectrumChannel::AddRx (phy);
  auto m = phy->GetMobility ();
  auto n = m->GetObject<Node> (); // TODO: convert spatial indexing to objects?
  m_spatialIndex->add (n, m->GetPosition ());
}

bool
SingleModelSpectrumChannelSpatialIndex::ProcessTxParams (Ptr<SpectrumSignalParameters> txParams)
{
  if (!SingleModelSpectrumChannel::ProcessTxParams (txParams))
    return false;
  Ptr<MobilityModel> senderMobility = txParams->txPhy->GetMobility ();
  if (m_spatialIndexingEnabled)
    {
      m_nodesInRange.clear ();
      m_nodesInRange =
          m_spatialIndex->getNodesInRange (m_receive_clip_range, senderMobility->GetPosition (),
                                           txParams->txPhy->GetDevice ()->GetNode ());
      //sort for efficient lookup using binary_search
      std::sort (m_nodesInRange.begin (), m_nodesInRange.end ());
    }
  return true;
}

// bool
// operator< (const ns3::Ptr<ns3::Node> _lhs, ns3::Ptr<const ns3::Node>)
// {
//   return _lhs.
// }
bool
SingleModelSpectrumChannelSpatialIndex::CheckValidPhy (Ptr<SpectrumPhy> phy)
{
  if (!SingleModelSpectrumChannel::CheckValidPhy (phy))
    return false;
  if (!m_spatialIndexingEnabled)
    return true;
  return std::binary_search (m_nodesInRange.begin (), m_nodesInRange.end (),
                            (phy->GetDevice ()->GetNode ()));
}

} // namespace ns3
