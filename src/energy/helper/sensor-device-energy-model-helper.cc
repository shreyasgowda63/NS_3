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

#include "sensor-device-energy-model-helper.h"

namespace ns3 {

SensorDeviceEnergyModelHelper::SensorDeviceEnergyModelHelper ()
{
  m_sensorEnergy.SetTypeId ("ns3::SensorDeviceEnergyModel");
}

void
SensorDeviceEnergyModelHelper::Set (std::string name, const AttributeValue &v)
{
  m_sensorEnergy.Set (name, v);
}

DeviceEnergyModelContainer
SensorDeviceEnergyModelHelper::Install (Ptr<Node> node, Ptr<EnergySource> source) const
{
  NS_ASSERT (node != NULL);
  NS_ASSERT (source != NULL);
  DeviceEnergyModelContainer container (DoInstall (node, source));
  return container;
}

DeviceEnergyModelContainer
SensorDeviceEnergyModelHelper::Install (NodeContainer nodeContainer,
                                        EnergySourceContainer sourceContainer) const
{
  DeviceEnergyModelContainer container;
  NodeContainer::Iterator node = nodeContainer.Begin ();
  EnergySourceContainer::Iterator src = sourceContainer.Begin ();
  while (node != nodeContainer.End ())
    {
      Ptr<DeviceEnergyModel> model = DoInstall (*node, *src);
      container.Add (model);
      node++;
      src++;
    }
  return container;
}

void
SensorDeviceEnergyModelHelper::AddMeasurementStartCallback (
    SensorDeviceEnergyModel::MeasurementStartCallback cb)
{
  m_measurementStartCallbacks.push_back (cb);
}

void
SensorDeviceEnergyModelHelper::AddMeasurementEndCallback (
    SensorDeviceEnergyModel::MeasurementEndCallback cb)
{
  m_measurementEndCallbacks.push_back (cb);
}

void
SensorDeviceEnergyModelHelper::AddEnergyDepletedCallback (
    SensorDeviceEnergyModel::EnergyDepletedCallback cb)
{
  m_energyDepletedCallbacks.push_back (cb);
}

void
SensorDeviceEnergyModelHelper::AddEnergyRechargedCallback (
    SensorDeviceEnergyModel::EnergyRechargedCallback cb)
{
  m_energyRechargedCallbacks.push_back (cb);
}

Ptr<SensorDeviceEnergyModel>
SensorDeviceEnergyModelHelper::DoInstall (Ptr<Node> node, Ptr<EnergySource> source) const
{
  Ptr<SensorDeviceEnergyModel> model = m_sensorEnergy.Create<SensorDeviceEnergyModel> ();
  model->SetNode (node);
  model->SetEnergySource (source);
  source->AppendDeviceEnergyModel (model);
  for (const SensorDeviceEnergyModel::EnergyDepletedCallback &cb : m_energyDepletedCallbacks)
    {
      model->RegisterEnergyDepletedCallback (cb);
    }
  for (const SensorDeviceEnergyModel::EnergyRechargedCallback &cb : m_energyRechargedCallbacks)
    {
      model->RegisterEnergyRechargedCallback (cb);
    }
  for (const SensorDeviceEnergyModel::MeasurementStartCallback &cb : m_measurementStartCallbacks)
    {
      model->RegisterMeasurementStartCallback (cb);
    }
  for (const SensorDeviceEnergyModel::MeasurementEndCallback &cb : m_measurementEndCallbacks)
    {
      model->RegisterMeasurementEndCallback (cb);
    }
  return model;
}

} // namespace ns3