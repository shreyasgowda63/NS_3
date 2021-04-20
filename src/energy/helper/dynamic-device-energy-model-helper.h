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

#ifndef DYNAMIC_DEVICE_ENERGY_MODEL_HELPER_H
#define DYNAMIC_DEVICE_ENERGY_MODEL_HELPER_H

#include <ns3/dynamic-device-energy-model.h>
#include <ns3/energy-model-helper.h>

namespace ns3 {

class DynamicDeviceEnergyModelHelper
{
public:
  DynamicDeviceEnergyModelHelper ();

  /**
   * \param name Name of attribute to set.
   * \param v Value of the attribute.
   *
   * Sets one of the attributes of underlying DynamicDeviceEnergyModel.
   */
  virtual void Set (std::string name, const AttributeValue &v);

  /**
   * \param device Pointer to the Node to install DynamicDeviceEnergyModel on.
   * \param source The EnergySource the DynamicDeviceEnergyModels will be using.
   * \returns A DeviceEnergyModelContainer containing all the DynamicDeviceEnergyModels.
   *
   * Installs a DynamicDeviceEnergyModel with a specified EnergySource onto a
   * Node.
   */
  DeviceEnergyModelContainer Install (Ptr<Node> node, Ptr<EnergySource> source) const;

  /**
   * \param nodeContainer List of Nodes to install DynamicDeviceEnergyModels on.
   * \param sourceContainer List of EnergySources the DynamicDeviceEnergyModels will be using.
   * \returns An DeviceEnergyModelContainer containing all the DynamicDeviceEnergyModels.
   *
   * Installs DynamicDeviceEnergyModels with specified EnergySources onto a list of Nodes.
   */
  DeviceEnergyModelContainer Install (NodeContainer nodeContainer,
                                      EnergySourceContainer sourceContainer) const;

  void AddEnergyDepletedCallback (DynamicDeviceEnergyModel::EnergyDepletedCallback cb);
  void AddEnergyRechargedCallback (DynamicDeviceEnergyModel::EnergyRechargedCallback cb);

private:
  /**
   * \param node The Node corresponding to this DynamicDeviceEnergyModel object.
   * \param source The EnergySource the DynamicDeviceEnergyModel will be using.
   * \returns Pointer to the created DynamicDeviceEnergyModel.
   */
  Ptr<DynamicDeviceEnergyModel> DoInstall (Ptr<Node> node, Ptr<EnergySource> source) const;

  std::vector<DynamicDeviceEnergyModel::EnergyDepletedCallback>
      m_energyDepletedCallbacks; ///< Callbacks for when the energy is depleted
  std::vector<DynamicDeviceEnergyModel::EnergyRechargedCallback>
      m_energyRechargedCallbacks; ///< Callbacks for when the energy is recharged

  ObjectFactory m_dynamicEnergy;
};
} // namespace ns3

#endif /* DYNAMIC_DEVICE_ENERGY_MODEL_HELPER_H */