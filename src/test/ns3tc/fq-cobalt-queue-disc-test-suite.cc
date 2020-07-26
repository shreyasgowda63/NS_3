/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Universita' degli Studi di Napoli Federico II
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
 * Authors: Pasquale Imputato <p.imputato@gmail.com>
 *          Stefano Avallone <stefano.avallone@unina.it>
*/

#include "ns3/test.h"
#include "ns3/simulator.h"
#include "ns3/fq-cobalt-queue-disc.h"
#include "ns3/cobalt-queue-disc.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-packet-filter.h"
#include "ns3/ipv4-queue-disc-item.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-header.h"
#include "ns3/ipv6-packet-filter.h"
#include "ns3/ipv6-queue-disc-item.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/string.h"
#include "ns3/pointer.h"

using namespace ns3;

// Variable to assign m_hash to a new packet's flow
int32_t m_hash;

/**
 * Simple test packet filter able to classify IPv4 packets
 *
 */
class Ipv4FqCobaltTestPacketFilter : public Ipv4PacketFilter {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  Ipv4FqCobaltTestPacketFilter ();
  virtual ~Ipv4FqCobaltTestPacketFilter ();

private:
  virtual int32_t DoClassify (Ptr<QueueDiscItem> item) const;
  virtual bool CheckProtocol (Ptr<QueueDiscItem> item) const;
};

TypeId
Ipv4FqCobaltTestPacketFilter::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ipv4FqCobaltTestPacketFilter")
    .SetParent<Ipv4PacketFilter> ()
    .SetGroupName ("Internet")
    .AddConstructor<Ipv4FqCobaltTestPacketFilter> ()
  ;
  return tid;
}

Ipv4FqCobaltTestPacketFilter::Ipv4FqCobaltTestPacketFilter ()
{
}

Ipv4FqCobaltTestPacketFilter::~Ipv4FqCobaltTestPacketFilter ()
{
}

int32_t
Ipv4FqCobaltTestPacketFilter::DoClassify (Ptr<QueueDiscItem> item) const
{
  return m_hash;
}

bool
Ipv4FqCobaltTestPacketFilter::CheckProtocol (Ptr<QueueDiscItem> item) const
{
  return true;
}

/**
 * This class tests packets for which there is no suitable filter
 */
class FqCobaltQueueDiscNoSuitableFilter : public TestCase
{
public:
  FqCobaltQueueDiscNoSuitableFilter ();
  virtual ~FqCobaltQueueDiscNoSuitableFilter ();

private:
  virtual void DoRun (void);
};

FqCobaltQueueDiscNoSuitableFilter::FqCobaltQueueDiscNoSuitableFilter ()
  : TestCase ("Test packets that are not classified by any filter")
{
}

FqCobaltQueueDiscNoSuitableFilter::~FqCobaltQueueDiscNoSuitableFilter ()
{
}

void
FqCobaltQueueDiscNoSuitableFilter::DoRun (void)
{
  // Packets that cannot be classified by the available filters should be dropped
  Ptr<FqCobaltQueueDisc> queueDisc = CreateObjectWithAttributes<FqCobaltQueueDisc> ("MaxSize", StringValue ("4p"));
  Ptr<Ipv4FqCobaltTestPacketFilter> filter = CreateObject<Ipv4FqCobaltTestPacketFilter> ();
  queueDisc->AddPacketFilter (filter);

  m_hash = -1;
  queueDisc->SetQuantum (1500);
  queueDisc->Initialize ();

  Ptr<Packet> p;
  p = Create<Packet> ();
  Ptr<Ipv6QueueDiscItem> item;
  Ipv6Header ipv6Header;
  Address dest;
  item = Create<Ipv6QueueDiscItem> (p, dest, 0, ipv6Header);
  queueDisc->Enqueue (item);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetNQueueDiscClasses (), 0, "no flow queue should have been created");

  p = Create<Packet> (reinterpret_cast<const uint8_t*> ("hello, world"), 12);
  item = Create<Ipv6QueueDiscItem> (p, dest, 0, ipv6Header);
  queueDisc->Enqueue (item);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetNQueueDiscClasses (), 0, "no flow queue should have been created");

  Simulator::Destroy ();
}

/**
 * This class tests the IP flows separation and the packet limit
 */
class FqCobaltQueueDiscIPFlowsSeparationAndPacketLimit : public TestCase
{
public:
  FqCobaltQueueDiscIPFlowsSeparationAndPacketLimit ();
  virtual ~FqCobaltQueueDiscIPFlowsSeparationAndPacketLimit ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<FqCobaltQueueDisc> queue, Ipv4Header hdr);
};

FqCobaltQueueDiscIPFlowsSeparationAndPacketLimit::FqCobaltQueueDiscIPFlowsSeparationAndPacketLimit ()
  : TestCase ("Test IP flows separation and packet limit")
{
}

FqCobaltQueueDiscIPFlowsSeparationAndPacketLimit::~FqCobaltQueueDiscIPFlowsSeparationAndPacketLimit ()
{
}

void
FqCobaltQueueDiscIPFlowsSeparationAndPacketLimit::AddPacket (Ptr<FqCobaltQueueDisc> queue, Ipv4Header hdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
  queue->Enqueue (item);
}

void
FqCobaltQueueDiscIPFlowsSeparationAndPacketLimit::DoRun (void)
{
  Ptr<FqCobaltQueueDisc> queueDisc = CreateObjectWithAttributes<FqCobaltQueueDisc> ("MaxSize", StringValue ("4p"));

  queueDisc->SetQuantum (1500);
  queueDisc->Initialize ();

  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (7);

  // Add three packets from the first flow
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 3, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the flow queue");

  // Add two packets from the second flow
  hdr.SetDestination (Ipv4Address ("10.10.1.7"));
  // Add the first packet
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 4, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the flow queue");
  // Add the second packet that causes two packets to be dropped from the fat flow (max backlog = 300, threshold = 150)
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 3, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the flow queue");

  Simulator::Destroy ();
}

/**
 * This class tests the deficit per flow
 */
class FqCobaltQueueDiscDeficit : public TestCase
{
public:
  FqCobaltQueueDiscDeficit ();
  virtual ~FqCobaltQueueDiscDeficit ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<FqCobaltQueueDisc> queue, Ipv4Header hdr);
};

FqCobaltQueueDiscDeficit::FqCobaltQueueDiscDeficit ()
  : TestCase ("Test credits and flows status")
{
}

FqCobaltQueueDiscDeficit::~FqCobaltQueueDiscDeficit ()
{
}

