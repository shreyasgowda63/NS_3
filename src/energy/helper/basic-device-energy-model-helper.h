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

#ifndef BASIC_DEVICE_ENERGY_MODEL_HELPER_H
#define BASIC_DEVICE_ENERGY_MODEL_HELPER_H

#include <ns3/basic-device-energy-model.h>
#include <ns3/energy-model-helper.h>

namespace ns3 {

/**
 * \ingroup energy
 * \brief Creates BasicDeviceEnergyModel objects.
 *
 * This class creates and installs BasicDeviceEnergyModel objects.
 *
 */
class BasicDeviceEnergyModelHelper
{
public:
  BasicDeviceEnergyModelHelper ();

  /**
   * \param name Name of attribute to set.
   * \param v Value of the attribute.
   *
   * Sets one of the attributes of underlying BasicDeviceEnergyModel.
   */
  virtual void Set (std::string name, const AttributeValue &v);

  /**
   * \param node Pointer to the Node to install BasicDeviceEnergyModel on.
   * \param source The EnergySource the BasicDeviceEnergyModels will be using.
   * \returns A DeviceEnergyModelContainer containing all the BasicDeviceEnergyModels.
   *
   * Installs a BasicDeviceEnergyModel with a specified EnergySource onto a
   * Node.
   */
  DeviceEnergyModelContainer Install (Ptr<Node> node, Ptr<EnergySource> source) const;

  /**
   * \param nodeContainer List of Nodes to install BasicDeviceEnergyModels on.
   * \param sourceContainer List of EnergySources the BasicDeviceEnergyModels will be using.
   * \returns An DeviceEnergyModelContainer containing all the BasicDeviceEnergyModels.
   *
   * Installs BasicDeviceEnergyModels with specified EnergySources onto a list of Nodes.
   */
  DeviceEnergyModelContainer Install (NodeContainer nodeContainer,
                                      EnergySourceContainer sourceContainer) const;

  /**
   * \param cb The callback to register
   *
   * Registers a new EnergyDepletedCallback to be called when the installed device's energy is depleted.
   */
  void AddEnergyDepletedCallback (BasicDeviceEnergyModel::EnergyDepletedCallback cb);

  /**
   * \param cb The callback to register
   *
   * Registers a new EnergyRechargeCallback to be called when the installed device's energy has been recharged.
   */
  void AddEnergyRechargedCallback (BasicDeviceEnergyModel::EnergyRechargedCallback cb);

private:
  /**
   * \param node The Node corresponding to this BasicDeviceEnergyModel object.
   * \param source The EnergySource the BasicDeviceEnergyModel will be using.
   * \returns Pointer to the created BasicDeviceEnergyModel.
   */
  Ptr<BasicDeviceEnergyModel> DoInstall (Ptr<Node> node, Ptr<EnergySource> source) const;

  std::vector<BasicDeviceEnergyModel::EnergyDepletedCallback>
      m_energyDepletedCallbacks; ///< Callbacks for when the energy is depleted
  std::vector<BasicDeviceEnergyModel::EnergyRechargedCallback>
      m_energyRechargedCallbacks; ///< Callbacks for when the energy is recharged

  /// ObjectFactory for creating the BasicDeviceEnergyModel objects
  ObjectFactory m_basicEnergy;
};
} // namespace ns3

#endif /* BASIC_DEVICE_ENERGY_MODEL_HELPER_H */