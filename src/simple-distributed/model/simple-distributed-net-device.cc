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
 * Based on SimpleNetDevice by Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "simple-distributed-net-device.h"
#include "simple-distributed-channel.h"
#include "simple-distributed-tag.h"
#include "channel-error-model.h"

#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/error-model.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/tag.h"
#include "ns3/simulator.h"
#include "ns3/queue.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/ethernet-header.h"
#include "ns3/ethernet-trailer.h"
#include "ns3/llc-snap-header.h"
#include "ns3/mobility-model.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimpleDistributedNetDevice");

NS_OBJECT_ENSURE_REGISTERED (SimpleDistributedNetDevice);

TypeId
SimpleDistributedNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimpleDistributedNetDevice")
    .SetParent<NetDevice> ()
    .SetGroupName ("SimpleDistributed")
    .AddConstructor<SimpleDistributedNetDevice> ()
    .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
                   UintegerValue (DEFAULT_MTU),
                   MakeUintegerAccessor (&SimpleDistributedNetDevice::SetMtu,
                                         &SimpleDistributedNetDevice::GetMtu),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("ReceiveErrorModel",
                   "The receiver error model used to simulate packet loss",
                   PointerValue (),
                   MakePointerAccessor (&SimpleDistributedNetDevice::m_receiveErrorModel),
                   MakePointerChecker<ErrorModel> ())
    .AddAttribute ("PointToPointMode",
                   "The device is configured in Point to Point mode",
                   BooleanValue (true),
                   MakeBooleanAccessor (&SimpleDistributedNetDevice::m_pointToPointMode),
                   MakeBooleanChecker ())
    .AddAttribute ("TxQueue",
                   "A queue to use as the transmit queue in the device.",
                   StringValue ("ns3::DropTailQueue<Packet>"),
                   MakePointerAccessor (&SimpleDistributedNetDevice::m_queue),
                   MakePointerChecker<Queue<Packet> > ())
    .AddAttribute ("Delay", "Transmission delay for net device",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&SimpleDistributedNetDevice::m_delay),
                   MakeTimeChecker ())
    .AddAttribute ("DataRate",
                   "The default data rate for net device. Zero means infinite",
                   DataRateValue (DataRate ("0b/s")),
                   MakeDataRateAccessor (&SimpleDistributedNetDevice::m_dataRate),
                   MakeDataRateChecker ())
    .AddAttribute ("InterframeGap",
                   "The time to wait between packet (frame) transmissions",
                   TimeValue (Seconds (0.0)),
                   MakeTimeAccessor (&SimpleDistributedNetDevice::m_tInterframeGap),
                   MakeTimeChecker ())

    //
    // Trace sources at the "top" of the net device, where packets transition
    // to/from higher layers.
    //
    .AddTraceSource ("MacTx", 
                     "Trace source indicating a packet has arrived "
                     "for transmission by this device",
                     MakeTraceSourceAccessor (&SimpleDistributedNetDevice::m_macTxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacTxDrop", 
                     "Trace source indicating a packet has been dropped "
                     "by the device before transmission",
                     MakeTraceSourceAccessor (&SimpleDistributedNetDevice::m_macTxDropTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacPromiscRx", 
                     "A packet has been received by this device, "
                     "has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack.  "
                     "This is a promiscuous trace,",
                     MakeTraceSourceAccessor (&SimpleDistributedNetDevice::m_macPromiscRxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacRx", 
                     "A packet has been received by this device, "
                     "has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack.  "
                     "This is a non-promiscuous trace,",
                     MakeTraceSourceAccessor (&SimpleDistributedNetDevice::m_macRxTrace),
                     "ns3::Packet::TracedCallback")
    //
    // Trace sources designed to simulate a packet sniffer facility (tcpdump).
    //
    .AddTraceSource ("Sniffer",
                     "Trace source simulating a non-promiscuous packet sniffer "
                     "attached to the device",
                     MakeTraceSourceAccessor (&SimpleDistributedNetDevice::m_snifferTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PromiscSniffer",
                     "Trace source simulating a promiscuous packet sniffer "
                     "attached to the device",
                     MakeTraceSourceAccessor (&SimpleDistributedNetDevice::m_promiscSnifferTrace),
                     "ns3::Packet::TracedCallback")

  ;
  return tid;
}

SimpleDistributedNetDevice::SimpleDistributedNetDevice ()
  : m_channel (0),
  m_node (0),
  m_mtu (DEFAULT_MTU),
  m_ifIndex (0),
  m_linkUp (false)
{
  NS_LOG_FUNCTION (this);
}

void
SimpleDistributedNetDevice::Receive (Ptr<Packet> packet, uint16_t protocol,
                                     Mac48Address to, Mac48Address from)
{
  NS_LOG_FUNCTION (this << packet << protocol << to << from);
  NetDevice::PacketType packetType;

  if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt (packet) )
    {
      m_phyRxDropTrace (packet);
      return;
    }

  if (to == m_address)
    {
      packetType = NetDevice::PACKET_HOST;
    }
  else if (to.IsBroadcast ())
    {
      packetType = NetDevice::PACKET_BROADCAST;
    }
  else if (to.IsGroup ())
    {
      packetType = NetDevice::PACKET_MULTICAST;
    }
  else
    {
      packetType = NetDevice::PACKET_OTHERHOST;
    }

  Ptr<Packet> snifferPacket = packet->Copy ();
  AddHeader (snifferPacket, to, from, protocol);

  snifferPacket->RemoveAllPacketTags ();
  snifferPacket->RemoveAllByteTags ();
  m_snifferTrace (snifferPacket);
  m_promiscSnifferTrace (snifferPacket);

  if (packetType != NetDevice::PACKET_OTHERHOST)
    {
      m_macRxTrace (packet);
      m_rxCallback (this, packet, protocol, from);
    }

  if (!m_promiscCallback.IsNull ())
    {
      m_macPromiscRxTrace (packet);
      m_promiscCallback (this, packet, protocol, from, to, packetType);
    }
}

void
SimpleDistributedNetDevice::ReceiveRemote (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);

  SimpleDistributedTag tag;
  packet->PeekPacketTag (tag);

  auto src = tag.GetSrc ();

  // Implement a deterministic tie breaking algorithm. ReceiveRemote
  // calls are scheduled in the order MPI receives the messages this
  // is not deterministic so ties in Time can cause non-deterministic
  // scheduling.  To avoid this, messages are first stored sorted
  // container and the scheduling occurs in the ProcessRemote event
  // after all ReceiveRemote events have been called for the current
  // time step.  This depends on Scheduler being FIFO processing for
  // ties; the ProcessRemote event must be executed after all
  // ReceiveRemote events.
  m_remoteIncoming.insert ({src, packet});

  if (m_remoteIncoming.size () == 1)
    {
      Simulator::ScheduleWithContext (Simulator::GetContext (), Time (0), &SimpleDistributedNetDevice::ProcessRemote, this);
    }
}

void
SimpleDistributedNetDevice::ProcessRemote (void)
{
  NS_LOG_FUNCTION (this);

  // Process packets in a deterministic but BIASED way!
  // Will be sorted by source address.

  // Unbiased algorithm would be to do this in a random ordering.
  for (auto element : m_remoteIncoming)
    {
      auto packet = element.second;

      SimpleDistributedTag tag;
      packet->RemovePacketTag (tag);

      Mac48Address src = tag.GetSrc ();
      Mac48Address dst = tag.GetDst ();
      uint16_t protocol = tag.GetProto ();

      auto srcNodeId = tag.GetSrcId ();
      auto srcPosition = tag.GetSrcPosition ();

      
      if (m_channel -> GetErrorModel () && m_channel -> GetErrorModel ()->IsCorrupt (packet,
                                                                                     srcNodeId,
                                                                                     srcPosition,
                                                                                     this))
        {
          m_phyRxDropTrace (packet);
        }
      else
        {
          auto delay = m_channel -> TransmitDelayReceiveSide(packet,
                                                             srcNodeId,
                                                             srcPosition,
                                                             this);
          
          Simulator::ScheduleWithContext(Simulator::GetContext (), delay, &SimpleDistributedNetDevice::Receive, this, packet, protocol, dst, src);
        }
    }

  m_remoteIncoming.clear ();
}

void
SimpleDistributedNetDevice::SetChannel (Ptr<SimpleDistributedChannel> channel)
{
  NS_LOG_FUNCTION (this << channel);
  m_channel = channel;
  m_channel->Add (this);
  m_linkUp = true;
  m_linkChangeCallbacks ();
}

Ptr<Queue<Packet> >
SimpleDistributedNetDevice::GetQueue () const
{
  NS_LOG_FUNCTION (this);
  return m_queue;
}

void
SimpleDistributedNetDevice::SetQueue (Ptr<Queue<Packet> > q)
{
  NS_LOG_FUNCTION (this << q);
  m_queue = q;
}

void
SimpleDistributedNetDevice::SetReceiveErrorModel (Ptr<ErrorModel> em)
{
  NS_LOG_FUNCTION (this << em);
  m_receiveErrorModel = em;
}

void
SimpleDistributedNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (this << index);
  m_ifIndex = index;
}
uint32_t
SimpleDistributedNetDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION (this);
  return m_ifIndex;
}
Ptr<Channel>
SimpleDistributedNetDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION (this);
  return m_channel;
}
void
SimpleDistributedNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_address = Mac48Address::ConvertFrom (address);
}
Address
SimpleDistributedNetDevice::GetAddress (void) const
{
  //
  // Implicit conversion from Mac48Address to Address
  //
  NS_LOG_FUNCTION (this);
  return m_address;
}
bool
SimpleDistributedNetDevice::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION (this << mtu);
  m_mtu = mtu;
  return true;
}
uint16_t
SimpleDistributedNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mtu;
}
bool
SimpleDistributedNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);
  return m_linkUp;
}
void
SimpleDistributedNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION (this << &callback);
  m_linkChangeCallbacks.ConnectWithoutContext (callback);
}
bool
SimpleDistributedNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_pointToPointMode)
    {
      return false;
    }
  return true;
}
Address
SimpleDistributedNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}
bool
SimpleDistributedNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_pointToPointMode)
    {
      return false;
    }
  return true;
}
Address
SimpleDistributedNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  return Mac48Address::GetMulticast (multicastGroup);
}

