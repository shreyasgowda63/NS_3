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

#ifndef SIMULATOR_EVENT_H_
#define SIMULATOR_EVENT_H_

#include "int64x64.h"

#include <ostream>

/**
 * \file
 * \ingroup events
 * ns3::SimEventKey declaration
 * ns3::SimEvent declaration
 */

namespace ns3 {

/**
 * \ingroup scheduler 
 * \defgroup event Events
 */

//forward declaration
class EventImpl;

/**
 * \ingroup events
 * Structure for sorting and comparing Events.
 */
struct SimEventKey
{
  uint64_t m_ts;         /**< Event time stamp. */
  uint32_t m_uid;        /**< Event unique id. */
  uint32_t m_context;    /**< Event context. */
};
/**
 * \ingroup events
 * Scheduler event.
 *
 * An Event consists of an EventKey, used for maintaining the schedule,
 * and an EventImpl which is the actual implementation.
 */
struct SimEvent
{
  EventImpl *impl;       /**< Pointer to the event implementation. */
  SimEventKey key;          /**< Key for sorting and ordering Events. */
};

/**
 * \relates SimEventKey
 * \brief Output stream operator for event keys
 *
 * \param stream The output stream
 * \param key The event key to print
 *
 * \return Reference to \p stream
 */
inline std::ostream&
operator << (std::ostream& stream, const SimEventKey& key)
{
    stream << key.m_ts << ' ' << key.m_uid << ' ' << key.m_context;

    return stream;
}

/**
 * \relates SimEventKey
 * \brief Less than operator for event keys 
 *
 * Note the invariants which this function must provide:
 * - irreflexibility: f (x,x) is false
 * - antisymmetry: f(x,y) = !f(y,x)
 * - transitivity: f(x,y) and f(y,z) => f(x,z)
 *
 * \param [in] a The first event key.
 * \param [in] b The second event key.
 * \returns \c true if \c a < \c b
 */
inline bool operator< (const SimEventKey &a,
                        const SimEventKey &b)
{
    if (a.m_ts == b.m_ts)
    {
        return a.m_uid < b.m_uid;
    }

    return a.m_ts < b.m_ts;
}

/**
 * \relates SimEventKey
 * \brief Equality operator for event keys
 *
 * Two event keys are equal if they have the same timestamp, uid, and context
 *
 * \param [in] a The first event key.
 * \param [in] b The second event key.
 * \returns \c true if \c a == \c b
 */
inline bool operator== (const SimEventKey &a,
                         const SimEventKey &b)
{
    return a.m_ts == b.m_ts
            && a.m_uid == b.m_uid
            && a.m_context == b.m_context;
}

/**
 * \relates SimEventKey
 * \brief Greater than operator for event keys
 *
 * \param [in] a The first event key.
 * \param [in] b The second event key.
 * \returns \c true if \c a > \c b
 */
inline bool operator> (const SimEventKey &a,
                        const SimEventKey &b)
{
    if (a.m_ts == b.m_ts)
    {
        return a.m_uid > b.m_uid;
    }

    return a.m_ts > b.m_ts;
}

/**
 * \relates SimEvent
 * \brief Output stream operator for events 
 *
 * \param stream The output stream
 * \param key The event to print
 *
 * \return Reference to \p stream
 */
inline std::ostream&
operator << (std::ostream& stream, const SimEvent& ev)
{
    stream << ev.key << ' ' << ev.impl; 

    return stream;
}

/**
 * \relates SimEvent
 * \brief Less than operator for events
 *
 * \param [in] a The first event.
 * \param [in] b The second event.
 * \returns \c true if \c a.key < \c b.key
 */
inline bool operator< (const SimEvent &a,
                        const SimEvent &b)
{
  return a.key < b.key;
}

/**
 * \relates SimEvent
 * \brief Equality operator for events
 *
 * \param [in] a The first event.
 * \param [in] b The second event.
 * \returns \c true if \c a.key == \c b.key
 */
inline bool operator== (const SimEvent &a,
                        const SimEvent &b)
{
  return a.key == b.key;
}

}   //  ns3 namespace

#endif

