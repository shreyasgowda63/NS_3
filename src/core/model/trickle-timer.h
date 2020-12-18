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

#ifndef TRICKLE_TIMER_H
#define TRICKLE_TIMER_H

#include "nstime.h"
#include "event-id.h"
#include "random-variable-stream.h"

/**
 * \file
 * \ingroup timer
 * ns3::TrickleTimer timer class declaration.
 */

namespace ns3 {

class TimerImpl;

/**
 * \ingroup timer
 * \brief A Trickle Timer following \RFC{6206}.
 *
 * A Trickle Timer is a timer that varies its frequency between a minimum
 * and a maximum, depending on events. It is typically used to exchange
 * information in a highly robust, energy efficient, simple, and scalable manner.
 *
 * Please refer to \RFC6206} for a full description.
 */
class TrickleTimer
{
public:
  /** Constructor. */
  TrickleTimer () = delete; // will never be generated

  /**
   * Constructor.
   *
   * The maximum interval is set to std::exp2 (doublings) * minInterval.
   *
   * \param minInterval Minimum interval.
   * \param doublings Number of doublings to reach the maximum interval.
   * \param redundancy Redundancy constant.
   *
   * A zero value in the redundancy constant means that the suppression
   * algorithm is disabled.
   *
   */
  TrickleTimer (Time minInterval, uint8_t doublings, uint16_t redundancy);

  /** Destructor. */
  ~TrickleTimer ();

  /**
   * Assigns the stream number for the uniform random number generator to use
   *
   * \param streamNum first stream index to use
   * \return the number of stream indices assigned by this helper
   */
  int64_t AssignStreams (int64_t streamNum);

  /**
   * \brief Enable the timer.
   */
  void Enable ();

  /**
   * \brief Records a consistent event.
   */
  void ConsistentEvent ();

  /**
   * \brief Records an inconsistent event.
   */
  void InconsistentEvent ();

  /**
   * \brief Reset the timer.
   */
  void Reset ();

  /**
   * Set the function to execute when the timer expires.
   *
   * \tparam FN \deduced The type of the function.
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
   */
  /**@{*/
  /**
   * \tparam Ts \deduced Argument types.
   * \param [in] args arguments
   */
  template <typename... Ts>
  void SetArguments (Ts&&... args);
  /**@}*/

private:
  /** Internal callback invoked when the timer expires. */
  void TimerExpire (void);
  /** Internal callback invoked when the interval expires. */
  void IntervalExpire (void);

  /**
   * The timer implementation, which contains the bound callback
   * function and arguments.
   */
  TimerImpl *m_impl;

  /** The future event scheduled to expire the timer. */
  EventId m_timerExpiration;

  /** The future event scheduled to expire the interval. */
  EventId m_intervalExpiration;


  Time m_minInterval;    //!< Minimum interval
  Time m_maxInterval;    //!< Maximum interval
  uint16_t m_redundancy; //!< Redundancy constant.

  uint64_t m_ticks;       //|< Interval span.
  Time m_currentInterval; //!< Current interval.
  uint16_t m_counter;     //!< Event counter.

  Ptr<UniformRandomVariable> m_uniRand; //!< Object to generate uniform random numbers
};

} // namespace ns3


/********************************************************************
 *  Implementation of the templates declared above.
 ********************************************************************/

#include "timer-impl.h"

namespace ns3 {


template <typename FN>
void
TrickleTimer::SetFunction (FN fn)
{
  delete m_impl;
  m_impl = MakeTimerImpl (fn);
}
template <typename MEM_PTR, typename OBJ_PTR>
void
TrickleTimer::SetFunction (MEM_PTR memPtr, OBJ_PTR objPtr)
{
  delete m_impl;
  m_impl = MakeTimerImpl (memPtr, objPtr);
}

template <typename... Ts>
void
TrickleTimer::SetArguments (Ts&&... args)
{
  if (m_impl == 0)
    {
      NS_FATAL_ERROR ("You cannot set the arguments of a TrickleTimer before setting its function.");
      return;
    }
  m_impl->SetArgs (std::forward<Ts>(args)...);
}

} // namespace ns3


#endif /* TRICKLE_TIMER_H */
