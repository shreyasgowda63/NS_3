/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Lawrence Livermore National Laboratory
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

#include "hex-position-allocator.h"

#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/uinteger.h"
#include "ns3/enum.h"
#include "ns3/log.h"

#include <array>
#include <algorithm>  // max
#include <cmath>      // round

/**
 * \file
 * \ingroup mobility
 * ns3::HexagonalPositionAllocator and ns3::HexagonalPositionAllocator::Hex
 * implementation.
 */

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HexPositionAllocator");

NS_OBJECT_ENSURE_REGISTERED (HexagonalPositionAllocator);

/**
 * \ingroup mobility
 * \defgroup hex-position-allocator Hexagonal Position Allocator implementation
 */

/**
 * \ingroup hex-position-allocator
 * Integer indices into the hexagonal array.
 */
class HexagonalPositionAllocator::Hex
{
public:
  typedef std::make_signed<std::size_t>::type coord_type;
  
  union 
  {
    /** Vector-like access to indices. */
    const std::array<coord_type, 3> v;
    /** Component-wise access. */
    struct 
    {
      const coord_type q;  /**< North and east coordinate. */
      const coord_type r;  /**< South coordinate.          */
      const coord_type s;  /**< North and west coordinate. */
    };  // struct
  };  // union

  /** Default constructor; this is the coordinate for the center node. */
  Hex (void);

  /**
   * Construct from two indices \pname{q}, \pname{r}.
   * The third coordinate is computed from the two arguments.
   * \param [in] q The north/east coordinate.
   * \param [in] r The south coordinate.
   */
  Hex (const coord_type q, const coord_type r);

  /**
   * \brief Construct from three indices \pname{q}, \pname{r}, \pname{s}.
   * This will assert if the invariant `q + r + s == 0` does not hold.
   * \param [in] q The north/east coordinate.
   * \param [in] r The south coordinate.
   * \param [in] s The north/west coordinate.
   */
  Hex (const coord_type  q, const coord_type  r, const coord_type  s);

  /**
   * Neighbor direction indicators.
   */
  // Note: the order is important,
  // since it drives how we walk around rings,
  // starting at the east most point.
  // See Populate
  enum Direction 
  {
    NW = 0, /**< Towards the north-west.     */
    W,      /**< Towards the west.           */
    SW,     /**< Towards the southwest.      */
    SE,     /**< Towards the southeast.      */
    E,      /**< Towards the east.           */
    NE,     /**< Towards the north-east.     */
    end     /**< Out of range flag; see Next */
  };

  /**
   * Get the offet in the given direction.
   * \param [in] d The direction to move
   * \returns The Hex to move in the requested direction.
   */
  static Hex GetDirection (const enum Direction d);

  /**
   * Get the next direction to walk around a ring.
   * \param [in] d The current direction.
   * \returns The direction for the next side of the ring.
   */
  static enum Direction Next (const enum Direction d);

  /**
   * Get the neigbor coordinates in the given direction.
   * \param [in] d The direction to move in.
   * \returns The Hex indices to that neighbor.
   */
  Hex Neighbor (const enum Direction d) const;

  /**
   * Length of this Hex coordinate, in coordinate units.
   * \returns This length.
   */
  coord_type Length (void) const;

  /**
   * Distance to another node point, in coordinate units.
   * \param [in] a The other node point.
   * \returns The distance in coordinate units.
   */
  coord_type Distance (const Hex & a) const;

};  // class HexagonalPositionAllocator::Hex


// Simplify following declarations  
using Hex = HexagonalPositionAllocator::Hex;
  
/**
 * \relates HexagonalPositionAllocator::Hex
 * Hex equality operator.
 * \param [in] a Right operand.
 * \param [in] b Right operand.
 * \returns \c true if the operands are equal.
 */
bool
operator == (const Hex & a, const Hex & b)
{
  return a.v == b.v;
}

/**
 * \relates Hex
 * Hex inequality operator.
 * \param [in] a Right operand.
 * \param [in] b Right operand.
 * \returns \c true if the operands are not equal.
 */
bool
operator != (const Hex & a, const Hex & b)
{
  return a.v != b.v;
}

/**
 * \relates Hex
 * Addition of Hex indices.
 * \param [in] a Right operand.
 * \param [in] b Right operand.
 * \returns The "sum" of Hex indices
 */
Hex
operator + (const Hex & a, const Hex & b)
{
  return Hex {a.q + b.q, a.r + b.r, a.s + b.s};
}

/**
 * \relates Hex
 * Subtraction of Hex indices.
 * \param [in] a Right operand.
 * \param [in] b Right operand.
 * \returns The "sum" of Hex indices
 */
Hex
operator - (const Hex & a, const Hex & b)
{
  return Hex {a.q - b.q, a.r - b.r, a.s - b.s};
}

