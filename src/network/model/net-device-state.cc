/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Ananthakrishnan S
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
 * Authors: Ananthakrishnan S <ananthakrishnan190@gmail.com>
 *          Tom Henderson <tomh@tomh.org>
 */

#include "net-device-state.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NetDeviceState");

NS_OBJECT_ENSURE_REGISTERED (NetDeviceState);

TypeId
NetDeviceState::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetDeviceState")
    .SetParent<Object> ()
    .SetGroupName ("Network")
    .AddTraceSource ("StateChange",
                     "Trace source indicating a state change in the NetDevice",
                     MakeTraceSourceAccessor (&NetDeviceState::m_stateChangeTrace),
                     "ns3::NetDeviceState::StateChangedTracedCallback")
  ;
  return tid;
}

NetDeviceState::NetDeviceState ()
  : m_isUp (false),
    m_operationalState (IF_OPER_DOWN)
{
  NS_LOG_FUNCTION (this);
}

void
NetDeviceState::SetUp (void)
{
  NS_LOG_FUNCTION (this);
  if (m_isUp)
    {
      NS_LOG_WARN ("Device is already up.");
      return;
    }
  DoSetUp ();
  m_isUp = true;
  m_stateChangeTrace (true, m_operationalState);
}

void
NetDeviceState::SetDown (void)
{
  NS_LOG_FUNCTION (this);
  if (!m_isUp)
    {
      NS_LOG_WARN ("Device is already down.");
      return;
    }
  DoSetDown ();
  SetOperationalState (IF_OPER_DOWN);
  m_isUp = false;
  m_stateChangeTrace (false, m_operationalState);
}

void
NetDeviceState::SetOperationalState (OperationalState opState)
{
  NS_LOG_FUNCTION (this << opState);
  if (m_operationalState == opState)
    {
      NS_LOG_WARN ("No state change made");
      return;
    }
  DoSetOperationalState (opState);
  m_operationalState = opState;
  m_stateChangeTrace (m_isUp, m_operationalState);
}

NetDeviceState::OperationalState
NetDeviceState::GetOperationalState (void) const
{
  return m_operationalState;
}

bool
NetDeviceState::IsUp () const
{
  return m_isUp;
}

void
NetDeviceState::DoSetUp (void)
{
  // Subclasses may override for device-specific actions
}

void
NetDeviceState::DoSetDown (void)
{
  // Subclasses may override for device-specific actions
}

void
NetDeviceState::DoSetOperationalState (OperationalState opState)
{
  // Subclasses may override for device-specific actions
}

} // namespace ns3