void
FqCobaltQueueDiscDeficit::AddPacket (Ptr<FqCobaltQueueDisc> queue, Ipv4Header hdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
  queue->Enqueue (item);
}

void
FqCobaltQueueDiscDeficit::DoRun (void)
{
  Ptr<FqCobaltQueueDisc> queueDisc = CreateObjectWithAttributes<FqCobaltQueueDisc> ();

  queueDisc->SetQuantum (90);
  queueDisc->Initialize ();

  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (7);

  // Add a packet from the first flow
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 1, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the first flow queue");
  Ptr<FqCobaltFlow> flow1 = StaticCast<FqCobaltFlow> (queueDisc->GetQueueDiscClass (0));
  NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), static_cast<int32_t> (queueDisc->GetQuantum ()), "the deficit of the first flow must equal the quantum");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), FqCobaltFlow::NEW_FLOW, "the first flow must be in the list of new queues");
  // Dequeue a packet
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 0, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 0, "unexpected number of packets in the first flow queue");
  // the deficit for the first flow becomes 90 - (100+20) = -30
  NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), -30, "unexpected deficit for the first flow");

  // Add two packets from the first flow
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 2, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), FqCobaltFlow::NEW_FLOW, "the first flow must still be in the list of new queues");

  // Add two packets from the second flow
  hdr.SetDestination (Ipv4Address ("10.10.1.10"));
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 4, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the second flow queue");
  Ptr<FqCobaltFlow> flow2 = StaticCast<FqCobaltFlow> (queueDisc->GetQueueDiscClass (1));
  NS_TEST_ASSERT_MSG_EQ (flow2->GetDeficit (), static_cast<int32_t> (queueDisc->GetQuantum ()), "the deficit of the second flow must equal the quantum");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetStatus (), FqCobaltFlow::NEW_FLOW, "the second flow must be in the list of new queues");

  // Dequeue a packet (from the second flow, as the first flow has a negative deficit)
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 3, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  // the first flow got a quantum of deficit (-30+90=60) and has been moved to the end of the list of old queues
  NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), 60, "unexpected deficit for the first flow");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), FqCobaltFlow::OLD_FLOW, "the first flow must be in the list of old queues");
  // the second flow has a negative deficit (-30) and is still in the list of new queues
  NS_TEST_ASSERT_MSG_EQ (flow2->GetDeficit (), -30, "unexpected deficit for the second flow");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetStatus (), FqCobaltFlow::NEW_FLOW, "the second flow must be in the list of new queues");

  // Dequeue a packet (from the first flow, as the second flow has a negative deficit)
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 2, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  // the first flow has a negative deficit (60-(100+20)= -60) and stays in the list of old queues
  NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), -60, "unexpected deficit for the first flow");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), FqCobaltFlow::OLD_FLOW, "the first flow must be in the list of old queues");
  // the second flow got a quantum of deficit (-30+90=60) and has been moved to the end of the list of old queues
  NS_TEST_ASSERT_MSG_EQ (flow2->GetDeficit (), 60, "unexpected deficit for the second flow");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetStatus (), FqCobaltFlow::OLD_FLOW, "the second flow must be in the list of new queues");

  // Dequeue a packet (from the second flow, as the first flow has a negative deficit)
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 1, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 0, "unexpected number of packets in the second flow queue");
  // the first flow got a quantum of deficit (-60+90=30) and has been moved to the end of the list of old queues
  NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), 30, "unexpected deficit for the first flow");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), FqCobaltFlow::OLD_FLOW, "the first flow must be in the list of old queues");
  // the second flow has a negative deficit (60-(100+20)= -60)
  NS_TEST_ASSERT_MSG_EQ (flow2->GetDeficit (), -60, "unexpected deficit for the second flow");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetStatus (), FqCobaltFlow::OLD_FLOW, "the second flow must be in the list of new queues");

  // Dequeue a packet (from the first flow, as the second flow has a negative deficit)
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 0, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 0, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 0, "unexpected number of packets in the second flow queue");
  // the first flow has a negative deficit (30-(100+20)= -90)
  NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), -90, "unexpected deficit for the first flow");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), FqCobaltFlow::OLD_FLOW, "the first flow must be in the list of old queues");
  // the second flow got a quantum of deficit (-60+90=30) and has been moved to the end of the list of old queues
  NS_TEST_ASSERT_MSG_EQ (flow2->GetDeficit (), 30, "unexpected deficit for the second flow");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetStatus (), FqCobaltFlow::OLD_FLOW, "the second flow must be in the list of new queues");

  // Dequeue a packet
  queueDisc->Dequeue ();
  // the first flow is at the head of the list of old queues but has a negative deficit, thus it gets a quantun
  // of deficit (-90+90=0) and is moved to the end of the list of old queues. Then, the second flow (which has a
  // positive deficit) is selected, but the second flow is empty and thus it is set to inactive. The first flow is
  // reconsidered, but it has a null deficit, hence it gets another quantum of deficit (0+90=90). Then, the first
  // flow is reconsidered again, now it has a positive deficit and hence it is selected. But, it is empty and
  // therefore is set to inactive, too.
  NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), 90, "unexpected deficit for the first flow");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), FqCobaltFlow::INACTIVE, "the first flow must be inactive");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetDeficit (), 30, "unexpected deficit for the second flow");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetStatus (), FqCobaltFlow::INACTIVE, "the second flow must be inactive");

  Simulator::Destroy ();
}

/**
 * This class tests the TCP flows separation
 */
class FqCobaltQueueDiscTCPFlowsSeparation : public TestCase
{
public:
  FqCobaltQueueDiscTCPFlowsSeparation ();
  virtual ~FqCobaltQueueDiscTCPFlowsSeparation ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<FqCobaltQueueDisc> queue, Ipv4Header ipHdr, TcpHeader tcpHdr);
};

FqCobaltQueueDiscTCPFlowsSeparation::FqCobaltQueueDiscTCPFlowsSeparation ()
  : TestCase ("Test TCP flows separation")
{
}

FqCobaltQueueDiscTCPFlowsSeparation::~FqCobaltQueueDiscTCPFlowsSeparation ()
{
}

void
FqCobaltQueueDiscTCPFlowsSeparation::AddPacket (Ptr<FqCobaltQueueDisc> queue, Ipv4Header ipHdr, TcpHeader tcpHdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  p->AddHeader (tcpHdr);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, ipHdr);
  queue->Enqueue (item);
}

