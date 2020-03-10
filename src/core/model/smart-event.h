/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Universita' di Firenze, Italy
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
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */
#ifndef SMARTEVENT_H
#define SMARTEVENT_H

#include "nstime.h"
#include "event-id.h"

/**
 * \file
 * \ingroup timer
 * ns3::SmartEvent class declaration.
 */

namespace ns3 {

class TimerImpl;

/**
 * \ingroup timer
 * \brief A very simple SmartEvent operating in virtual time.
 *
 * The SmartEvent timer is heavily based on ns3::Watchdog.
 * Once started the timer can be suspended, cancelled, shortened, or delayed.
 *
 * This implementation tries to minimize the number of times an event must be
 * canceled and rescheduled, resulting in a smaller footprint of the Simulator event queue.
 *
 * \see Timer for a more sophisticated general purpose timer.
 */
class SmartEvent
{
public:
  /** Constructor. */
  SmartEvent ();
  /** Destructor. */
  ~SmartEvent ();

  /**
   * Set a new timer expiration.
   *
   * \param [in] delay The SmartEvent delay
   *
   * After a call to this method, the SmartEvent will not be triggered
   * until the delay specified has been expired. This operation is
   * sometimes named "re-arming" a SmartEvent in some operating systems.
   */
  void SetNewExpiration (Time delay);

  /**
   * Cancel the timer. A call to SmartEvent::SetNewExpiration() will re-arm the timer.
   */
  void Cancel ();

  /**
   * Checks if the SmartEvent is pending (i.e., not canceled and not expired).
   * \returns \c true if the event is pending, \c false otherwise.
   */
  bool IsPending ();

  /**
   * Set the function to execute when the timer expires.
   *
   * \param [in] fn The function
   *
   * Store this function in this Timer for later use by Timer::Schedule.
   */
  template <typename FN>
  void SetFunction (FN fn);

  /**
   * Set the function to execute when the timer expires.
   *
   * \tparam MEM_PTR \deduced Class method function type.
   * \tparam OBJ_PTR \deduced Class type containing the function.
   * \param [in] memPtr The member function pointer
   * \param [in] objPtr The pointer to object
   *
   * Store this function and object in this Timer for later use by Timer::Schedule.
   */
  template <typename MEM_PTR, typename OBJ_PTR>
  void SetFunction (MEM_PTR memPtr, OBJ_PTR objPtr);

  /**
   * Set the arguments to be used when invoking the expire function.
   * \tparam Ts \deduced Type of the arguments.
   * \param [in] a the arguments
   */
  template <typename... Ts>
  void SetArguments (Ts&&... a);

private:
  /** Internal callback invoked when the timer expires. */
  void Expire (void);
  /**
   * The timer implementation, which contains the bound callback
   * function and arguments.
   */
  TimerImpl *m_impl;
  /** The future event scheduled to expire the timer. */
  EventId m_event;
  /** The absolute time when the timer will expire. */
  Time m_end;
  /** The SmartEvent is canceled. */
  bool m_cancelled;
};

} // namespace ns3


/********************************************************************
 *  Implementation of the templates declared above.
 ********************************************************************/

#include "timer-impl.h"

namespace ns3 {


template <typename FN>
void 
SmartEvent::SetFunction (FN fn)
{
  delete m_impl;
  m_impl = MakeTimerImpl (fn);
}
template <typename MEM_PTR, typename OBJ_PTR>
void 
SmartEvent::SetFunction (MEM_PTR memPtr, OBJ_PTR objPtr)
{
  delete m_impl;
  m_impl = MakeTimerImpl (memPtr, objPtr);
}

template <typename... Ts>
void 
SmartEvent::SetArguments (Ts&&... a)
{
  if (m_impl == 0)
    {
      NS_FATAL_ERROR ("You cannot set the arguments of a SmartEvent before setting its function.");
      return;
    }
  m_impl->SetArgs (a...);
}

} // namespace ns3


#endif /* SmartEvent_H */
