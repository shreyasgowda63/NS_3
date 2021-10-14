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

#include "dynamic-device-energy-model-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DynamicDeviceEnergyModelHelper");

NS_OBJECT_ENSURE_REGISTERED (DynamicDeviceEnergyModel);

DynamicDeviceEnergyModelHelper::DynamicDeviceEnergyModelHelper ()
{
  m_dynamicEnergy.SetTypeId ("ns3::DynamicDeviceEnergyModel");
}

void
DynamicDeviceEnergyModelHelper::Set (std::string name, const AttributeValue &v)
{
  m_dynamicEnergy.Set (name, v);
}

DeviceEnergyModelContainer
DynamicDeviceEnergyModelHelper::Install (Ptr<Node> node, Ptr<EnergySource> source) const
{
  NS_ASSERT (node != nullptr);
  NS_ASSERT (source != nullptr);
  DeviceEnergyModelContainer container (DoInstall (node, source));
  return container;
}

DeviceEnergyModelContainer
DynamicDeviceEnergyModelHelper::Install (NodeContainer nodeContainer,
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
DynamicDeviceEnergyModelHelper::AddEnergyDepletedCallback (
    DynamicDeviceEnergyModel::EnergyDepletedCallback cb)
{
  m_energyDepletedCallbacks.push_back (cb);
}

void
DynamicDeviceEnergyModelHelper::AddEnergyRechargedCallback (
    DynamicDeviceEnergyModel::EnergyRechargedCallback cb)
{
  m_energyRechargedCallbacks.push_back (cb);
}

Ptr<DynamicDeviceEnergyModel>
DynamicDeviceEnergyModelHelper::DoInstall (Ptr<Node> node, Ptr<EnergySource> source) const
{
  Ptr<DynamicDeviceEnergyModel> model = m_dynamicEnergy.Create<DynamicDeviceEnergyModel> ();
  model->SetNode (node);
  model->SetEnergySource (source);
  source->AppendDeviceEnergyModel (model);
  for (const DynamicDeviceEnergyModel::EnergyDepletedCallback &cb : m_energyDepletedCallbacks)
    {
      model->RegisterEnergyDepletedCallback (cb);
    }
  for (const DynamicDeviceEnergyModel::EnergyRechargedCallback &cb : m_energyRechargedCallbacks)
    {
      model->RegisterEnergyRechargedCallback (cb);
    }
  return model;
}


} // namespace ns3