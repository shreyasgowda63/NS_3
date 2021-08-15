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


#include "kdtree-index.h"

#include "kdtree.h"

#include "ns3/node.h"
#include <cmath>   //sqrt
#include <utility> //pair

NS_LOG_COMPONENT_DEFINE ("KDTree");
namespace ns3 {
  /// Type of tree to be used
  using Tree_T = kdTree<Ptr<const Node>>;
  KDTreeSpatialIndexing::KDTreeSpatialIndexing()
  {
    m_tree = (void*) new Tree_T(0,3);
    //TODO: This needs to be set appropriately because it governs rebalancing.
    ((Tree_T*)(m_tree))->adjust_allowed_depth(100);
  }

  KDTreeSpatialIndexing::~KDTreeSpatialIndexing()
  {
    delete (Tree_T*) m_tree;
    m_tree=nullptr;
  }

  void
  KDTreeSpatialIndexing::doAdd( Ptr<const Node> _node, const Vector3D& _position)
  {
    NS_LOG_FUNCTION (this);
    ((Tree_T*)(m_tree))->insert({_position.x,_position.y,_position.z},_node);
  }


  void
  KDTreeSpatialIndexing::remove( Ptr<const Node> _node)
  {
    NS_LOG_FUNCTION (this);
    ((Tree_T*)(m_tree))->delete_id(_node);
  }

  void
  KDTreeSpatialIndexing::update( Ptr<const Node> _node, const Vector3D& _position)
  {
    NS_LOG_FUNCTION (this);
    ((Tree_T*)(m_tree))->update_id({_position.x,_position.y,_position.z},_node);
  }

  void
  KDTreeSpatialIndexing::ProcessUpdates()
  {
    NS_LOG_FUNCTION (this);
    std::vector<std::vector<double>> points;
    std::vector<Ptr<const Node>> ids;
    for (auto &n : m_nodesToUpdate) {
        auto nd = n.first;
        auto p = nd->GetObject<MobilityModel> ()->GetPosition ();
        points.push_back({p.x,p.y,p.z});
        ids.push_back(nd);
      }
    ((Tree_T*)(m_tree))->update_ids(points,ids);
    m_nodesToUpdate.clear();
  }

  std::vector<Ptr<const Node> > //todo make WifiPhy later
  KDTreeSpatialIndexing::getNodesInRange( double _range, const Vector& _position, const Ptr<const Node> _sourceNode)
  {
    NS_LOG_FUNCTION (this);
    ProcessUpdates();
    std::vector<Ptr<const Node> > nodes;
    double point_this[3] = {_position.x,_position.y,_position.z};
    double point_low[3] = {point_this[0],point_this[1],point_this[2]};
    double point_high[3] = {point_low[0],point_low[1],point_low[2]};
    point_low[0] -= _range;
    point_high[0] += _range;
    point_low[1] -= _range;
    point_high[1] += _range;
    point_low[2] -= _range;
    point_high[2] += _range;
    double range_squared = _range*_range;
  
    ((Tree_T*)(m_tree))->range_search(point_low,point_high,nodes);
    nodes.erase (std::remove_if (nodes.begin (), nodes.end (),
                                [&] (Ptr<const Node> &_n) {
                                  auto p = ((Tree_T*)(m_tree))->get_position(_n);
                                  return CalculateDistanceSquared (_position, {p[0], p[1], p[2]}) >
                                         range_squared;
                                }),
                nodes.end());
    return nodes;
  }

  void
  KDTreeSpatialIndexing::HandlePositionChange(Ptr<const PositionAware> _position_aware)
  {
    NS_LOG_FUNCTION (this);
    ++m_nodesToUpdate[_position_aware->GetObject<Node>()];
  }

} //end namespace ns3

