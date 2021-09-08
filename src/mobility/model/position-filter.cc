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

#include "position-filter.h"
#include "ns3/log.h"
#include "ns3/pointer.h"

/**
 * \file
 * \ingroup mobility
 * ns3::PositionFilter, ns3::PositionAllocatorFilter,
 * and ns3::FilteredPositionAllocator implementations.
 */


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PositionFilter");

NS_OBJECT_ENSURE_REGISTERED (FilteredPositionAllocator);


PositionFilter::~PositionFilter (void)
{
}


/* static */
TypeId
FilteredPositionAllocator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FilteredPositionAllocator")
    .SetParent<PositionAllocator> ()
    .SetGroupName ("Mobility")
    .AddConstructor<FilteredPositionAllocator> ()
    .AddAttribute ("Allocator",
                   "The underlying PositionAllocator which should be filtered.",
                   PointerValue (0),
                   MakePointerAccessor (&FilteredPositionAllocator::SetPositionAllocator),
                   MakePointerChecker<PositionAllocator> ())
    .AddAttribute ("Filter",
                   "The PositionFilter to use filtering the PositionAllocator.",
                   PointerValue (0),
                   MakePointerAccessor (&FilteredPositionAllocator::SetPositionFilter),
                   MakePointerChecker<PositionAllocator> ())
    ;
  return tid;
}  // GetTypeId

  
FilteredPositionAllocator::FilteredPositionAllocator ()
  : m_normal (true)
{
}

  
FilteredPositionAllocator::~FilteredPositionAllocator ()
{
}

void  
FilteredPositionAllocator::SetPositionAllocator (Ptr<PositionAllocator> p)
{
  m_allocator = p;
}

void
FilteredPositionAllocator::SetPositionFilter (Ptr<PositionFilter> f)
{
  m_filter = f;
}

void
FilteredPositionAllocator::SetInvert (bool invert)
{
  m_normal = !invert;
}
  

Vector3D
FilteredPositionAllocator::GetNext (void) const
{
  NS_ASSERT_MSG (m_allocator != 0, "Need to set PositionAllocator.");
  NS_ASSERT_MSG (m_filter != 0,    "Needt o set PositionFilter.");

  Vector3D p;
  bool good = true;
  do
    {
      p = m_allocator->GetNext ();
      good = ( m_normal == m_filter->IsInside (p));
    }
  while (!good);
  
  return p;
}

int64_t
FilteredPositionAllocator::AssignStreams (int64_t stream)
{
  return m_allocator->AssignStreams (stream);
}
  
  
}  // namespace ns3

