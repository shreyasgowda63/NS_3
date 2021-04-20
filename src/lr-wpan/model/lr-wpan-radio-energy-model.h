/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Philip Hönnecke.
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
 * Authors:
 *  Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu> (wifi-radio-energy-model)
 *  Philip Hönnecke <p.hoennecke@tu-braunschweig.de> (this code)
 */

#ifndef LR_WPAN_RADIO_ENERGY_MODEL_H
#define LR_WPAN_RADIO_ENERGY_MODEL_H

#include "ns3/device-energy-model.h"
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "lr-wpan-phy.h"
#include "ns3/lr-wpan-helper.h"
#include "ns3/wifi-tx-current-model.h"

namespace ns3 {

/**
 * \ingroup energy
 * \brief A LR-WPAN radio energy model.
 *
 * 4 states are defined for the radio: IDLE, TX, RX, and OFF. Default state is
 * IDLE.
 * Every time the LrWpanPhy starts to transmitt or receive something,
 * as well as ends such an activity in some way,
 * this class is notified using callbacks (PhyTxBegin, PhyTxEnd, PhyTxDrop, PhyRxBegin, PhyRxEnd, PhyRxDrop).
 * This class will handle the energy consumption of the LrWpanNetDevice
 * according to the above mentioned callbacks.
 * Additionally, if the connected energy source is depleted,
 * the LrWpanRadio will be denied to change its state (from PHY_OFF) until the energy is recharged.
 *
 * Energy calculation: For each transaction, this model notifies EnergySource
 * object. The EnergySource object will query this model for the total current.
 * Then the EnergySource object uses the total current to calculate energy.
 *
 * Default values for power consumption are based on data reported in:
 *
 * Nicolas Fourty, Adrien van den Bossche, Thierry Val,
 * "An advanced study of energy consumption in an IEEE 802.15.4 based network: Everything but the truth on 802.15.4 node lifetime",
 * https://doi.org/10.1016/j.comcom.2012.05.008
 *
 * From Table 1:
 *
 * \f$ Tx (0dBm): 30 mA \f$
 *
 * \f$ Rx: 38 mA \f$
 *
 * \f$ Idle: 1.3 mA \f$
 *
 * The dependence of the power consumption in transmission mode on the nominal
 * transmit power can also be achieved through a wifi TX current model.
 *
 */
class LrWpanRadioEnergyModel : public DeviceEnergyModel
{
public:
  /**
   * Enum for the different states the radio can be in regarding energy.
   */
  typedef enum {
    Undefined = 0, ///< Unknown
    Idle, ///< Not transceiving
    Tx, ///< Transmitting
    Rx, ///< Receiving
    Off ///< Power off
  } LrWpanRadioEnergyState;

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
   * \brief Sets the pointer to LrWpanPhy that should be connected to this device.
   * 
   * \param phy Pointer to the LrWpanPhy that should be connected.
   * 
   * This method sets up all the callbacks and traces from the phy.
   */
  void SetLrWpanPhy (Ptr<LrWpanPhy> phy);

  /**
   * \returns Total energy consumption of the wifi device in watts.
   *
   * Implements DeviceEnergyModel::GetTotalEnergyConsumption.
   */
  double GetTotalEnergyConsumption (void) const;

  /**
   * \param newState New state the LrWpanNetDevice is in.
   * 
   * Implements DeviceEnergyModel::ChangeState
   */
  void ChangeState (int newState);

  /**
   * Callback for receiving notifications about a packet starting to be sent.
   * 
   * \param p The packet.
   */
  void PhyTxBegin (Ptr<const Packet> p);

  /**
   * Callback for receiving notifications about a packet being sent.
   * 
   * \param p The packet.
   */
  void PhyTxEnd (Ptr<const Packet> p);

  /**
   * Callback for receiving notifications about a packet being dropped while being sent.
   * 
   * \param p The packet.
   */
  void PhyTxDrop (Ptr<const Packet> p);

  /**
   * Callback for receiving notifications about a packet starting to be received.
   * 
   * \param p The packet.
   */
  void PhyRxBegin (Ptr<const Packet> p);

  /**
   * Callback for receiving notifications about a packet being received.
   * 
   * \param p The packet.
   * \param sinr Received SINR.
   */
  void PhyRxEnd (Ptr<const Packet> p, double sinr);

  /**
   * Callback for receiving notifications about a packet being droppped while being received.
   * 
   * \param p The packet.
   */
  void PhyRxDrop (Ptr<const Packet> p);

  /**
   * \brief Callback for receiving notifications about a new state of the connected LrWpanPhy.
   * 
   * \param oldState The old state the LrWpanPhy was in.
   * \param newState The new state the LrWpanPhy changed into.
   *
   * This method defines which state this DeviceEnergyModel should change into after receiving the information about a new LrWpanPhyEnumeration.
   * That means, if an LrWpanRadioEnergyState should be affiliated with a different LrWpanPhyEnumeration,
   * those changes would be done here.
   */
  void TrxStateChanged (LrWpanPhyEnumeration oldState, LrWpanPhyEnumeration newState);

  /**
   * \returns Current state.
   */
  LrWpanRadioEnergyState GetCurrentState (void) const;

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
   * \return double Current for RX state in Amperes.
   */
  double GetRxCurrentA (void) const;

  /**
   * \param currentA Current for RX state in Amperes.
   */
  void SetRxCurrentA (double currentA);

  /**
   * \return double Current for TX state in Amperes.
   */
  double GetTxCurrentA (void) const;
  /**
   * \param currentA Current for TX state in Amperes.
   */
  void SetTxCurrentA (double currentA);

  /**
   * \return double Current for IDLE state in Amperes.
   */
  double GetIdleCurrentA (void) const;
  /**
   * \param currentA Current for IDLE state in Amperes.
   */
  void SetIdleCurrentA (double currentA);

private:
  void DoDispose (void);

  /**
   * \param state the LrWpanPhyEnumeration value
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
   * Sets the energy model's current state without any more checking.
   * 
   * \param state The state to set.
   */
  void SetLrWpanRadioEnergyState (LrWpanRadioEnergyState state);

  /**
   * Updates the m_switchToOffEvent if needed.
   * 
   * \param state The state to update the switchOffEvent for.
   */
  void UpdateSwitchToOffEvent (LrWpanRadioEnergyState state);

  Ptr<EnergySource> m_source; ///< EnergySource

  Ptr<LrWpanPhy> m_phy; ///< The connected LrWpanPhy

  // Member variables for current draw in different radio modes.
  double m_idleCurrentA; ///< IDLE current in Amperes
  double m_rxCurrentA; ///< RX current in Amperes
  double m_txCurrentA; ///< TX current in Amperes
  // Ptr<LrWpanTxCurrentModel> m_txCurrentModel; ///< current model

  /// This variable keeps track of the total energy consumed by this model in watts.
  TracedValue<double> m_totalEnergyConsumption;
  TracedValue<double> m_currentA; ///< Current A for tracing
  bool m_energyDepleted;

  // State variables.
  TracedValue<LrWpanRadioEnergyState>
      m_currentState; ///< current energy relevant state the radio is in
  Time m_lastUpdateTime; ///< time stamp of previous energy update

  uint8_t m_nPendingChangeState; ///< pending state change

  /// Energy depletion callback
  LrWpanRadioEnergyDepletionCallback m_energyDepletionCallback;

  /// Energy recharged callback
  LrWpanRadioEnergyRechargedCallback m_energyRechargedCallback;

  EventId m_switchToOffEvent; ///< switch to off event
};

} // namespace ns3

#endif /* LR_WPAN_RADIO_ENERGY_MODEL_H */
