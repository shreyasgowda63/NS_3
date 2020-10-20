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
 * Authors: Bhaskar Kataria <bhaskar.k7920@gmail.com>
 *          Tom Henderson <tomhend@u.washington.edu> 
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 *          Vivek Jain <jain.vivek.anand@gmail.com>
 *          Ankit Deepak <adadeepak8@gmail.com>
 * 
*/

#include "ns3/test.h"
#include "ns3/simulator.h"
#include "ns3/fq-pie-queue-disc.h"
#include "ns3/pie-queue-disc.h"
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

/**
 * This class tests L4S mode. This test is divided to sub test one without hash collisions and so ECT0 and ECT1 flows are
 * classified into different flows.
 * Sub Test 1
 * 70 packets are enqueued into both the flows with the delay of 0.5ms between two enqueues, and dequeued with the delay of 
 * 1ms between two dequeues.
 * Sub Test 2
 * 140(70 ECT0 + 70 ECT1) packets are enqueued such that ECT1 packets are enqueued at 0.5ms, 1.5ms, 2.5ms and so on, and ECT0 packets are
 * enqueued are enqueued at 1ms, 2ms, 3ms and so on
 * Any future classifier options (e.g. SetAssociativehash) should be disabled to prevent a hash collision on this test case.
 */
class FqPieQueueDiscL4sMode : public TestCase
{
public:
  FqPieQueueDiscL4sMode ();
  virtual ~FqPieQueueDiscL4sMode ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<FqPieQueueDisc> queue, Ipv4Header hdr, u_int32_t nPkt);
  void AddPacketWithDelay (Ptr<FqPieQueueDisc> queue,Ipv4Header hdr, double delay, uint32_t nPkt);
  void Dequeue (Ptr<FqPieQueueDisc> queue, uint32_t nPkt);
  void DequeueWithDelay (Ptr<FqPieQueueDisc> queue, double delay, uint32_t nPkt);
};

FqPieQueueDiscL4sMode::FqPieQueueDiscL4sMode ()
  : TestCase ("Test L4S mode")
{
}

FqPieQueueDiscL4sMode::~FqPieQueueDiscL4sMode ()
{
}

void
FqPieQueueDiscL4sMode::AddPacket (Ptr<FqPieQueueDisc> queue, Ipv4Header hdr, uint32_t nPkt)
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
FqPieQueueDiscL4sMode::AddPacketWithDelay (Ptr<FqPieQueueDisc> queue,Ipv4Header hdr, double delay, uint32_t nPkt)
{
  for (uint32_t i = 0; i < nPkt; i++)
    {
      Simulator::Schedule (Time (Seconds ((i + 1) * delay)), &FqPieQueueDiscL4sMode::AddPacket, this, queue, hdr, 1);
    }
}

void
FqPieQueueDiscL4sMode::Dequeue (Ptr<FqPieQueueDisc> queue, uint32_t nPkt)
{
  for (uint32_t i = 0; i < nPkt; i++)
    {
      Ptr<QueueDiscItem> item = queue->Dequeue ();
    }
}

void
FqPieQueueDiscL4sMode::DequeueWithDelay (Ptr<FqPieQueueDisc> queue, double delay, uint32_t nPkt)
{
  for (uint32_t i = 0; i < nPkt; i++)
    {
      Simulator::Schedule (Time (Seconds ((i + 1) * delay)), &FqPieQueueDiscL4sMode::Dequeue, this, queue, 1);
    }
}

