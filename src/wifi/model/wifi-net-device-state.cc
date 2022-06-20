/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Ananthakrishnan S
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
 * Author: Ananthakrishnan S <ananthakrishnan190@gmail.com>
 */

#include "ns3/wifi-net-device-state.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-mac.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiNetDeviceState");

NS_OBJECT_ENSURE_REGISTERED (WifiNetDeviceState);


TypeId
WifiNetDeviceState::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiNetDeviceState")
    .SetParent<NetDeviceState> ()
    .AddConstructor<WifiNetDeviceState> ()
    .SetGroupName ("Wifi")
  ;
  return tid;
}

void
WifiNetDeviceState::DoInitialize (void)
{
  m_stateChangeTrace (true, GetOperationalState());
  NS_LOG_INFO ("Notification sent: Device is administratively UP.");
  NetDeviceState::DoInitialize ();
}

WifiNetDeviceState::WifiNetDeviceState ()
{
  NS_LOG_FUNCTION (this);
  SetUp();
  NS_LOG_INFO ("WifiNetDevice is set admin UP during construction.");
}

void
WifiNetDeviceState::DoSetUp ()
{
  NS_LOG_FUNCTION (this);
  m_device->GetMac ()->EnableMacAndPhy ();
}

void
WifiNetDeviceState::DoSetDown ()
{
  NS_LOG_FUNCTION (this);
  m_device->GetMac ()->DisableMacAndPhy ();
}

void
WifiNetDeviceState::SetDevice (Ptr<WifiNetDevice> device)
{
  NS_LOG_FUNCTION (this << device );
  m_device = device;
}

} // namespace ns3
