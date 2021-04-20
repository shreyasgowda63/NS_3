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

#ifndef SENSOR_DEVICE_ENERGY_MODEL_H
#define SENSOR_DEVICE_ENERGY_MODEL_H

#include <ns3/device-energy-model.h>
#include <ns3/nstime.h>
#include <ns3/traced-value.h>
#include <ns3/simulator.h>

namespace ns3 {

/**
 * \ingroup energy
 * 
 * \brief Device Energy Model for a sensor device.
 * 
 * This DeviceEnergyModel models the energy consumption by a sensor device.
 * For this, four states are defined: Measure, Idle, Sleep, and Off.
 * 
 * Unlike other DeviceEnergyModels like the WifiRadioEnergyModel,
 * changing between states at the right moment is the user's responsibility.
 * 
 * The methods SensorDeviceEnergyModel::ScheduleMeasure and SensorDeviceEnergyModel::MeasureNow
 * can be used to set up "measurement activities" for this model.
 * Those activities will be assigned an SensorDeviceEnergyModel::Id which can be used by the user to figure out whether 
 * the activity was successful.
 * Activities can fail when there is not enough energy or they are canceled by the user (by changing the state manually).
 * 
 * Changes into/between other states (i.e., SensorDeviceEnergyModel::SensorModelState::Idle,
 * SensorDeviceEnergyModel::SensorModelState::Sleep, SensorDeviceEnergyModel::SensorModelState::Off (manually))
 * have to be done directly using the SensorDeviceEnergyModel::ChangeState method.
 */
class SensorDeviceEnergyModel : public DeviceEnergyModel
{
public:
  // ##################################################################### //
  // ############################## Typedefs ############################# //
  // ##################################################################### //

  /**
   * \brief The states defined for the SensorDeviceEnergyModel.
   */
  enum SensorModelState : int {
    Measure, ///< The device is measuring something
    Idle, ///< Idle state
    Sleep, ///< Sleep state
    Off, ///< Turned off
    Default, ///< Special state that is only used for switching into a new state and is a placeholder for m_defaultState
    Previous ///< Special state that is only used for setting afterState when scheduling a measurement and will reset the state to its previous value
  };

  /**
   * \brief Error codes used to give information about measurement activities.
   * 
   * Success: The activity started successfully / ended successfully.
   * 
   * EnergyDepleted: The activity couldn't start because the energy is depleted / couldn't run until the desired end because the energy ran out while it was running.
   * 
   * Overlap: The activity couldn't start because another measurement activity is already running. A check for this is *not* performed at scheduling time! The user is responsible for this.
   * 
   * Unknown: Anything else went wrong.
   */
  enum ErrorCode {
    Success, ///< No errors
    EnergyDepleted, ///< The energy is depleted
    Overlap, ///< The activity couldn't be started because another activity is already running
    Unknown ///< In case another ErrorCode is needed
  };

  /**
   * \brief An id used to identify measurement activities on this device.
   * 
   * This is used for the MeasurementStartCallback and MeasurementEndCallback
   * and can be used to get information about whether a specific measurement activity was successful.
   * In case a measurement activity is failed (i.e., the Start or End callbacks are called with the second parameter being false),
   * the user should not "use" its data.
   */
  typedef uint32_t Id;

  /**
   * \brief A callback for notifying of the start of a measurement activity.
   * 
   * \param model Pointer to the SensorDeviceEnergyModel on which the activity started.
   * \param id The SensorDeviceEnergyModel::Id of the started measurement activity.
   * \param code Return code for whether the measurement activity started successfully.
   */
  typedef Callback<void, Ptr<SensorDeviceEnergyModel>, Id, ErrorCode> MeasurementStartCallback;

  /**
   * \brief A callback for notifying of the end of a measurement activity.
   * 
   * \param model Pointer to the SensorDeviceEnergyModel on which the activity ended.
   * \param id The SensorDeviceEnergyModel::Id of the ended measurement activity.
   * \param code Return code for whether the measurement activity fully ran successfully.
   */
  typedef Callback<void, Ptr<SensorDeviceEnergyModel>, Id, ErrorCode> MeasurementEndCallback;

  /**
   * \brief Callback for notifying that the energy for this device is depleted.
   * 
   * \param model Pointer to the SensorDeviceEnergyModel where the energy is depleted.
   */
  typedef Callback<void, Ptr<SensorDeviceEnergyModel>> EnergyDepletedCallback;

  /**
   * \brief Callback for notifying that the energy for this device is recharged.
   * 
   * \param model Pointer to the SensorDeviceEnergyModel where the energy is recharged.
   */
  typedef Callback<void, Ptr<SensorDeviceEnergyModel>> EnergyRechargedCallback;

  // ##################################################################### //
  // ############################## Methods ############################## //
  // ##################################################################### //

  SensorDeviceEnergyModel ();

  static TypeId GetTypeId (void);

  virtual void SetEnergySource (Ptr<EnergySource> source);
  virtual double GetTotalEnergyConsumption (void) const;
  virtual void ChangeState (int newState);
  virtual void HandleEnergyDepletion (void);
  virtual void HandleEnergyRecharged (void);
  virtual void HandleEnergyChanged (void);

