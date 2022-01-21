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

#include "bruteforce.h"

#include "ns3/mobility-model.h"
#include <cfloat>
#include <cmath>

namespace ns3 {
void
BruteForceSpatialIndexing::DoAdd (PointerType _node, const Vector3D& _position)
{
  m_map[_node] = _position;
}

void
BruteForceSpatialIndexing::Remove (PointerType _node)
{
  m_map.erase (_node);
}

void
BruteForceSpatialIndexing::Update (PointerType _node, const Vector3D& _position)
{
  Add (_node, _position); //update is just the same as Add in this case
}

std::vector<ns3::SpatialIndexing::PointerType > BruteForceSpatialIndexing::GetNodesInRange ( double _range, const Vector& _position, const PointerType _sourceNode) //MobilityModel& _node, double _range)
{
  static std::vector<PointerType > nodes;
  nodes.clear ();
  double range_squared = _range * _range;

  for (auto it = m_map.begin (); it != m_map.end (); ++it)
    {
      AddIfInRange (*it, _position, range_squared, nodes);
    }
  return nodes;
}

void
BruteForceSpatialIndexing::HandlePositionChange (Ptr<const PositionAware> /*_position_aware*/)
{
  //nothing for now.
}

}//namespace ns3
