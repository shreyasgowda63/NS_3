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

/**
 * \ingroup spatial-index
 * \brief Stores the recent position of nodes enabling quick spatial queries.
 *
 * This is a base class for specific spatial indices.
 */
class SpatialIndexing : public Object
{
public:
  typedef std::pair<Ptr<const Node>,double> RangeEntry_t;
  typedef std::vector<RangeEntry_t>         RangeList_t;
  /** @brief Add in-range nodes to a list.
   * Add given node to given list if within some given range of a given
   * position
   * 
   * @param _nodeVec The node in question
   * @param _position reference position
   * @param range_squared The square of the range to use
   * @param _nodes vector to add node to if it is in range
   */
  void AddIfInRange (std::pair<ns3::Ptr<const ns3::Node>, ns3::Vector> _nodeVec, const Vector& _position, double _range_squared, std::vector<Ptr<const Node> >& _nodes);
  /** @brief Begin tracking the given node in the spatial index.
   * Concrete interface to add node to the spatial indexing scheme
   * 
   * @param _node The node to add
   * @param _position The position to add it at
   */
  void add (Ptr<const Node> _node, const Vector &_position);
  /**
   * @brief Implementation of spatial index insertion
   * @note all implementations need to override this.
   * 
   * @param _node The node to add
   * @param _position The position to add it at
   */
  virtual void doAdd (Ptr<const Node> _node, const Vector &_position) = 0;
  /**
   * @brief  Stop tracking node in spatial indexing
   * Implentation of spatial indexing removal
   * @note all implementations need to override this
   * 
   * @param _node The node to remove
   */
  virtual void remove (Ptr<const Node> _node) = 0;
  /**
   * @brief Update node in spatial indexing scheme
   * 
   * @param _node node to update
   * @param _position  the new position of the node
   */
  virtual void update (Ptr<const Node> _node, const Vector& _position)      = 0;
  /**
   * @brief Get the Nodes In Range of a reference position
   * 
   * @param _range range to use
   * @param _position  reference position
   * @param _sourceNode  reference node (can be used to filter self from results)
   * @return std::vector<Ptr<const Node> > vector of nodes in range
   */
  virtual std::vector<Ptr<const Node> > getNodesInRange (double _range, const Vector& _position, const Ptr<const Node> _sourceNode) = 0; //todo make generic return type?
  /**
   * @brief Callback for handling position change events from position-aware module
   * 
   * @param _position_aware 
   */
  virtual void HandlePositionChange (Ptr<const PositionAware> _position_aware) = 0;
};

}//namespace ns3
#endif//SPATIAL_INDEX_H

