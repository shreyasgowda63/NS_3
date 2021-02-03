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
using KeyT = ns3::SpatialIndexing::PointerType;
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
    static const auto shift = size_t (std::log2 (1u + sizeof (KeyT)));
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
  KDTreeSpatialIndexing ();
  ~KDTreeSpatialIndexing ();
  /**
   * \brief k-d-tree implementation of add, begins tracking of node
   *
   * \param node The node to track
   * \param position The position at time of insertion
   */
  void DoAdd (PointerType node, const Vector &position) override;
  /**
   * \brief k-d-tree implementation of remove, stop tracking node
   *
   * \param node The node to remove from spatial indexing
   */
  void Remove (PointerType node) override;
  /**
   * \brief k-d-tree implementation of update
   * Includes some optimizations over basic remove and add.
   * \param node The node to update the position of
   * \param position the new position to use
   */
  void Update (PointerType node, const Vector &position) override;
  /**
   * \brief k-d-tree implementation of get nodes in range. gets the nodes within a specified range
   *
   * \param range range to use
   * \param position reference position
   * \param sourceNode node to filter from results
   * \return std::vector<PointerType > list of nodes within range
   */
  std::vector<PointerType> GetNodesInRange (double range, const Vector &position,
                                                const PointerType sourceNode) override;

private:
  /**
  * \brief Called to process the list of nodes that need updating
  */
  void ProcessUpdates ();
  /**
   * \brief Process position change events from the PositionAware object
   * \param position_aware The PositionAware that triggered the callback.
   */
  void HandlePositionChange (Ptr<const PositionAware> position_aware) override;
  void *m_tree; ///< K-D-tree structure
  std::unordered_map<PointerType, size_t> m_nodesToUpdate; ///< List of nodes that need their position updated before a query is valid
  std::vector<std::vector<double>> m_initialPoints; ///< List of points to use at initial bulk insert
  std::vector<PointerType> m_initialRefs; ///< List of references to use at initial bulk insert
protected:
  void DoInitialize () override;
};

} // namespace ns3

#endif /* K_D_TREE_INDEX_H */
