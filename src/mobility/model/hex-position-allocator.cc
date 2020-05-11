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
  union
  {
    /** Vector-like access to indices. */
    const std::array<int, 3> v;
    /** Component-wise access. */
    struct
    {
      const int q;  /**< North and east coordinate. */
      const int r;  /**< South coordinate.          */
      const int s;  /**< North and west coordinate. */
    };  // struct
  };  // union

  /** Default constructor; this is the index for the center node. */
  Hex (void);

  /**
   * Construct from two indices \pname{q}, \pname{r}.
   * The third coordinate is computed from the two arguments.
   * \param [in] qq The north/east coordinate.
   * \param [in] rr The south coordinate.
   */
  Hex (const int qq, const int rr);

  /**
   * \brief Construct from three indices \pname{q}, \pname{r}, \pname{s}.
   * This will assert if the invariant `q + r + s == 0` does not hold.
   * \param [in] qq The north/east coordinate.
   * \param [in] rr The south coordinate.
   * \param [in] ss The north/west coordinate.
   */
  Hex (const int qq, const int rr, const int ss);

  /**
   * Neighbor direction indicators.
   */
  // Note: the order is important,
  // since it drives how we walk around rings,
  // starting at the east most point.
  // See Populate()
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
   * Get the neigbor index in the given direction.
   * \param [in] d The direction to move in.
   * \returns The Hex indices to that neighbor.
   */
  Hex Neighbor (const enum Direction d) const;

  /**
   * Length of this Hex coordinate, in index units.
   * \returns This length.
   */
  int Length (void) const;

  /**
   * Distance to another node point, in index units.
   * \param [in] a The other node point.
   * \returns The distance in index units.
   */
  int Distance (const Hex & a) const;

};  // class HexagonalPositionAllocator::Hex

/**
 * \relates HexagonalPositionAllocator::Hex
 * Hex equality operator.
 * \param [in] a Right operand.
 * \param [in] b Right operand.
 * \returns \c true if the operands are equal.
 */
bool
operator == (const HexagonalPositionAllocator::Hex & a,
             const HexagonalPositionAllocator::Hex & b)
{
  return a.v == b.v;
}

/**
 * \relates HexagonalPositionAllocator::Hex
 * Hex inequality operator.
 * \param [in] a Right operand.
 * \param [in] b Right operand.
 * \returns \c true if the operands are not equal.
 */
bool
operator != (const HexagonalPositionAllocator::Hex & a,
             const HexagonalPositionAllocator::Hex & b)
{
  return a.v != b.v;
}

/**
 * \relates HexagonalPositionAllocator::Hex
 * Addition of Hex indices.
 * \param [in] a Right operand.
 * \param [in] b Right operand.
 * \returns The "sum" of Hex indices
 */
HexagonalPositionAllocator::Hex
operator + (const HexagonalPositionAllocator::Hex & a,
            const HexagonalPositionAllocator::Hex & b)
{
  return HexagonalPositionAllocator::Hex {a.q + b.q, a.r + b.r, a.s + b.s};
}

/**
 * \relates HexagonalPositionAllocator::Hex
 * Subtraction of Hex indices.
 * \param [in] a Right operand.
 * \param [in] b Right operand.
 * \returns The "difference" of Hex indices
 */
HexagonalPositionAllocator::Hex
operator - (const HexagonalPositionAllocator::Hex & a,
            const HexagonalPositionAllocator::Hex & b)
{
  return HexagonalPositionAllocator::Hex {a.q - b.q, a.r - b.r, a.s - b.s};
}

/**
 * \relates HexagonalPositionAllocator::Hex
 * Scaling of Hex indices.
 * \param [in] h The Hex operand
 * \param [in] a The scale operand.
 * \returns The hex index scaled by the factor.
 */
HexagonalPositionAllocator::Hex
operator * (const HexagonalPositionAllocator::Hex & h, int a)
{
  return HexagonalPositionAllocator::Hex {h.q * a, h.r * a, h.s * a};
}

/**
 * \relates HexagonalPositionAllocator::Hex
 * Output streamer for Hex indices.
 * \param os The output stream.
 * \param h The Hex index.
 * \returns The output stream.
 */
std::ostream &
operator << (std::ostream & os, HexagonalPositionAllocator::Hex & h)
{
  os << "Hex (" << h.q << "," << h.r << "," << h.s << ")";
  return os;
}

HexagonalPositionAllocator::Hex::Hex (void)
  : v {0, 0, 0}
{
  NS_LOG_FUNCTION (this << "0,0,0");
}

HexagonalPositionAllocator::Hex::Hex (const int qq, const int rr)
  : v {qq, rr, -qq - rr}
{
  NS_LOG_FUNCTION (this << q << r << s);
}

HexagonalPositionAllocator::Hex::Hex (const int qq, const int rr, const int ss)
  : v {qq, rr, ss}
{
  NS_LOG_FUNCTION (this << q << r << s);
  NS_ASSERT_MSG (q + r + s == 0, "Hex index invariant not satisfied: "
                 << q << "," << r << "," << s);
}

/* static */
HexagonalPositionAllocator::Hex
HexagonalPositionAllocator::Hex::GetDirection (const Direction d)
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
  return Directions[static_cast<int> (d)];
}

/* static */
HexagonalPositionAllocator::Hex::Direction
HexagonalPositionAllocator::Hex::Next (const HexagonalPositionAllocator::Hex::Direction d)
{
  int di = static_cast<int> (d);
  int nd = di + 1;
  NS_LOG_INFO ("dir: " << di << ", nex dir: " << nd );
  return static_cast<Direction> (nd);
}

HexagonalPositionAllocator::Hex
HexagonalPositionAllocator::Hex::Neighbor (const Direction d) const
{
  return (*this) + GetDirection (d);
}

int
HexagonalPositionAllocator::Hex::Length (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return std::max ( {std::abs (q), std::abs (r), std::abs (s)} );
}

int
HexagonalPositionAllocator::Hex::Distance (const Hex & a) const
{
  NS_LOG_FUNCTION (this << a);
  return ((*this) - a).Length ();
}

/** Layout vectors for transforming to real coordinates. @{ */
static const ns3::Vector2D xBasis {std::sqrt (3.0), std::sqrt (3) / 2};
static const ns3::Vector2D yBasis {0,               1.5};
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
                   MakeUintegerChecker<int> (0))
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
  m_hexSize = s / std::sqrt (3);
  NS_LOG_FUNCTION (this << s << m_hexSize);
}

double
HexagonalPositionAllocator::GetSpacing (void) const
{
  return m_hexSize * std::sqrt (3);
}

void
HexagonalPositionAllocator::SetRings (int r)
{
  NS_LOG_FUNCTION (this << r);
  m_rings = r;
}

int
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
  // We need N = 1 + 6 * Sum (i = 1..r) (r)
  //           = 1 + 3 r (r + 1)
  int n = 1 + 3 * m_rings * (m_rings + 1);
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
    + Vector3D (m_hexSize * std::sqrt (3) / 2, 0, 0);
  return point.GetLength ();
}

void
HexagonalPositionAllocator::SetZ (double z)
{
  m_z = z;
}

Vector3D
HexagonalPositionAllocator::ToSpace (const Hex & h) const
{
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
  for (int r = 1; r <= m_rings; ++r)
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
          for (int i = 0; i < r; ++i)
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
