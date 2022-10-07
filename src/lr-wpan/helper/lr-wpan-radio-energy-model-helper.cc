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

#include "lr-wpan-radio-energy-model-helper.h"

#include "ns3/lr-wpan-net-device.h"
#include "ns3/lr-wpan-tx-current-model.h"
#include "ns3/lr-wpan-phy.h"

namespace ns3 {

LrWpanRadioEnergyModelHelper::LrWpanRadioEnergyModelHelper ()
{
  m_radioEnergy.SetTypeId ("ns3::LrWpanRadioEnergyModel");
  m_depletionCallback.Nullify ();
  m_rechargedCallback.Nullify ();
}

LrWpanRadioEnergyModelHelper::~LrWpanRadioEnergyModelHelper ()
{}

void
LrWpanRadioEnergyModelHelper::Set (std::string name, const AttributeValue &v)
{
  m_radioEnergy.Set (name, v);
}

void
LrWpanRadioEnergyModelHelper::SetDepletionCallback (
  LrWpanRadioEnergyModel::LrWpanRadioEnergyDepletionCallback callback)
{
  m_depletionCallback = callback;
}

void
LrWpanRadioEnergyModelHelper::SetRechargedCallback (
  LrWpanRadioEnergyModel::LrWpanRadioEnergyRechargedCallback callback)
{
  m_rechargedCallback = callback;
}

void
LrWpanRadioEnergyModelHelper::SetTxCurrentModel (std::string name,
                                                 std::string n0, const AttributeValue& v0,
                                                 std::string n1, const AttributeValue& v1,
                                                 std::string n2, const AttributeValue& v2,
                                                 std::string n3, const AttributeValue& v3,
                                                 std::string n4, const AttributeValue& v4,
                                                 std::string n5, const AttributeValue& v5,
                                                 std::string n6, const AttributeValue& v6,
                                                 std::string n7, const AttributeValue& v7)
{
  ObjectFactory factory;
  factory.SetTypeId (name);
  factory.Set (n0, v0);
  factory.Set (n1, v1);
  factory.Set (n2, v2);
  factory.Set (n3, v3);
  factory.Set (n4, v4);
  factory.Set (n5, v5);
  factory.Set (n6, v6);
  factory.Set (n7, v7);
  m_txCurrentModel = factory;
}


/*
 * Private function starts here.
 */

Ptr<DeviceEnergyModel>
LrWpanRadioEnergyModelHelper::DoInstall (Ptr<NetDevice> device,
                                         Ptr<EnergySource> source) const
{
  NS_ASSERT (device != NULL);
  NS_ASSERT (source != NULL);
  // check if device is LrWpanNetDevice
  std::string deviceName = device->GetInstanceTypeId ().GetName ();
  if (deviceName.compare ("ns3::LrWpanNetDevice") != 0)
    {
      NS_FATAL_ERROR ("NetDevice type is not LrWpanNetDevice!");
    }
  Ptr<Node> node = device->GetNode ();
  Ptr<LrWpanRadioEnergyModel> model = m_radioEnergy.Create ()->GetObject<LrWpanRadioEnergyModel> ();
  NS_ASSERT (model != NULL);

  // set energy depletion callback
  // if none is specified, make a callback to LrWpanPhy::SetOffMode
  Ptr<LrWpanNetDevice> LrWpanDevice = DynamicCast<LrWpanNetDevice> (device);
  Ptr<LrWpanPhy> LrWpanPhy = LrWpanDevice->GetPhy ();
  LrWpanPhy->SetLrWpanRadioEnergyModel (model);
  if (m_depletionCallback.IsNull ())
    {
      model->SetEnergyDepletionCallback (MakeCallback (&LrWpanPhy::ChangeToOffState, LrWpanPhy));
    }

  else
    {
      model->SetEnergyDepletionCallback (m_depletionCallback);
    }
  // set energy recharged callback
  // if none is specified, make a callback to LrWpanPhy::ResumeFromOff
  if (m_rechargedCallback.IsNull ())
    {
      model->SetEnergyRechargedCallback (MakeCallback (&LrWpanPhy::ResumeFromOff, LrWpanPhy));
    }
  else
    {
      model->SetEnergyRechargedCallback (m_rechargedCallback);
    }
  // add model to device model list in energy source
  source->AppendDeviceEnergyModel (model);
  // set energy source pointer
  model->SetEnergySource (source);
  // create and register energy model PHY listener
  LrWpanPhy->RegisterListener (model->GetPhyListener ());
  //
  if (m_txCurrentModel.GetTypeId ().GetUid ())
    {
      model->SetTxCurrentModel (m_txCurrentModel.Create<LrWpanTxCurrentModel> ());
    }
  else
    {
      model->SetTxCurrentModel (CreateObject<LinearLrWpanTxCurrentModel>());
    }
  return model;
}

} // namespace ns3