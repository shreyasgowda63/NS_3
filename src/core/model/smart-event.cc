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
#include "smart-event.h"
#include "log.h"


/**
 * \file
 * \ingroup timer
 * ns3::SmartEvent timer class implementation.
 */

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SmartEvent");

SmartEvent::SmartEvent ()
  : m_impl (0),
    m_event (),
    m_end (MicroSeconds (0)),
    m_isCanceled (false)
{
  NS_LOG_FUNCTION_NOARGS ();
}

SmartEvent::~SmartEvent ()
{
  NS_LOG_FUNCTION (this);
  m_event.Cancel ();
  delete m_impl;
}

void
SmartEvent::SetNewExpiration (Time delay)
{
  NS_LOG_FUNCTION (this << delay);
  Time end = Simulator::Now () + delay;
  Time delayUntileExpiration = Simulator::GetDelayLeft (m_event);

  m_isCanceled = false;

  if (!m_event.IsRunning ())
    {
      m_event = Simulator::Schedule (delay, &SmartEvent::Expire, this);
      m_end = end;
    }
  else if (delayUntileExpiration >= delay && m_end <= end) // event will be delayed
    {
      m_end = end;
    }
  else if (delayUntileExpiration < delay ) // event must be rescheduled
    {
      m_event.Cancel ();
      m_end = end;
      m_event = Simulator::Schedule (delay, &SmartEvent::Expire, this);
    }
  return;
}

void
SmartEvent::Cancel (void)
{
  NS_LOG_FUNCTION (this);
  m_isCanceled = true;
}

void
SmartEvent::Expire (void)
{
  NS_LOG_FUNCTION (this);

  if (m_isCanceled)
    {
      return;
    }
  if (m_end == Simulator::Now ())
    {
      m_impl->Invoke ();
    }
  else
    {
      m_event = Simulator::Schedule (m_end - Now (), &SmartEvent::Expire, this);
    }
}

} // namespace ns3

