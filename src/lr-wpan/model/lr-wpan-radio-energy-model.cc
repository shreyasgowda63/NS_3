/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2022 TU Wien, Vienna
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
 * Author: Daniel Lukitsch <daniel.lukitsch96@gmail.com>
 */

#include "lr-wpan-radio-energy-model.h"
#include "lr-wpan-tx-current-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/pointer.h"
#include "ns3/energy-source.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LrWpanRadioEnergyModel");

NS_OBJECT_ENSURE_REGISTERED (LrWpanRadioEnergyModel);

TypeId
LrWpanRadioEnergyModel::GetTypeId (void)
{
  // Standard values taken from Nordic nrf52840 for +4dBm
  static TypeId tid = TypeId ("ns3::LrWpanRadioEnergyModel")
    .SetParent<DeviceEnergyModel> ()
    .SetGroupName ("Energy")
    .AddConstructor<LrWpanRadioEnergyModel> ()
    .AddAttribute ("TxOnCurrentA",
                   "The default Tx on idle current in Ampere.",
                   DoubleValue (0.0059),
                   MakeDoubleAccessor (&LrWpanRadioEnergyModel::SetTxOnCurrentA,
                                       &LrWpanRadioEnergyModel::GetTxOnCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RxOnCurrentA",
                   "The default Rx on idle current in Ampere.",
                   DoubleValue (0.0059),
                   MakeDoubleAccessor (&LrWpanRadioEnergyModel::SetRxOnCurrentA,
                                       &LrWpanRadioEnergyModel::GetRxOnCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxSendCurrentA",
                   "The radio TX current in Ampere.",
                   DoubleValue (0.0101),
                   MakeDoubleAccessor (&LrWpanRadioEnergyModel::SetTxCurrentA,
                                       &LrWpanRadioEnergyModel::GetTxCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RxReceiveCurrentA",
                   "The radio RX current in Ampere.",
                   DoubleValue (0.00875),
                   MakeDoubleAccessor (&LrWpanRadioEnergyModel::SetRxCurrentA,
                                       &LrWpanRadioEnergyModel::GetRxCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxCurrentModel", "A pointer to the attached TX current model.",
                   PointerValue (),
                   MakePointerAccessor (&LrWpanRadioEnergyModel::m_txCurrentModel),
                   MakePointerChecker<LrWpanTxCurrentModel> ())
    .AddTraceSource ("TotalEnergyConsumption",
                     "Total energy consumption of the radio device.",
                     MakeTraceSourceAccessor (&LrWpanRadioEnergyModel::m_totalEnergyConsumption),
                     "ns3::TracedValueCallback::Double")
    .AddTraceSource ("TotalEnergyDepleted",
                     "Total energy consumption of the radio device.",
                     MakeTraceSourceAccessor (&LrWpanRadioEnergyModel::m_totalEnergyDepleated),
                     "ns3::TracedValueCallback::Bool")
  ;
  return tid;
}

LrWpanRadioEnergyModel::LrWpanRadioEnergyModel ()
  : m_source (0),
    m_currentState (IEEE_802_15_4_PHY_RX_ON),
    m_lastUpdateTime (Seconds (0.0)),
    m_nPendingChangeState (0),
    m_totalEnergyConsumption (0),
    m_totalEnergyDepleated (false)
{
  NS_LOG_FUNCTION (this);
  m_energyDepletionCallback.Nullify ();
  // set callback for WifiPhy listener
  m_listener = new LrWpanRadioEnergyModelPhyListener;
  m_listener->SetChangeStateCallback (MakeCallback (&DeviceEnergyModel::ChangeState, this));
  // set callback for updating the TX current
  m_listener->SetUpdateTxCurrentCallback (MakeCallback (&LrWpanRadioEnergyModel::SetTxCurrentFromModel, this));
}

LrWpanRadioEnergyModel::~LrWpanRadioEnergyModel ()
{
  NS_LOG_FUNCTION (this);
  m_txCurrentModel = 0;
  delete m_listener;
}

void
LrWpanRadioEnergyModel::SetEnergySource (const Ptr<EnergySource> source)
{
  NS_LOG_FUNCTION (this << source);
  NS_ASSERT (source != NULL);
  m_source = source;
  m_switchToOffEvent.Cancel ();
  Time durationToOff = GetMaximumTimeInState (m_currentState);
  m_switchToOffEvent = Simulator::Schedule (durationToOff, &LrWpanRadioEnergyModel::ChangeState, this, IEEE_802_15_4_PHY_FORCE_TRX_OFF);
}

double
LrWpanRadioEnergyModel::GetTotalEnergyConsumption (void) const
{
  NS_LOG_FUNCTION (this);

  Time duration = Simulator::Now () - m_lastUpdateTime;
  NS_ASSERT (duration.IsPositive ()); // check if duration is valid

  // energy to decrease = current * voltage * time
  double supplyVoltage = m_source->GetSupplyVoltage ();
  double energyToDecrease = duration.GetSeconds () * GetStateA (m_currentState) * supplyVoltage;

  // notify energy source
  m_source->UpdateEnergySource ();

  return m_totalEnergyConsumption + energyToDecrease;
}

double
LrWpanRadioEnergyModel::GetTxOnCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_txOnCurrentA;
}

void
LrWpanRadioEnergyModel::SetTxOnCurrentA (double idleCurrentA)
{
  NS_LOG_FUNCTION (this << idleCurrentA);
  m_txOnCurrentA = idleCurrentA;
}

double
LrWpanRadioEnergyModel::GetRxOnCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_rxOnCurrentA;
}

void
LrWpanRadioEnergyModel::SetRxOnCurrentA (double CcaBusyCurrentA)
{
  NS_LOG_FUNCTION (this << CcaBusyCurrentA);
  m_rxOnCurrentA = CcaBusyCurrentA;
}

double
LrWpanRadioEnergyModel::GetTxCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_txCurrentA;
}

void
LrWpanRadioEnergyModel::SetTxCurrentA (double txCurrentA)
{
  NS_LOG_FUNCTION (this << txCurrentA);
  m_txCurrentA = txCurrentA;
}

double
LrWpanRadioEnergyModel::GetRxCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_rxCurrentA;
}