Address SimpleDistributedNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address::GetMulticast (addr);
}

bool
SimpleDistributedNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_pointToPointMode)
    {
      return true;
    }
  return false;
}

bool
SimpleDistributedNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

bool
SimpleDistributedNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);

  return SendFrom (packet, m_address, dest, protocolNumber);
}

bool
SimpleDistributedNetDevice::SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << source << dest << protocolNumber);

  //
  // If IsLinkUp() is false it means there is no channel to send any packet 
  // over so we just hit the drop trace on the packet and return an error.
  //
  if (IsLinkUp () == false)
    {
      m_macTxDropTrace (packet);
      return false;
    }
  
  if (packet->GetSize () > GetMtu ())
    {
      return false;
    }
  Mac48Address to = Mac48Address::ConvertFrom (dest);
  Mac48Address from = Mac48Address::ConvertFrom (source);

  Vector srcPosition (0,0,0);
  auto srcNode = GetNode ();
  Ptr<MobilityModel> srcMobilityModel = srcNode->GetObject<MobilityModel> ();
  if (srcMobilityModel)
    {
      srcPosition = srcMobilityModel->GetPosition ();
    }

  SimpleDistributedTag tag(from, srcNode->GetId (), srcPosition, to, protocolNumber);

  // Add packet tag for queued packets
  packet->AddPacketTag (tag);

  m_macTxTrace (packet);

  if (m_queue->Enqueue (packet))
    {
      if (m_queue->GetNPackets () == 1 && !TransmitCompleteEvent.IsRunning ())
        {
          packet = m_queue->Dequeue ();

          // Remove packet tag for queued packet
          packet->RemovePacketTag (tag);

          Mac48Address src = tag.GetSrc ();
          Mac48Address dst = tag.GetDst ();
          uint16_t protocol = tag.GetProto ();

          Ptr<Packet> snifferPacket = packet->Copy ();
          AddHeader (snifferPacket, dst, src, protocolNumber);

          snifferPacket->RemoveAllPacketTags ();
          snifferPacket->RemoveAllByteTags ();
          m_snifferTrace (snifferPacket);
          m_promiscSnifferTrace (snifferPacket);

          Time txTime = m_delay;
          if (m_dataRate > DataRate (0))
            {
              NS_LOG_LOGIC ("Packet Size " << packet->GetSize ());
              txTime += m_dataRate.CalculateBytesTxTime (packet->GetSize ());
            }

          Time txCompleteTime = txTime + m_tInterframeGap;

          NS_LOG_LOGIC ("Sending packet at " << txTime.GetSeconds () << "sec");
          m_channel->Send (packet, protocol, dst, src, this, txTime);

          NS_LOG_LOGIC ("Schedule TransmitCompleteEvent in " << txCompleteTime.GetSeconds () << "sec");
          TransmitCompleteEvent = Simulator::Schedule (txCompleteTime, &SimpleDistributedNetDevice::TransmitComplete, this);
        }
      return true;
    }

  // Enqueue may fail (overflow)
  m_macTxDropTrace (packet);
  return false;
}


