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

#ifndef LR_WPAN_RADIO_ENERGY_MODEL_H
#define LR_WPAN_RADIO_ENERGY_MODEL_H

#include "ns3/device-energy-model.h"
#include "ns3/traced-value.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/nstime.h"
#include "lr-wpan-phy-listener.h"
#include "lr-wpan-tx-current-model.h"
#include "lr-wpan-phy.h"

namespace ns3 {

/**
 * \ingroup energy
 * A WifiPhy listener class for notifying the LrWpanRadioEnergyModel of Wifi radio
 * state change.
 *
 */

class LrWpanRadioEnergyModelPhyListener : public LrWpanPhyListener
{
public:
  /**
   * Callback type for updating the transmit current based on the nominal TX power.
   */
  typedef Callback<void, double> UpdateTxCurrentCallback;

  LrWpanRadioEnergyModelPhyListener ();
  virtual ~LrWpanRadioEnergyModelPhyListener ();

  /**
   * \brief Sets the change state callback. Used by helper class.
   *
   * \param callback Change state callback.
   */
  void SetChangeStateCallback (DeviceEnergyModel::ChangeStateCallback callback);

  /**
   * \brief Sets the update TX current callback.
   *
   * \param callback Update TX current callback.
   */
  void SetUpdateTxCurrentCallback (UpdateTxCurrentCallback callback);

  /**
   * \brief Register reception start.
   *
   * \param duration not used
   */
  void NotifyRxStart (Time duration) override;
  /**
   * \brief Register end of successful reception.
   *
   */
  void NotifyRxEndOk (void) override;
  /**
   * \brief Register end of unsuccessful reception.
   *
   */
  void NotifyRxEndError (void) override;
  /**
   * \brief Register transmission start.
   *
   * \param duration when to swtich bach to idle
   * \param txPowerDbm the used transmit power (gain)
   */
  void NotifyTxStart (Time duration, double txPowerDbm) override;
  /**
   * \brief Register transmit on
   *
   */
  void NotifyTxOn (void) override;
  /**
   * \brief Register transmit and receive off
   *
   */
  void NotifyTxOffRxOff (void) override;
  void NotifyTxOffRxOffByForce (void) override;
  /**
   * \brief Register receive on
   *
   */
  void NotifyRxOn (void) override;

private:
  /**
   * A helper function that makes scheduling m_changeStateCallback possible.
   */
  void SwitchToRxOn (void);

  /**
   * Change state callback used to notify the LrWpanRadioEnergyModel of a state
   * change.
   */
  DeviceEnergyModel::ChangeStateCallback m_changeStateCallback;

  /**
   * Callback used to update the TX current stored in LrWpanRadioEnergyModel based on
   * the nominal TX power used to transmit the current frame.
   */
  UpdateTxCurrentCallback m_updateTxCurrentCallback;

  EventId m_switchToIdleEvent; ///< switch to idle event
};


/**
 * \ingroup energy
 * \brief A WiFi radio energy model.
 *
 * 4 states are defined for the radio: TX, RX, IDLE, SLEEP. Default state is
 * IDLE.
 * The different types of transactions that are defined are:
 *  1. Tx: State goes from IDLE to TX, radio is in TX state for TX_duration,
 *     then state goes from TX to IDLE.
 *  2. Rx: State goes from IDLE to RX, radio is in RX state for RX_duration,
 *     then state goes from RX to IDLE.
 *  3. Go_to_Sleep: State goes from IDLE to SLEEP.
 *  4. End_of_Sleep: State goes from SLEEP to IDLE.
 * The class keeps track of what state the radio is currently in.
 *
 * Energy calculation: For each transaction, this model notifies EnergySource
 * object. The EnergySource object will query this model for the total current.
 * Then the EnergySource object uses the total current to calculate energy.
 *
 * Default values for power consumption are based on measurements reported in:
 *
 * Daniel Halperin, Ben Greenstein, Anmol Sheth, David Wetherall,
 * "Demystifying 802.11n power consumption", Proceedings of HotPower'10
 *
 * Power consumption in Watts (single antenna):
 *
 * \f$ P_{tx} = 1.14 \f$ (transmit at 0dBm)
 *
 * \f$ P_{rx} = 0.94 \f$
 *
 * \f$ P_{idle} = 0.82 \f$
 *
 * \f$ P_{sleep} = 0.10 \f$
 *
 * Hence, considering the default supply voltage of 3.0 V for the basic energy
 * source, the default current values in Ampere are:
 *
 * \f$ I_{tx} = 0.380 \f$
 *
 * \f$ I_{rx} = 0.313 \f$
 *
 * \f$ I_{idle} = 0.273 \f$
 *
 * \f$ I_{sleep} = 0.033 \f$
 *
 * The dependence of the power consumption in transmission mode on the nominal
 * transmit power can also be achieved through a wifi TX current model.
 *
 */
class LrWpanRadioEnergyModel : public DeviceEnergyModel
{
public:
  /**
   * Callback type for energy depletion handling.
   */
  typedef Callback<void> LrWpanRadioEnergyDepletionCallback;

  /**
   * Callback type for energy recharged handling.
   */
  typedef Callback<void> LrWpanRadioEnergyRechargedCallback;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  LrWpanRadioEnergyModel ();
  virtual ~LrWpanRadioEnergyModel ();

  /**
   * \brief Sets pointer to EnergySouce installed on node.
   *
   * \param source Pointer to EnergySource installed on node.
   *
   * Implements DeviceEnergyModel::SetEnergySource.
   */
  void SetEnergySource (const Ptr<EnergySource> source);

