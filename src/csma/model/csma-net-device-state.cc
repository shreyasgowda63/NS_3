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

#include "ns3/csma-net-device-state.h"
#include "ns3/csma-channel.h"
#include "ns3/csma-net-device.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CsmaNetDeviceState");

NS_OBJECT_ENSURE_REGISTERED (CsmaNetDeviceState);

TypeId
CsmaNetDeviceState::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CsmaNetDeviceState")
    .SetParent<NetDeviceState> ()
    .SetGroupName ("Csma")
    .AddConstructor<CsmaNetDeviceState> ()
  ;
  return tid;
}

CsmaNetDeviceState::CsmaNetDeviceState ()
{
  NS_LOG_FUNCTION (this);
  SetUp ();
}

void
CsmaNetDeviceState::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  m_stateChangeTrace (IsUp (), GetOperationalState ());
  NetDeviceState::DoInitialize ();
}

void
CsmaNetDeviceState::DoSetUp ()
{
  NS_LOG_FUNCTION (this);
  if (!m_device->GetChannel ())
  {
    NS_LOG_WARN ("Channel not found.");
    // Channel not found. No need to check for operational state for now.
    return;
  }
  Ptr<CsmaChannel> csmaChannel = DynamicCast<CsmaChannel> (m_device->GetChannel ());
  // Check whether the device is active in channel record. If yes make device operational.
  if (csmaChannel->IsActive (m_device->GetDeviceId ()))
    {
      SetOperationalState (IF_OPER_UP);
    }  
}

void
CsmaNetDeviceState::DoSetDown ()
{
  NS_LOG_FUNCTION (this);
  //Clear packet queue
  Ptr<Queue<Packet> > queue = m_device->GetQueue ();
  queue->Flush ();
  NS_LOG_LOGIC ("Device queue of " << m_device << " flushed.");
}

void
CsmaNetDeviceState::SetDevice (Ptr<CsmaNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  m_device = device;
}

}
