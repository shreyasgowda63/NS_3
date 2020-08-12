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
 * Author: Ananthakrishnan S <ananthakrishnan190@gmail.com>
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
  : m_administrativeState (false),
    m_operationalState (OperationalState::IF_OPER_DOWN)
{
  NS_LOG_FUNCTION (this);
}


void
NetDeviceState::RegisterAdministrativeStateNotifierWithoutContext (Callback<void, bool>
                                                                   callback)
{
  NS_LOG_FUNCTION (&callback);
  m_administrativeStateChangeCallback.ConnectWithoutContext (callback);
}

void
NetDeviceState::UnRegisterAdministrativeStateNotifierWithoutContext (Callback<void,
                                                                              bool> callback)
{
  NS_LOG_FUNCTION (&callback);
  m_administrativeStateChangeCallback.DisconnectWithoutContext (callback);
}

void
NetDeviceState::RegisterAdministrativeStateNotifierWithContext (Callback<void, bool>
                                                                callback, std::string contextPath)
{
  NS_LOG_FUNCTION (this << &callback << contextPath);
  m_administrativeStateChangeCallback.Connect (callback, contextPath);
}

void
NetDeviceState::UnRegisterAdministrativeStateNotifierWithContext (Callback<void, bool>
                                                                  callback, std::string contextPath)
{
  NS_LOG_FUNCTION (this << &callback << contextPath);
  m_administrativeStateChangeCallback.Disconnect (callback, contextPath);
}

void
NetDeviceState::RegisterOperationalStateNotifierWithoutContext (Callback<void, OperationalState> callback)
{
  NS_LOG_FUNCTION (&callback);
  m_operationalStateChangeCallback.ConnectWithoutContext (callback);
}

void
NetDeviceState::UnRegisterOperationalStateNotifierWithoutContext (Callback<void, OperationalState> callback)
{
  NS_LOG_FUNCTION (&callback);
  m_operationalStateChangeCallback.ConnectWithoutContext (callback);
}

void
NetDeviceState::RegisterOperationalStateNotifierWithContext (Callback<void, OperationalState> callback,
                                                             std::string contextPath)
{
  NS_LOG_FUNCTION (&callback);
  m_operationalStateChangeCallback.Connect (callback, contextPath);
}

void
NetDeviceState::UnRegisterOperationalStateNotifierWithContext (Callback<void, OperationalState> callback,
                                                               std::string contextPath)
{
  NS_LOG_FUNCTION (&callback);
  m_operationalStateChangeCallback.Disconnect (callback, contextPath);
}

void
NetDeviceState::SetUp (void)
{
  NS_LOG_FUNCTION (this);
  DoSetUp ();
  m_administrativeState = true;
  NS_LOG_INFO ("NetDevice has been set administratively UP.");
  m_stateChangeTrace(true, GetOperationalState ());
  m_administrativeStateChangeCallback (true);
}

void
NetDeviceState::SetDown (void)
{
  NS_LOG_FUNCTION (this);
  DoSetDown ();
  m_administrativeState = false;
  NS_LOG_INFO ("NetDevice has been set administratively DOWN.");
  m_stateChangeTrace (false, GetOperationalState ());
  m_administrativeStateChangeCallback (true);
}

void
NetDeviceState::SetOperationalState (OperationalState opState)
{
  NS_LOG_FUNCTION (this);

  if (!m_administrativeState)
    {
      NS_LOG_WARN ("Device is not administratively UP; Not setting operational state.");
      return;
    }

  if (opState == OperationalState::IF_OPER_UP
      && !(m_operationalState == OperationalState::IF_OPER_UP))
    {
      NS_LOG_INFO ("Operational state of the device is changed to IF_OPER_UP.");
      m_operationalState = OperationalState::IF_OPER_UP;
      m_operationalStateChangeCallback (OperationalState::IF_OPER_UP);
    }
  else if (opState == OperationalState::IF_OPER_DOWN
           && !(m_operationalState == OperationalState::IF_OPER_DOWN))
    {
      NS_LOG_INFO ("Operational state of the device is changed to IF_OPER_DOWN.");
      m_operationalState = OperationalState::IF_OPER_DOWN;
      m_operationalStateChangeCallback (OperationalState::IF_OPER_DOWN);
    }
  else if (opState == OperationalState::IF_OPER_DORMANT
           && !(m_operationalState == OperationalState::IF_OPER_DORMANT))
    {
      NS_LOG_INFO ("Operational state of the device is changed to IF_OPER_DORMANT.");
      m_operationalState = OperationalState::IF_OPER_DORMANT;
      m_operationalStateChangeCallback (OperationalState::IF_OPER_DORMANT);
    }
  else if (opState == OperationalState::IF_OPER_LOWERLAYERDOWN
           && !(m_operationalState == OperationalState::IF_OPER_DORMANT))
    {
      NS_LOG_INFO ("Operational state of the device is changed to IF_OPER_DORMANT.");
      m_operationalState = OperationalState::IF_OPER_LOWERLAYERDOWN;
      m_operationalStateChangeCallback (OperationalState::IF_OPER_LOWERLAYERDOWN);
    }
  else
    {
      NS_LOG_WARN ("Device is already in the specified state or given state is invalid.");
    }
}

OperationalState
NetDeviceState::GetOperationalState (void) const
{
  return m_operationalState;
}

bool
NetDeviceState::IsUp () const
{
  return m_administrativeState;
}

} // namespace ns3
