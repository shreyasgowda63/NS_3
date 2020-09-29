/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *  Copyright 2020. Lawrence Livermore National Security, LLC.
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
 * Author: Steven Smith <smith84@llnl.gov>
 *
 */

#include "channel-delay-model.h"

#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/boolean.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/net-device.h"
#include "ns3/channel.h"
#include "ns3/vector.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ChannelDelayModel");

NS_OBJECT_ENSURE_REGISTERED (ChannelDelayModel);

TypeId ChannelDelayModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ChannelDelayModel")
    .SetParent<Object> ()
    .SetGroupName ("SimpleDistributed")
    .AddAttribute ("IsEnabled", "Whether this ChannelDelayModel is enabled or not.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&ChannelDelayModel::m_enable),
                   MakeBooleanChecker ())
  ;
  return tid;
}

ChannelDelayModel::ChannelDelayModel () :
  m_enable (true)
{
  NS_LOG_FUNCTION (this);
}

ChannelDelayModel::~ChannelDelayModel ()
{
  NS_LOG_FUNCTION (this);
}

Time
ChannelDelayModel::ComputeDelay (Ptr<Packet> pkt, uint32_t srcId, Vector srcPosition, Ptr<NetDevice> dstNetDevice) const
{
  NS_LOG_FUNCTION (this << pkt << srcPosition << dstNetDevice);
  return DoComputeDelay (pkt, srcId, srcPosition, dstNetDevice);
}

Time
ChannelDelayModel::GetMinimumDelay (void) const
{
  NS_LOG_FUNCTION (this);
  return DoGetMinimumDelay ();
}

void
ChannelDelayModel::Reset (void)
{
  NS_LOG_FUNCTION (this);
  DoReset ();
}

void
ChannelDelayModel::Enable (void)
{
  NS_LOG_FUNCTION (this);
  m_enable = true;
}

void
ChannelDelayModel::Disable (void)
{
  NS_LOG_FUNCTION (this);
  m_enable = false;
}

bool
ChannelDelayModel::IsEnabled (void) const
{
  NS_LOG_FUNCTION (this);
  return m_enable;
}

} // namespace ns3

