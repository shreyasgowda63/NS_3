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

#ifndef WIFI_NET_DEVICE_STATE_H
#define WIFI_NET_DEVICE_STATE_H

#include "ns3/net-device-state.h"

namespace ns3 {

class WifiNetDevice;

class WifiNetDeviceState : public NetDeviceState
{
public:
/**
 * \brief Get the type ID.
 * \return the object TypeId
 */
  static TypeId GetTypeId (void);

  WifiNetDeviceState ();

/**
 * Set a pointer to the WifiNetDevice the object of this class
 * is aggregated to.
 */
  void SetDevice (Ptr<WifiNetDevice> device);


// Implementations of virtual functions from base class
  virtual void DoInitialize (void);
  virtual void DoSetUp ();
  virtual void DoSetDown ();

private:

  Ptr<WifiNetDevice> m_device; ///< Pointer to the aggregated NetDevice.
};

}   // namespace ns3

#endif /* WIFI_NET_DEVICE_STATE_H */