void
FqCobaltQueueDiscTCPFlowsSeparation::DoRun (void)
{
  Ptr<FqCobaltQueueDisc> queueDisc = CreateObjectWithAttributes<FqCobaltQueueDisc> ("MaxSize", StringValue ("10p"));

  queueDisc->SetQuantum (1500);
  queueDisc->Initialize ();

  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (6);

  TcpHeader tcpHdr;
  tcpHdr.SetSourcePort (7);
  tcpHdr.SetDestinationPort (27);

  // Add three packets from the first flow
  AddPacket (queueDisc, hdr, tcpHdr);
  AddPacket (queueDisc, hdr, tcpHdr);
  AddPacket (queueDisc, hdr, tcpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 3, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");

  // Add a packet from the second flow
  tcpHdr.SetSourcePort (8);
  AddPacket (queueDisc, hdr, tcpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 4, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");

  // Add a packet from the third flow
  tcpHdr.SetDestinationPort (28);
  AddPacket (queueDisc, hdr, tcpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 5, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the third flow queue");

  // Add two packets from the fourth flow
  tcpHdr.SetSourcePort (7);
  AddPacket (queueDisc, hdr, tcpHdr);
  AddPacket (queueDisc, hdr, tcpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 7, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the third flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (3)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the third flow queue");

  Simulator::Destroy ();
}

/**
 * This class tests the UDP flows separation
 */
class FqCobaltQueueDiscUDPFlowsSeparation : public TestCase
{
public:
  FqCobaltQueueDiscUDPFlowsSeparation ();
  virtual ~FqCobaltQueueDiscUDPFlowsSeparation ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<FqCobaltQueueDisc> queue, Ipv4Header ipHdr, UdpHeader udpHdr);
};

FqCobaltQueueDiscUDPFlowsSeparation::FqCobaltQueueDiscUDPFlowsSeparation ()
  : TestCase ("Test UDP flows separation")
{
}

FqCobaltQueueDiscUDPFlowsSeparation::~FqCobaltQueueDiscUDPFlowsSeparation ()
{
}

void
FqCobaltQueueDiscUDPFlowsSeparation::AddPacket (Ptr<FqCobaltQueueDisc> queue, Ipv4Header ipHdr, UdpHeader udpHdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  p->AddHeader (udpHdr);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, ipHdr);
  queue->Enqueue (item);
}

void
FqCobaltQueueDiscUDPFlowsSeparation::DoRun (void)
{
  Ptr<FqCobaltQueueDisc> queueDisc = CreateObjectWithAttributes<FqCobaltQueueDisc> ("MaxSize", StringValue ("10p"));

  queueDisc->SetQuantum (1500);
  queueDisc->Initialize ();

  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (17);

  UdpHeader udpHdr;
  udpHdr.SetSourcePort (7);
  udpHdr.SetDestinationPort (27);

  // Add three packets from the first flow
  AddPacket (queueDisc, hdr, udpHdr);
  AddPacket (queueDisc, hdr, udpHdr);
  AddPacket (queueDisc, hdr, udpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 3, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");

  // Add a packet from the second flow
  udpHdr.SetSourcePort (8);
  AddPacket (queueDisc, hdr, udpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 4, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");

  // Add a packet from the third flow
  udpHdr.SetDestinationPort (28);
  AddPacket (queueDisc, hdr, udpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 5, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the third flow queue");

  // Add two packets from the fourth flow
  udpHdr.SetSourcePort (7);
  AddPacket (queueDisc, hdr, udpHdr);
  AddPacket (queueDisc, hdr, udpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 7, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the third flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (3)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the third flow queue");

  Simulator::Destroy ();
}

/**
 * This class tests ECN marking
 * Any future classifier options (e.g. SetAssociativehash) should be disabled to prevent a hash collision on this test case.
 */
// class FqCobaltQueueDiscECNMarking : public TestCase
// {
// public:
//   FqCobaltQueueDiscECNMarking ();
//   virtual ~FqCobaltQueueDiscECNMarking ();

// private:
//   virtual void DoRun (void);
//   void AddPacket (Ptr<FqCobaltQueueDisc> queue, Ipv4Header hdr, u_int32_t nPkt, u_int32_t nPktEnqueued, u_int32_t nQueueFlows);
//   void Dequeue (Ptr<FqCobaltQueueDisc> queue, uint32_t nPkt);
//   void DequeueWithDelay (Ptr<FqCobaltQueueDisc> queue, double delay, uint32_t nPkt);
// };

// FqCobaltQueueDiscECNMarking::FqCobaltQueueDiscECNMarking ()
//   : TestCase ("Test ECN marking")
// {
// }

// FqCobaltQueueDiscECNMarking::~FqCobaltQueueDiscECNMarking ()
// {
// }

// void
// FqCobaltQueueDiscECNMarking::AddPacket (Ptr<FqCobaltQueueDisc> queue, Ipv4Header hdr, u_int32_t nPkt, u_int32_t nPktEnqueued, u_int32_t nQueueFlows)
// {
//   Address dest;
//   Ptr<Packet> p = Create<Packet> (100);
//   for (uint32_t i = 0; i < nPkt; i++)
//     {
//       Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
//       queue->Enqueue (item);
//     }
//   NS_TEST_EXPECT_MSG_EQ (queue->GetNQueueDiscClasses (), nQueueFlows, "unexpected number of flow queues");
//   NS_TEST_EXPECT_MSG_EQ (queue->GetNPackets (), nPktEnqueued, "unexpected number of enqueued packets");
// }

// void
// FqCobaltQueueDiscECNMarking::Dequeue (Ptr<FqCobaltQueueDisc> queue, uint32_t nPkt)
// {
//   // Ptr<CobaltQueueDisc> q0 = queue->GetQueueDiscClass (0)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
//   // Ptr<CobaltQueueDisc> q1 = queue->GetQueueDiscClass (1)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
//   // Ptr<CobaltQueueDisc> q2 = queue->GetQueueDiscClass (2)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
//   // Ptr<CobaltQueueDisc> q3 = queue->GetQueueDiscClass (3)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
//   // Ptr<CobaltQueueDisc> q4 = queue->GetQueueDiscClass (4)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
//   // std::cout << q0->GetNPackets () << "  "<< q1->GetNPackets () << "  "<< q2->GetNPackets () << "  "
//   // << q3->GetNPackets () << "  "<< q4->GetNPackets () << "  " << q0->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP)
//   // << q1->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP) << "  "
//   // << q2->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP)<< "  "
//   // << q3->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP)<< "  "
//   // << q4->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP)<< "           "
//   // << q0->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK) << "  "
//   // << q1->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK) << "  "
//   // << q2->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK)<< "  "
//   // << q3->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK)<< "  "
//   // << q4->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK)<< "  "
//   // << std::endl;
//   // std::cout << q0->GetNPackets () << "  " << q1->GetNPackets () << "  " << q2->GetNPackets () << "  "<< q3->GetNPackets ()<<"       "  
//   // <<q0->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP)<< "  " 
//   // <<q1->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP) << "  "
//   // <<q2->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP) << "  "
//   // <<q3->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP) << "          "
//   // << q0->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK) << "  "
//   // << q1->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK) << "  "
//   // << q2->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK) << "  "
//   // << q3->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK) << "  "
//   //  << std::endl;
//   for (uint32_t i = 0; i < nPkt; i++)
//     {
//       Ptr<QueueDiscItem> item = queue->Dequeue ();
//     }
// }

// void
// FqCobaltQueueDiscECNMarking::DequeueWithDelay (Ptr<FqCobaltQueueDisc> queue, double delay, uint32_t nPkt)
// {
//   for (uint32_t i = 0; i < nPkt; i++)
//     {
//       Simulator::Schedule (Time (Seconds ((i + 1) * delay)), &FqCobaltQueueDiscECNMarking::Dequeue, this, queue, 1);
//     }
// }

// void
// FqCobaltQueueDiscECNMarking::DoRun (void)
// {
//   // Test is divided into 3 sub test cases:
//   // 1) CeThreshold disabled
//   // 2) CeThreshold enabled
//   // 3) Same as 2 but with higher queue delay

//   // Test case 1, CeThreshold disabled
//   Ptr<FqCobaltQueueDisc> queueDisc = CreateObjectWithAttributes<FqCobaltQueueDisc> ("MaxSize", StringValue ("10240p"), "UseEcn", BooleanValue (true),
//                                                                                   "Perturbation", UintegerValue (0));

//   queueDisc->SetQuantum (1514);
//   queueDisc->Initialize ();
//   Ipv4Header hdr;
//   hdr.SetPayloadSize (100);
//   hdr.SetSource (Ipv4Address ("10.10.1.1"));
//   hdr.SetDestination (Ipv4Address ("10.10.1.2"));
//   hdr.SetProtocol (7);
//   hdr.SetEcn (Ipv4Header::ECN_ECT0);

//   // Add 20 ECT0 (ECN capable) packets from the first flow
//   Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscECNMarking::AddPacket, this, queueDisc, hdr, 20, 20, 1);

//   // Add 20 ECT0 (ECN capable) packets from second flow
//   hdr.SetDestination (Ipv4Address ("10.10.1.10"));
//   Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscECNMarking::AddPacket, this, queueDisc, hdr, 20, 40, 2);

//   // Add 20 ECT0 (ECN capable) packets from third flow
//   hdr.SetDestination (Ipv4Address ("10.10.1.20"));
//   Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscECNMarking::AddPacket, this, queueDisc, hdr, 20, 60, 3);

//   // Add 20 NotECT packets from fourth flow
//   hdr.SetDestination (Ipv4Address ("10.10.1.30"));
//   hdr.SetEcn (Ipv4Header::ECN_NotECT);
//   Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscECNMarking::AddPacket, this, queueDisc, hdr, 20, 80, 4);

//   // Add 20 NotECT packets from fifth flow
//   hdr.SetDestination (Ipv4Address ("10.10.1.40"));
//   Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscECNMarking::AddPacket, this, queueDisc, hdr, 20, 100, 5);

//   //Dequeue 60 packets with delay 110ms to induce packet drops and keep some remaining packets in each queue
//   DequeueWithDelay (queueDisc, 0.11, 60);
//   Simulator::Run ();
//   Simulator::Stop (Seconds (8.0));
//   Ptr<CobaltQueueDisc> q0 = queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
//   Ptr<CobaltQueueDisc> q1 = queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
//   Ptr<CobaltQueueDisc> q2 = queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
  // Ptr<CobaltQueueDisc> q3 = queueDisc->GetQueueDiscClass (3)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
  // Ptr<CobaltQueueDisc> q4 = queueDisc->GetQueueDiscClass (4)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
  

  //Ensure there are some remaining packets in the flow queues to check for flow queues with ECN capable packets
  // NS_TEST_EXPECT_MSG_NE (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 0, "There should be some remaining packets");
  // NS_TEST_EXPECT_MSG_NE (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 0, "There should be some remaining packets");
  // NS_TEST_EXPECT_MSG_NE (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 0, "There should be some remaining packets");
  // NS_TEST_EXPECT_MSG_NE (queueDisc->GetQueueDiscClass (3)->GetQueueDisc ()->GetNPackets (), 0, "There should be some remaining packets");
  // NS_TEST_EXPECT_MSG_NE (queueDisc->GetQueueDiscClass (4)->GetQueueDisc ()->GetNPackets (), 0, "There should be some remaining packets");

//  // As packets in flow queues are ECN capable
//   NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK), 6, "There should be 6 marked packets"
//                         "with 20 packets, total bytes in the queue = 120 * 20 = 2400. First packet dequeues at 110ms which is greater than"
//                         "test's default target value 5ms. Sojourn time has just gone above target from below, need to stay above for at"
//                         "least q->interval before packet can be dropped. Second packet dequeues at 220ms which is greater than last dequeue"
//                         "time plus q->interval(test default 100ms) so the packet is marked. Third packet dequeues at 330ms and the sojourn"
//                         "time stayed above the target and dropnext value is less than 320 hence the packet is marked. 4 subsequent packets"
//                         "are marked as the sojourn time stays above the target. With 8th dequeue number of bytes in queue = 120 * 12 = 1440"
//                         "which is less m_minBytes(test's default value 1500 bytes) hence the packets stop getting marked");
//   NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
//   NS_TEST_EXPECT_MSG_EQ (q1->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK), 6, "There should be 6 marked packets");
//   NS_TEST_EXPECT_MSG_EQ (q1->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
//   NS_TEST_EXPECT_MSG_EQ (q2->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK), 6, "There should be 6 marked packets");
//   NS_TEST_EXPECT_MSG_EQ (q2->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
  
//   // As packets in flow queues are not ECN capable
//   NS_TEST_EXPECT_MSG_EQ (q3->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 4, "There should be 4 dropped packets"
//                         "with 20 packets, total bytes in the queue = 120 * 20 = 2400. First packet dequeues at 110ms which is greater than"
//                         "test's default target value 5ms. Sojourn time has just gone above target from below, need to stay above for at"
//                         "least q->interval before packet can be dropped. Second packet dequeues at 220ms which is greater than last dequeue"
//                         "time plus q->interval(test default 100ms) so packet is dropped and next is dequeued. 4th packet dequeues at 330ms"
//                         "and the sojourn time stayed above the target and dropnext value is less than 320 hence the packet is dropped and next"
//                         "packet is dequeued. 6th packet dequeues at 440ms and 2 more packets are dropped as dropnext value is increased twice."
//                         "12 Packets remaining in the queue, total number of bytes int the queue = 120 * 12 = 1440 which is less"
//                         "m_minBytes(test's default value 1500 bytes) hence the packets stop getting dropped");
//   NS_TEST_EXPECT_MSG_EQ (q3->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK), 0, "There should not be any marked packets");
//   NS_TEST_EXPECT_MSG_EQ (q4->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 4, "There should be 4 dropped packets");
//   NS_TEST_EXPECT_MSG_EQ (q4->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK), 0, "There should not be any marked packets");
//   // Ensure flow queue 0,1 and 2 have ECN capable packets
//   // Peek () changes the stats of the queue and that is reason to be keep this test at last
//   Ptr<const Ipv4QueueDiscItem> pktQ0 = DynamicCast<const Ipv4QueueDiscItem> (q0->Peek ());
//   NS_TEST_EXPECT_MSG_NE (pktQ0->GetHeader ().GetEcn (), Ipv4Header::ECN_NotECT,"flow queue should have ECT0 packets");
//   Ptr<const Ipv4QueueDiscItem> pktQ1 = DynamicCast<const Ipv4QueueDiscItem> (q1->Peek ());
//   NS_TEST_EXPECT_MSG_NE (pktQ1->GetHeader ().GetEcn (), Ipv4Header::ECN_NotECT,"flow queue should have ECT0 packets");
//   Ptr<const Ipv4QueueDiscItem> pktQ2 = DynamicCast<const Ipv4QueueDiscItem> (q2->Peek ());
//   NS_TEST_EXPECT_MSG_NE (pktQ2->GetHeader ().GetEcn (), Ipv4Header::ECN_NotECT,"flow queue should have ECT0 packets");

//   Simulator::Destroy ();

//   // Test case 2, CeThreshold set to 2ms
//   queueDisc = CreateObjectWithAttributes<FqCobaltQueueDisc> ("MaxSize", StringValue ("10240p"), "UseEcn", BooleanValue (true),
//                                                                                    "CeThreshold", TimeValue (MilliSeconds (2)));
//   queueDisc->SetQuantum (1514);
//   queueDisc->Initialize ();
  
//   // Add 20 ECT0 (ECN capable) packets from first flow
//   hdr.SetDestination (Ipv4Address ("10.10.1.2"));
//   hdr.SetEcn (Ipv4Header::ECN_ECT0);
//   Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscECNMarking::AddPacket, this, queueDisc, hdr, 20, 20, 1);

//   // Add 20 ECT0 (ECN capable) packets from second flow
//   hdr.SetDestination (Ipv4Address ("10.10.1.10"));
//   Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscECNMarking::AddPacket, this, queueDisc, hdr, 20, 40, 2);

//   // Add 20 ECT0 (ECN capable) packets from third flow
//   hdr.SetDestination (Ipv4Address ("10.10.1.20"));
//   Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscECNMarking::AddPacket, this, queueDisc, hdr, 20, 60, 3);

//   // Add 20 NotECT packets from fourth flow
//   hdr.SetDestination (Ipv4Address ("10.10.1.30"));
//   hdr.SetEcn (Ipv4Header::ECN_NotECT);
//   Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscECNMarking::AddPacket, this, queueDisc, hdr, 20, 80, 4);

//   // Add 20 NotECT packets from fifth flow
//   hdr.SetDestination (Ipv4Address ("10.10.1.40"));
//   Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscECNMarking::AddPacket, this, queueDisc, hdr, 20, 100, 5);

//   //Dequeue 60 packets with delay 0.1ms to induce packet drops and keep some remaining packets in each queue
//   DequeueWithDelay (queueDisc, 0.0001, 60);
//   Simulator::Run ();
//   Simulator::Stop (Seconds (8.0));
//   q0 = queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
//   q1 = queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
//   q2 = queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
//   q3 = queueDisc->GetQueueDiscClass (3)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
//   q4 = queueDisc->GetQueueDiscClass (4)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();

//   //Ensure there are some remaining packets in the flow queues to check for flow queues with ECN capable packets
//   NS_TEST_EXPECT_MSG_NE (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 0, "There should be some remaining packets");
//   NS_TEST_EXPECT_MSG_NE (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 0, "There should be some remaining packets");
//   NS_TEST_EXPECT_MSG_NE (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 0, "There should be some remaining packets");
//   NS_TEST_EXPECT_MSG_NE (queueDisc->GetQueueDiscClass (3)->GetQueueDisc ()->GetNPackets (), 0, "There should be some remaining packets");
//   NS_TEST_EXPECT_MSG_NE (queueDisc->GetQueueDiscClass (4)->GetQueueDisc ()->GetNPackets (), 0, "There should be some remaining packets");

//   // As packets in flow queues are ECN capable
//   NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
//   NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK), 0, "There should not be any marked packets"
//                         "with quantum of 1514, 13 packets of size 120 bytes can be dequeued. sojourn time of 13th packet is 1.3ms which is"
//                         "less than CE threshold");
//   NS_TEST_EXPECT_MSG_EQ (q1->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
//   NS_TEST_EXPECT_MSG_EQ (q1->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK), 6, "There should be 6 marked packets"
//                         "with quantum of 1514, 13 packets of size 120 bytes can be dequeued. sojourn time of 8th packet is 2.1ms which is greater"
//                         "than CE threshold and subsequent packet also have sojourn time more 8th packet hence remaining packet are marked.");
//   NS_TEST_EXPECT_MSG_EQ (q2->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
//   NS_TEST_EXPECT_MSG_EQ (q2->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK), 13, "There should be 13 marked packets"
//                         "with quantum of 1514, 13 packets of size 120 bytes can be dequeued and all of them have sojourn time more than CE threshold");

//   // As packets in flow queues are not ECN capable
//   NS_TEST_EXPECT_MSG_EQ (q3->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK), 0, "There should not be any marked packets");
//   NS_TEST_EXPECT_MSG_EQ (q3->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
//   NS_TEST_EXPECT_MSG_EQ (q4->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK), 0, "There should not be any marked packets");
//   NS_TEST_EXPECT_MSG_EQ (q4->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");

//   // Ensure flow queue 0,1 and 2 have ECN capable packets
//   // Peek () changes the stats of the queue and that is reason to be keep this test at last
//   pktQ0 = DynamicCast<const Ipv4QueueDiscItem> (q0->Peek ());
//   NS_TEST_EXPECT_MSG_NE (pktQ0->GetHeader ().GetEcn (), Ipv4Header::ECN_NotECT,"flow queue should have ECT0 packets");
//   pktQ1 = DynamicCast<const Ipv4QueueDiscItem> (q1->Peek ());
//   NS_TEST_EXPECT_MSG_NE (pktQ1->GetHeader ().GetEcn (), Ipv4Header::ECN_NotECT,"flow queue should have ECT0 packets");
//   pktQ2 = DynamicCast<const Ipv4QueueDiscItem> (q2->Peek ());
//   NS_TEST_EXPECT_MSG_NE (pktQ2->GetHeader ().GetEcn (), Ipv4Header::ECN_NotECT,"flow queue should have ECT0 packets");

//   Simulator::Destroy ();

//   // Test case 3, CeThreshold set to 2ms with higher queue delay
//   queueDisc = CreateObjectWithAttributes<FqCobaltQueueDisc> ("MaxSize", StringValue ("10240p"), "UseEcn", BooleanValue (true),
//                                                                                    "CeThreshold", TimeValue (MilliSeconds (2)));
//   queueDisc->SetQuantum (1514);
//   queueDisc->Initialize ();
  
//   // Add 20 ECT0 (ECN capable) packets from first flow
//   hdr.SetDestination (Ipv4Address ("10.10.1.2"));
//   hdr.SetEcn (Ipv4Header::ECN_ECT0);
//   Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscECNMarking::AddPacket, this, queueDisc, hdr, 20, 20, 1);

//   // Add 20 ECT0 (ECN capable) packets from second flow
//   hdr.SetDestination (Ipv4Address ("10.10.1.10"));
//   Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscECNMarking::AddPacket, this, queueDisc, hdr, 20, 40, 2);

//   // Add 20 ECT0 (ECN capable) packets from third flow
//   hdr.SetDestination (Ipv4Address ("10.10.1.20"));
//   Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscECNMarking::AddPacket, this, queueDisc, hdr, 20, 60, 3);

//   // Add 20 NotECT packets from fourth flow
//   hdr.SetDestination (Ipv4Address ("10.10.1.30"));
//   hdr.SetEcn (Ipv4Header::ECN_NotECT);
//   Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscECNMarking::AddPacket, this, queueDisc, hdr, 20, 80, 4);

//   // Add 20 NotECT packets from fifth flow
//   hdr.SetDestination (Ipv4Address ("10.10.1.40"));
//   Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscECNMarking::AddPacket, this, queueDisc, hdr, 20, 100, 5);

//   //Dequeue 60 packets with delay 110ms to induce packet drops and keep some remaining packets in each queue
//   DequeueWithDelay (queueDisc, 0.110, 60);
//   Simulator::Run ();
//   Simulator::Stop (Seconds (8.0));
//   q0 = queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
//   q1 = queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
//   q2 = queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
//   q3 = queueDisc->GetQueueDiscClass (3)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
//   q4 = queueDisc->GetQueueDiscClass (4)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();

//   //Ensure there are some remaining packets in the flow queues to check for flow queues with ECN capable packets
//   NS_TEST_EXPECT_MSG_NE (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 0, "There should be some remaining packets");
//   NS_TEST_EXPECT_MSG_NE (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 0, "There should be some remaining packets");
//   NS_TEST_EXPECT_MSG_NE (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 0, "There should be some remaining packets");
//   NS_TEST_EXPECT_MSG_NE (queueDisc->GetQueueDiscClass (3)->GetQueueDisc ()->GetNPackets (), 0, "There should be some remaining packets");
//   NS_TEST_EXPECT_MSG_NE (queueDisc->GetQueueDiscClass (4)->GetQueueDisc ()->GetNPackets (), 0, "There should be some remaining packets");

//   // As packets in flow queues are ECN capable
//   NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
//   NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK) + 
//                          q0->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK), 20 - q0->GetNPackets (), "Number of CE threshold"
//                         " exceeded marks plus Number of Target exceeded marks should be equal to total number of packets dequeued");
//   NS_TEST_EXPECT_MSG_EQ (q1->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
//   NS_TEST_EXPECT_MSG_EQ (q1->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK) + 
//                          q1->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK), 20 - q1->GetNPackets (), "Number of CE threshold"
//                         " exceeded marks plus Number of Target exceeded marks should be equal to total number of packets dequeued");
//   NS_TEST_EXPECT_MSG_EQ (q2->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
//   NS_TEST_EXPECT_MSG_EQ (q2->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK) + 
//                          q2->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK), 20 - q2->GetNPackets (), "Number of CE threshold"
//                         " exceeded marks plus Number of Target exceeded marks should be equal to total number of packets dequeued");

//   // As packets in flow queues are not ECN capable
//   NS_TEST_EXPECT_MSG_EQ (q3->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK), 0, "There should not be any marked packets");
//   NS_TEST_EXPECT_MSG_EQ (q3->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 4, "There should be 4 dropped packets"
//                          " As queue delay is same as in test case 1, number of dropped packets should also be same");
//   NS_TEST_EXPECT_MSG_EQ (q4->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK), 0, "There should not be any marked packets");
//   NS_TEST_EXPECT_MSG_EQ (q4->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 4, "There should be 4 dropped packets");

//   // Ensure flow queue 0,1 and 2 have ECN capable packets
//   // Peek () changes the stats of the queue and that is reason to be keep this test at last
//   pktQ0 = DynamicCast<const Ipv4QueueDiscItem> (q0->Peek ());
//   NS_TEST_EXPECT_MSG_NE (pktQ0->GetHeader ().GetEcn (), Ipv4Header::ECN_NotECT,"flow queue should have ECT0 packets");
//   pktQ1 = DynamicCast<const Ipv4QueueDiscItem> (q1->Peek ());
//   NS_TEST_EXPECT_MSG_NE (pktQ1->GetHeader ().GetEcn (), Ipv4Header::ECN_NotECT,"flow queue should have ECT0 packets");
//   pktQ2 = DynamicCast<const Ipv4QueueDiscItem> (q2->Peek ());
//   NS_TEST_EXPECT_MSG_NE (pktQ2->GetHeader ().GetEcn (), Ipv4Header::ECN_NotECT,"flow queue should have ECT0 packets");

//   Simulator::Destroy ();
// }

/*
 * This class tests linear probing, collision response, and set
 * creation capability of set associative hashing in FqCodel.
 * We modified DoClassify () and CheckProtocol () so that we could control
 * the hash returned for each packet. In the beginning, we use flow hashes
 * ranging from 0 to 7. These must go into different queues in the same set. 
 * The set number for these is obtained using outerhash, which is 0.  
 * When a new packet arrives with flow hash 1024, outerhash = 0 is obtained
 * and the first set is iteratively searched.
 * The packet is eventually added to queue 0 since the tags of queues 
 * in the set do not match with the hash of the flow. The tag of queue 0 is 
 * updated as 1024. When a packet with hash 1025 arrives, outerhash = 0
 * is obtained and the first set is iteratively searched. 
 * Since there is no match, it is added to queue 0 and the tag of queue 0 is
 * updated to 1025.
 *
 * The variable outerhash stores the nearest multiple of 8 that is lesser than
 * the hash. When a flow hash of 20 arrives, the value of outerhash
 * is 16. Since m_flowIndices[16] wasn’t previously allotted, a new flow
 * is created, and the tag corresponding to this queue is set to 20.
*/

class FqCobaltQueueDiscSetLinearProbing : public TestCase
{
public:
  FqCobaltQueueDiscSetLinearProbing ();
  virtual ~FqCobaltQueueDiscSetLinearProbing ();
private:
  virtual void DoRun (void);
  void AddPacket (Ptr<FqCobaltQueueDisc> queue, Ipv4Header hdr);
};

FqCobaltQueueDiscSetLinearProbing::FqCobaltQueueDiscSetLinearProbing ()
    : TestCase ("Test credits and flows status")
{
}

FqCobaltQueueDiscSetLinearProbing::~FqCobaltQueueDiscSetLinearProbing ()
{
}

void
FqCobaltQueueDiscSetLinearProbing::AddPacket (Ptr<FqCobaltQueueDisc> queue, Ipv4Header hdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
  queue->Enqueue (item);
}

void
FqCobaltQueueDiscSetLinearProbing::DoRun (void)
{
  Ptr<FqCobaltQueueDisc> queueDisc = CreateObjectWithAttributes<FqCobaltQueueDisc> ("EnableSetAssociativeHash", BooleanValue (true));
  queueDisc->SetQuantum (90);
  queueDisc->Initialize ();

  Ptr<Ipv4FqCobaltTestPacketFilter> filter = CreateObject<Ipv4FqCobaltTestPacketFilter> ();
  queueDisc->AddPacketFilter (filter);

  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (7);

  m_hash = 0;
  AddPacket (queueDisc, hdr);
  m_hash = 1;
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  m_hash = 2;
  AddPacket (queueDisc, hdr);
  m_hash = 3;
  AddPacket (queueDisc, hdr);
  m_hash = 4;
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  m_hash = 5;
  AddPacket (queueDisc, hdr);
  m_hash = 6;
  AddPacket (queueDisc, hdr);
  m_hash = 7;
  AddPacket (queueDisc, hdr);
  m_hash = 1024;
  AddPacket (queueDisc, hdr);

  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 11,
                         "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 2,
                         "unexpected number of packets in the first flow queue of set one");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 2,
                         "unexpected number of packets in the second flow queue of set one");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 1,
                         "unexpected number of packets in the third flow queue of set one");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (3)->GetQueueDisc ()->GetNPackets (), 1,
                         "unexpected number of packets in the fourth flow queue of set one");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (4)->GetQueueDisc ()->GetNPackets (), 2,
                         "unexpected number of packets in the fifth flow queue of set one");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (5)->GetQueueDisc ()->GetNPackets (), 1,
                         "unexpected number of packets in the sixth flow queue of set one");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (6)->GetQueueDisc ()->GetNPackets (), 1,
                         "unexpected number of packets in the seventh flow queue of set one");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (7)->GetQueueDisc ()->GetNPackets (), 1,
                         "unexpected number of packets in the eighth flow queue of set one");
  m_hash = 1025;
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3,
                         "unexpected number of packets in the first flow of set one");
  m_hash = 10;
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (8)->GetQueueDisc ()->GetNPackets (), 1,
                         "unexpected number of packets in the first flow of set two");
  Simulator::Destroy ();
}


