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
#ifndef POSITION_FILTER_H
#define POSITION_FILTER_H

#include "position-allocator.h"

/**
 * \file
 * \ingroup mobility
 * Declaration of classes ns3::PositionFilter, ns3::PositionAllocatorFilter,
 * and ns3::FilteredPositionAllocator.
 */


namespace ns3 {

/**
 * \ingroup mobility
 * \brief Apply spatial filtering to a PositionAllocator.
 * Subclasses of this class implement the IsInside function,
 * which a FilteredPositionAllocator uses to determine if the
 * points from an underlying PositionAllocator should be accepted.
 */
class PositionFilter : public SimpleRefCount<PositionFilter>
{
public:

  /** Destructor. */
  virtual ~PositionFilter (void);
  
  /**
   * Check if a point is acceptable.
   * \param v [in] The space point to test, as a Vector, in meters.
   * \returns \c true if the point is acceptable.
   */
  virtual bool IsInside (const Vector3D & v) const = 0;
  
};  // class PositionFilter

/**
 * \ingroup mobility
 * Adapt a PositionAllocator as a PositionAllocatorFilter.
 * The PositionAllocator must implement it's own IsInside function.
 */
template <class P>  
class PositionAllocatorFilter : public PositionFilter
{
public:
  /**
   * Construct from a PositionAllocator
   * \param p [in] The PositionAllocator to use for filtering.
   */
  PositionAllocatorFilter (const Ptr<P> p);

  /** Destructor. */
  virtual ~PositionAllocatorFilter (void);
  
  // Inherited
  virtual bool IsInside (const Vector3D & v) const;

private:
  /** The PositionAllocator to use for filtering decisions. */
  Ptr<P> m_filter;

};  // class PositionAllocatorFilter 

/**
 * \ingroup mobility
 * \brief Convenience function to streamline creating PositionAllocatorFilters.
 * \tparam P The type of the argument to be used as a PositionFilter.
 * \param p [in] The instance to be used as the a PositionFilter.
 * \returns A PositionAllocatorFilter using \pname{p}
 */
template <class P>
Ptr<PositionAllocatorFilter<P>>
MakePositionAllocatorFilter (Ptr<P> p);
  
/**
 * \ingroup mobility
 * \brief Apply a PositionAllocatorFilter to an underlying PositionAllocator.
 * Only points which pass the filter will be returned from this
 * PositionAllocator.  Points pass the filter when IsInside returns \c true.
 * The sense of the filter can be inverted using SetInvert().
 *
 * When the underlying PositionAllocator yields a point which
 * doesn't pass the filter it is sampled again.
 */
class FilteredPositionAllocator : public PositionAllocator
{
public: 
  /**
   * Register this type with the TypeId system.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /** Default constructor. */
  FilteredPositionAllocator ();
  /** Destructor. */
  virtual ~FilteredPositionAllocator ();

  /**
   * Set the underlying PositionAllocator, which will determine the
   * positions to use.
   * \param p [in] The underlying PositionAllocator.
   */
  void SetPositionAllocator (Ptr<PositionAllocator> p);

  /**
   * Set the filter to apply to points from the underlying PositionAllocator.
   * Points from the PositionAllocator have to pass the filter's IsInside test
   * to be returned by GetNext.
   * \param f [in] The PositionFilter to use.
   */
  void SetPositionFilter (Ptr<PositionFilter> f);

  /**
   * Invert the sense of the filter: only accept points for which
   * the filter IsInside returns false.
   */
  void SetInvert (bool invert);

  // Inherited
  virtual Vector3D GetNext (void) const;
  virtual int64_t AssignStreams (int64_t stream);
  
private:

  /** The underlying PositionAllocator. */
  Ptr<PositionAllocator> m_allocator;

  /** The filter. */
  Ptr<PositionFilter> m_filter;

  /** Whether to pass normal (\c true) or inverted (\c false). */
  bool m_normal;
  
};  // class FilteredPositionAllocator


/****************************************************
 *      PositionAllocatorFilter implementations.
 ***************************************************/

template <class P>  
PositionAllocatorFilter<P>::PositionAllocatorFilter (const Ptr<P> p)
  : m_filter (p)
{
}

template <class P>  
PositionAllocatorFilter<P>::~PositionAllocatorFilter (void)
{
}

template <class P>  
bool
PositionAllocatorFilter<P>::IsInside (const Vector3D & v) const
{
  return m_filter->IsInside (v);
}

template <class P>
Ptr<PositionAllocatorFilter<P>>
MakePositionAllocatorFilter (Ptr<P> p)
{
  return Create <PositionAllocatorFilter<P>> (p);
}


}  // namespace ns3

#endif  /* POSITION_FILTER_H */