void
FqPieQueueDiscL4sMode::DoRun (void)
{
  // Test is divided into 2 sub test cases:
  // 1) Without hash collisions
  // 2) With hash collisions

  // Test case 1, Without hash collisions
  Ptr<FqPieQueueDisc> queueDisc = CreateObjectWithAttributes<FqPieQueueDisc> ("MaxSize", StringValue ("10240p"), "UseEcn", BooleanValue (true),
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
  Simulator::Schedule (Time (Seconds (0)), &FqPieQueueDiscL4sMode::AddPacketWithDelay, this, queueDisc, hdr, delay, 70);

  // Add 70 ECT0 (ECN capable) packets from second flow
  hdr.SetEcn (Ipv4Header::ECN_ECT0);
  hdr.SetDestination (Ipv4Address ("10.10.1.10"));
  Simulator::Schedule (Time (Seconds (0)), &FqPieQueueDiscL4sMode::AddPacketWithDelay, this, queueDisc, hdr, delay, 70);

  //Dequeue 140 packets with delay 1ms
  delay = 0.001;
  DequeueWithDelay (queueDisc, delay, 140);
  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  
  Ptr<PieQueueDisc> q0 = queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetObject <PieQueueDisc> ();
  Ptr<PieQueueDisc> q1 = queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetObject <PieQueueDisc> ();

  NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNMarkedPackets (PieQueueDisc::CE_THRESHOLD_EXCEEDED_MARK), 66, "There should be 66 marked packets"
                        "4th packet is enqueued at 2ms and dequeued at 4ms hence the delay of 2ms which not greater than CE threshold"
                        "5th packet is enqueued at 2.5ms and dequeued at 5ms hence the delay of 2.5ms and subsequent packet also do have delay"
                        "greater than CE threshold so all the packets after 4th packet are marked");
  NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNDroppedPackets (PieQueueDisc::UNFORCED_DROP), 0, "Queue delay is less than max burst allowance so"
                        "There should not be any dropped packets");
  NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNMarkedPackets (PieQueueDisc::UNFORCED_MARK), 0, "There should not be any marked packets");
  NS_TEST_EXPECT_MSG_EQ (q1->GetStats ().GetNMarkedPackets (PieQueueDisc::UNFORCED_MARK), 0, "There should not be marked packets.");
  NS_TEST_EXPECT_MSG_EQ (q1->GetStats ().GetNDroppedPackets (PieQueueDisc::UNFORCED_DROP), 0, "There should not be any dropped packets");

  Simulator::Destroy ();

  // Test case 2, With hash collisions
  queueDisc = CreateObjectWithAttributes<FqPieQueueDisc> ("MaxSize", StringValue ("10240p"), "UseEcn", BooleanValue (true),
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
  Simulator::Schedule (Time (Seconds (0.0005)), &FqPieQueueDiscL4sMode::AddPacket, this, queueDisc, hdr, 1);
  Simulator::Schedule (Time (Seconds (0.0005)), &FqPieQueueDiscL4sMode::AddPacketWithDelay, this, queueDisc, hdr, delay, 69);

  // Add 70 ECT0 (ECN capable) packets from first flow
  hdr.SetEcn (Ipv4Header::ECN_ECT0);
  Simulator::Schedule (Time (Seconds (0)), &FqPieQueueDiscL4sMode::AddPacketWithDelay, this, queueDisc, hdr, delay, 70);

  //Dequeue 140 packets with delay 1ms
  DequeueWithDelay (queueDisc, delay, 140);
  Simulator::Stop (Seconds (1.0));
  Simulator::Run ();
  q0 = queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetObject <PieQueueDisc> ();
  q0 = queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetObject <PieQueueDisc> ();

  NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNMarkedPackets (PieQueueDisc::CE_THRESHOLD_EXCEEDED_MARK), 68, "There should be 68 marked packets"
                        "2nd ECT1 packet is enqueued at 1.5ms and dequeued at 3ms hence the delay of 1.5ms which not greater than CE threshold"
                        "3rd packet is enqueued at 2.5ms and dequeued at 5ms hence the delay of 2.5ms and subsequent packet also do have delay"
                        "greater than CE threshold so all the packets after 2nd packet are marked");
  NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNDroppedPackets (PieQueueDisc::UNFORCED_DROP), 0, "Queue delay is less than max burst allowance so"
                        "There should not be any dropped packets");
  NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNMarkedPackets (PieQueueDisc::UNFORCED_MARK), 0, "There should not be any marked packets");

  Simulator::Destroy ();
}

class FqPieQueueDiscTestSuite : public TestSuite
{
public:
  FqPieQueueDiscTestSuite ();
};

FqPieQueueDiscTestSuite::FqPieQueueDiscTestSuite ()
  : TestSuite ("fq-pie-queue-disc", UNIT)
{
  AddTestCase (new FqPieQueueDiscL4sMode, TestCase::QUICK);
}

static FqPieQueueDiscTestSuite fqPieQueueDiscTestSuite;