/**
 * This class tests L4S mode
 * Any future classifier options (e.g. SetAssociativehash) should be disabled to prevent a hash collision on this test case.
 */
class FqCobaltQueueDiscL4sMode : public TestCase
{
public:
  FqCobaltQueueDiscL4sMode ();
  virtual ~FqCobaltQueueDiscL4sMode ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<FqCobaltQueueDisc> queue, Ipv4Header hdr, u_int32_t nPkt);
  void AddPacketWithDelay (Ptr<FqCobaltQueueDisc> queue,Ipv4Header hdr, double delay, uint32_t nPkt);
  void Dequeue (Ptr<FqCobaltQueueDisc> queue, uint32_t nPkt);
  void DequeueWithDelay (Ptr<FqCobaltQueueDisc> queue, double delay, uint32_t nPkt);
};

FqCobaltQueueDiscL4sMode::FqCobaltQueueDiscL4sMode ()
  : TestCase ("Test L4S mode")
{
}

FqCobaltQueueDiscL4sMode::~FqCobaltQueueDiscL4sMode ()
{
}

void
FqCobaltQueueDiscL4sMode::AddPacket (Ptr<FqCobaltQueueDisc> queue, Ipv4Header hdr, uint32_t nPkt)
{
  Address dest;
  Ptr<Packet> p = Create<Packet> (100);
  for (uint32_t i = 0; i < nPkt; i++)
    {
      Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
      queue->Enqueue (item);
    }
}