void
LrWpanRadioEnergyModel::SetRxCurrentA (double rxCurrentA)
{
  NS_LOG_FUNCTION (this << rxCurrentA);
  m_rxCurrentA = rxCurrentA;
}


int
LrWpanRadioEnergyModel::GetCurrentState (void) const
{
  NS_LOG_FUNCTION (this);
  return m_currentState;
}

void
LrWpanRadioEnergyModel::SetEnergyDepletionCallback (
  LrWpanRadioEnergyDepletionCallback callback)
{
  NS_LOG_FUNCTION (this);
  if (callback.IsNull ())
    {
      NS_LOG_DEBUG ("LrWpanRadioEnergyModel:Setting NULL energy depletion callback!");
    }
  m_energyDepletionCallback = callback;
}

void
LrWpanRadioEnergyModel::SetEnergyRechargedCallback (
  LrWpanRadioEnergyRechargedCallback callback)
{
  NS_LOG_FUNCTION (this);
  if (callback.IsNull ())
    {
      NS_LOG_DEBUG ("LrWpanRadioEnergyModel:Setting NULL energy recharged callback!");
    }
  m_energyRechargedCallback = callback;
}

void
LrWpanRadioEnergyModel::SetTxCurrentModel (const Ptr<LrWpanTxCurrentModel> model)
{
  m_txCurrentModel = model;
}

void
LrWpanRadioEnergyModel::SetTxCurrentFromModel (double txPowerDbm)
{
  if (m_txCurrentModel)
    {
      m_txCurrentA = m_txCurrentModel->CalcTxCurrent (txPowerDbm);
    }
}

Time
LrWpanRadioEnergyModel::GetMaximumTimeInState (int state) const
{
  if (state == IEEE_802_15_4_PHY_FORCE_TRX_OFF)
    {
      NS_FATAL_ERROR ("Requested maximum remaining time for OFF state");
    }
  double remainingEnergy = m_source->GetRemainingEnergy ();
  double supplyVoltage = m_source->GetSupplyVoltage ();
  double current = GetStateA (state);

  double time = remainingEnergy / (current * supplyVoltage);

  return Seconds (time);
}

