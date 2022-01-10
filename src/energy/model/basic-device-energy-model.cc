/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Institute of Operating Systems and Computer Networks, TU Braunschweig
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
 * Author: Philip Hönnecke <p.hoennecke@tu-braunschweig.de>
 */

#include "basic-device-energy-model.h"
#include <ns3/energy-source.h>

namespace ns3 {

// ##################################################################### //
// ####################### BasicEnergyModelStates ###################### //
// ##################################################################### //

NS_LOG_COMPONENT_DEFINE ("BasicDeviceEnergyModel");

BasicEnergyModelStates::BasicEnergyModelStates ()
{
  NS_LOG_FUNCTION (this);

  uint32_t offIndex = AddState ("Off", 0.0); // Always added

  NS_ASSERT (offIndex == 0);
}

TypeId
BasicEnergyModelStates::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BasicEnergyModelStates")
                          .SetParent<Object> ()
                          .SetGroupName ("Energy")
                          .AddConstructor<BasicEnergyModelStates> ();
  return tid;
}

uint32_t
BasicEnergyModelStates::AddState (State state)
{
  NS_LOG_FUNCTION (this << &state << state.first << state.second);

  m_states.insert (std::pair<uint32_t, State> (m_indexCounter, state));

  return m_indexCounter++;
}

uint32_t
BasicEnergyModelStates::AddState (std::string name, double currentA)
{
  NS_LOG_FUNCTION (this << name << currentA);

  return AddState (State (name, currentA));
}

bool
BasicEnergyModelStates::RemoveState (uint32_t index)
{
  NS_LOG_FUNCTION (this << index);

  if (m_states.find (index) != m_states.end ())
    {
      // Index exists
      m_states.erase (index);
      return true;
    }
  else
    {
      // Index does not exist
      NS_LOG_WARN ("BasicEnergyModelStates::RemoveState: Index did not exists: " << index << ".");
      return false;
    }
}

std::string
BasicEnergyModelStates::GetStateName (uint32_t index) const
{
  NS_LOG_FUNCTION (this << index);

  std::map<uint32_t, State>::const_iterator i = m_states.find (index);
  if (i != m_states.end ())
    {
      // Index exists
      return i->second.first;
    }
  else
    {
      // Index does not exist
      NS_LOG_WARN ("BasicEnergyModelStates::GetStateName: Index did not exists: " << index
                                                                                    << ".");
      return "Not found";
    }
}

BasicEnergyModelStates::State
BasicEnergyModelStates::GetState (uint32_t index) const
{
  NS_LOG_FUNCTION (this << index);

  std::map<uint32_t, State>::const_iterator i = m_states.find (index);
  if (i != m_states.end ())
    {
      // Index exists
      return i->second;
    }
  else
    {
      // Index does not exist
      NS_LOG_WARN ("BasicEnergyModelStates::GetState: Index did not exists: " << index << ".");
      return State ("Not found", 0.0);
    }
}

bool
BasicEnergyModelStates::SetCurrent (uint32_t index, double currentA)
{
  NS_LOG_FUNCTION (this << index << currentA);

  std::map<uint32_t, State>::iterator i = m_states.find (index);
  if (i != m_states.end ())
    {
      // Index exists
      i->second.second = currentA;
      return true;
    }
  else
    {
      // Index does not exist
      NS_LOG_WARN ("BasicEnergyModelStates::SetCurrent: Index did not exists: " << index << ".");
      return false;
    }
}

// ##################################################################### //
// ##################ä#### BasicDeviceEnergyModel ###################### //
// ##################################################################### //

NS_OBJECT_ENSURE_REGISTERED (BasicDeviceEnergyModel);

BasicDeviceEnergyModel::BasicDeviceEnergyModel ()
{
  NS_LOG_FUNCTION (this);
  m_state = 0;
  m_energyDepleted = false;
  m_lastUpdateTime = Simulator::Now();
  m_totalEnergyConsumption = 0.0;
  m_defaultState = 0;
}

TypeId
BasicDeviceEnergyModel::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::BasicDeviceEnergyModel")
          .SetParent<DeviceEnergyModel> ()
          .SetGroupName ("Energy")
          .AddConstructor<BasicDeviceEnergyModel> ()
          .AddAttribute ("BasicEnergyModelStates",
                         "The BasicEnergyModelStates object to assign to this model.",
                         PointerValue (), MakePointerAccessor (&BasicDeviceEnergyModel::m_states),
                         MakePointerChecker<BasicEnergyModelStates> ())
          .AddAttribute (
              "DefaultState", "Default state for beginning and after energy is recharged.",
              UintegerValue (0), MakeUintegerAccessor (&BasicDeviceEnergyModel::m_defaultState),
              MakeUintegerChecker<uint32_t> (0, UINT32_MAX))
          .AddTraceSource (
              "TotalEnergyConsumption", "Total energy consumption of the sensor device.",
              MakeTraceSourceAccessor (&BasicDeviceEnergyModel::m_totalEnergyConsumption),
              "ns3::TracedValueCallback::Double")
          .AddTraceSource ("State", "The state of this model.",
                           MakeTraceSourceAccessor (&BasicDeviceEnergyModel::m_state),
                           "ns3::TracedValueCallback::UInt32")
          .AddTraceSource ("CurrentA", "The current in A of this model.",
                           MakeTraceSourceAccessor (&BasicDeviceEnergyModel::m_currentA),
                           "ns3::TracedValueCallback::Double");
  return tid;
}

