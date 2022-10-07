/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 Emmanuelle Laprise
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
 * Author: Emmanuelle Laprise <emmanuelle.laprise@bluekazoo.ca>
 */

#include "csma-channel.h"

#include "csma-net-device.h"
#include "csma-net-device-state.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CsmaChannel");

NS_OBJECT_ENSURE_REGISTERED (CsmaChannel);

TypeId
CsmaChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CsmaChannel")
    .SetParent<Channel> ()
    .SetGroupName ("Csma")
    .AddConstructor<CsmaChannel> ()
    .AddAttribute ("DataRate",
                   "The transmission data rate to be provided to devices connected to the channel",
                   DataRateValue (DataRate (0xffffffff)),
                   MakeDataRateAccessor (&CsmaChannel::m_bps),
                   MakeDataRateChecker ())
    .AddAttribute ("Delay", "Transmission delay through the channel",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&CsmaChannel::m_delay),
                   MakeTimeChecker ())
  ;
  return tid;
}

CsmaChannel::CsmaChannel ()
  :
    Channel ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_state = IDLE;
  m_deviceList.clear ();
}

CsmaChannel::~CsmaChannel ()
{
  NS_LOG_FUNCTION (this);
  m_deviceList.clear ();
}

uint32_t
CsmaChannel::Attach (Ptr<CsmaNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT (device);

  Ptr<CsmaNetDeviceState> csmaNetDeviceState = device->GetObject<CsmaNetDeviceState> ();

  CsmaDeviceRec rec (device);

  uint32_t id = m_deviceList.size ();
  if (!m_removedDeviceIds.empty ())
    {
      id = m_removedDeviceIds.front ();
      m_removedDeviceIds.pop_front ();
    }

  m_deviceList[id] = rec;

  // This device is up whenever a channel is attached to it.

  csmaNetDeviceState->SetOperationalState (NetDeviceState::IF_OPER_UP);

  // The channel provides us with the transmitter data rate.
  device->m_bps = GetDataRate ();
  // We use the Ethernet interframe gap of 96 bit times.
  device->m_tInterframeGap = GetDataRate ().CalculateBytesTxTime (96 / 8);

  return (id);
}

bool
CsmaChannel::Reattach (Ptr<CsmaNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT (device);

  Ptr<CsmaNetDeviceState> csmaNetDeviceState = device->GetObject<CsmaNetDeviceState> ();
  for (auto it = m_deviceList.begin (); it != m_deviceList.end ( ); it++)
    {
      if (it->second.devicePtr == device)
        {
          if (!it->second.active)
            {
              it->second.active = true;
              csmaNetDeviceState->SetOperationalState (NetDeviceState::IF_OPER_UP);
              return true;
            }
          else
            {
              return false;
            }
        }
    }
  return false;
}

bool
CsmaChannel::Reattach (uint32_t deviceId)
{
  NS_LOG_FUNCTION (this << deviceId);

  auto iter = m_deviceList.find (deviceId);
  if (iter == m_deviceList.end ())
    {
      return false;
    }

  if (m_deviceList[deviceId].active)
    {
      return false;
    }
  else
    {
      Ptr<CsmaNetDevice> device = m_deviceList[deviceId].devicePtr;
      Ptr<CsmaNetDeviceState> csmaNetDeviceState = device->GetObject<CsmaNetDeviceState> ();
      m_deviceList[deviceId].active = true;
      csmaNetDeviceState->SetOperationalState (NetDeviceState::IF_OPER_UP);
      return true;
    }
}

bool
CsmaChannel::Detach (uint32_t deviceId)
{
  NS_LOG_FUNCTION (this << deviceId);

  auto iter = m_deviceList.find (deviceId);
  if (iter == m_deviceList.end ())
    {
      NS_LOG_WARN ("CsmaChannel::Detach(): Can not find Device (" << deviceId << ")");
      return false;
    }
  Ptr<CsmaNetDevice> device = m_deviceList[deviceId].devicePtr;
  Ptr<CsmaNetDeviceState> csmaNetDeviceState = device->GetObject<CsmaNetDeviceState> ();

  if (!m_deviceList[deviceId].active)
    {
      NS_LOG_WARN ("CsmaChannel::Detach(): Device is already detached (" << deviceId << ")");
      return false;
    }

  m_deviceList[deviceId].active = false;

  if ((m_state == TRANSMITTING) && (m_currentSrc == deviceId))
    {
      NS_LOG_WARN ("CsmaChannel::Detach(): Device is currently" << "transmitting (" << deviceId << ")");
    }

  csmaNetDeviceState->SetOperationalState (NetDeviceState::IF_OPER_DOWN);
  return true;
}