void
FqCobaltQueueDiscL4sMode::AddPacketWithDelay (Ptr<FqCobaltQueueDisc> queue,Ipv4Header hdr, double delay, uint32_t nPkt)
{
  for (uint32_t i = 0; i < nPkt; i++)
    {
      Simulator::Schedule (Time (Seconds ((i + 1) * delay)), &FqCobaltQueueDiscL4sMode::AddPacket, this, queue, hdr, 1);
    }
}

void
FqCobaltQueueDiscL4sMode::Dequeue (Ptr<FqCobaltQueueDisc> queue, uint32_t nPkt)
{
  for (uint32_t i = 0; i < nPkt; i++)
    {
      Ptr<QueueDiscItem> item = queue->Dequeue ();
    }
}

void
FqCobaltQueueDiscL4sMode::DequeueWithDelay (Ptr<FqCobaltQueueDisc> queue, double delay, uint32_t nPkt)
{
  for (uint32_t i = 0; i < nPkt; i++)
    {
      Simulator::Schedule (Time (Seconds ((i + 1) * delay)), &FqCobaltQueueDiscL4sMode::Dequeue, this, queue, 1);
    }
}

void
FqCobaltQueueDiscL4sMode::DoRun (void)
{
  // Test is divided into 2 sub test cases:
  // 1) Without hash collisions
  // 2) With hash collisions

  // Test case 1, Without hash collisions
  Ptr<FqCobaltQueueDisc> queueDisc = CreateObjectWithAttributes<FqCobaltQueueDisc> ("MaxSize", StringValue ("10240p"), "UseEcn", BooleanValue (true),
                                                                                  "Perturbation", UintegerValue (0), "UseL4s", BooleanValue (true),
                                                                                  "CeThreshold", TimeValue (MilliSeconds (2)));

  queueDisc->SetQuantum (1514);
  queueDisc->Initialize ();
  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (7);
  hdr.SetEcn (Ipv4Header::ECN_ECT1);

  // Add 70 ECT1 (ECN capable) packets from the first flow
  // Set delay = 0.5ms
  double delay = 0.0005;
  Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscL4sMode::AddPacketWithDelay, this, queueDisc, hdr, delay, 70);

  // Add 70 ECT0 (ECN capable) packets from second flow
  hdr.SetEcn (Ipv4Header::ECN_ECT0);
  hdr.SetDestination (Ipv4Address ("10.10.1.10"));
  Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscL4sMode::AddPacketWithDelay, this, queueDisc, hdr, delay, 70);

  //Dequeue 140 packets with delay 1ms
  delay = 0.001;
  DequeueWithDelay (queueDisc, delay, 140);
  Simulator::Run ();
  Simulator::Stop (Seconds (8.0));
  Ptr<CobaltQueueDisc> q0 = queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
  Ptr<CobaltQueueDisc> q1 = queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();

  NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK), 66, "There should be 66 marked packets"
                        "4th packet is enqueued at 2ms and dequeued at 4ms hence the delay of 2ms which not greater than CE threshold"
                        "5th packet is enqueued at 2.5ms and dequeued at 5ms hence the delay of 2.5ms and subsequent packet also do have delay"
                        "greater than CE threshold so all the packets after 4th packet are marked");
  NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
  NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK), 0, "There should not be any marked packets");
  NS_TEST_EXPECT_MSG_EQ (q1->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK), 2, "There should be 2 marked packets. Packets are dequeued"
                        "from q0 first, which leads to delay greater than 5ms for the first dequeue from q1. Because of inactivity (started with high queue delay)"
                        "Cobalt keeps drop_next as now and the next packet is marked. With second dequeue count increases to 2, drop_next becomes now plus around"
                        "70ms which is less than the running time(140), and as the queue delay is persistantly higher than 5ms, second packet is marked.");
  NS_TEST_EXPECT_MSG_EQ (q1->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");

  Simulator::Destroy ();

  // Test case 2, With hash collisions
  queueDisc = CreateObjectWithAttributes<FqCobaltQueueDisc> ("MaxSize", StringValue ("10240p"), "UseEcn", BooleanValue (true),
                                                                                  "Perturbation", UintegerValue (0), "UseL4s", BooleanValue (true),
                                                                                  "CeThreshold", TimeValue (MilliSeconds (2)));

  queueDisc->SetQuantum (1514);
  queueDisc->Initialize ();
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (7);
  hdr.SetEcn (Ipv4Header::ECN_ECT1);

  // Add 70 ECT1 (ECN capable) packets from the first flow
  // Set delay = 1ms
  delay = 0.001;
  Simulator::Schedule (Time (Seconds (0.0005)), &FqCobaltQueueDiscL4sMode::AddPacket, this, queueDisc, hdr, 1);
  Simulator::Schedule (Time (Seconds (0.0005)), &FqCobaltQueueDiscL4sMode::AddPacketWithDelay, this, queueDisc, hdr, delay, 69);

  // Add 70 ECT0 (ECN capable) packets from first flow
  hdr.SetEcn (Ipv4Header::ECN_ECT0);
  Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscL4sMode::AddPacketWithDelay, this, queueDisc, hdr, delay, 70);

  //Dequeue 140 packets with delay 1ms
  DequeueWithDelay (queueDisc, delay, 140);
  Simulator::Run ();
  Simulator::Stop (Seconds (8.0));
  q0 = queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
  q0 = queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();

  NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK), 68, "There should be 68 marked packets"
                        "2nd ECT1 packet is enqueued at 1.5ms and dequeued at 3ms hence the delay of 1.5ms which not greater than CE threshold"
                        "3rd packet is enqueued at 2.5ms and dequeued at 5ms hence the delay of 2.5ms and subsequent packet also do have delay"
                        "greater than CE threshold so all the packets after 2nd packet are marked");
  NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
  NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK), 1, "There should be 1 marked packets");

  Simulator::Destroy ();

}

class FqCobaltQueueDiscTestSuite : public TestSuite
{
public:
  FqCobaltQueueDiscTestSuite ();
};

FqCobaltQueueDiscTestSuite::FqCobaltQueueDiscTestSuite ()
  : TestSuite ("fq-cobalt-queue-disc", UNIT)
{
  AddTestCase (new FqCobaltQueueDiscNoSuitableFilter, TestCase::QUICK);
  AddTestCase (new FqCobaltQueueDiscIPFlowsSeparationAndPacketLimit, TestCase::QUICK);
  AddTestCase (new FqCobaltQueueDiscDeficit, TestCase::QUICK);
  AddTestCase (new FqCobaltQueueDiscTCPFlowsSeparation, TestCase::QUICK);
  AddTestCase (new FqCobaltQueueDiscUDPFlowsSeparation, TestCase::QUICK);
  // AddTestCase (new FqCobaltQueueDiscECNMarking, TestCase::QUICK);
  AddTestCase (new FqCobaltQueueDiscSetLinearProbing, TestCase::QUICK);
  AddTestCase (new FqCobaltQueueDiscL4sMode, TestCase::QUICK);
}

static FqCobaltQueueDiscTestSuite fqCobaltQueueDiscTestSuite;
