/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2009 INESC Porto
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: Caleb Bowers <caleb.bowers@nrl.navy.mil>
//

#include "ns3/multicast-flow-probe.h"
#include "ns3/multicast-flow-monitor.h"

namespace ns3 {

/* static */
TypeId MulticastFlowProbe::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MulticastFlowProbe")
    .SetParent<Object> ()
    .SetGroupName ("MulticastFlowMonitor")
    // No AddConstructor because this class has no default constructor.
  ;

  return tid;
}

MulticastFlowProbe::~MulticastFlowProbe ()
{}

MulticastFlowProbe::MulticastFlowProbe (Ptr<MulticastFlowMonitor> multicastFlowMonitor)
  : m_multicastFlowMonitor (multicastFlowMonitor)
{
  m_multicastFlowMonitor->AddProbe (this);
}

void
MulticastFlowProbe::DoDispose (void)
{
  m_multicastFlowMonitor = 0;
  Object::DoDispose ();
}

void
MulticastFlowProbe::AddPacketStats (MulticastFlowId flowId, uint32_t packetSize, Time delayFromFirstProbe, uint32_t nodeId)
{
  MulticastFlowStats &flow = m_stats[flowId];
  flow.delayFromFirstProbeSum[nodeId] += delayFromFirstProbe;
  flow.bytes[nodeId] += packetSize;
  ++flow.packets[nodeId];
}

void
MulticastFlowProbe::AddPacketDropStats (MulticastFlowId flowId, uint32_t packetSize, uint32_t reasonCode, uint32_t nodeId)
{
  MulticastFlowStats &flow = m_stats[flowId];

  if (flow.packetsDropped[nodeId].size () < reasonCode)
    {
      flow.packetsDropped[nodeId].resize (reasonCode, 0);
      flow.bytesDropped[nodeId].resize (reasonCode, 0);
    }
  ++flow.packetsDropped[nodeId][reasonCode];
  flow.bytesDropped[nodeId][reasonCode] += packetSize;
}

MulticastFlowProbe::Stats
MulticastFlowProbe::GetStats () const
{
  return m_stats;
}

}