bool
CsmaChannel::Detach (Ptr<CsmaNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT (device);

  Ptr<CsmaNetDeviceState> csmaNetDeviceState = device->GetObject<CsmaNetDeviceState> ();

  for (auto it = m_deviceList.begin (); it != m_deviceList.end (); it++)
    {
      if ((it->second.devicePtr == device))
        {
          if (!it->second.active)
            {
              NS_LOG_WARN ("CsmaChannel::Detach(): Device is already detached (" << it->first << ")");
              return false;
            }

          if ((m_state == TRANSMITTING) && (m_currentSrc == it->first))
            {
              NS_LOG_WARN ("CsmaChannel::Detach(): Device is currently" << "transmitting (" << it->first << ")");
            }

          it->second.active = false;
          csmaNetDeviceState->SetOperationalState (NetDeviceState::IF_OPER_DOWN);
          return true;
        }
    }
  NS_LOG_WARN ("CsmaChannel::Detach(): Can not find Device (" << device << ")");
  return false;
}

bool
CsmaChannel::Remove (uint32_t deviceId)
{
  NS_LOG_FUNCTION (this << deviceId);

  auto iter = m_deviceList.find (deviceId);
  if (iter == m_deviceList.end ())
    {
      NS_LOG_WARN ("CsmaChannel::Remove(): Can not find Device (" << deviceId << ")");
      return false;
    }

  Ptr<CsmaNetDevice> device = m_deviceList[deviceId].devicePtr;
  Ptr<CsmaNetDeviceState> csmaNetDeviceState = device->GetObject<CsmaNetDeviceState> ();

  if ((m_state == TRANSMITTING) && (m_currentSrc == deviceId))
    {
      NS_LOG_WARN ("CsmaChannel::Remove(): Device is currently transmitting (" << deviceId << ")");
    }
  
  csmaNetDeviceState->SetOperationalState (NetDeviceState::IF_OPER_DOWN);
  m_removedDeviceIds.push_back (deviceId);
  m_deviceList.erase (iter);
  return true;
}

bool
CsmaChannel::Remove (Ptr<CsmaNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT (device != 0);

  Ptr<CsmaNetDeviceState> csmaNetDeviceState = device->GetObject<CsmaNetDeviceState> ();

  for (auto it = m_deviceList.begin (); it != m_deviceList.end (); it++)
    {
      if ((it->second.devicePtr == device))
        {
          if ((m_state == TRANSMITTING) && (m_currentSrc == it->first))
            {
              NS_LOG_WARN ("CsmaChannel::Remove(): Device is currently transmitting (" << it->first << ")");
            }

          csmaNetDeviceState->SetOperationalState (NetDeviceState::IF_OPER_DOWN);
          uint32_t deviceId = it->first;
          m_removedDeviceIds.push_back (deviceId);
          m_deviceList.erase (deviceId);
          
          return true;
        }
    }
  NS_LOG_WARN ("CsmaChannel::Remove(): Can not find Device (" << device << ")");

  return false;
}

bool
CsmaChannel::TransmitStart (Ptr<const Packet> p, uint32_t srcId)
{
  NS_LOG_FUNCTION (this << p << srcId);
  NS_LOG_INFO ("UID is " << p->GetUid () << ")");

  if (m_state != IDLE)
    {
      NS_LOG_WARN ("CsmaChannel::TransmitStart(): State is not IDLE");
      return false;
    }

  if (!IsActive (srcId))
    {
      NS_LOG_ERROR ("CsmaChannel::TransmitStart(): Selected source is not currently attached to network");
      return false;
    }

  NS_LOG_LOGIC ("switch to TRANSMITTING");
  m_currentPkt = p->Copy ();
  m_currentSrc = srcId;
  m_state = TRANSMITTING;
  return true;
}

