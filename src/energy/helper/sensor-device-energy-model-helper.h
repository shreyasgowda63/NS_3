/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Philip Hönnecke
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
#ifndef SENSOR_DEVICE_ENERGY_MODEL_HELPER_H
#define SENSOR_DEVICE_ENERGY_MODEL_HELPER_H

#include <ns3/energy-model-helper.h>
#include <ns3/sensor-device-energy-model.h>

namespace ns3 {

class SensorDeviceEnergyModelHelper
{
public:
  SensorDeviceEnergyModelHelper ();

  /**
   * \param name Name of attribute to set.
   * \param v Value of the attribute.
   *
   * Sets one of the attributes of underlying SensorDeviceEnergyModel.
   */
  virtual void Set (std::string name, const AttributeValue &v);

  /**
   * \param device Pointer to the Node to install SensorDeviceEnergyModel on.
   * \param source The EnergySource the SensorDeviceEnergyModel will be using.
   * \returns A DeviceEnergyModelContainer containing all the SensorDeviceEnergyModels.
   *
   * Installs a SensorDeviceEnergyModel with a specified EnergySource onto a
   * Node.
   */
  DeviceEnergyModelContainer Install (Ptr<Node> node, Ptr<EnergySource> source) const;

  /**
   * \param nodeContainer List of Nodes to install SensorDeviceEnergyModels on.
   * \param sourceContainer List of EnergySources the SensorDeviceEnergyModels will be using.
   * \returns An DeviceEnergyModelContainer containing all the SensorDeviceEnergyModels.
   *
   * Installs SensorDeviceEnergyModels with specified EnergySources onto a list of Nodes.
   */
  DeviceEnergyModelContainer Install (NodeContainer nodeContainer,
                                      EnergySourceContainer sourceContainer) const;

  void AddMeasurementStartCallback (SensorDeviceEnergyModel::MeasurementStartCallback cb);
  void AddMeasurementEndCallback (SensorDeviceEnergyModel::MeasurementEndCallback cb);
  void AddEnergyDepletedCallback (SensorDeviceEnergyModel::EnergyDepletedCallback cb);
  void AddEnergyRechargedCallback (SensorDeviceEnergyModel::EnergyRechargedCallback cb);

private:
  /**
   * \param node The Node corresponding to this SensorDeviceEnergyModel object.
   * \param source The EnergySource the SensorDeviceEnergyModel will be using.
   * \returns Pointer to the created SensorDeviceEnergyModel.
   */
  Ptr<SensorDeviceEnergyModel> DoInstall (Ptr<Node> node, Ptr<EnergySource> source) const;

  std::vector<SensorDeviceEnergyModel::MeasurementStartCallback>
      m_measurementStartCallbacks; ///< Callbacks for starts of measurement activities
  std::vector<SensorDeviceEnergyModel::MeasurementEndCallback>
      m_measurementEndCallbacks; ///< Callbacks for ends of measurement activities
  std::vector<SensorDeviceEnergyModel::EnergyDepletedCallback>
      m_energyDepletedCallbacks; ///< Callbacks for when the energy is depleted
  std::vector<SensorDeviceEnergyModel::EnergyRechargedCallback>
      m_energyRechargedCallbacks; ///< Callbacks for when the energy is recharged

  ObjectFactory m_sensorEnergy;
};

} // namespace ns3

#endif /* SENSOR_DEVICE_ENERGY_MODEL_HELPER_H */