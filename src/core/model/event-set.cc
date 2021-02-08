/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019   Lawrence Livermore National Laboratory
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
 * Author: Mathew Bielejeski <bielejeski1@gmail.com>
 */

#include "event-set.h"

#include "event-impl.h"
#include "log.h"
#include "pointer.h"
#include "string.h"
#include "uinteger.h"

#include <algorithm>
#include <array>
#include <limits>
#include <sstream>

/**
 * \file
 * \ingroup events
 */
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EventSet");

NS_OBJECT_ENSURE_REGISTERED (EventSet);

TypeId
EventSet::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::EventSet")
    .SetParent<Object> ()
    .SetGroupName ("Core")
  ;
  return tid;
}

/*============================================
 *
 * FifoEventSet
 *
 *============================================*/
NS_OBJECT_ENSURE_REGISTERED (FifoEventSet);

TypeId
FifoEventSet::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::FifoEventSet")
    .SetParent<EventSet> ()
    .SetGroupName ("Core")
    .AddConstructor<FifoEventSet> ()
    .AddAttribute ("MaxSize",
                   "The maximum number of events that the set can hold",
                   UintegerValue (512),
                   MakeUintegerAccessor (&FifoEventSet::GetMaxSize,
                                         &FifoEventSet::SetMaxSize),
                   MakeUintegerChecker<uint32_t> (1))
  ;
  return tid;
}

FifoEventSet::FifoEventSet ()
  :   m_maxSize (512),
    m_head (0),
    m_tail (0),
    m_count (0),
    m_buffer (m_maxSize)

{
  NS_LOG_FUNCTION (this);
}

FifoEventSet::~FifoEventSet ()
{
  NS_LOG_FUNCTION (this);
}

void
FifoEventSet::SetMaxSize (uint32_t newSize)
{
  NS_LOG_FUNCTION (this << newSize);

  NS_ASSERT_MSG (IsEmpty () == true,
                 "Set must be empty when changing the maximum size");

  if (IsEmpty ())
    {
      m_maxSize = newSize;
      m_buffer.resize (m_maxSize);
      m_head = 0;
      m_tail = 0;
    }
}

uint32_t
FifoEventSet::GetMaxSize () const
{
  NS_LOG_FUNCTION (this);

  return m_maxSize;
}

bool
FifoEventSet::IsEmpty () const
{
  NS_LOG_FUNCTION (this << m_count);

  return m_count == 0;
}

bool
FifoEventSet::IsFull () const
{
  NS_LOG_FUNCTION (this << m_count << m_maxSize);

  return m_count == m_maxSize;
}

bool
FifoEventSet::Insert (SimEvent ev)
{
  NS_LOG_FUNCTION (this << ev);

  if (IsFull ())
    {
      NS_LOG_LOGIC ("Attempted to insert event " << ev << " to a set that is full");
      return false;
    }

  m_buffer[m_tail] = ev;
  m_tail = (m_tail + 1) % m_maxSize;
  ++m_count;

  return true;
}

const SimEvent&
FifoEventSet::Peek () const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (!IsEmpty (), "Attempted to peek the next event from an empty set");

  return m_buffer[m_head];
}

SimEvent
FifoEventSet::Next ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (!IsEmpty (), "Attempted to get the next event from an empty set");

  SimEvent ev = m_buffer[m_head];

  m_head = (m_head + 1) % m_maxSize;
  --m_count;

  return ev;
}

bool
FifoEventSet::Remove (const SimEventKey& key)
{
  NS_LOG_FUNCTION (this << key);

  auto pos = m_head;
  for ( uint32_t i = 0; i < m_count; ++i)
    {
      pos = (pos + i) % m_maxSize;
      auto& ev = m_buffer[pos];

      if ( ev.key == key )
        {
          ev.impl->Cancel ();
          return true;
        }
    }

  return false;
}

/*============================================
 *
 * LifoEventSet
 *
 *============================================*/
NS_OBJECT_ENSURE_REGISTERED (LifoEventSet);

TypeId
LifoEventSet::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::LifoEventSet")
    .SetParent<EventSet> ()
    .SetGroupName ("Core")
    .AddConstructor<LifoEventSet> ()
    .AddAttribute ("MaxSize",
                   "The maximum number of events that the set can hold",
                   UintegerValue (512),
                   MakeUintegerAccessor (&LifoEventSet::GetMaxSize,
                                         &LifoEventSet::SetMaxSize),
                   MakeUintegerChecker<uint32_t> (1))
  ;
  return tid;
}

LifoEventSet::LifoEventSet ()
  :   m_maxSize (512),
    m_buffer ()

{
  NS_LOG_FUNCTION (this);

  m_buffer.reserve (m_maxSize);
}

LifoEventSet::~LifoEventSet ()
{
  NS_LOG_FUNCTION (this);
}

void
LifoEventSet::SetMaxSize (uint32_t newSize)
{
  NS_LOG_FUNCTION (this << newSize);

  NS_ASSERT_MSG (IsEmpty () == true,
                 "Set must be empty before changing the maximum size");

  if (IsEmpty ())
    {
      m_maxSize = newSize;
      m_buffer.reserve (m_maxSize);
    }
}

uint32_t
LifoEventSet::GetMaxSize () const
{
  NS_LOG_FUNCTION (this);

  return m_maxSize;
}