void
LrWpanRadioEnergyModel::ChangeState (int newState)
{
  NS_LOG_FUNCTION (this << newState);

  if (m_currentState == IEEE_802_15_4_PHY_FORCE_TRX_OFF && m_currentState == newState)
    {
      return;
    }

  m_nPendingChangeState++;

  if (m_nPendingChangeState > 1 && newState == IEEE_802_15_4_PHY_FORCE_TRX_OFF)
    {
      SetLrWpanRadioState (newState);
      m_nPendingChangeState--;
      EnergyDepletionEventReceived ();
      return;
    }

  m_lastUpdateTime = Simulator::Now ();

  // notify energy source
  m_source->UpdateEnergySource ();

  // use the calculated energy from the energy-source instead of calculate a separate one that does not match with the actual energy-source
  m_totalEnergyConsumption = m_source->GetInitialEnergy () - m_source->GetRemainingEnergy ();

  NS_ASSERT (m_totalEnergyConsumption <= m_source->GetInitialEnergy ());

  if (newState != IEEE_802_15_4_PHY_FORCE_TRX_OFF)
    {
      m_switchToOffEvent.Cancel ();
      Time durationToOff = GetMaximumTimeInState (newState);
      m_switchToOffEvent = Simulator::Schedule (durationToOff, &LrWpanRadioEnergyModel::ChangeState, this, IEEE_802_15_4_PHY_FORCE_TRX_OFF);
    }

  // in case the energy source is found to be depleted during the last update, a callback might be
  // invoked that might cause a change in the Wifi PHY state (e.g., the PHY is put into SLEEP mode).
  // This in turn causes a new call to this member function, with the consequence that the previous
  // instance is resumed after the termination of the new instance. In particular, the state set
  // by the previous instance is erroneously the final state stored in m_currentState. The check below
  // ensures that previous instances do not change m_currentState.

  if (m_nPendingChangeState <= 1)
    {
      if (newState == IEEE_802_15_4_PHY_FORCE_TRX_OFF)
        {
          EnergyDepletionEventReceived ();
        }

      // update current state & last update time stamp
      SetLrWpanRadioState (newState);

      // some debug message
      NS_LOG_DEBUG ("LrWpanRadioEnergyModel:Total energy consumption is " <<
                    m_totalEnergyConsumption << "J");
    }
  m_nPendingChangeState--;
}

void
LrWpanRadioEnergyModel::HandleEnergyDepletion (void)
{}

void
LrWpanRadioEnergyModel::EnergyDepletionEventReceived (void)
{
  m_totalEnergyDepleated = true;


  if (!m_energyDepletionCallback.IsNull ())
    {
      m_energyDepletionCallback ();
    }
}

void
LrWpanRadioEnergyModel::HandleEnergyRecharged (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("LrWpanRadioEnergyModel:Energy is recharged!");
  // invoke energy recharged callback, if set.
  if (!m_energyRechargedCallback.IsNull ())
    {
      m_energyRechargedCallback ();
    }
}

void
LrWpanRadioEnergyModel::HandleEnergyChanged (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("LrWpanRadioEnergyModel:Energy is changed!");
  if (m_currentState != IEEE_802_15_4_PHY_FORCE_TRX_OFF)
    {
      m_switchToOffEvent.Cancel ();
      Time durationToOff = GetMaximumTimeInState (m_currentState);
      m_switchToOffEvent = Simulator::Schedule (durationToOff, &LrWpanRadioEnergyModel::ChangeState, this, IEEE_802_15_4_PHY_FORCE_TRX_OFF);
    }
}

LrWpanRadioEnergyModelPhyListener *
LrWpanRadioEnergyModel::GetPhyListener (void)
{
  NS_LOG_FUNCTION (this);
  return m_listener;
}

/*
 * Private functions start here.
 */

void
LrWpanRadioEnergyModel::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_source = NULL;
  m_energyDepletionCallback.Nullify ();
}

double
LrWpanRadioEnergyModel::GetStateA (int state) const
{
  // get the correct values for the states in "LrWpanPhyEnumeration"
  switch (state)
    {
      case 0x01: //IEEE_802_15_4_PHY_BUSY_RX:
        return m_rxCurrentA;
      case 0x02: //IEEE_802_15_4_PHY_BUSY_TX:
        return m_txCurrentA;
      case 0x06: //IEEE_802_15_4_PHY_RX_ON:
        return m_rxOnCurrentA;
      case 0x09: //IEEE_802_15_4_PHY_TX_ON:
        return m_txOnCurrentA;
      default:
        return 0.0;
    }
  NS_FATAL_ERROR ("LrWpanRadioEnergyModel: undefined radio state " << state);
}

double
LrWpanRadioEnergyModel::DoGetCurrentA (void) const
{
  return GetStateA (m_currentState);
}

void
LrWpanRadioEnergyModel::SetLrWpanRadioState (const int state)
{
  NS_LOG_FUNCTION (this << state);
  m_currentState = state;
  std::string stateName;

  std::string stateNames[] = { "IEEE_802_15_4_PHY_BUSY",
                               "IEEE_802_15_4_PHY_BUSY_RX",
                               "IEEE_802_15_4_PHY_BUSY_TX",
                               "IEEE_802_15_4_PHY_FORCE_TRX_OFF",
                               "IEEE_802_15_4_PHY_IDLE",
                               "IEEE_802_15_4_PHY_INVALID_PARAMETER",
                               "IEEE_802_15_4_PHY_RX_ON",
                               "IEEE_802_15_4_PHY_SUCCESS",
                               "IEEE_802_15_4_PHY_TRX_OFF",
                               "IEEE_802_15_4_PHY_TX_ON",
                               "IEEE_802_15_4_PHY_UNSUPPORTED_ATTRIBUTE",
                               "IEEE_802_15_4_PHY_READ_ONLY",
                               "IEEE_802_15_4_PHY_UNSPECIFIED"};

  stateName = stateNames[state];

  NS_LOG_DEBUG ("LrWpanRadioEnergyModel:Switching to state: " << stateName <<
                " at time = " << Simulator::Now ());
}

