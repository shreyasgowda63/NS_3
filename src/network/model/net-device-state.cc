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
    m_operationalState (IF_OPER_DOWN)
{
  NS_LOG_FUNCTION (this);
}


void
NetDeviceState::RegisterAdministrativeStateNotifierWithoutContext (Callback<void, bool> callback)
{
  NS_LOG_FUNCTION (&callback);
  m_administrativeStateChangeCallback.ConnectWithoutContext (callback);
}

void
NetDeviceState::UnRegisterAdministrativeStateNotifierWithoutContext (Callback<void, bool> callback)
{
  NS_LOG_FUNCTION (&callback);
  m_administrativeStateChangeCallback.DisconnectWithoutContext (callback);
}

void
NetDeviceState::RegisterAdministrativeStateNotifierWithContext (Callback<void, bool> callback,
                                                                std::string contextPath)
{
  NS_LOG_FUNCTION (this << &callback << contextPath);
  m_administrativeStateChangeCallback.Connect (callback, contextPath);
}

void
NetDeviceState::UnRegisterAdministrativeStateNotifierWithContext (Callback<void, bool> callback,
                                                                  std::string contextPath)
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
  // If Device is already enabled, then return.
  if (m_administrativeState)
    {
      NS_LOG_WARN ("Device is already enabled.");
      return;
    }

  DoSetUp ();

  m_administrativeState = true;
  m_administrativeStateChangeCallback (true);
  NS_LOG_INFO ("NetDevice has been set administratively UP.");

  m_stateChangeTrace (true, GetOperationalState ());
}

void
NetDeviceState::SetDown (void)
{
  NS_LOG_FUNCTION (this);
  // If device is already disabled, then return.
  if (!m_administrativeState)
    {
      return;
    }

  DoSetDown ();

  m_administrativeState = false;
  NS_LOG_INFO ("NetDevice has been set administratively DOWN.");
  m_administrativeStateChangeCallback (false);

  m_stateChangeTrace (false, GetOperationalState ());
}

void
NetDeviceState::SetOperationalState (OperationalState opState)
{
  NS_LOG_FUNCTION (this);

  switch (opState)
    {
      case IF_OPER_UP:
      case IF_OPER_DOWN:
      case IF_OPER_DORMANT:
      case IF_OPER_LOWERLAYERDOWN:
        if (opState != m_operationalState)
          {
            NS_LOG_INFO ("Operational state of the device is changed to " << opState);
            m_operationalState = opState;
            m_operationalStateChangeCallback (opState);
            m_stateChangeTrace (IsUp (), opState);
          }
        else
          {
            NS_LOG_WARN ("Device is already in OperationalState: " << opState );
          }
        break;

      default:
        NS_LOG_WARN ("Given state: " << opState << " is invalid.");
        break;
    }
}

NetDeviceState::OperationalState
NetDeviceState::GetOperationalState (void) const
{
  return m_operationalState;
}

bool
NetDeviceState::IsUp () const
{
  return m_administrativeState;
}

bool
NetDeviceState::IsOperational () const
{
  return m_operationalState == IF_OPER_UP;
}

} // namespace ns3
