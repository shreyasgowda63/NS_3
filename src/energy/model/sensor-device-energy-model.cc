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

#include "sensor-device-energy-model.h"
#include <ns3/energy-source.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SensorDeviceEnergyModel");

NS_OBJECT_ENSURE_REGISTERED (SensorDeviceEnergyModel);

SensorDeviceEnergyModel::SensorDeviceEnergyModel ()
{
  NS_LOG_FUNCTION (this);
  m_lastUpdateTime = Simulator::Now ();
  m_activityIdCounter = 0;
  m_runningActivity = 0;
  m_energyDepleted = false;
  m_strictEnergyChecking = true;
  m_source = nullptr;
  m_node = nullptr;
  m_totalEnergyConsumption = 0;
}

TypeId
SensorDeviceEnergyModel::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::SensorDeviceEnergyModel")
          .SetParent<DeviceEnergyModel> ()
          .SetGroupName ("Energy")
          .AddConstructor<SensorDeviceEnergyModel> ()
          .AddAttribute ("MeasureCurrentA", "Current Ampere for the Measure state.",
                         DoubleValue (1.0),
                         MakeDoubleAccessor (&SensorDeviceEnergyModel::GetMeasureCurrentA,
                                             &SensorDeviceEnergyModel::SetMeasureCurrentA),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("IdleCurrentA", "Current Ampere for the Idle state.", DoubleValue (0.3),
                         MakeDoubleAccessor (&SensorDeviceEnergyModel::GetIdleCurrentA,
                                             &SensorDeviceEnergyModel::SetIdleCurrentA),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("SleepCurrentA", "Current Ampere for the Sleep state.", DoubleValue (0.1),
                         MakeDoubleAccessor (&SensorDeviceEnergyModel::GetSleepCurrentA,
                                             &SensorDeviceEnergyModel::SetSleepCurrentA),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("DefaultState", "The default state to use at the beginning and when using Default.",
                         EnumValue (SensorModelState::Idle),
                         MakeEnumAccessor (&SensorDeviceEnergyModel::m_defaultState),
                         MakeEnumChecker (SensorModelState::Measure, "Measure",
                                          SensorModelState::Idle, "Idle",
                                          SensorModelState::Sleep, "Sleep",
                                          SensorModelState::Off, "Off"))
          .AddAttribute ("StrictEnergyChecking", "How energy depletions at the end of measurement activities should be handled. "
                         "See SensorDeviceEnergyModel::m_strictEnergyChecking for more info.",
                         BooleanValue (true),
                         MakeBooleanAccessor (&SensorDeviceEnergyModel::m_strictEnergyChecking),
                         MakeBooleanChecker ())
          .AddTraceSource ("TotalEnergyConsumption", "Total energy consumption of the sensor device.",
                           MakeTraceSourceAccessor (&SensorDeviceEnergyModel::m_totalEnergyConsumption),
                           "ns3::TracedValueCallback::Double")
          .AddTraceSource ("State", "The SensorModelState of this model.",
                           MakeTraceSourceAccessor (&SensorDeviceEnergyModel::m_state),
                           "ns3::TracedValueCallback::SensorDeviceEnergyModel")
          .AddTraceSource ("CurrentA", "The SensorModelState's current in Ampere.",
                           MakeTraceSourceAccessor (&SensorDeviceEnergyModel::m_currentA),
                           "ns3::TracedValueCallback::Double");
  return tid;
}

void
SensorDeviceEnergyModel::SetEnergySource (Ptr<EnergySource> source)
{
  NS_LOG_FUNCTION (this << source);
  m_source = source;
}

double
SensorDeviceEnergyModel::GetTotalEnergyConsumption (void) const
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
SensorDeviceEnergyModel::ChangeState (int newState)
{
  NS_LOG_FUNCTION (this << newState);

  bool isDepleted = m_energyDepleted;

  SensorModelState state = SensorModelState (newState);

  if (m_energyDepleted)
    {
      if (state != Off)
        {
          NS_LOG_DEBUG ("SensorDeviceEnergyModel: Tried to change into a new state other than Off "
                        "while the energy is depleted.");
        }
      m_state = Off;
      m_currentA = DoGetCurrentA ();
      return;
    }

  // Must not be Previous
  NS_ASSERT_MSG (
      state != Previous,
      "SensorDeviceEnergyModel: Calling ChangeState with newState == Previous is not allowed.");

  // If Default: Replace with m_defaultState
  if (state == Default)
    {
      state = m_defaultState;
    }

  // Must be one of the following valid states
  NS_ASSERT_MSG (state == Measure || state == Idle || state == Sleep || state == Off,
                 "This newState is not supported: " << newState);

  // Get duration until last update
  Time duration = Simulator::Now () - m_lastUpdateTime;
  NS_ASSERT (duration.IsPositive ()); // check if duration is valid

  // Energy to decrease = current * voltage * time
  double supplyVoltage = m_source->GetSupplyVoltage ();
  double energyToDecrease = (duration * GetCurrentA () * supplyVoltage).GetSeconds ();

  // Update total energy consumption
  m_totalEnergyConsumption += energyToDecrease;

  // Update last update time stamp
  m_lastUpdateTime = Simulator::Now ();

  // Notify energy source
  m_source->UpdateEnergySource ();

  // Check if we *just now* ran out of energy
  if (m_energyDepleted && m_energyDepleted != isDepleted)
    {
      NS_LOG_DEBUG (
          "SensorDeviceEnergyModel: The energy got depleted while ChangeState updated the lastly "
          "consumed energy. This could mean that we should have switched to Off earlier.");
      // Switch state to off and return to prevent any other state changes
      m_state =
          Off; // This line, however, should be unnecessary because this should already have happened in the call to HandleEnergyDepletion
      m_currentA = DoGetCurrentA ();
      return;
    }

  // Manual changes while a measurement activity is running
  if (m_runningActivity != 0 && m_state == Measure)
    {
      switch (state)
        {
        case Measure:
          NS_LOG_DEBUG (
              "SensorDeviceEnergyModel: Manually switching into the Measure state while a "
              "measurement activity is running! This does not stop the current activity "
              "from changing the state after it is finished!");
          return; // This prevents unnecessary updates to m_switchToOffEvent
          break;

        case Idle:
        case Sleep:
        case Off:
          NS_LOG_DEBUG (
              "SensorDeviceEnergyModel: Manually switching into a non-Measure state while a "
              "measurement activity is running! This may be a mistake.");
          break;

        default:
          NS_ABORT_MSG ("SensorDeviceEnergyModel: This case should never be reached!");
          break;
        }
    }

  NS_LOG_INFO ("SensorDeviceEnergyModel: Switching from state " << m_state << " into " << state);

  if (m_state != state)
    {
      m_state = state;
    }
  m_currentA = DoGetCurrentA ();
}

void
SensorDeviceEnergyModel::HandleEnergyDepletion (void)
{
  NS_LOG_FUNCTION (this);

  m_energyDepleted = true;
  ChangeState (Off);

  m_energyDepletedCallbacks (Ptr<SensorDeviceEnergyModel> (this));
}

void
SensorDeviceEnergyModel::HandleEnergyRecharged (void)
{
  NS_LOG_FUNCTION (this);

  m_energyDepleted = false;
  ChangeState (m_defaultState);

  m_energyRechargedCallbacks (Ptr<SensorDeviceEnergyModel> (this));
}

void
SensorDeviceEnergyModel::HandleEnergyChanged (void)
{
  // Empty
}

SensorDeviceEnergyModel::Id
SensorDeviceEnergyModel::ScheduleMeasure (Time start, Time end, SensorModelState afterState)
{
  NS_LOG_FUNCTION (this << start << end << afterState);

  // Increase running counter for ids
  m_activityIdCounter++;

  // Replace Default placeholder
  if (afterState == Default)
    {
      afterState = m_defaultState;
    }

  // Schedule the start of the measurement activity
  Simulator::Schedule (start, &SensorDeviceEnergyModel::DoStartMeasure, this,
                       Id (m_activityIdCounter), end, afterState);

  return m_activityIdCounter;
}

SensorDeviceEnergyModel::Id
SensorDeviceEnergyModel::MeasureNow (Time end, SensorModelState afterState)
{
  NS_LOG_FUNCTION (this << end << afterState);

  return ScheduleMeasure (Seconds (0.0), end, afterState);
}

void
SensorDeviceEnergyModel::RegisterMeasurementStartCallback (MeasurementStartCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);

  m_measurementStartCallbacks.ConnectWithoutContext (cb);
}

void
SensorDeviceEnergyModel::RegisterMeasurementEndCallback (MeasurementEndCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);

  m_measurementEndCallbacks.ConnectWithoutContext (cb);
}

