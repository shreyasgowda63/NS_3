/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 NITK Surathkal
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

#ifndef CSMA_NET_DEVICE_STATE_H
#define CSMA_NET_DEVICE_STATE_H

#include "ns3/net-device-state.h"

namespace ns3 {

class CsmaNetDevice;

class CsmaNetDeviceState : public NetDeviceState
{
public:

/**
 * \brief Get the type ID.
 * \return the object TypeId
 */
  static TypeId GetTypeId (void);

  CsmaNetDeviceState ();

  virtual void DoInitialize (void);

  /**
   * \brief Set the pointer to the NetDevice that this class is aggregated to.
   * 
   * \param device Pointer to the CsmaNetDevice which this class is aggregated to.
   */
  void SetDevice (Ptr<CsmaNetDevice> device);

  //
  // The follwing methods are inherited from NetDeviceState base class.
  //

  /**
   * \brief Prepares a device for use. Adminsitrative state of the device is set true indicating
   *  that the device is enabled for use. If the device is plugged in to the channel,
   *  operational state is set to CSMA_OPER_STATE_UP as well.
   */
  virtual void DoSetUp ();

  /**
   * \brief Shutsdown a device.
   *  Queue is flushed and operational state of the device is set to CSMA_OPER_STATE_DOWN.
   */
  virtual void DoSetDown ();

private:

  /**
   * Pointer to CsmaNetDevice to which this class is aggragated to.
   */
  Ptr<CsmaNetDevice> m_device;

};

}

#endif /* CSMA_NET_DEVICE_STATE_H */
