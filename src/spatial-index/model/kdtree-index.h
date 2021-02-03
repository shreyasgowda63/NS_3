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

#ifndef K_D_TREE_INDEX_H
#define K_D_TREE_INDEX_H

#include "spatial-index.h"

#include "ns3/mobility-model.h"

#include "ns3/position-aware.h"
#include <cstdint> //uint64_t
#include <queue>
#include <unordered_map>

/// What we are using as a key type for an unordered map.
using KeyT = ns3::Ptr<const ns3::Node>;
namespace std {

/**
 * \brief Defining has functor so that unordered map can deal with our key type
 */
template <>
struct hash<KeyT>
{
  /**
   * \brief Callable of the functor, provides a hash method for key type.
   * 
   * \param _key Key to hash
   * \return std::size_t The hash
   */
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

  /**
   * \ingroup spatial-index
   *
   * \brief Spatial index implementation using a k-d-tree.
   */
  class KDTreeSpatialIndexing : public ns3::SpatialIndexing
  {
  public:
    KDTreeSpatialIndexing();
    ~KDTreeSpatialIndexing();
    /**
     * \brief k-d-tree implementation of add, begins tracking of node
     * 
     * \param _node The node to track
     * \param _position The position at time of insertion
     */
    void doAdd(Ptr<const Node> _node, const Vector& _position)    override;
    /**
     * \brief k-d-tree implementation of remove, stop tracking node
     * 
     * \param _node The node to remove from spatial indexing
     */
    void remove(Ptr<const Node> _node)                         override;
    /**
     * \brief k-d-tree implementation of update
     * Includes some optimizations over basic remove and add. 
     * \param _node The node to update the position of
     * \param _position the new position to use
     */
    void update(Ptr<const Node> _node, const Vector& _position) override;
    /**
     * \brief k-d-tree implementation of get nodes in range. gets the nodes within a specified range
     * 
     * \param _range range to use
     * \param _position reference position
     * \param _sourceNode node to filter from results
     * \return std::vector<Ptr<const Node> > list of nodes within range
     */
    std::vector<Ptr<const Node> > getNodesInRange(double _range, const Vector& _position, const Ptr<const Node> _sourceNode) override;
 private:
    /**
    * \brief Called to process the list of nodes that need updatings
    */
    void ProcessUpdates();
    /**
     * \brief Process position change events from the PositionAware object
     * \param _position_aware The PositionAware that triggered the callback.
     */
    void HandlePositionChange(Ptr<const PositionAware> _position_aware) override;
    void* m_tree; ///< K-D-tree structure
    std::unordered_map<ns3::Ptr<const ns3::Node>, size_t> m_nodesToUpdate; ///< List of nodes that need their position updated before a query is valid
  };

}

#endif /* K_D_TREE_INDEX_H */