void
SimpleDistributedNetDevice::TransmitComplete ()
{
  NS_LOG_FUNCTION (this);

  if (m_queue->GetNPackets () == 0)
    {
      return;
    }

  Ptr<Packet> packet = m_queue->Dequeue ();

  SimpleDistributedTag tag;
  packet->RemovePacketTag (tag);

  Mac48Address src = tag.GetSrc ();
  Mac48Address dst = tag.GetDst ();
  uint16_t proto = tag.GetProto ();

  Ptr<Packet> snifferPacket = packet->Copy ();
  AddHeader (snifferPacket, src, dst, proto);
  snifferPacket->RemoveAllPacketTags ();
  snifferPacket->RemoveAllByteTags ();
  m_snifferTrace (snifferPacket);
  m_promiscSnifferTrace (snifferPacket);

  Time txTime = m_delay;
  if (m_dataRate > DataRate (0))
    {
      txTime += m_dataRate.CalculateBytesTxTime (packet->GetSize ());
    }

  Time txCompleteTime = txTime + m_tInterframeGap;

  NS_LOG_LOGIC ("Sending packet at " << txTime.GetSeconds () << "sec");
  m_channel->Send (packet, proto, dst, src, this, txTime);

  NS_LOG_LOGIC ("Schedule TransmitCompleteEvent in " << txCompleteTime.GetSeconds () << "sec");
  TransmitCompleteEvent = Simulator::Schedule (txCompleteTime, &SimpleDistributedNetDevice::TransmitComplete, this);

  return;
}