/**
 * \relates Hex
 * Scaling of Hex indices.
 * \param [in] h The Hex operand
 * \param [in] a The scale operand.
 * \returns The hex coordiante scaled by the (integer) factor.
 */
Hex
operator * (const Hex & h, Hex::coord_type a)
{
  return Hex {h.q * a, h.r * a, h.s * a};
}

/**
 * \relates Hex
 * Output streamer for Hex indices.
 * \param os The output stream.
 * \param h The Hex coordinate.
 * \returns The output stream.
 */
std::ostream &
operator << (std::ostream & os, Hex & h)
{
  os << "Hex (" << h.q << "," << h.r << "," << h.s << ")";
  return os;
}

Hex::Hex (void)
  : v {0, 0, 0}
{
  NS_LOG_FUNCTION (this << "0,0,0");
}

Hex::Hex (const coord_type q, const coord_type r)
  : v {q, r, -q - r}
{
  NS_LOG_FUNCTION (this << q << r << s);
}

Hex::Hex (const coord_type q, const coord_type r, const coord_type s)
  : v {q, r, s}
{
  NS_LOG_FUNCTION (this << q << r << s);
  NS_ASSERT_MSG (q + r + s == 0, "Hex coordinate invariant not satisfied: "
                 << q << "," << r << "," << s);
}

/* static */
Hex
Hex::GetDirection (const Hex::Direction d)
{
  // Order of these vectors has to match the order of the enum declarations
  static const std::array<Hex, 6> Directions
  {
    Hex ( 0, -1,  1),  // NW
    Hex (-1,  0,  1),  // W
    Hex (-1,  1,  0),  // SW
    Hex ( 0,  1, -1),  // SE
    Hex ( 1,  0, -1),  // E
    Hex ( 1, -1,  0)   // NE
  };
  return Directions[static_cast<std::size_t> (d)];
}

/* static */
Hex::Direction
Hex::Next (const Hex::Direction d)
{
  coord_type di = static_cast<coord_type> (d);
  coord_type nd = di + 1;
  NS_LOG_INFO ("dir: " << di << ", nex dir: " << nd );
  return static_cast<Hex::Direction> (nd);
}

Hex
Hex::Neighbor (const Direction d) const
{
  return (*this) + GetDirection (d);
}

Hex::coord_type
Hex::Length (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  auto l = std::max ({std::abs (q), std::abs (r), std::abs (s) });
  NS_LOG_INFO ("length: " << l);
  return l;
}

Hex::coord_type
Hex::Distance (const Hex & a) const
{
  NS_LOG_FUNCTION (this << a);
  return ((*this) - a).Length ();
}

/** Layout vectors for transforming to/from real coordinates. @{ */
static const ns3::Vector2D xBasis {std::sqrt (3.0),   std::sqrt (3) / 2.0 };
static const ns3::Vector2D yBasis {      0,                 3/2.0         };
static const ns3::Vector2D qBasis {std::sqrt (3.0) / 3.0,  -1/3.0         };
static const ns3::Vector2D rBasis {      0,                 2/3.0         };
/**@}*/


TypeId 
HexagonalPositionAllocator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HexagonalPositionAllocator")
    .SetParent<PositionAllocator> ()
    .SetGroupName ("Mobility")
    .AddConstructor<HexagonalPositionAllocator> ()
    .AddAttribute ("Spacing",
                   "The distance between points in the hexagonal grid, in meters.",
                   DoubleValue (1000),
                   MakeDoubleAccessor (&HexagonalPositionAllocator::SetSpacing),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Rings", "The number of rings making up the entire grid.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&HexagonalPositionAllocator::m_rings),
                   MakeUintegerChecker<int> (1))
    .AddAttribute ("Z",
                   "The z coordinate of all the positions allocated, in meters..",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&HexagonalPositionAllocator::m_z),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}

HexagonalPositionAllocator::HexagonalPositionAllocator (void)
  : m_populated (false)
{
  NS_LOG_FUNCTION (this);
}

HexagonalPositionAllocator::~HexagonalPositionAllocator ()
{
  NS_LOG_FUNCTION (this);
}

void
HexagonalPositionAllocator::SetSpacing (const double s)
{
  m_hexSize = s / std::sqrt (3.0);
  NS_LOG_FUNCTION (this << s << m_hexSize);
}

double
HexagonalPositionAllocator::GetSpacing (void) const
{
  return m_hexSize * std::sqrt (3.0);
}

void
HexagonalPositionAllocator::SetRings (std::size_t r)
{
  NS_LOG_FUNCTION (this << r);
  m_rings = r;
}

std::size_t
HexagonalPositionAllocator::GetRings (void) const
{
  return m_rings;
}

std::size_t
HexagonalPositionAllocator::GetN (void) const
{
  NS_LOG_FUNCTION (this);
  // At radius 0 there is the central node.
  // At radius r there are 6 * r nodes
  // We need  N = 1 + 6 * Sum (i = 1..r) (r)
  //            = 1 + 6 r (r + 1) / 2
  std::size_t n = 1 + 3 * m_rings * (m_rings + 1);
  NS_LOG_INFO ("total nodes: " << n);
  return n;
}