bool
LifoEventSet::IsEmpty () const
{
  NS_LOG_FUNCTION (this << m_buffer.empty ());

  return m_buffer.empty ();
}

bool
LifoEventSet::IsFull () const
{
  NS_LOG_FUNCTION (this << m_buffer.size () << m_maxSize);

  return m_buffer.size () == m_maxSize;
}

bool
LifoEventSet::Insert (SimEvent ev)
{
  NS_LOG_FUNCTION (this << ev);

  if (IsFull ())
    {
      NS_LOG_LOGIC ("Attempted to insert event " << ev << " to a set that is full");
      return false;
    }

  m_buffer.emplace_back (std::move (ev));

  return true;
}

const SimEvent&
LifoEventSet::Peek () const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (!IsEmpty (), "Attempted to peek the next event from an empty set");

  return *m_buffer.rbegin ();
}

SimEvent
LifoEventSet::Next ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (!IsEmpty (), "Attempted to get the next event from an empty set");

  SimEvent ev = *m_buffer.rbegin ();
  m_buffer.pop_back ();

  return ev;
}

bool
LifoEventSet::Remove (const SimEventKey& key)
{
  NS_LOG_FUNCTION (this << key);

  auto iter = std::find_if(m_buffer.begin (), m_buffer.end (),
                            [&key](const SimEvent& ev)
                            {
                                return key == ev.key;
                            });
  
  if (iter == m_buffer.end ())
  {
      return false;
  }

  iter->impl->Cancel ();

  return true;
}

/*============================================
 *
 * RandomEventSet
 *
 *============================================*/
NS_OBJECT_ENSURE_REGISTERED (RandomEventSet);

TypeId
RandomEventSet::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::RandomEventSet")
    .SetParent<EventSet> ()
    .SetGroupName ("Core")
    .AddConstructor<RandomEventSet> ()
    .AddAttribute ("MaxSize",
                   "The maximum number of events that the set can hold",
                   UintegerValue (512),
                   MakeUintegerAccessor (&RandomEventSet::GetMaxSize,
                                         &RandomEventSet::SetMaxSize),
                   MakeUintegerChecker<uint32_t> (2))
    .AddAttribute ("Random",
                   "The source of randomness used to shuffle events in a tie set. "
                   "The maximum value should be greater than or equal to the "
                   "buffer size",
                   StringValue ("ns3::UniformRandomVariable[Min=0|Max=512]"),
                   MakePointerAccessor (&RandomEventSet::SetRandomSource),
                   MakePointerChecker<RandomVariableStream> ())
  ;
  return tid;
}

RandomEventSet::RandomEventSet ()
  :   m_maxSize (512),
      m_buffer (),
      m_random ()

{
  NS_LOG_FUNCTION (this);
}

RandomEventSet::~RandomEventSet ()
{
  NS_LOG_FUNCTION (this);
}

void
RandomEventSet::SetRandomSource (Ptr<RandomVariableStream> rand)
{
  NS_LOG_FUNCTION (this << rand->GetInstanceTypeId ().GetName ());

  m_random = rand;
}

void
RandomEventSet::SetMaxSize (uint32_t newSize)
{
  NS_LOG_FUNCTION (this << newSize);

  NS_ASSERT_MSG (IsEmpty () == true,
                 "The set must be empty before changing the maximum size");

  if (IsEmpty ())
    {
      m_maxSize = newSize;
    }

}

uint32_t
RandomEventSet::GetMaxSize () const
{
  NS_LOG_FUNCTION (this);

  return m_maxSize;
}

bool
RandomEventSet::IsEmpty () const
{
  NS_LOG_FUNCTION (this);

  return m_buffer.empty ();
}

bool
RandomEventSet::IsFull () const
{
  NS_LOG_FUNCTION (this);

  return m_buffer.size () == m_maxSize;
}

bool
RandomEventSet::Insert (SimEvent ev)
{
  NS_LOG_FUNCTION (this << ev);

  if (IsFull ())
    {
      NS_LOG_LOGIC ("Attempted to insert event " << ev << " to a set that is full");
      return false;
    }

  if ( !m_buffer.empty () )
    {
      //pick a random event to swap
      uint32_t position = m_random->GetInteger () % m_buffer.size ();

      //move the event at the current position to the back of the line
      m_buffer.emplace_back (std::move (m_buffer[position]));

      //insert the new event at the random position
      m_buffer[position] = std::move (ev);
    }
  else
    {
        //only one item, just add it to the buffer
        m_buffer.emplace_back (std::move (ev));
    }

  return true;
}

const SimEvent&
RandomEventSet::Peek () const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (!IsEmpty (), "Attempted to peek the next event from an empty set");

  return m_buffer.front ();
}

SimEvent
RandomEventSet::Next ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (!IsEmpty (), "Attempted to get the next event from an empty set");

  SimEvent ev = m_buffer.front ();

  m_buffer.pop_front ();

  return ev;
}

bool
RandomEventSet::Remove (const SimEventKey& key)
{
  NS_LOG_FUNCTION (this << key);

  //Unfortunately we have to perform a linear scan to find the event
  for ( auto& event : m_buffer )
    {
      if ( event.key == key )
        {
          event.impl->Cancel ();
          return true;
        }
    }

  return false;
}

}   //  ns3 namespace
