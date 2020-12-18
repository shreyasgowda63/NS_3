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

#include "trickle-timer.h"
#include "log.h"
#include <limits>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TrickleTimer");

TrickleTimer::TrickleTimer (Time minInterval, uint8_t doublings, uint16_t redundancy)
  : m_impl (0),
    m_timerExpiration (),
    m_intervalExpiration (),
    m_currentInterval (Time(0)),
    m_counter (0),
    m_uniRand (CreateObject<UniformRandomVariable> ())
{
  NS_LOG_FUNCTION_NOARGS ();

  NS_ASSERT_MSG (std::exp2 (doublings) < std::numeric_limits<uint64_t>::max(), "Doublings value too large " << std::exp2 (doublings) << " > " << std::numeric_limits<uint64_t>::max());

  m_minInterval = minInterval;
  m_ticks = std::exp2 (doublings);
  m_maxInterval = m_ticks * minInterval;
  m_redundancy = redundancy;
}

TrickleTimer::~TrickleTimer ()
{
  NS_LOG_FUNCTION (this);
  m_timerExpiration.Cancel ();
  m_intervalExpiration.Cancel ();
  delete m_impl;
}

int64_t
TrickleTimer::AssignStreams (int64_t streamNum)
{
  m_uniRand->SetStream (streamNum);
  return 1;
}

void
TrickleTimer::Enable ()
{
  uint64_t randomInt;
  double random;

  randomInt = m_uniRand->GetInteger (1, m_ticks);
  random = randomInt;
  if (randomInt < m_ticks)
    {
      random += m_uniRand->GetValue (0, 1);
    }

  m_currentInterval = m_minInterval * random;
  m_intervalExpiration = Simulator::Schedule (m_currentInterval, &TrickleTimer::IntervalExpire, this);

  m_counter = 0;

  Time timerExpitation = m_uniRand->GetValue (0.5, 1) * m_currentInterval;
  m_timerExpiration = Simulator::Schedule (timerExpitation, &TrickleTimer::TimerExpire, this);

  return;
}


void
TrickleTimer::ConsistentEvent ()
{
  m_counter ++;
}

void
TrickleTimer::InconsistentEvent ()
{
  if (m_currentInterval > m_minInterval)
    {
      Reset ();
    }
}

void
TrickleTimer::Reset ()
{
  m_currentInterval = m_minInterval;
  m_intervalExpiration.Cancel ();
  m_timerExpiration.Cancel ();

  m_intervalExpiration = Simulator::Schedule (m_currentInterval, &TrickleTimer::IntervalExpire, this);

  m_counter = 0;

  Time timerExpitation = m_uniRand->GetValue (0.5, 1) * m_currentInterval;
  m_timerExpiration = Simulator::Schedule (timerExpitation, &TrickleTimer::TimerExpire, this);

  return;
}

void
TrickleTimer::TimerExpire(void)
{
  NS_LOG_FUNCTION (this);

  if (m_counter < m_redundancy || m_redundancy == 0)
    {
      m_impl->Invoke ();
    }
}

void
TrickleTimer::IntervalExpire(void)
{
  NS_LOG_FUNCTION (this);

  m_currentInterval = m_currentInterval * 2;
  if (m_currentInterval > m_maxInterval)
    {
      m_currentInterval = m_maxInterval;
    }

  m_intervalExpiration = Simulator::Schedule (m_currentInterval, &TrickleTimer::IntervalExpire, this);

  m_counter = 0;

  Time timerExpitation = m_uniRand->GetValue (0.5, 1) * m_currentInterval;
  m_timerExpiration = Simulator::Schedule (timerExpitation, &TrickleTimer::TimerExpire, this);
}



} // namespace ns3

