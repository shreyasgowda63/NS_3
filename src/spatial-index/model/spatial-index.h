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

#ifndef SPATIAL_INDEX_H
#define SPATIAL_INDEX_H

#include "ns3/vector.h"
#include "ns3/object.h"
#include "ns3/node.h"
#include "ns3/position-aware.h"

#include <vector>

namespace ns3 {

class Node;
class MobilityModel;

class SpatialIndexing : public Object
{
public:
  typedef std::pair<Ptr<const Node>,double> RangeEntry_t;
  typedef std::vector<RangeEntry_t>                  RangeList_t;

  void AddIfInRange (std::pair<ns3::Ptr<const ns3::Node>, ns3::Vector> nodeVec, const Vector& position, double range_squared, std::vector<Ptr<const Node> >& nodes);
  void add (Ptr<const Node> _node, const Vector &position);
  virtual void doAdd (Ptr<const Node> _node, const Vector &position) = 0;
  virtual //size_t
  void remove (Ptr<const Node> _node)              = 0;
  virtual void update (Ptr<const Node> _node, const Vector& position)      = 0;
  virtual std::vector<Ptr<const Node> > getNodesInRange (double _range, const Vector& position, const Ptr<const Node> sourceNode) = 0; //todo make generic return type?
  virtual void HandlePositionChange (Ptr<const PositionAware> position_aware) = 0;
};

}//namespace ns3
#endif//SPATIAL_INDEX_H

