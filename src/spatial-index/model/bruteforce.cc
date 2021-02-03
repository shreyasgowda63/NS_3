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
BruteForceSpatialIndexing::doAdd(Ptr<const Node> _node, const Vector3D& position)
{
  m_map[_node] = position;
}

void
BruteForceSpatialIndexing::remove(Ptr<const Node> _node)
{
  //auto node=std::find(m_list.begin(),m_list.end(),_node);
  //m_list.erase(node);
  m_map.erase(_node);
}

void
BruteForceSpatialIndexing::update(Ptr<const Node> _node, const Vector3D& position)
{
  add(_node, position); //update is just the same as add in this case
}

std::vector<Ptr<const Node> > BruteForceSpatialIndexing::getNodesInRange( double _range, const Vector& position, const Ptr<const Node> sourceNode) //MobilityModel& _node, double _range)
{
  static std::vector<Ptr<const Node> > nodes;
  nodes.clear();
  double range_squared = _range*_range;

  for(auto it = m_map.begin(); it != m_map.end(); ++it)
    {
    AddIfInRange(*it, position, range_squared, nodes);
    }
  return nodes;
}

void
BruteForceSpatialIndexing::HandlePositionChange(Ptr<const PositionAware> position_aware)
{
  //nothing for now.  need to add later?
}

}//namespace ns3
