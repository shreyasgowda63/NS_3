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

#include "dynamic-device-energy-model.h"
#include <ns3/energy-source.h>

namespace ns3 {

// ##################################################################### //
// ###################### DynamicEnergyModelStates ##################### //
// ##################################################################### //

NS_LOG_COMPONENT_DEFINE ("DynamicDeviceEnergyModel");

DynamicEnergyModelStates::DynamicEnergyModelStates ()
{
  NS_LOG_FUNCTION (this);
  m_indexCounter = 0;

  uint32_t offIndex = AddState ("Off", 0.0); // Always added

  NS_ASSERT (offIndex == 0);
}

TypeId
DynamicEnergyModelStates::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DynamicEnergyModelStates")
                          .SetParent<Object> ()
                          .SetGroupName ("Energy")
                          .AddConstructor<DynamicEnergyModelStates> ();
  return tid;
}

uint32_t
DynamicEnergyModelStates::AddState (State state)
{
  NS_LOG_FUNCTION (this << &state << state.first << state.second);

  m_states.insert (std::pair<uint32_t, State> (m_indexCounter, state));

  return m_indexCounter++;
}

uint32_t
DynamicEnergyModelStates::AddState (std::string name, double currentA)
{
  NS_LOG_FUNCTION (this << name << currentA);

  return AddState (State (name, currentA));
}

void
DynamicEnergyModelStates::RemoveState (uint32_t index)
{
  NS_LOG_FUNCTION (this << index);

  if (m_states.find (index) != m_states.end ())
    {
      // Index exists
      m_states.erase (index);
    }
  else
    {
      // Index does not exist
      NS_LOG_WARN ("DynamicEnergyModelStates::RemoveState: Index did not exists: " << index << ".");
    }
}

double
DynamicEnergyModelStates::GetStateA (uint32_t index) const
{
  NS_LOG_FUNCTION (this << index);

  std::map<uint32_t, State>::const_iterator i = m_states.find (index);
  if (i != m_states.end ())
    {
      // Index exists
      return i->second.second;
    }
  else
    {
      // Index does not exist
      NS_LOG_WARN ("DynamicEnergyModelStates::GetStateA: Index did not exists: " << index << ".");
      return 0.0;
    }
}

std::string
DynamicEnergyModelStates::GetStateName (uint32_t index) const
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
      NS_LOG_WARN ("DynamicEnergyModelStates::GetStateName: Index did not exists: " << index
                                                                                    << ".");
      return "Not found";
    }
}

DynamicEnergyModelStates::State
DynamicEnergyModelStates::GetState (uint32_t index) const
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
      NS_LOG_WARN ("DynamicEnergyModelStates::GetState: Index did not exists: " << index << ".");
      return State ("Not found", 0.0);
    }
}

void
DynamicEnergyModelStates::SetCurrent (uint32_t index, double currentA)
{
  NS_LOG_FUNCTION (this << index << currentA);

  std::map<uint32_t, State>::iterator i = m_states.find (index);
  if (i != m_states.end ())
    {
      // Index exists
      i->second.second = currentA;
    }
  else
    {
      // Index does not exist
      NS_LOG_WARN ("DynamicEnergyModelStates::SetCurrent: Index did not exists: " << index << ".");
    }
}

// ##################################################################### //
// ###################### DynamicDeviceEnergyModel ##################### //
// ##################################################################### //

NS_OBJECT_ENSURE_REGISTERED (DynamicDeviceEnergyModel);

DynamicDeviceEnergyModel::DynamicDeviceEnergyModel ()
{
  NS_LOG_FUNCTION (this);
  m_state = 0;
  m_energyDepleted = false;
  m_lastUpdateTime = Simulator::Now();
  m_totalEnergyConsumption = 0.0;
  m_defaultState = 0;
}

