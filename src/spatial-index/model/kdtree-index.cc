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

#include "spatial/point_multiset.hpp"
#include "spatial/region_iterator.hpp" //spatial::region_end

#include "ns3/node.h"
#include <cmath>   //sqrt
#include <utility> //pair

NS_LOG_COMPONENT_DEFINE ("KDTree");
namespace ns3 {

  typedef std::pair<Ptr<const Node>,Vector> KDEntry_t;

  struct Vector3DAccessor{
    double operator() (spatial::dimension_type _dim, const KDEntry_t _p) const {
      switch(_dim){
      case 0: return _p.second.x;
      case 1: return _p.second.y;
      case 2: return _p.second.z;
      default: throw std::out_of_range("dim");
      }
    }
  };

  typedef spatial::point_multiset<3, KDEntry_t, spatial::accessor_less<Vector3DAccessor, KDEntry_t> > Tree_t;

  KDTreeSpatialIndexing::KDTreeSpatialIndexing()
  {
    m_tree = (void*) new Tree_t();
    m_tree_size = 0;
  }

  KDTreeSpatialIndexing::~KDTreeSpatialIndexing()
  {
    delete (Tree_t*) m_tree;
    m_tree=nullptr;
  }

  void
  KDTreeSpatialIndexing::doAdd( Ptr<const Node> _node, const Vector3D& _position)
  {
    NS_LOG_FUNCTION (this);
    ((Tree_t*)(m_tree))->insert(std::make_pair(_node,_position));
    this->m_referencePositions[_node] = _position;
    ++m_tree_size;
  }


  void
  KDTreeSpatialIndexing::remove( Ptr<const Node> _node)
  {
    NS_LOG_FUNCTION (this);
    auto entry = this->m_referencePositions.find (_node);
    if (entry == this->m_referencePositions.end ())
      return;

    auto n = ((Tree_t *) (m_tree))->erase (KDEntry_t (entry->first, entry->second ));
    this->m_referencePositions.erase(entry);
    m_tree_size -= n;
  }

  void
  KDTreeSpatialIndexing::update( Ptr<const Node> _node, const Vector3D& _position)
  {
    NS_LOG_FUNCTION (this);
    auto entry = this->m_referencePositions.find (_node);
    if (entry == this->m_referencePositions.end ())
      return;

    ((Tree_t *) (m_tree))->erase (KDEntry_t (entry->first, entry->second));
    ((Tree_t *) (m_tree))->insert (std::make_pair (_node, _position));
    this->m_referencePositions[_node] = _position;
  }

  void
  KDTreeSpatialIndexing::ProcessUpdates()
  {
    for (auto &n : m_nodesToUpdate) {
        auto nd = n.first;
        update (nd, nd->GetObject<MobilityModel> ()->GetPosition ());
      }
    m_nodesToUpdate.clear();
  }

  std::vector<Ptr<const Node> > //todo make WifiPhy later
  KDTreeSpatialIndexing::getNodesInRange( double _range, const Vector& _position, const Ptr<const Node> _sourceNode)
  {
    NS_LOG_FUNCTION (this);
    ProcessUpdates();
    std::vector<Ptr<const Node> > nodes;
    Vector3D point_this = _position;
    Vector3D point_low = point_this;
    Vector3D point_high = point_low;
    point_low.x -= _range;
    point_high.x += _range;
    point_low.y -= _range;
    point_high.y += _range;
    point_low.z -= _range;
    point_high.z += _range;
    double range_squared = _range*_range;

    auto end= spatial::closed_region_end(*((Tree_t*)(m_tree)), KDEntry_t(NULL,point_low), KDEntry_t(NULL,point_high));
    for(auto it = spatial::closed_region_begin(*((Tree_t*)(m_tree)), KDEntry_t(NULL,point_low), KDEntry_t(NULL,point_high));it != end; ++it)
      {
        AddIfInRange(*it, _position, range_squared, nodes);
      }
    return nodes;
  }

  void
  KDTreeSpatialIndexing::HandlePositionChange(Ptr<const PositionAware> _position_aware)
  {
    NS_LOG_FUNCTION (this);
    ++m_nodesToUpdate[_position_aware->GetObject<Node>()];
  }

} //end namespace ns3