  /**
   * \returns Total energy consumption of the device in watts.
   *
   * Implements DeviceEnergyModel::GetTotalEnergyConsumption.
   */
  double GetTotalEnergyConsumption (void) const;

  /**
   * \brief Gets current consumption of device in transmit mode.
   *
   * \returns Current consumption of the device in transmit mode in Ampere.
   */
  double GetTxOnCurrentA (void) const;

  /**
   * \brief Sets transmit current in Amperes.
   *
   * \param idleCurrentA transmit current of the device.
   */
  void SetTxOnCurrentA (double idleCurrentA);

  /**
   * \brief Gets receive current in Amperes.
   *
   * \returns transmit current of the device.
   */
  double GetRxOnCurrentA (void) const;

  /**
   * \brief Sets receive current in Amperes.
   *
   * \param CcaBusyCurrentA transmit current of the device.
   */
  void SetRxOnCurrentA (double CcaBusyCurrentA);
  /**
   * \brief Gets transmit current in Amperes.
   *
   * \returns transmit current of the device.
   */
  double GetTxCurrentA (void) const;
  /**
   * \brief Sets transmit current in Amperes.
   *
   * \param txCurrentA the transmit current
   */
  void SetTxCurrentA (double txCurrentA);
  /**
   * \brief Gets receive current in Amperes.
   *
   * \returns receive current of the wifi device.
   */
  double GetRxCurrentA (void) const;
  /**
   * \brief Sets receive current in Amperes.
   *
   * \param rxCurrentA the receive current
   */
  void SetRxCurrentA (double rxCurrentA);

  /**
   * \returns Current state.
   */
  int GetCurrentState (void) const;

  /**
   * \param callback Callback function.
   *
   * Sets callback for energy depletion handling.
   */
  void SetEnergyDepletionCallback (LrWpanRadioEnergyDepletionCallback callback);

  /**
   * \param callback Callback function.
   *
   * Sets callback for energy recharged handling.
   */
  void SetEnergyRechargedCallback (LrWpanRadioEnergyRechargedCallback callback);

  /**
   * \param model the model used to compute the wifi TX current.
   */
  void SetTxCurrentModel (const Ptr<LrWpanTxCurrentModel> model);

  /**
   * \brief Calls the CalcTxCurrent method of the TX current model to
   *        compute the TX current based on such model
   *
   * \param txPowerDbm the nominal TX power in dBm
   */
  void SetTxCurrentFromModel (double txPowerDbm);

  /**
   * \brief Changes state of the WifiRadioEnergyMode.
   *
   * \param newState New state the wifi radio is in.
   *
   * Implements DeviceEnergyModel::ChangeState.
   */
  void ChangeState (int newState);

  /**
   * \param state the wifi state
   *
   * \returns the time the radio can stay in that state based on the remaining energy.
   */
  Time GetMaximumTimeInState (int state) const;

  /**
   * \brief Handles energy depletion.
   *
   * Implements DeviceEnergyModel::HandleEnergyDepletion
   */
  void HandleEnergyDepletion (void);

  void EnergyDepletionEventReceived (void);

  /**
   * \brief Handles energy recharged.
   *
   * Implements DeviceEnergyModel::HandleEnergyRecharged
   */
  void HandleEnergyRecharged (void);

  /**
   * \brief Handles energy changed.
   *
   * Implements DeviceEnergyModel::HandleEnergyChanged
   */
  void HandleEnergyChanged (void);

  /**
   * \returns Pointer to the PHY listener.
   */
  LrWpanRadioEnergyModelPhyListener * GetPhyListener (void);

private:
  void DoDispose (void);

  /**
   * \param state the wifi state
   * \returns draw of device in Amperes, at given state.
   */
  double GetStateA (int state) const;

  /**
   * \returns Current draw of device in Amperes, at current state.
   *
   * Implements DeviceEnergyModel::GetCurrentA.
   */
  double DoGetCurrentA (void) const;

  /**
   * \param state New state the radio device is currently in.
   *
   * Sets current state. This function is private so that only the energy model
   * can change its own state.
   */
  void SetLrWpanRadioState (const int state);

  Ptr<EnergySource> m_source; ///< energy source

  // Member variables for current draw in different radio modes.
  double m_txCurrentA; ///< transmit current in Amperes
  double m_rxCurrentA; ///< receive current in Amperes
  double m_txOnCurrentA; ///< tx-on current in Amperes
  double m_rxOnCurrentA; ///< rx-on current in Amperes

  Ptr<LrWpanTxCurrentModel> m_txCurrentModel; ///< current model

  // State variables.
  int m_currentState;  ///< current state the radio is in
  Time m_lastUpdateTime;          ///< time stamp of previous energy update

  uint8_t m_nPendingChangeState; ///< pending state change

  /// This variable keeps track of the total energy consumed by this model in watts.
  TracedValue<double> m_totalEnergyConsumption;

  /// This variable keeps track of the depletion state of the device
  TracedValue<bool> m_totalEnergyDepleated;

  /// Callback to register depletion state
  TracedCallback<Time, bool, bool> m_DepletionStateLogger;

  /// Energy depletion callback
  LrWpanRadioEnergyDepletionCallback m_energyDepletionCallback;

  /// Energy recharged callback
  LrWpanRadioEnergyRechargedCallback m_energyRechargedCallback;

  /// LrWpanPhy listener
  LrWpanRadioEnergyModelPhyListener *m_listener;

  EventId m_switchToOffEvent; ///< switch to off event
};

} // namespace ns3

#endif /* LR_WPAN_RADIO_ENERGY_MODEL_H */