void
SensorDeviceEnergyModel::RegisterEnergyDepletedCallback (EnergyDepletedCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);

  m_energyDepletedCallbacks.ConnectWithoutContext (cb);
}

void
SensorDeviceEnergyModel::RegisterEnergyRechargedCallback (EnergyRechargedCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);

  m_energyDepletedCallbacks.ConnectWithoutContext (cb);
}

// ====================================================== //
// ================== Getters & Setters ================= //
// ====================================================== //

Ptr<Node>
SensorDeviceEnergyModel::GetNode () const
{
  NS_LOG_FUNCTION (this);
  return m_node;
}

void
SensorDeviceEnergyModel::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  m_node = node;
}

double
SensorDeviceEnergyModel::GetMeasureCurrentA () const
{

  NS_LOG_FUNCTION (this);
  return m_measureCurrentA;
}

void
SensorDeviceEnergyModel::SetMeasureCurrentA (double current)
{
  NS_LOG_FUNCTION (this << current);
  m_measureCurrentA = current;
}

double
SensorDeviceEnergyModel::GetIdleCurrentA () const
{

  NS_LOG_FUNCTION (this);
  return m_idleCurrentA;
}

void
SensorDeviceEnergyModel::SetIdleCurrentA (double current)
{
  NS_LOG_FUNCTION (this << current);
  m_idleCurrentA = current;
}