bool
CsmaChannel::IsActive (uint32_t deviceId)
{
  auto iter = m_deviceList.find (deviceId);
  if (iter == m_deviceList.end ())
    {
      return false;
    }
  return (iter->second.active);
}

bool
CsmaChannel::TransmitEnd ()
{
  NS_LOG_FUNCTION (this << m_currentPkt << m_currentSrc);
  NS_LOG_INFO ("UID is " << m_currentPkt->GetUid () << ")");

  NS_ASSERT (m_state == TRANSMITTING);
  m_state = PROPAGATING;

  // schedule the channel to go back to IDLE
  Simulator::Schedule (m_delay, &CsmaChannel::PropagationCompleteEvent, this);

  if (!IsActive (m_currentSrc))
    {
      NS_LOG_ERROR ("CsmaChannel::TransmitEnd(): Selected source was detached or removed before the end of the transmission");
      return false;
    }

  NS_LOG_LOGIC ("Schedule event in " << m_delay.As (Time::S));


  NS_LOG_LOGIC ("Receive");

  uint32_t devId = 0;
  for (auto it = m_deviceList.begin (); it != m_deviceList.end (); it++)
    {
      if (it->second.IsActive ())
        {
          // schedule reception events
          Simulator::ScheduleWithContext (it->second.devicePtr->GetNode ()->GetId (),
                                          m_delay,
                                          &CsmaNetDevice::Receive, it->second.devicePtr,
                                          m_currentPkt->Copy (), m_deviceList[m_currentSrc].devicePtr);
        }
    }

  return true;
}

void
CsmaChannel::PropagationCompleteEvent ()
{
  NS_LOG_FUNCTION (this << m_currentPkt);
  NS_LOG_INFO ("UID is " << m_currentPkt->GetUid () << ")");

  NS_ASSERT (m_state == PROPAGATING);
  m_state = IDLE;
}

uint32_t
CsmaChannel::GetNumActDevices (void)
{
  int numActDevices = 0;
  for (auto it = m_deviceList.begin (); it != m_deviceList.end (); it++)
    {
      if (it->second.active)
        {
          numActDevices++;
        }
    }
  return numActDevices;
}

std::size_t
CsmaChannel::GetNDevices (void) const
{
  return m_deviceList.size ();
}

Ptr<CsmaNetDevice>
CsmaChannel::GetCsmaDevice (uint32_t i) const
{
  Ptr<CsmaNetDevice> foundDev;

  auto iter = m_deviceList.find (i);
  if (iter == m_deviceList.end ())
    {
      NS_LOG_WARN ("CsmaChannel::Detach(): Can not find Device (" << i << ")");
      return foundDev;
    }

  return iter->second.devicePtr;
}

int32_t
CsmaChannel::GetDeviceNum (Ptr<CsmaNetDevice> device)
{
  for (auto it = m_deviceList.begin (); it != m_deviceList.end (); it++)
    {
      if (it->second.devicePtr == device)
        {
          if (it->second.active)
            {
              return it->first;
            }
          else
            {
              return -2;
            }
        }
    }
  return -1;
}

bool
CsmaChannel::IsBusy (void)
{
  if (m_state == IDLE)
    {
      return false;
    }
  else
    {
      return true;
    }
}

DataRate
CsmaChannel::GetDataRate (void)
{
  return m_bps;
}

Time
CsmaChannel::GetDelay (void)
{
  return m_delay;
}

WireState
CsmaChannel::GetState (void)
{
  return m_state;
}

Ptr<NetDevice>
CsmaChannel::GetDevice (std::size_t i) const
{
  return GetCsmaDevice (i);
}

CsmaDeviceRec::CsmaDeviceRec ()
{
  active = false;
}

CsmaDeviceRec::CsmaDeviceRec (Ptr<CsmaNetDevice> device)
{
  devicePtr = device;
  active = true;
}

CsmaDeviceRec::CsmaDeviceRec (CsmaDeviceRec const &deviceRec)
{
  devicePtr = deviceRec.devicePtr;
  active = deviceRec.active;
}

bool
CsmaDeviceRec::IsActive ()
{
  return active;
}

} // namespace ns3
