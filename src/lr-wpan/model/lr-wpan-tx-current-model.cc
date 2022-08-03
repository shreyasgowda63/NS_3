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

#include "lr-wpan-tx-current-model.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LrWpanTxCurrentModel");

NS_OBJECT_ENSURE_REGISTERED (LrWpanTxCurrentModel);

TypeId
LrWpanTxCurrentModel::GetTypeId (void)
{
  static TypeId tid =
    TypeId ("ns3::LrWpanTxCurrentModel").SetParent<Object> ().SetGroupName ("LrWpan");
  return tid;
}

LrWpanTxCurrentModel::LrWpanTxCurrentModel ()
{}

LrWpanTxCurrentModel::~LrWpanTxCurrentModel ()
{}

NS_OBJECT_ENSURE_REGISTERED (LinearLrWpanTxCurrentModel);

TypeId
LinearLrWpanTxCurrentModel::GetTypeId (void)
{
  static TypeId tid =
    TypeId ("ns3::LinearLrWpanTxCurrentModel")
    .SetParent<LrWpanTxCurrentModel> ()
    .SetGroupName ("Energy")
    .AddConstructor<LinearLrWpanTxCurrentModel> ()
    .AddAttribute ("Eta", "The efficiency of the power amplifier.", DoubleValue (0.10),
                   MakeDoubleAccessor (&LinearLrWpanTxCurrentModel::m_eta),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Voltage", "The supply voltage (in Volts).", DoubleValue (3.0),
                   MakeDoubleAccessor (&LinearLrWpanTxCurrentModel::m_voltage),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("IdleCurrent", "The current in the IDLE state (in Ampere).",
                   DoubleValue (0.006746667),
                   MakeDoubleAccessor (&LinearLrWpanTxCurrentModel::m_idleCurrent),
                   MakeDoubleChecker<double> ());
  return tid;
}

LinearLrWpanTxCurrentModel::LinearLrWpanTxCurrentModel ()
{
  NS_LOG_FUNCTION (this);
}

LinearLrWpanTxCurrentModel::~LinearLrWpanTxCurrentModel ()
{
  NS_LOG_FUNCTION (this);
}

double
LinearLrWpanTxCurrentModel::CalcTxCurrent (double txPowerDbm) const
{
  NS_LOG_FUNCTION (this << txPowerDbm);

  double watt = std::pow (10.0, 0.1 * (txPowerDbm - 30.0));
  return watt / (m_voltage * m_eta) + m_idleCurrent;
}

} // namespace ns3