Ptr<Node>
SimpleDistributedNetDevice::GetNode (void) const
{
  NS_LOG_FUNCTION (this);
  return m_node;
}
void
SimpleDistributedNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  m_node = node;
}
bool
SimpleDistributedNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_pointToPointMode)
    {
      return false;
    }
  return true;
}
void
SimpleDistributedNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_rxCallback = cb;
}

void
SimpleDistributedNetDevice::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_channel = 0;
  m_node = 0;
  m_receiveErrorModel = 0;
  // SGS SimpleNetDevice was flushing but this causes dropped packets
  // and double counting in flowmon.
  //m_queue->Flush ();
  m_queueInterface = 0;
  if (TransmitCompleteEvent.IsRunning ())
    {
      TransmitCompleteEvent.Cancel ();
    }
  NetDevice::DoDispose ();
}


void
SimpleDistributedNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_promiscCallback = cb;
}

bool
SimpleDistributedNetDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

void
SimpleDistributedNetDevice::AddHeader (Ptr<Packet> p,   Mac48Address source,  Mac48Address dest,  uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (p << source << dest << protocolNumber);

  EthernetHeader header (false);
  header.SetSource (source);
  header.SetDestination (dest);

  EthernetTrailer trailer;

  NS_LOG_LOGIC ("p->GetSize () = " << p->GetSize ());
  NS_LOG_LOGIC ("m_mtu = " << m_mtu);

  NS_LOG_LOGIC ("Encapsulating packet as LLC (length interpretation)");

  LlcSnapHeader llc;
  llc.SetType (protocolNumber);
  p->AddHeader (llc);

  uint16_t lengthType = 0;
  //
  // This corresponds to the length interpretation of the lengthType
  // field but with an LLC/SNAP header added to the payload as in
  // IEEE 802.2
  //
  lengthType = p->GetSize ();

  //
  // All Ethernet frames must carry a minimum payload of 46 bytes.  The
  // LLC SNAP header counts as part of this payload.  We need to padd out
  // if we don't have enough bytes.  These must be real bytes since they
  // will be written to pcap files and compared in regression trace files.
  //
  if (p->GetSize () < 46)
    {
      uint8_t buffer[46];
      memset (buffer, 0, 46);
      Ptr<Packet> padd = Create<Packet> (buffer, 46 - p->GetSize ());
      p->AddAtEnd (padd);
    }

  NS_LOG_LOGIC ("header.SetLengthType (" << lengthType << ")");
  header.SetLengthType (lengthType);
  p->AddHeader (header);

  if (Node::ChecksumEnabled ())
    {
      trailer.EnableFcs (true);
    }
  trailer.CalcFcs (p);
  p->AddTrailer (trailer);
}

DataRate
SimpleDistributedNetDevice::GetDataRate (void)
{
  NS_LOG_FUNCTION (this);
  return m_dataRate;
}

void
SimpleDistributedNetDevice::SetDataRate (DataRate dataRate)
{
  NS_LOG_FUNCTION (this << dataRate);
  m_dataRate = dataRate;
}

Time
SimpleDistributedNetDevice::GetDelay (void)
{
  NS_LOG_FUNCTION (this);
  return m_delay;
}

void
SimpleDistributedNetDevice::SetDelay (Time delay)
{
  NS_LOG_FUNCTION (this << delay);
  m_delay = delay;
}

void
SimpleDistributedNetDevice::SetInterframeGap (Time t)
{
  NS_LOG_FUNCTION (this << t.GetSeconds ());
  m_tInterframeGap = t;
}

} // namespace ns3
