/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Lawrence Livremore National Laboratory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */
#ifndef HEX_POSITION_ALLOCATOR_H
#define HEX_POSITION_ALLOCATOR_H

#include "position-allocator.h"

#include <cmath>  // sqrt


namespace ns3 {

/**
 * \ingroup mobility
 * \brief Allocate positions from a hexagonal grid.
 * The distance between hexagons, as well as the total size of the grid
 * describe the overall layout.
 *
 * The implementation used here closely follows the excellent article
 * "Hexagonal Grids", https://www.redblobgames.com/grids/hexagons/#basics
 *
 * In the language of that article, this generates points
 * in a hexagonal grid in "pointy-topped" orientation.
 * The overall layout is itself a hexagon.
 */
class HexagonalPositionAllocator : public PositionAllocator
{
public:
  /**
   * Register this type with the TypeId system.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /** Default constructor. */
  HexagonalPositionAllocator (void);
  /** Destructor */
  virtual ~HexagonalPositionAllocator ();

  /**
   * Set the distance between the hexagon centers, in meters.
   * \param [in] s Distance between adjacent hexagons, in meters
   */
  void SetSpacing (const double s);

  /**
   * Get the spacing between node points in the grid, in meters.
   * \return The distance in meters between grid points.
   */
  double GetSpacing (void) const;

  /**
   * \brief Set the overall size of the grid, in numbers of rings.
   * The central node is notionally in ring 0.
   * Ring 1 contains 6 additional nodes.
   * 
   * The total number of grid points will be `1 + 3 r (r + 1)`.
   * You can also get the total number of grid points with GetN
   * \param [in] r The number of rings in the grid.
   */
  void SetRings (int r);

  /**
   * Get the number of rings in the grid.
   * \returns The number of rings in the grid.
   */
  int GetRings (void) const;

  /**
   * Get the total number of points in the grid.
   * \returns The total number of points in the grid.
   */
  std::size_t GetN (void) const;

  /**
   * \brief Get the physical distance to a corner of the grid from the center,
   * in meters.
   * \return The radius of the overall grid, from the center to any
   *         of the corner points, in meters.
   */
  double GetRadius (void) const;

  /**
   * Set the \c z height of the grid points, in meters.
   * \param [in] z The z height of the grid points in meters.
   */
  void SetZ (double z);

  // Inherited
  virtual Vector GetNext (void) const;
  virtual int64_t AssignStreams (int64_t stream);

  // Forward declaration.
  // Needs to be public for operators declared in the .cc file
  class Hex;

private:

  /** Keep the hexagonal points in a ListPositionAllocator */
  // mutable: see note at m_populated
  mutable ListPositionAllocator m_list;

  /**
   * Size of the underlying hexagon, in meters.
   * This is the distance from hexagon center to any corner.
   */
  double m_hexSize;

  /** Size of the overall grid, in rings. */
  int m_rings;

  /** \c z coordinate of the positions, in meters. */
  double m_z;

  /**
   * Compute the space coordinates from the Hex index.
   * \param [in] h The Hex index.
   * \returns The spatial coordinate Vector, in meters.
   */
  Vector3D ToSpace (const Hex & h) const;

  /**
   * Populate the underlying PositionAllocator on the first call
   * to GetNext.
   */
  void PopulateAllocator (void) const;

  /** Has the underlying PositionAllocator been populated? */
  // Mutable because GetNext is const in PositionAllocator;
  // first call to GetNext invokes PopulateAllocator, which also has to be const;
  // and PopulateAllocator has to record that it has run.
  mutable bool m_populated;

};  // class HexagonalPositionAllocator

}  // namespace ns3

#endif /* HEX_POSITION_ALLOCATOR_H */
