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

#include <algorithm>  // max
#include <cmath>      // round

/**
 * \file
 * \ingroup mobility
 * ns3::HexagonalPositionAllocator and Hex
 * implementation.
 */

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (HexagonalPositionAllocator);

NS_LOG_COMPONENT_DEFINE ("HexPositionAllocator");

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
   * The symbol names are for PointyTop orientation;
   * the FlatTop compass directions are given in parentheses.
   */
  // Note: the order is important,
  // since it drives how we walk around rings,
  // starting at the east most point.
  // See Populate
  enum Direction 
  {
    NW = 0, /**< Towards the northwest (northwest). */
    W,      /**< Towards the west (southwest)       */
    SW,     /**< Towards the southwest (sout).      */
    SE,     /**< Towards the southeast (southeast). */
    E,      /**< Towards the east (northeast).      */
    NE,     /**< Towards the north-east (north).    */
    end     /**< Out of range flag; see Next */
  };

  typedef std::array<Hex, 6> Direction_t;

  /**
   * Get the offet in the given direction.
   * \param [in] d The direction to move
   * \returns The Hex to move in the requested direction.
   */
  static Hex GetDirection (Direction_t const * directions,
                           const enum Direction d);

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
  Hex Neighbor (Direction_t const * directions,
                const enum Direction d) const;

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

  /**
   * Hex equality operator.
   * \param [in] other Right operand.
   * \returns \c true if the operands are equal.
   */
  bool operator == (const Hex & other);
  
  /**
   * Hex inequality operator.
   * \param [in] other Right operand.
   * \returns \c true if the operands are not equal.
   */
  bool operator != (const Hex & other);

  /**
   * Addition of Hex indices.
   * \param [in] other Right operand.
   * \returns The "sum" of Hex indices
   */
  Hex operator + (const Hex & other);
  
  /**
   * Subtraction of Hex indices.
   * \param [in] other Right operand.
   * \returns The "sum" of Hex indices
   */
  Hex operator - (const Hex & other);
  
  /**
   * Scaling of Hex indices.
   * \param [in] scale The scale operand.
   * \returns The hex coordiante scaled by the (integer) factor.
   */
  Hex operator * (Hex::coord_type scale);
  
};  // class HexagonalPositionAllocator::Hex

// Simplify the following definitions
using Hex = HexagonalPositionAllocator::Hex;


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

// Order of these vectors has to match the order of the enum declarations
const Hex::Direction_t
PointyDirections {
  Hex ( 0, -1,  1),  // NW
  Hex (-1,  0,  1),  // W
  Hex (-1,  1,  0),  // SW
  Hex ( 0,  1, -1),  // SE
  Hex ( 1,  0, -1),  // E
  Hex ( 1, -1,  0)   // NE
};

const Hex::Direction_t
FlatDirections {
  Hex (-1,  0,  1),  // NW, symbol NW
  Hex (-1,  1,  0),  // SW, symbol W 
  Hex ( 0,  1, -1),  // S,  symbol SW
  Hex ( 1,  0, -1),  // SE, symbol SE
  Hex ( 1, -1,  0),  // NE, symbol E 
  Hex ( 0, -1,  1)   // N,  symbol NE
};


