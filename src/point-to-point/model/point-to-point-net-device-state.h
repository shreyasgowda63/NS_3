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

#ifndef POINT_TO_POINT_NET_DEVICE_STATE_H
#define POINT_TO_POINT_NET_DEVICE_STATE_H

#include "ns3/net-device-state.h"

namespace ns3 {

class PointToPointNetDevice;

/**
 * \defgroup point-to-point Point-To-Point Network Device States
 * This class defines device state change behavior of point-to-point
 * network device.
 */

/**
 * \ingroup point-to-point
 * \class PointToPointNetDeviceState
 * \brief Device state changes of point-to-point network link
 *
 */
class PointToPointNetDeviceState : public NetDeviceState
{
public:

/**
 * \brief Get the type ID.
 * \return the object TypeId
 */
  static TypeId GetTypeId (void);

  PointToPointNetDeviceState ();

  virtual void DoSetUp ();

  virtual void DoSetDown ();

  void SetDevice (Ptr<PointToPointNetDevice>);

private:

  Ptr<PointToPointNetDevice> m_device;

};

}   // namespace ns3

#endif /* POINT_TO_POINT_NET_DEVICE_STATE_H */