// -------------------------------------------------------------------------- //

LrWpanRadioEnergyModelPhyListener::LrWpanRadioEnergyModelPhyListener ()
{
  NS_LOG_FUNCTION (this);
  m_changeStateCallback.Nullify ();
  m_updateTxCurrentCallback.Nullify ();
}

LrWpanRadioEnergyModelPhyListener::~LrWpanRadioEnergyModelPhyListener ()
{
  NS_LOG_FUNCTION (this);
}

void
LrWpanRadioEnergyModelPhyListener::SetChangeStateCallback (DeviceEnergyModel::ChangeStateCallback callback)
{
  NS_LOG_FUNCTION (this << &callback);
  NS_ASSERT (!callback.IsNull ());
  m_changeStateCallback = callback;
}

void
LrWpanRadioEnergyModelPhyListener::SetUpdateTxCurrentCallback (UpdateTxCurrentCallback callback)
{
  NS_LOG_FUNCTION (this << &callback);
  NS_ASSERT (!callback.IsNull ());
  m_updateTxCurrentCallback = callback;
}

void
LrWpanRadioEnergyModelPhyListener::NotifyRxStart (Time duration)
{
  NS_LOG_FUNCTION (this << duration);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("LrWpanRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (IEEE_802_15_4_PHY_BUSY_RX);
  m_switchToIdleEvent.Cancel ();
  m_switchToIdleEvent = Simulator::Schedule (duration, &LrWpanRadioEnergyModelPhyListener::NotifyRxOn, this);
}

void
LrWpanRadioEnergyModelPhyListener::NotifyRxEndOk (void)
{
  NS_LOG_FUNCTION (this);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("LrWpanRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (IEEE_802_15_4_PHY_RX_ON);
  m_switchToIdleEvent.Cancel ();
}

void
LrWpanRadioEnergyModelPhyListener::NotifyRxEndError (void)
{
  NS_LOG_FUNCTION (this);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("LrWpanRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (IEEE_802_15_4_PHY_RX_ON);
  m_switchToIdleEvent.Cancel ();
}

void
LrWpanRadioEnergyModelPhyListener::NotifyTxStart (Time duration, double txPowerDbm)
{
  NS_LOG_FUNCTION (this << duration << txPowerDbm);
  if (m_updateTxCurrentCallback.IsNull ())
    {
      NS_FATAL_ERROR ("LrWpanRadioEnergyModelPhyListener:Update tx current callback not set!");
    }
  m_updateTxCurrentCallback (txPowerDbm);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("LrWpanRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (IEEE_802_15_4_PHY_BUSY_TX);
  // schedule changing state back to IDLE after TX duration
  m_switchToIdleEvent.Cancel ();
  m_switchToIdleEvent = Simulator::Schedule (duration, &LrWpanRadioEnergyModelPhyListener::NotifyRxOn, this);
}

void
LrWpanRadioEnergyModelPhyListener::NotifyTxOn (void)
{
  NS_LOG_FUNCTION (this);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("LrWpanRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (IEEE_802_15_4_PHY_TX_ON);
  m_switchToIdleEvent.Cancel ();
}

void
LrWpanRadioEnergyModelPhyListener::NotifyTxOffRxOff (void)
{
  NS_LOG_FUNCTION (this);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("LrWpanRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (IEEE_802_15_4_PHY_TRX_OFF);
  m_switchToIdleEvent.Cancel ();
}

void
LrWpanRadioEnergyModelPhyListener::NotifyTxOffRxOffByForce (void)
{
  NS_LOG_FUNCTION (this);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("LrWpanRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (IEEE_802_15_4_PHY_FORCE_TRX_OFF);
  m_switchToIdleEvent.Cancel ();
}

void
LrWpanRadioEnergyModelPhyListener::NotifyRxOn (void)
{
  NS_LOG_FUNCTION (this);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("LrWpanRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (IEEE_802_15_4_PHY_RX_ON);
  m_switchToIdleEvent.Cancel ();
}

} // namespace ns3
