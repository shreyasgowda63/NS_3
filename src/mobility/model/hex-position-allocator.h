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
#include "ns3/vector.h"

#include <array>
#include <cmath>  // sqrt
#include <map>

/**
 * \file
 * \ingroup mobility
 * Declaration of class ns3::HexagonalPositionAllocator.
 */


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
 * In the language of that article, this position allocator can generate
 * points from either a "flat top" layout, or a "point top" layout,
 * via the "Orientation" attribute.
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
  void SetRings (std::size_t r);

  /**
   * Get the number of rings in the grid.
   * \returns The number of rings in the grid.
   */
  std::size_t GetRings (void) const;

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

  /**
   * Get the \c z height of the grid points, in meters.
   * \returns The z height of the grid points, in meters.
   */
  double GetZ (void) const;

  /**
   * Get the index of the grid point closest to an arbitrary point.
   * If the nearest grid point is outside the actual grid this returns -1.
   * \param v [in] The space point as a Vector, in meters.
   * \return The index of the nearest grid point, in the order
   *         returned by GetNext.
   */
  long long int GetIndex (const Vector3D & v) const;
 
  /**
   * Get the positon of the nearest grid point from an arbitrary point.
   * If one cached the points returned by GetNext, this function
   * returns the grid point at `GetIndex (p)` in the cache.
   * \param v [in] The space point as a Vector, in meters.
   * \returns The position of the nearest hex grid point.
   */
  Vector3D GetNearestGridPoint (const Vector3D & v) const;

  /**
   * Check if a point is within the layout.
   * A point is within the layout if the nearest grid point
   * is part of the layout.
   * \param v [in] The space point as a Vector, in meters.
   * \returns \c true if the point is within the layout.
   */
  bool IsInside (const Vector3D & v) const;

  /** Which orientation for the hexagons. */
  enum Orientation {
    /** Flat topped: two edges aligned with the x-axis. */
    FlatTop,
    /** Pointy topped: two edges aligned with the y-axis */
    PointyTop
  };
    
  /**
   * Set the orientation
   * \param [in] o The orientation.
   */
  void SetOrientation (enum Orientation o);
  
  // Inherited
  virtual Vector3D GetNext (void) const;
  virtual int64_t AssignStreams (int64_t stream);

  // Forward declarations, need to be public :(
  class Hex;
  struct Orienter;
  
private:

  /** Keep the hexagonal points in a ListPositionAllocator */
  // mutable: see note at m_populated
  mutable ListPositionAllocator m_list;

  /**
   * Invert from a node position to its "index" in the
   * position allocator list.
   */
  // Using ordered map, instead of unordered_map,
  // since Vector3D doesn't specialize std::hash
  // mutable: see note at m_populated
  mutable std::map<Vector3D, std::size_t> m_nodeLookup;

  /**
   * Size of the underlying hexagon, in meters.
   * This is the distance from hexagon center to any corner.
   */
  double m_hexSize;

  /** Size of the overall grid, in rings. */
  std::size_t m_rings;

  /** \c z coordinate of the positions, in meters. */
  double m_z;

  /** The Orienter helper. */
  const Orienter * m_orienter;

  /** Type for holding moving directions. */
  typedef std::array<Hex, 6> Direction_t;
  
  /** The movement directions. */
  const Direction_t * m_directions;

  
  /**
   * Compute the space coordinates from the Hex coordinates.
   * \param [in] h The Hex coordinates.
   * \returns The spatial coordinate Vector, in meters.
   */
  Vector3D ToSpace (const Hex & h) const;

  /**
   * Get the hex grid coordinates of a space point.
   * \param v [in] The space point as a Vector, in meters.
   * \returns the Hex node closest to the input space point.
   */
  Hex NearestHexPoint (const Vector3D & v) const;

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
