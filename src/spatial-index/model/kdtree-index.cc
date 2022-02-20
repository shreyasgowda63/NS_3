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
using Tree_T = kdTree<ns3::SpatialIndexing::PointerType>;
KDTreeSpatialIndexing::KDTreeSpatialIndexing ()
{
}

KDTreeSpatialIndexing::~KDTreeSpatialIndexing ()
{
  delete (Tree_T*) m_tree;
  m_tree = nullptr;
}

void
KDTreeSpatialIndexing::DoAdd (PointerType node, const Vector3D &position)
{
  NS_LOG_FUNCTION (this);
  if (IsInitialized ())
    {
      ((Tree_T *) (m_tree))->insert ({position.x, position.y, position.z}, node);
    }
  else
    {
      m_initialPoints.push_back ({{position.x, position.y, position.z}});
      m_initialRefs.push_back (node);
    }
}

void
KDTreeSpatialIndexing::Remove ( PointerType node)
{
  NS_LOG_FUNCTION (this);
  ((Tree_T*)(m_tree))->delete_id (node);
}

void
KDTreeSpatialIndexing::Update ( PointerType node, const Vector3D& position)
{
  NS_LOG_FUNCTION (this);
  ((Tree_T*)(m_tree))->update_id ({position.x,position.y,position.z},node);
}

void
KDTreeSpatialIndexing::ProcessUpdates ()
{
  NS_LOG_FUNCTION (this);
  std::vector<std::vector<double> > points;
  std::vector<PointerType > ids;
  for (auto &n : m_nodesToUpdate)
    {
      auto nd = n.first;
      auto p = nd->GetObject<MobilityModel> ()->GetPosition ();
      points.push_back ({p.x,p.y,p.z});
      ids.push_back (nd);
    }
  if (!points.empty ())
    {
      ((Tree_T *) (m_tree))->update_ids (points, ids);
      m_nodesToUpdate.clear ();
    }
}

std::vector<ns3::SpatialIndexing::PointerType >   //todo make WifiPhy later
KDTreeSpatialIndexing::GetNodesInRange ( double range, const Vector& position, const PointerType sourceNode)
{
  NS_LOG_FUNCTION (this);
  if (!IsInitialized ())
    {
      Initialize ();
    }
  ProcessUpdates ();
  std::vector<PointerType > nodes;
  double point_this[3] = {position.x,position.y,position.z};
  double point_low[3] = {point_this[0],point_this[1],point_this[2]};
  double point_high[3] = {point_low[0],point_low[1],point_low[2]};
  point_low[0] -= range;
  point_high[0] += range;
  point_low[1] -= range;
  point_high[1] += range;
  point_low[2] -= range;
  point_high[2] += range;
  double range_squared = range * range;

  ((Tree_T*)(m_tree))->range_search (point_low,point_high,nodes);
  nodes.erase (std::remove_if (nodes.begin (), nodes.end (),
                               [&] (PointerType &_n) {
      auto p = ((Tree_T*)(m_tree))->get_position (_n);
      return CalculateDistanceSquared (position, {p[0], p[1], p[2]}) >
      range_squared;
    }),
               nodes.end ());
  return nodes;
}

void
KDTreeSpatialIndexing::HandlePositionChange (Ptr<const PositionAware> position_aware)
{
  NS_LOG_FUNCTION (this);
  ++m_nodesToUpdate[position_aware->GetObject<Node>()];
}

void
KDTreeSpatialIndexing::DoInitialize ()
{
  if (!IsInitialized ())
    {
      m_tree = (void *) new Tree_T (m_initialPoints.size (), 3);
      ((Tree_T *) (m_tree))
          ->build_kdTree_median_nthelement (m_initialPoints.size (), 3, m_initialPoints,
                                            m_initialRefs);
      m_initialRefs.clear ();
      m_initialPoints.clear ();
    }
}

} //end namespace ns3

