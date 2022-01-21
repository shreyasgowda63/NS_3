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
SpatialIndexing::Add (PointerType node, const Vector &position)
{
  auto p = node->GetObject<PositionAware>();
  if (nullptr == p)
    {
      NS_LOG_WARN ("Using Spatial Indexing when Position Aware is not installed");
    }
  else
    {
      p->TraceConnectWithoutContext (
        "PositionChange", MakeCallback (&SpatialIndexing::HandlePositionChange, this));
      p->TraceConnectWithoutContext (
        "Timeout", MakeCallback (&SpatialIndexing::HandlePositionChange, this));
    }
  DoAdd (node,position);
}

void SpatialIndexing::AddIfInRange (
  std::pair<PointerType, ns3::Vector> nodeVec,
  const Vector &                                    position,
  double                                            range_squared,
  std::vector<PointerType > &                    nodes)
{
  if (CalculateDistanceSquared (nodeVec.second, position) <= range_squared)
    {
      nodes.push_back (nodeVec.first);
    }
}

}
