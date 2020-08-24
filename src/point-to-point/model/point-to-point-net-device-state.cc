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

#include "ns3/point-to-point-net-device-state.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("PointToPointNetDeviceState");

NS_OBJECT_ENSURE_REGISTERED (PointToPointNetDeviceState);

TypeId
PointToPointNetDeviceState::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PointToPointNetDeviceState")
    .SetParent<NetDeviceState> ()
    .SetGroupName ("PointToPoint")
  ;
  return tid;
}

PointToPointNetDeviceState::PointToPointNetDeviceState ()
{
  NS_LOG_FUNCTION (this);
}

void
PointToPointNetDeviceState::DoSetUp ()
{
  NS_LOG_FUNCTION (this);
  Ptr<PointToPointChannel> channel = StaticCast<PointToPointChannel>(m_device->GetChannel ());

  if (channel)
  {
    if(channel->GetNDevices () == 2)
    {
      // Channel is live with devices on either end.
      SetOperationalState (IF_OPER_UP);
    }
  }
  else
  {
    NS_LOG_INFO ("PointToPointNetDeviceState::DoSetUp () : Device is not connected to a channel.");
  }
}

void
PointToPointNetDeviceState::DoSetDown ()
{
  NS_LOG_FUNCTION (this);
  Ptr<Queue<Packet> > queue = m_device->GetQueue ();
  queue->Flush ();  
}

void
PointToPointNetDeviceState::SetDevice (Ptr<PointToPointNetDevice> device)
{
  m_device = device;
}

}