double
HexagonalPositionAllocator::GetRadius (void) const
{
  NS_LOG_FUNCTION (this);
  // The grid point farthest east
  Hex east {Hex::GetDirection (Hex::Direction::E) * m_rings};
  Vector3D point = ToSpace (east)
    // plus the distance to the corner of the east node
    + Vector3D (m_hexSize * std::sqrt (3.0) / 2, 0, 0);
  auto l = point.GetLength ();
  NS_LOG_INFO ("radius: " << l);
  return l;
}

void
HexagonalPositionAllocator::SetZ (double z)
{
  m_z = z;
}

Hex
HexagonalPositionAllocator::ClosestGridPoint (const Vector3D & v) const
{
  NS_LOG_FUNCTION (v);
  
  // Scale to dimensionless units
  double px = v.x / m_hexSize;
  double py = v.y / m_hexSize;

  // Apply the inverse rotation matrix
  double qd = qBasis.x * px + qBasis.y * py;
  double rd = rBasis.x * py + rBasis.y * py;
  double sd = -qd - rd;

  // Round
  double qr = std::round (qd);
  double rr = std::round (rd);
  double sr = std::round (sd);
    
  // Absolute differences (deltas) from rounding
  double dq = std::abs (qr - qd);
  double dr = std::abs (rr - rd);
  double ds = std::abs (sr - sd);

  // Fix up the one with the biggest delta
  if ( (dq > dr) && (dq > ds) )
    {
      qr = -rr - sr;
    }
  else if ( dr > ds )
    {
      rr = -sr - qr;
    }
  else
    {
      sr = -qr - rr;
    }

  auto q = static_cast<Hex::coord_type> (qr);
  auto r = static_cast<Hex::coord_type> (rr);

  Hex h {q, r};
  return h;
}
  
Vector3D
HexagonalPositionAllocator::FromSpace (const Vector & v) const
{
  NS_LOG_FUNCTION (v);
  Hex h = ClosestGridPoint (v);
  Vector3D p = ToSpace (h);
  return p;
}

bool
HexagonalPositionAllocator::IsInside (const Vector3D & v) const
{
  NS_LOG_FUNCTION (v);
  Hex h = ClosestGridPoint (v);
  auto l = static_cast<std::size_t> (h.Length ());
  bool inside =  l <  m_rings + 1;
  return inside;
}
  
Vector3D
HexagonalPositionAllocator::ToSpace (const Hex & h) const
{
  NS_LOG_FUNCTION (h);
  double x = (xBasis.x * h.q  + xBasis.y * h.r) * m_hexSize;
  double y = (yBasis.x * h.q  + yBasis.y * h.r) * m_hexSize;
  return Vector3D (x, y, m_z);
}

void
HexagonalPositionAllocator::PopulateAllocator (void) const
{
  NS_LOG_FUNCTION (this);

  // Hex isn't copyable (const data members)
  // So simple incremental things like
  //   Hex n;  ... n = n + ...
  // don't work
  // Instead, we incrementally populate a vector of all nodes
  // using emplace_back to construct in place.
  std::vector<Hex> nodes;
  const std::size_t N = GetN ();
  nodes.reserve (N);

  // Central grid point
  nodes.emplace_back (Hex ());
  Vector3D p = ToSpace (nodes[0]);
  NS_LOG_INFO ("center: " << p);
  m_list.Add (p);

  // Each ring
  for (std::size_t r = 1; r <= m_rings; ++r)
    {
      NS_LOG_LOGIC ("ring:   " << r);
      // Start in the east
      nodes.emplace_back (Hex ().Neighbor (Hex::Direction::E) * r);
      for ( auto d = Hex::Direction::NW;
            d != Hex::Direction::end;
            d = Hex::Next (d)
            )
        {
          NS_LOG_LOGIC ("  edge: " << static_cast<int> (d)
                                   << ", steps: " << r);
          for (std::size_t i = 0; i < r; ++i)
            {
              Vector3D p = ToSpace (nodes.back ()); 
              NS_LOG_INFO ("        " << p);
              m_list.Add (p);
              nodes.emplace_back (nodes.back ().Neighbor (d));
            }
        }
      if (m_list.GetSize () > N)
        {
          NS_LOG_LOGIC ("over ran the list, breaking out of loop");
          break;
        }
    }
  NS_LOG_INFO ("total points: " << m_list.GetSize ());
  m_populated = true;
}

Vector
HexagonalPositionAllocator::GetNext (void) const
{
  if (!m_populated)
    {
      PopulateAllocator ();
    }
  return m_list.GetNext ();
}

int64_t
HexagonalPositionAllocator::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this);
  return 0;
}

}  // namespace ns3
