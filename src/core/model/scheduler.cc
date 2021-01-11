/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "scheduler.h"
#include "assert.h"
#include "log.h"
#include "event-impl.h"
#include "event-stream.h"
#include "pointer.h"
#include "string.h"

#include <algorithm>

/**
 * \file
 * \ingroup scheduler
 * ns3::Scheduler implementation.
 */

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Scheduler");

NS_OBJECT_ENSURE_REGISTERED (Scheduler);

Scheduler::Scheduler ()
  :   m_currentTimestamp (0),
    m_stream ()

{
  NS_LOG_FUNCTION (this);
}

Scheduler::~Scheduler ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
Scheduler::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Scheduler")
    .SetParent<Object> ()
    .SetGroupName ("Core")
    .AddAttribute ("EventStream",
                   "Class which controls the ordering of events with the "
                   "same timestamp",
                   StringValue ("ns3::FifoEventStream"),
                   MakePointerAccessor (&Scheduler::SetEventStream),
                   MakePointerChecker<EventStream> ())
  ;
  return tid;
}

bool
Scheduler::IsEmpty () const
{
  NS_LOG_FUNCTION (this);

  return DoIsEmpty () && m_stream->IsEmpty ();
}

const Scheduler::Event&
Scheduler::PeekNext () const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (!IsEmpty (), "Called PeekNext() when no events are available");

  if (m_stream->IsEmpty ())
    {
      //hack to call non const function inside const function
      const_cast<Scheduler*> (this)->FillStream ();
    }

  return m_stream->Peek ();
}

Scheduler::Event
Scheduler::RemoveNext ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (!IsEmpty (), "Called RemoveNext() when no events are available");

  if (m_stream->IsEmpty ())
    {
      FillStream ();
    }

  return m_stream->Next ();
}

void
Scheduler::Remove (const Event& ev)
{
  NS_LOG_FUNCTION (this);

  bool found = m_stream->Remove (ev.key);

  if (!found)
    {
      //not in the event stream, try the event source
      DoRemove (ev);
    }
}

void
Scheduler::SetEventStream (Ptr<EventStream> stream)
{
  NS_LOG_FUNCTION (this << stream);

  NS_ASSERT_MSG (stream, "EventStream cannot be a null pointer");

  while (m_stream && !m_stream->IsEmpty ())
    {
      if (stream->IsFull ())
        {
          //no more room in the new stream, add the events back to
          //the event store
          Insert (m_stream->Next ());
        }
      else
        {
          stream->Insert (m_stream->Next ());
        }
    }

  m_stream = stream;
}

void
Scheduler::FillStream ()
{
  NS_LOG_FUNCTION (this);

  while (!m_stream->IsFull ())
    {
      if (DoIsEmpty ())
        {
          NS_LOG_LOGIC ("Event store is empty");
          return;
        }

      const auto& ev = DoPeekNext ();
      const auto& key = ev.key;

      if ( key.m_ts != m_currentTimestamp)
        {
          //dont add events with a new timestamp until all of the events with
          //the current timestamp have been processed
          if (!m_stream->IsEmpty ())
            {
              NS_LOG_LOGIC ("No more events with timestamp " << m_currentTimestamp);
              return;
            }

          NS_LOG_LOGIC ("Updating filter timestamp from " << m_currentTimestamp
                                                          << " to " << key.m_ts);

          m_currentTimestamp = key.m_ts;
        }

      m_stream->Insert (DoRemoveNext ());
    }
}

} // namespace ns3