void
BasicDeviceEnergyModel::SetEnergySource (Ptr<EnergySource> source)
{
  NS_LOG_FUNCTION (this << source);
  m_source = source;
}

double
BasicDeviceEnergyModel::GetTotalEnergyConsumption (void) const
{
  NS_LOG_FUNCTION (this);

  // This method does similar updates as in ChangeState.
  // However, no changes to this are made.

  // Get duration until last update
  Time duration = Simulator::Now () - m_lastUpdateTime;
  NS_ASSERT (duration.IsPositive ()); // check if duration is valid

  // Energy to decrease = current * voltage * time
  double supplyVoltage = m_source->GetSupplyVoltage ();
  double energyToDecrease = (duration * GetCurrentA () * supplyVoltage).GetSeconds ();

  // Notify energy source
  m_source->UpdateEnergySource ();

  return m_totalEnergyConsumption + energyToDecrease;
}

void
BasicDeviceEnergyModel::ChangeState (int newState)
{
  NS_LOG_FUNCTION (this << newState);

  // energy depleted and newState != Off
  if (m_energyDepleted && newState != 0)
    {
      NS_LOG_INFO ("BasicDeviceEnergyModel::ChangeState: Cannot change into new state '"
                   << m_states->GetStateName (newState) << "' (" << newState
                   << "). Energy depleted!");
      return;
    }

  // Get duration until last update
  Time duration = Simulator::Now () - m_lastUpdateTime;
  NS_ASSERT (duration.IsPositive ()); // check if duration is valid

  // Energy to decrease = current * voltage * time
  double supplyVoltage = m_source->GetSupplyVoltage ();
  double energyToDecrease = (duration * GetCurrentA () * supplyVoltage).GetSeconds ();

  // Notify energy source
  m_source->UpdateEnergySource ();

  m_totalEnergyConsumption += energyToDecrease;

  m_lastUpdateTime = Simulator::Now ();

  // Check if the new state's current is negative
  if (m_states->GetState (newState).second < 0)
    {
      NS_LOG_WARN ("BasicDeviceEnergyModel::ChangeState: Switching into new state "
                   << m_states->GetStateName (newState) << "' (" << newState
                   << ") with current being negative: " << m_states->GetState (newState).second);
    }

    m_state = newState;

    NS_LOG_INFO ("BasicDeviceEnergyModel::ChangeState: '"
                 << m_states->GetStateName (m_state) << "' (" << m_state
                 << ") -> '" << m_states->GetStateName (newState) << "' (" << newState << ")");

  m_currentA = DoGetCurrentA ();
}

void
BasicDeviceEnergyModel::HandleEnergyDepletion (void)
{
  NS_LOG_FUNCTION (this);

  m_energyDepleted = true;
  ChangeState (0);

  m_energyDepletedCallbacks (Ptr<BasicDeviceEnergyModel> (this));
}

void
BasicDeviceEnergyModel::HandleEnergyRecharged (void)
{
  NS_LOG_FUNCTION (this);

  m_energyDepleted = false;
  ChangeState (m_defaultState);

  m_energyRechargedCallbacks (Ptr<BasicDeviceEnergyModel> (this));
}

void
BasicDeviceEnergyModel::HandleEnergyChanged (void)
{
  // Empty
}

void
BasicDeviceEnergyModel::ScheduleChangeState (Time delay, uint32_t state)
{
  NS_LOG_FUNCTION (this << delay << state);

  Simulator::Schedule (delay, &BasicDeviceEnergyModel::ChangeState, this, state);
}

void
BasicDeviceEnergyModel::RegisterEnergyDepletedCallback (EnergyDepletedCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);

  m_energyDepletedCallbacks.ConnectWithoutContext (cb);
}

void
BasicDeviceEnergyModel::RegisterEnergyRechargedCallback (EnergyRechargedCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);

  m_energyDepletedCallbacks.ConnectWithoutContext (cb);
}

// ##################################################################### //
// ############################## Private ############################## //
// ##################################################################### //

double
BasicDeviceEnergyModel::DoGetCurrentA (void) const
{
  NS_LOG_FUNCTION (this);

  return m_states->GetState (m_state).second;
}

void
BasicDeviceEnergyModel::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);

  m_state = m_defaultState;
}

} // namespace ns3