double
SensorDeviceEnergyModel::GetSleepCurrentA () const
{
  NS_LOG_FUNCTION (this);
  return m_sleepCurrentA;
}

void
SensorDeviceEnergyModel::SetSleepCurrentA (double current)
{
  NS_LOG_FUNCTION (this << current);
  m_sleepCurrentA = current;
}

SensorDeviceEnergyModel::SensorModelState
SensorDeviceEnergyModel::GetDefaultState () const
{
  NS_LOG_FUNCTION (this);
  return m_defaultState;
}

void
SensorDeviceEnergyModel::SetDefaultState (SensorModelState state)
{
  NS_LOG_FUNCTION (this << state);
  m_defaultState = state;
}

// ##################################################################### //
// ############################## Private ############################## //
// ##################################################################### //

double
SensorDeviceEnergyModel::DoGetCurrentA (void) const
{
  NS_LOG_FUNCTION (this);

  switch (m_state)
    {
    case Measure:
      return m_measureCurrentA;
      break;

    case Idle:
      return m_idleCurrentA;
      break;

    case Sleep:
      return m_sleepCurrentA;
      break;

    case Off:
      return 0.0;
      break;

    default:
      NS_ABORT_MSG ("SensorDeviceEnergyModel: Invalid m_state:" << m_state);
      return 0.0;
      break;
    }
}

void
SensorDeviceEnergyModel::DoStartMeasure (Id id, Time end, SensorModelState afterState)
{
  NS_LOG_FUNCTION (this << id << end << afterState);

  if (afterState == Previous)
    {
      afterState = m_state;
    }

  ErrorCode code = Unknown;

  if (m_runningActivity != 0) // Another measurement activity is already running
    {
      code = Overlap;
    }
  else if (m_energyDepleted)
    {
      code = EnergyDepleted;
    }
  else
    {
      ChangeState (Measure);
      if (m_state == Measure) // Check if state change was successful
        {
          code = Success;
          m_runningActivity = id;
          Simulator::Schedule (end, &SensorDeviceEnergyModel::DoEndMeasure, this, id, afterState);
        }
      else if (m_state == Off)
        {
          code = EnergyDepleted;
        }
    }

  m_measurementStartCallbacks (Ptr<SensorDeviceEnergyModel> (this), id, code);
}

void
SensorDeviceEnergyModel::DoEndMeasure (Id id, SensorModelState afterState)
{
  NS_LOG_FUNCTION (this << id << afterState);

  ErrorCode code = Unknown;

  NS_ASSERT_MSG (
      m_runningActivity == id,
      "SensorDeviceEnergyModel: The currently running activity (m_runningActivity = "
          << m_runningActivity
          << ") is not the same as the one that is supposed to be stopped right now (id = " << id
          << ").");

  if (m_energyDepleted)
    {
      code = EnergyDepleted;
    }
  else if (m_state !=
      Measure) // The measurement got interrupted by something (for example HandleEnergyDepletion)
    {
      NS_LOG_DEBUG ("SensorDeviceEnergyModel: The measurement " << id << " got interrupted.");
      code = Unknown;
    }
  else
    {
      m_runningActivity = 0;
      ChangeState (afterState);
      // TODO: If the energy is depleted *just in* the moment when the measurement activity is over, it is still labeled as EnergyDepleted
      // This is, however, only the case, if m_strictEnergyChecking is set to true
      if (m_state == afterState)
        {
          code = Success;
        }
      else if (m_energyDepleted)
        {
          if (m_strictEnergyChecking)
            {
              code = EnergyDepleted;
            }
          else
            {
              code = Success;
            }
        }
      else
        {
          code = Unknown;
        }
    }
  m_measurementEndCallbacks (Ptr<SensorDeviceEnergyModel> (this), id, code);
}

void
SensorDeviceEnergyModel::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  m_state = m_defaultState;
  DeviceEnergyModel::DoInitialize ();
}

} // namespace ns3