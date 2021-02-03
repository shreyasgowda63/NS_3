/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

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
    double operator() (spatial::dimension_type dim, const KDEntry_t p) const {
      switch(dim){
      case 0: return p.second.x;
      case 1: return p.second.y;
      case 2: return p.second.z;
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
  KDTreeSpatialIndexing::doAdd( Ptr<const Node> _node, const Vector3D& position)
  {
    NS_LOG_FUNCTION (this);
    ((Tree_t*)(m_tree))->insert(std::make_pair(_node,position));
    this->m_referencePositions[_node] = position;
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
  KDTreeSpatialIndexing::update( Ptr<const Node> _node, const Vector3D& position)
  {
    NS_LOG_FUNCTION (this);
    // remove(_node);
    // doAdd(_node, position);
    auto entry = this->m_referencePositions.find (_node);
    if (entry == this->m_referencePositions.end ())
      return;

    ((Tree_t *) (m_tree))->erase (KDEntry_t (entry->first, entry->second));
    ((Tree_t *) (m_tree))->insert (std::make_pair (_node, position));
    this->m_referencePositions[_node] = position;
  }

  void
  KDTreeSpatialIndexing::ProcessUpdates()
  {
    //TODO handle duplicates either here or in the add
    /*while(m_min_heap.top().first < Simulator::Now())
        auto mobility = m_min_heap.top().second;
        update(mobility, mobility.GetPosition());
        m_minheap.pop();*/
    for (auto &n : m_nodesToUpdate) {
        auto nd = n.first;
        update (nd, nd->GetObject<MobilityModel> ()->GetPosition ());
      }
    m_nodesToUpdate.clear();
    // while(!m_nodesToUpdate.empty()){
    //   Ptr<const Node> nd = m_nodesToUpdate.front();
    //   update(nd, nd->GetObject<MobilityModel>()->GetPosition());
    //   m_nodesToUpdate.pop();
    // }
  }

  std::vector<Ptr<const Node> > //todo make WifiPhy later
  KDTreeSpatialIndexing::getNodesInRange( double range, const Vector& position, const Ptr<const Node> sourceNode)
  {
    NS_LOG_FUNCTION (this);
    //later also return list of positions!!! todo
    //std::vector<Ptr<YansWifiPhy> > phys;  //could make static.
    ProcessUpdates();
    std::vector<Ptr<const Node> > nodes;
    Vector3D point_this = position;
    Vector3D point_low = point_this;
    Vector3D point_high = point_low;
    point_low.x -= range;
    point_high.x += range;
    point_low.y -= range;
    point_high.y += range;
    point_low.z -= range;
    point_high.z += range;
    double range_squared = range*range;

    auto end= spatial::closed_region_end(*((Tree_t*)(m_tree)), KDEntry_t(NULL,point_low), KDEntry_t(NULL,point_high));
    for(auto it = spatial::closed_region_begin(*((Tree_t*)(m_tree)), KDEntry_t(NULL,point_low), KDEntry_t(NULL,point_high));it != end; ++it)
      {
        AddIfInRange(*it, position, range_squared, nodes);
      }
    return nodes;
  }

  void
  KDTreeSpatialIndexing::HandlePositionChange(Ptr<const PositionAware> position_aware)
  {
    NS_LOG_FUNCTION (this);
    ++m_nodesToUpdate[position_aware->GetObject<Node>()];
    //  m_nodesToUpdate.push (position_aware->GetObject<Node> ());
  }

} //end namespace ns3