TypeId
DynamicDeviceEnergyModel::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::DynamicDeviceEnergyModel")
          .SetParent<DeviceEnergyModel> ()
          .SetGroupName ("Energy")
          .AddConstructor<DynamicDeviceEnergyModel> ()
          .AddAttribute ("DynamicEnergyModelStates",
                         "The DynamicEnergyModelStates object to assign to this model.",
                         PointerValue (), MakePointerAccessor (&DynamicDeviceEnergyModel::m_states),
                         MakePointerChecker<DynamicEnergyModelStates> ())
          .AddAttribute (
              "DefaultState", "Default state for beginning and after energy is recharged.",
              UintegerValue (0), MakeUintegerAccessor (&DynamicDeviceEnergyModel::m_defaultState),
              MakeUintegerChecker<uint32_t> (0, UINT32_MAX))
          .AddTraceSource (
              "TotalEnergyConsumption", "Total energy consumption of the sensor device.",
              MakeTraceSourceAccessor (&DynamicDeviceEnergyModel::m_totalEnergyConsumption),
              "ns3::TracedValueCallback::Double")
          .AddTraceSource ("State", "The state of this model.",
                           MakeTraceSourceAccessor (&DynamicDeviceEnergyModel::m_state),
                           "ns3::TracedValueCallback::UInt32")
          .AddTraceSource ("CurrentA", "The current in A of this model.",
                           MakeTraceSourceAccessor (&DynamicDeviceEnergyModel::m_currentA),
                           "ns3::TracedValueCallback::Double");
  return tid;
}

void
DynamicDeviceEnergyModel::SetEnergySource (Ptr<EnergySource> source)
{
  NS_LOG_FUNCTION (this << source);
  m_source = source;
}

double
DynamicDeviceEnergyModel::GetTotalEnergyConsumption (void) const
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
DynamicDeviceEnergyModel::ChangeState (int newState)
{
  NS_LOG_FUNCTION (this << newState);

  // energy depleted and newState != Off
  if (m_energyDepleted && newState != 0)
    {
      NS_LOG_INFO ("DynamicDeviceEnergyModel::ChangeState: Cannot change into new state '"
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

  if (m_states->GetStateA (newState) < 0)
    {
      NS_LOG_WARN ("DynamicDeviceEnergyModel::ChangeState: Switching into new state "
                   << m_states->GetStateName (newState) << "' (" << newState
                   << ") with current being negative: " << m_states->GetStateA (newState));
    }

    m_state = newState;

    NS_LOG_INFO ("DynamicDeviceEnergyModel::ChangeState: '"
                 << m_states->GetStateName (m_state) << "' (" << m_state
                 << ") -> '" << m_states->GetStateName (newState) << "' (" << newState << ")");

  m_currentA = DoGetCurrentA ();
}

void
DynamicDeviceEnergyModel::HandleEnergyDepletion (void)
{
  NS_LOG_FUNCTION (this);

  m_energyDepleted = true;
  ChangeState (0);

  m_energyDepletedCallbacks (Ptr<DynamicDeviceEnergyModel> (this));
}

void
DynamicDeviceEnergyModel::HandleEnergyRecharged (void)
{
  NS_LOG_FUNCTION (this);

  m_energyDepleted = false;
  ChangeState (m_defaultState);

  m_energyRechargedCallbacks (Ptr<DynamicDeviceEnergyModel> (this));
}

void
DynamicDeviceEnergyModel::HandleEnergyChanged (void)
{
  // Empty
}

void
DynamicDeviceEnergyModel::ScheduleChangeState (Time delay, uint32_t state)
{
  NS_LOG_FUNCTION (this << delay << state);

  Simulator::Schedule (delay, &DynamicDeviceEnergyModel::ChangeState, this, state);
}

void
DynamicDeviceEnergyModel::RegisterEnergyDepletedCallback (EnergyDepletedCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);

  m_energyDepletedCallbacks.ConnectWithoutContext (cb);
}

void
DynamicDeviceEnergyModel::RegisterEnergyRechargedCallback (EnergyRechargedCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);

  m_energyDepletedCallbacks.ConnectWithoutContext (cb);
}

// ====================================================== //
// ================== Getters & Setters ================= //
// ====================================================== //

Ptr<Node>
DynamicDeviceEnergyModel::GetNode () const
{
  NS_LOG_FUNCTION (this);

  return m_node;
}

void
DynamicDeviceEnergyModel::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);

  m_node = node;
}

// ##################################################################### //
// ############################## Private ############################## //
// ##################################################################### //

double
DynamicDeviceEnergyModel::DoGetCurrentA (void) const
{
  NS_LOG_FUNCTION (this);

  return m_states->GetStateA (m_state);
}

void
DynamicDeviceEnergyModel::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);

  m_state = m_defaultState;
}

} // namespace ns3