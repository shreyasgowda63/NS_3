/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2022 IITP
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
 * Author: Alexander Krotov <krotov@iitp.ru>
 */
#include "packet-loop-helper.h"
#include "ns3/uinteger.h"

#include "ns3/sequence-tag.h"

namespace ns3 {

PacketLoopHelper::PacketLoopHelper (PacketSocketAddress address)
  : m_address{address}
{
  m_sourceFactory.SetTypeId (PacketSocketClient::GetTypeId ());
  m_sinkFactory.SetTypeId (PacketSocketServer::GetTypeId ());
}

void
PacketLoopHelper::SetSourceAttribute (std::string name, const AttributeValue &value)
{
  m_sourceFactory.Set (name, value);
}

void
PacketLoopHelper::SetSinkAttribute (std::string name, const AttributeValue &value)
{
  m_sinkFactory.Set (name, value);
}

/// Adds sequence tags to packets before they are sent.
static void
TxTrace (Ptr<PacketSocketClient> source, Ptr<const Packet> packet, const Address &address)
{
  SequenceTag seqTag;
  seqTag.SetSequence (source->GetSent ());

  packet->AddPacketTag (seqTag);
}

/// Updates maximum number on the sender when a packet is received by the sink.
static void
RxTrace (Ptr<PacketSocketClient> source, Ptr<PacketSocketServer> sink, uint32_t packetsInFlight, Ptr<const Packet> packet, const Address &address)
{
  SequenceTag seqTag;

  if (packet->PeekPacketTag (seqTag))
    {
      source->SetMaxPackets (seqTag.GetSequence () + packetsInFlight);
    }
}

ApplicationContainer
PacketLoopHelper::Install (Ptr<Node> sourceNode, Ptr<Node> sinkNode, uint32_t packetsInFlight)
{
  auto source = m_sourceFactory.Create<PacketSocketClient> ();
  auto sink = m_sinkFactory.Create<PacketSocketServer> ();

  source->SetRemote (m_address);
  source->TraceConnectWithoutContext ("Tx", MakeBoundCallback (TxTrace, source));
  sink->TraceConnectWithoutContext ("Rx", MakeBoundCallback (RxTrace, source, sink, packetsInFlight));

  sourceNode->AddApplication (source);
  sinkNode->AddApplication (sink);

  ApplicationContainer apps;
  apps.Add (source);
  apps.Add (sink);
  return apps;
}

} // namespace ns3
