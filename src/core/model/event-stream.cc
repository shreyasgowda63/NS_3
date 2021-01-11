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

#include "event-stream.h"

#include "event-impl.h"
#include "log.h"
#include "pointer.h"
#include "string.h"
#include "uinteger.h"

#include <array>
#include <limits>
#include <sstream>

/**
 * \file
 * \ingroup events
 */
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EventStream");

NS_OBJECT_ENSURE_REGISTERED (EventStream);

TypeId
EventStream::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::EventStream")
    .SetParent<Object> ()
    .SetGroupName ("Core")
  ;
  return tid;
}

/*============================================
 *
 * FifoEventStream
 *
 *============================================*/
NS_OBJECT_ENSURE_REGISTERED (FifoEventStream);

TypeId
FifoEventStream::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::FifoEventStream")
    .SetParent<EventStream> ()
    .SetGroupName ("Core")
    .AddConstructor<FifoEventStream> ()
    .AddAttribute ("StreamSize",
                   "The maximum number of events that the stream can hold",
                   UintegerValue (128),
                   MakeUintegerAccessor (&FifoEventStream::GetStreamSize,
                                         &FifoEventStream::SetStreamSize),
                   MakeUintegerChecker<uint32_t> (1))
  ;
  return tid;
}

FifoEventStream::FifoEventStream ()
  :   m_streamSize (128),
    m_head (0),
    m_tail (0),
    m_count (0),
    m_buffer (m_streamSize)

{
  NS_LOG_FUNCTION (this);
}

FifoEventStream::~FifoEventStream ()
{
  NS_LOG_FUNCTION (this);
}

void
FifoEventStream::SetStreamSize (uint32_t newSize)
{
  NS_LOG_FUNCTION (this << newSize);

  NS_ASSERT_MSG (IsEmpty () == true,
                 "Stream must be empty when changing the stream size");

  if (IsEmpty ())
    {
      m_streamSize = newSize;
      m_buffer.resize (m_streamSize);
      m_head = 0;
      m_tail = 0;
    }
}

uint32_t
FifoEventStream::GetStreamSize () const
{
  NS_LOG_FUNCTION (this);

  return m_streamSize;
}

bool
FifoEventStream::IsEmpty () const
{
  NS_LOG_FUNCTION (this << m_count);

  return m_count == 0;
}

bool
FifoEventStream::IsFull () const
{
  NS_LOG_FUNCTION (this << m_count << m_streamSize);

  return m_count == m_streamSize;
}

bool
FifoEventStream::Insert (SimEvent ev)
{
  NS_LOG_FUNCTION (this << ev);

  if (IsFull ())
    {
      NS_LOG_LOGIC ("Attempted to insert event " << ev << " to a stream that is full");
      return false;
    }

  m_buffer[m_tail] = ev;
  m_tail = (m_tail + 1) % m_streamSize;
  ++m_count;

  return true;
}

const SimEvent&
FifoEventStream::Peek () const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (!IsEmpty (), "Attempted to peek the next event from an empty stream");

  return m_buffer[m_head];
}

SimEvent
FifoEventStream::Next ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (!IsEmpty (), "Attempted to get the next event from an empty stream");

  SimEvent ev = m_buffer[m_head];

  m_head = (m_head + 1) % m_streamSize;
  --m_count;

  return ev;
}

bool
FifoEventStream::Remove (const SimEventKey& key)
{
  NS_LOG_FUNCTION (this << key);

  auto pos = m_head;
  for ( uint32_t i = 0; i < m_count; ++i)
    {
      pos = (pos + i) % m_streamSize;
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
 * RandomEventStream
 *
 *============================================*/
NS_OBJECT_ENSURE_REGISTERED (RandomEventStream);

TypeId
RandomEventStream::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::RandomEventStream")
    .SetParent<EventStream> ()
    .SetGroupName ("Core")
    .AddConstructor<RandomEventStream> ()
    .AddAttribute ("StreamSize",
                   "The maximum number of events that the stream can hold",
                   UintegerValue (100),
                   MakeUintegerAccessor (&RandomEventStream::GetStreamSize,
                                         &RandomEventStream::SetStreamSize),
                   MakeUintegerChecker<uint32_t> (2))
    .AddAttribute ("Random",
                   "The source of randomness used to shuffle events in a tie set. "
                   "The maximum value should be equal to or greater than the buffer "
                   "size",
                   StringValue ("ns3::UniformRandomVariable[Min=0|Max=100]"),
                   MakePointerAccessor (&RandomEventStream::SetRandomSource),
                   MakePointerChecker<RandomVariableStream> ())
  ;
  return tid;
}

RandomEventStream::RandomEventStream ()
  :   m_buffer (),
    m_random ()

{
  NS_LOG_FUNCTION (this);
}

RandomEventStream::~RandomEventStream ()
{
  NS_LOG_FUNCTION (this);
}

void
RandomEventStream::SetRandomSource (Ptr<RandomVariableStream> rand)
{
  NS_LOG_FUNCTION (this << rand->GetInstanceTypeId ().GetName ());

  m_random = rand;
}

void
RandomEventStream::SetStreamSize (uint32_t newSize)
{
  NS_LOG_FUNCTION (this << newSize);

  NS_ASSERT_MSG (IsEmpty () == true,
                 "The stream must be empty when changing the stream size");

  if (IsEmpty ())
    {
      m_streamSize = newSize;
    }

}

uint32_t
RandomEventStream::GetStreamSize () const
{
  NS_LOG_FUNCTION (this);

  return m_streamSize;
}

bool
RandomEventStream::IsEmpty () const
{
  NS_LOG_FUNCTION (this);

  return m_buffer.empty ();
}

bool
RandomEventStream::IsFull () const
{
  NS_LOG_FUNCTION (this);

  return m_buffer.size () == m_streamSize;
}

bool
RandomEventStream::Insert (SimEvent ev)
{
  NS_LOG_FUNCTION (this << ev);

  if (IsFull ())
    {
      NS_LOG_LOGIC ("Attempted to insert event " << ev << " to a stream that is full");
      return false;
    }

  m_buffer.emplace_back (std::move (ev));

  if ( m_buffer.size () > 1 )
    {
      uint32_t currPos = m_buffer.size () - 1;

      //pick a random event
      uint32_t newPos = m_random->GetInteger () % m_buffer.size ();

      if (newPos != currPos)
        {
          NS_LOG_LOGIC ("Swapping events at positions " << newPos
                                                        << " and " << currPos);

          //swap places with the new event
          std::swap (m_buffer[currPos], m_buffer[newPos]);
        }
    }

  return true;
}

const SimEvent&
RandomEventStream::Peek () const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (!IsEmpty (), "Attempted to peek the next event from an empty stream");

  return m_buffer.front ();
}

SimEvent
RandomEventStream::Next ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (!IsEmpty (), "Attempted to get the next event from an empty stream");

  SimEvent ev = m_buffer.front ();

  m_buffer.pop_front ();

  return ev;
}

bool
RandomEventStream::Remove (const SimEventKey& key)
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
