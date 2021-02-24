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

#include "spatial-index.h"

#include "ns3/log.h"
#include "ns3/mobility-model.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("SpatialIndexing");
void
SpatialIndexing::add (Ptr<const Node> _node, const Vector &_position) {
  auto p = _node->GetObject<PositionAware>();
  if(nullptr == p) {
    NS_LOG_WARN("Using Spatial Indexing when Position Aware is not installed");
  } else {
  p->TraceConnectWithoutContext (
      "PositionChange", MakeCallback (&SpatialIndexing::HandlePositionChange, this));
  p->TraceConnectWithoutContext (
      "Timeout", MakeCallback (&SpatialIndexing::HandlePositionChange, this));
  }
  doAdd(_node,_position);
}

void SpatialIndexing::AddIfInRange (
    std::pair<ns3::Ptr<const ns3::Node>, ns3::Vector> _nodeVec,
    const Vector &                                    _position,
    double                                            _range_squared,
    std::vector<Ptr<const Node>> &                    _nodes)
{
  if(CalculateDistanceSquared(_nodeVec.second, _position) <= _range_squared)
    {
    _nodes.push_back(_nodeVec.first);
    }
}

}