/* static */
Hex
Hex::GetDirection (Direction_t const * directions, const Hex::Direction d)
{
  return (*directions)[static_cast<std::size_t> (d)];
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
Hex::Neighbor (Direction_t const * directions, const Direction d) const
{
  Hex h = *this;
  return h + GetDirection (directions, d);
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
  Hex h = *this;
  return (h - a).Length ();
}

bool
Hex::operator == (const Hex & other)
{
  return v == other.v;
}

bool
Hex::operator != (const Hex & other)
{
  return v != other.v;
}

Hex
Hex::operator + (const Hex & other)
{
  return Hex {q + other.q, r + other.r, s + other.s};
}

Hex
Hex::operator - (const Hex & other)
{
  return Hex {q - other.q, r - other.r, s - other.s};
}

Hex
Hex::operator * (Hex::coord_type scale)
{
  return Hex (q * scale, r * scale, s * scale);
}

/**
 * \relates HexagonalPositionAllocator::Hex
 * Output streamer for Hex indices.
 * \param os The output stream.
 * \returns The output stream.
 */
std::ostream &
operator << (std::ostream & os, const Hex & h)
{
  os << "Hex (" << h.q << "," << h.r << "," << h.s << ")";
  return os;
}


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
    .AddAttribute ("Orientation",
                   "The hexagon orientation.",
                   EnumValue (FlatTop),
                   MakeEnumAccessor (&HexagonalPositionAllocator::SetOrientation),
                   MakeEnumChecker (FlatTop, "FlatTop", PointyTop, "PointyTop"))
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

void
HexagonalPositionAllocator::SetZ (double z)
{
  m_z = z;
}

double
HexagonalPositionAllocator::GetZ (void) const
{
  return m_z;
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
  bool inside =  l <=  m_rings;
  return inside;
}
  
struct HexagonalPositionAllocator::Orienter
{
  Orienter (ns3::Vector2D x, ns3::Vector2D y,
            ns3::Vector2D q, ns3::Vector2D r,
            Hex::Direction initial)
    : xBasis (x),
      yBasis (y),
      qBasis (q),
      rBasis (r),
      initial (initial)
  {}
  
  ns3::Vector2D xBasis;
  ns3::Vector2D yBasis;
  ns3::Vector2D qBasis;
  ns3::Vector2D rBasis;
  Hex::Direction initial;
  
};  // struct Orienter

/** Orienter for pointy topped hexagons */
const HexagonalPositionAllocator::Orienter
PointyOrienter ({std::sqrt (3.),      std::sqrt (3) / 2 },
                {     0,                  -3/2.0        },
                {std::sqrt (3.) / 3,       1/3.0        },
                {     0,                  -2/3.0        },
                Hex::E);


/** Orienter for flat topped hexagons */
const HexagonalPositionAllocator::Orienter
FlatOrienter   ({     3/2.,                 0.              },
                { -std::sqrt (3.) / 2., -std::sqrt (3.)     },
                {     2/3.,                 0               },
                {    -1/3.,             -std::sqrt(3.) / 3. },
                Hex::E);


void
HexagonalPositionAllocator::SetOrientation (enum Orientation o)
{
  if (o == FlatTop)
    {
      m_orienter = & FlatOrienter;
      m_directions = &FlatDirections;
    }
  else
    {
      m_orienter = & PointyOrienter;
      m_directions = &PointyDirections;
    }
}
  
double
HexagonalPositionAllocator::GetRadius (void) const
{
  NS_LOG_FUNCTION (this);
  
  // The grid point farthest to the east
  Hex edge = Hex ().Neighbor (m_directions, m_orienter->initial) * m_rings;
  Vector3D point = ToSpace (edge);
  // Get the right offset to the corner
  if (m_orienter == & PointyOrienter)
    {
      // Vector arithmetic is sadly lacking...
      point = point + Vector3D (m_hexSize * std::sqrt (3.0) / 2,
                                m_hexSize * 1 / 2.,
                                0);
    }
  else
    {
      point = point + Vector3D (m_hexSize * 1 / 2.,
                                m_hexSize * std::sqrt (3.0) / 2,
                                0);
    }
  auto l = point.GetLength ();
  NS_LOG_INFO ("radius: " << l);
  return l;
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

Vector3D
HexagonalPositionAllocator::ToSpace (const Hex & h) const
{
  NS_LOG_FUNCTION (h);
  double x = (m_orienter->xBasis.x * h.q  + m_orienter->xBasis.y * h.r) * m_hexSize;
  double y = (m_orienter->yBasis.x * h.q  + m_orienter->yBasis.y * h.r) * m_hexSize;
  return Vector3D (x, y, m_z);
}

Hex
HexagonalPositionAllocator::ClosestGridPoint (const Vector3D & v) const
{
  // Scale to dimensionless units
  double px = v.x / m_hexSize;
  double py = v.y / m_hexSize;

  // Apply the inverse rotation matrix
  double qd = m_orienter->qBasis.x * px + m_orienter->qBasis.y * py;
  double rd = m_orienter->rBasis.x * px + m_orienter->rBasis.y * py;
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
  
  NS_LOG_FUNCTION (v <<
                   " p " << px << py <<
                   " d " << qd << rd << sd <<
                   " r " << qr << rr << sr <<
                   " h " << q  <<  r);
  
  return h;
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
  Vector3D p = ToSpace (nodes.back ());
  m_list.Add (p);
  Vector3D pp = FromSpace (p);
  NS_LOG_INFO ("Node[" << nodes.size () - 1 << "]: " <<
               nodes.back () << " " << p << " --> " << pp);

  // Each ring
  for (std::size_t r = 1; r <= m_rings; ++r)
    {
      NS_LOG_LOGIC ("ring:   " << r);
      // Start in the initial direction
      nodes.emplace_back (Hex ().Neighbor (m_directions, m_orienter->initial) * r);
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
              m_list.Add (p);
              pp = FromSpace (p);
              NS_LOG_INFO ("Node[" << nodes.size () - 1 << "]: " <<
                           nodes.back () << " " << p << " --> " << pp);
              nodes.emplace_back (nodes.back ().Neighbor (m_directions, d));
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

}  // namespace ns3