  /**
   * \brief Schedules a measurement activity with defined start and end time.
   * 
   * \param start The relative Time when to start.
   * \param end The Time after which to stop, relative to start.
   * \param afterState The SensorDeviceEnergyModel::SensorModelState to switch into after this activity finished. Default: The active state before starting
   * \return Id The SensorDeviceEnergyModel::Id for this measurement activity.
   */
  Id ScheduleMeasure (Time start, Time end, SensorModelState afterState = Previous);

  /**
   * \brief Simulates a measurement activity starting now and ending after a defined time span.
   * 
   * \param end The Time after which to stop, relative to now.
   * \param afterState The SensorDeviceEnergyModel::SensorModelState to switch into after this activity finished. Default: The active state before starting
   * \return Id The SensorDeviceEnergyModel::Id for this measurement activity.
   */
  Id MeasureNow (Time end, SensorModelState afterState = Previous);

  void RegisterMeasurementStartCallback (MeasurementStartCallback cb);
  void RegisterMeasurementEndCallback (MeasurementEndCallback cb);

  void RegisterEnergyDepletedCallback (EnergyDepletedCallback cb);
  void RegisterEnergyRechargedCallback (EnergyRechargedCallback cb);

  // ====================================================== //
  // ================== Getters & Setters ================= //
  // ====================================================== //

  Ptr<Node> GetNode () const;
  void SetNode (Ptr<Node> node);

  double GetMeasureCurrentA () const;
  void SetMeasureCurrentA (double current);

  double GetIdleCurrentA () const;
  void SetIdleCurrentA (double current);

  double GetSleepCurrentA () const;
  void SetSleepCurrentA (double current);

  SensorModelState GetDefaultState () const;
  void SetDefaultState (SensorModelState state);

  // ##################################################################### //
  // ############################## Private ############################## //
  // ##################################################################### //

private:
  virtual double DoGetCurrentA (void) const;

  /**
   * \brief Starts a measurement activity.
   * 
   * \param id The SensorDeviceEnergyModel::Id of the activity.
   * \param afterState The SensorDeviceEnergyModel::SensorModelState to switch into after this activity finished.
   * \param end How long the activity is supposed to run.
   */
  void DoStartMeasure (Id id, Time end, SensorModelState afterState);

  /**
   * \brief Ends a measurement activity.
   * 
   * \param id The SensorDeviceEnergyModel::Id of the activity.
   * \param afterState The SensorDeviceEnergyModel::SensorModelState to switch into after this activity finished.
   */
  void DoEndMeasure (Id id, SensorModelState afterState);

  virtual void DoInitialize (void);

  // ====================================================== //
  // ===================== Attributes ===================== //
  // ====================================================== //

  Time m_lastUpdateTime; ///< The last time m_totalEnergyConsumption was updated

  double m_measureCurrentA; ///< The current for the Measure state
  double m_idleCurrentA; ///< The current for the SensorDeviceEnergyModel::State::Idle state
  double m_sleepCurrentA; ////< The current for the Sleep state
  /**
   * \brief The default SensorDeviceEnergyModel::SensorModelState to be in after initialization and after the energy is recharged.
   */
  SensorModelState m_defaultState;

  TracedValue<SensorModelState> m_state; ///< The SensorModelState this model is currently in
  /**
   * \brief Counter for assigning a new SensorDeviceEnergyModel::Id for each new measurement activity. This also represents the total amount of scheduled activities.
   */
  uint32_t m_activityIdCounter;
  uint32_t
      m_runningActivity; ///< The SensorDeviceEnergyModel::Id of the currently running measurement activity. 0 -> None
  bool m_energyDepleted; ///< Whether the energy is currently depleted
  /**
   * \brief Whether a measurement activity should be seen as failed if the energy is depleted afterwards.
   * 
   * When a measurement activity ends, the state is changed using ChangeState.
   * If during that call the energy is set to be depleted, the state is afterwards set to SensorDeviceEnergyModel::SensorModelState::Off
   * and m_energyDepleted is true.
   * However, it is not possible to 
   * 
   * Setting this value to 
   */
  bool m_strictEnergyChecking;

  Ptr<EnergySource> m_source; ///< The connected EnergySource
  Ptr<Node> m_node; ///< The Node this device's model is installed on
  TracedValue<double> m_totalEnergyConsumption; ///< The total energy consumed by this device
  TracedValue<double> m_currentA; ///< The current current in A for this device

  /**
   * \brief Callbacks for starts of measurement activities
   */
  TracedCallback<Ptr<SensorDeviceEnergyModel>, Id, ErrorCode> m_measurementStartCallbacks;
  /**
   * \brief Callbacks for ends of measurement activities
   */
  TracedCallback<Ptr<SensorDeviceEnergyModel>, Id, ErrorCode> m_measurementEndCallbacks;

  TracedCallback<Ptr<SensorDeviceEnergyModel>> m_energyDepletedCallbacks; ///< Callbacks for when the energy is depleted
  TracedCallback<Ptr<SensorDeviceEnergyModel>> m_energyRechargedCallbacks; ///< Callbacks for when the energy is recharged
};

} // namespace ns3

#endif /* SENSOR_DEVICE_ENERGY_MODEL_H */
