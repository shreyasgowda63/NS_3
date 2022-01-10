/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
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

#ifndef DYNAMIC_DEVICE_ENERGY_MODEL_H
#define DYNAMIC_DEVICE_ENERGY_MODEL_H

#include <ns3/device-energy-model.h>
#include <ns3/nstime.h>
#include <ns3/pointer.h>
#include <ns3/traced-value.h>
#include <ns3/simulator.h>

namespace ns3 {

/**
 * \brief A class for saving different states that a BasicDeviceEnergyModel can be in.
 * 
 * By default, the Off state (index 0) is always added. This can not be changed.
 */
class BasicEnergyModelStates : public Object
{
public:
  /** Type defining a state (string Name, double current in Amperes) */
  typedef std::pair<std::string, double> State;

  BasicEnergyModelStates ();

  /**
   * \brief Get the TypeId
   * 
   * \return TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Adds a new state.
   * 
   * \param state The BasicEnergyModelStates::State to add.
   * 
   * \return uint32_t The index assigned to the new state.
   */
  uint32_t AddState (State state);

  /**
   * \brief Adds a new state.
   * 
   * \param name The name of the state to add.
   * \param currentA The current in A of the state to add.
   * 
   * \return uint32_t The index assigned to the new state.
   */
  uint32_t AddState (std::string name, double currentA);

  /**
   * \brief Removes a state.
   * 
   * \param index The index of the state to remove.
   * 
   * \return bool Whether the state was removed successfully.
   */
  bool RemoveState (uint32_t index);

  /**
   * \brief Gets the name of a given state.
   * 
   * \param index The index of the state.
   * \return std::string The state's name.
   */
  std::string GetStateName (uint32_t index) const;

  /**
   * \brief Gets the name and current for a given state.
   * 
   * \param index The index of the state.
   * \return State The State associated to the given index.
   */
  State GetState (uint32_t index) const;

  /**
   * \brief Sets a new current in A for an already existing state.
   * 
   * \param index The index of the state to override.
   * \param currentA The current in A of the state to override.
   * 
   * \return bool Whether the current was successfully set.
   */
  bool SetCurrent (uint32_t index, double currentA);

private:
  uint32_t m_indexCounter {0}; ///< Running counter for assigning indices to the states

  std::map<uint32_t, State> m_states; ///< Map of all states onto their indices
};

/**
 * \ingroup energy
 * 
 * @brief Device Energy Model for dynamic devices like MCUs.
 * 
 * This DeviceEnergyModel can model the energy consumption of a wide range of different devices.
 * By connecting this class to a BasicEnergyModelStates object, dynamic states can
 * be created during the simulation while not having to set the currents in A directly.
 * 
 * This class also provides automatic functionality for changing the state to Off
 * when the energy is depleted, and energy depletion and recharged callbacks
 * can be registered.
 */
class BasicDeviceEnergyModel : public DeviceEnergyModel
{
public:
  // ##################################################################### //
  // ############################## Typedefs ############################# //
  // ##################################################################### //

  /**
   * \brief Callback for notifying that the energy for this device is depleted.
   * 
   * \param model Pointer to the BasicDeviceEnergyModel where the energy is depleted.
   */
  typedef Callback<void, Ptr<BasicDeviceEnergyModel>> EnergyDepletedCallback;

  /**
   * \brief Callback for notifying that the energy for this device is recharged.
   * 
   * \param model Pointer to the BasicDeviceEnergyModel where the energy is recharged.
   */
  typedef Callback<void, Ptr<BasicDeviceEnergyModel>> EnergyRechargedCallback;

  // ##################################################################### //
  // ############################## Methods ############################## //
  // ##################################################################### //

  BasicDeviceEnergyModel ();

  /**
   * \brief Get the TypeId
   * 
   * \return TypeId 
   */
  static TypeId GetTypeId (void);

  virtual void SetEnergySource (Ptr<EnergySource> source);
  virtual double GetTotalEnergyConsumption (void) const;
  virtual void ChangeState (int newState);
  virtual void HandleEnergyDepletion (void);
  virtual void HandleEnergyRecharged (void);
  virtual void HandleEnergyChanged (void);

  /**
   * \brief Schedules a call to the ChangeState method.
   * 
   * \param delay The time after which the method should be called
   * \param state The argument that will be passed to the method when it is called
   *
   * This method is intended to make it easier to schedule a call to the
   * BasicDeviceEnergyModel::ChangeState method compared to calling Simulator::Schedule.
   * This method does nothing else than calling Simulator::Schedule with the provided arguments.
   */
  void ScheduleChangeState (Time delay, uint32_t state);

  /**
   * \brief Registers a new EnergyDepletedCallback to be called when this device's energy is depleted.
   * 
   * \param cb The callback to register
   */
  void RegisterEnergyDepletedCallback (EnergyDepletedCallback cb);

  /**
   * \brief Registers a new EnergyRechargedCallback to be called when this device's energy was recharged.
   * 
   * \param cb The callback to register
   */
  void RegisterEnergyRechargedCallback (EnergyRechargedCallback cb);

  // ====================================================== //
  // ================== Getters & Setters ================= //
  // ====================================================== //

  /**
   * \brief Get a pointer to the Node on which this device is installed.
   * 
   * \return Ptr<Node> The node
   */
  Ptr<Node> GetNode () const;

  /**
   * \brief Set the Node on which this device is installed.
   * 
   * \param node The node
   * 
   * This method does not install anything, but only reassigns a pointer.
   */
  void SetNode (Ptr<Node> node);

  // ##################################################################### //
  // ############################## Private ############################## //
  // ##################################################################### //

private:
  virtual double DoGetCurrentA (void) const;

  virtual void DoInitialize (void);

  // ====================================================== //
  // ===================== Attributes ===================== //
  // ====================================================== //

  Time m_lastUpdateTime; ///< The last time m_totalEnergyConsumption was updated

  bool m_energyDepleted; ///< Whether the energy is currently depleted
  Ptr<EnergySource> m_source; ///< The connected EnergySource
  Ptr<Node> m_node; ///< The Node this device's model is installed on
  Ptr<BasicEnergyModelStates> m_states; ///< The BasicEnergyModelStates object associated with this
  TracedValue<double> m_totalEnergyConsumption; ///< The total energy consumed by this device
  TracedValue<uint32_t> m_state; ///< The state this model is currently in
  TracedValue<double> m_currentA; ///< The current in A of this model (for tracing)
  uint32_t m_defaultState; ///< The default state for the beginning and after energy is recharged

  TracedCallback<Ptr<BasicDeviceEnergyModel>>
      m_energyDepletedCallbacks; ///< Callbacks for when the energy is depleted
  TracedCallback<Ptr<BasicDeviceEnergyModel>>
      m_energyRechargedCallbacks; ///< Callbacks for when the energy is recharged
};
} // namespace ns3

#endif /* DYNAMIC_DEVICE_ENERGY_MODEL_H */