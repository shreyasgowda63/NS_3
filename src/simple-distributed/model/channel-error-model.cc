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

#include "channel-error-model.h"

#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/boolean.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/net-device.h"
#include "ns3/vector.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ChannelErrorModel");

NS_OBJECT_ENSURE_REGISTERED (ChannelErrorModel);

TypeId ChannelErrorModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ChannelErrorModel")
    .SetParent<Object> ()
    .SetGroupName ("SimpleDistributed")
    .AddAttribute ("IsEnabled", "Whether this ChannelErrorModel is enabled or not.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&ChannelErrorModel::m_enable),
                   MakeBooleanChecker ())
  ;
  return tid;
}

ChannelErrorModel::ChannelErrorModel () :
  m_enable (true)
{
  NS_LOG_FUNCTION (this);
}

ChannelErrorModel::~ChannelErrorModel ()
{
  NS_LOG_FUNCTION (this);
}

bool
ChannelErrorModel::IsCorrupt (Ptr<Packet> pkt, uint32_t srcId, Vector srcPosition, Ptr<NetDevice> dstNetDevice) const
{
  NS_LOG_FUNCTION (this << pkt << srcId << srcPosition << dstNetDevice);
  bool result;

  result = DoIsCorrupt (pkt, srcId, srcPosition, dstNetDevice);

  return result;
}

void
ChannelErrorModel::Reset (void)
{
  NS_LOG_FUNCTION (this);
  DoReset ();
}

void
ChannelErrorModel::Enable (void)
{
  NS_LOG_FUNCTION (this);
  m_enable = true;
}

void
ChannelErrorModel::Disable (void)
{
  NS_LOG_FUNCTION (this);
  m_enable = false;
}

bool
ChannelErrorModel::IsEnabled (void) const
{
  NS_LOG_FUNCTION (this);
  return m_enable;
}

} // namespace ns3

