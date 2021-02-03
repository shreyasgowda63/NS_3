/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef KDTREE_INDEX_H
#define KDTREE_INDEX_H

#include "spatial-index.h"

#include "ns3/mobility-model.h"

#include "ns3/position-aware.h"
#include <cstdint> //uint64_t
#include <queue>
#include <unordered_map>

using KeyT = ns3::Ptr<const ns3::Node>;
namespace std {

template <>
struct hash<KeyT>
{
  std::size_t
  operator() (const KeyT &_key) const
  {
    auto val = ns3::GetPointer (_key);
    static const auto shift = size_t(std::log2 (1u + sizeof (KeyT)));
    return (size_t) (val) >> shift;
  }
};

} // namespace std

namespace ns3 {

  class KDTreeSpatialIndexing : public ns3::SpatialIndexing
  {
  public:
    KDTreeSpatialIndexing();
    ~KDTreeSpatialIndexing();

    void doAdd(Ptr<const Node> _node, const Vector& position)    override;
    //size_t
    void remove(Ptr<const Node> _node)                         override;
    void update(Ptr<const Node> _node, const Vector& position) override;
    std::vector<Ptr<const Node> > getNodesInRange(double _range, const Vector& position, const Ptr<const Node> sourceNode) override;
 private:
    void ProcessUpdates();
    void HandlePositionChange(Ptr<const PositionAware> position_aware) override;
    void* m_tree; //why void*?
    uint64_t m_tree_size=0;
    std::unordered_map<KeyT, Vector> m_referencePositions;
    //MinHeap::MinHeap m_min_heap;
    std::unordered_map<KeyT, size_t> m_nodesToUpdate;
  };

}

#endif /* KDTREE_INDEX_H */

