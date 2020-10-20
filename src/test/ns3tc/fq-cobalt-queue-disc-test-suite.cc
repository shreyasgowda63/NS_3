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
/**
 * This class tests ECN marking
 * The test is divided into 3 sub test cases.
 * 1) CE threshold disabled
 * This test enqueues 100 packets in the beginning of the test and dequeues 60 (some packets are dropped too) packets with the
 * delay of 110ms. This test checks that ECT0 packets are marked and are marked appropriately and NotECT packets are dropped.
 * 
 * 2) CE threshold enabled.
 * This test enqueues 100 packets in the beginning of the test and dequeues 60 packets with delay of 1ms. This test checks that
 * the ECT0 packets are marked appropriately at CE threshold.
 * 
 * 3) CE threshold enabled with higher queue delay.
 * This test is similar to the 2nd sub test cases just with higher queue delay and aims to test that the packets are not
 * marked twice
 * Any future classifier options (e.g. SetAssociativehash) should be disabled to prevent a hash collision on this test case.
 */
class FqCobaltQueueDiscEcnMarking : public TestCase
{
public:
  FqCobaltQueueDiscEcnMarking ();
  virtual ~FqCobaltQueueDiscEcnMarking ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<FqCobaltQueueDisc> queue, Ipv4Header hdr, u_int32_t nPkt, u_int32_t nPktEnqueued, u_int32_t nQueueFlows);
  void Dequeue (Ptr<FqCobaltQueueDisc> queue, uint32_t nPkt);
  void DequeueWithDelay (Ptr<FqCobaltQueueDisc> queue, double delay, uint32_t nPkt);
  void DropNextTracer (int64_t oldVal, int64_t newVal);
  uint32_t m_dropNextCount;    ///< count the number of times m_dropNext is recalculated
};

FqCobaltQueueDiscEcnMarking::FqCobaltQueueDiscEcnMarking ()
  : TestCase ("Test ECN marking")
{
  m_dropNextCount = 0;
}

FqCobaltQueueDiscEcnMarking::~FqCobaltQueueDiscEcnMarking ()
{
}

void
FqCobaltQueueDiscEcnMarking::AddPacket (Ptr<FqCobaltQueueDisc> queue, Ipv4Header hdr, u_int32_t nPkt, u_int32_t nPktEnqueued, u_int32_t nQueueFlows)
{
  Address dest;
  Ptr<Packet> p = Create<Packet> (100);
  for (uint32_t i = 0; i < nPkt; i++)
    {
      Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
      queue->Enqueue (item);
    }
  NS_TEST_EXPECT_MSG_EQ (queue->GetNQueueDiscClasses (), nQueueFlows, "unexpected number of flow queues");
  NS_TEST_EXPECT_MSG_EQ (queue->GetNPackets (), nPktEnqueued, "unexpected number of enqueued packets");
}

void
FqCobaltQueueDiscEcnMarking::Dequeue (Ptr<FqCobaltQueueDisc> queue, uint32_t nPkt)
{
  Ptr<CobaltQueueDisc> q3 = queue->GetQueueDiscClass (3)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();

  // Trace DropNext after the first dequeue as m_dropNext value is set after the first dequeue
  if (q3->GetNPackets () == 19)
    {
      q3->TraceConnectWithoutContext ("DropNext", MakeCallback (&FqCobaltQueueDiscEcnMarking::DropNextTracer, this));
    }

  for (uint32_t i = 0; i < nPkt; i++)
    {
      Ptr<QueueDiscItem> item = queue->Dequeue ();
    }
}

void
FqCobaltQueueDiscEcnMarking::DequeueWithDelay (Ptr<FqCobaltQueueDisc> queue, double delay, uint32_t nPkt)
{
  for (uint32_t i = 0; i < nPkt; i++)
    {
      Simulator::Schedule (Time (Seconds ((i + 1) * delay)), &FqCobaltQueueDiscEcnMarking::Dequeue, this, queue, 1);
    }
}

void
FqCobaltQueueDiscEcnMarking::DropNextTracer (int64_t oldVal, int64_t newVal)
{
  NS_UNUSED (oldVal);
  NS_UNUSED (newVal);
  m_dropNextCount++;
}

void
FqCobaltQueueDiscEcnMarking::DoRun (void)
{
  // Test is divided into 3 sub test cases:
  // 1) CeThreshold disabled
  // 2) CeThreshold enabled
  // 3) Same as 2 but with higher queue delay

  // Test case 1, CeThreshold disabled
  Ptr<FqCobaltQueueDisc> queueDisc = CreateObjectWithAttributes<FqCobaltQueueDisc> ("MaxSize", StringValue ("10240p"), "UseEcn", BooleanValue (true),
                                                                                  "Perturbation", UintegerValue (0),
                                                                                  "BlueThreshold", TimeValue (Time::Max ()));

  queueDisc->SetQuantum (1514);
  queueDisc->Initialize ();
  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (7);
  hdr.SetEcn (Ipv4Header::ECN_ECT0);

  // Add 20 ECT0 (ECN capable) packets from the first flow
  Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscEcnMarking::AddPacket, this, queueDisc, hdr, 20, 20, 1);

  // Add 20 ECT0 (ECN capable) packets from second flow
  hdr.SetDestination (Ipv4Address ("10.10.1.10"));
  Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscEcnMarking::AddPacket, this, queueDisc, hdr, 20, 40, 2);

  // Add 20 ECT0 (ECN capable) packets from third flow
  hdr.SetDestination (Ipv4Address ("10.10.1.20"));
  Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscEcnMarking::AddPacket, this, queueDisc, hdr, 20, 60, 3);

  // Add 20 NotECT packets from fourth flow
  hdr.SetDestination (Ipv4Address ("10.10.1.30"));
  hdr.SetEcn (Ipv4Header::ECN_NotECT);
  Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscEcnMarking::AddPacket, this, queueDisc, hdr, 20, 80, 4);

  // Add 20 NotECT packets from fifth flow
  hdr.SetDestination (Ipv4Address ("10.10.1.40"));
  Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscEcnMarking::AddPacket, this, queueDisc, hdr, 20, 100, 5);

  //Dequeue 60 packets with delay 110ms to induce packet drops and keep some remaining packets in each queue
  DequeueWithDelay (queueDisc, 0.11, 60);
  Simulator::Run ();
  Simulator::Stop (Seconds (8.0));
  Ptr<CobaltQueueDisc> q0 = queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
  Ptr<CobaltQueueDisc> q1 = queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
  Ptr<CobaltQueueDisc> q2 = queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
  Ptr<CobaltQueueDisc> q3 = queueDisc->GetQueueDiscClass (3)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
  Ptr<CobaltQueueDisc> q4 = queueDisc->GetQueueDiscClass (4)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();

 // As packets in flow queues are ECN capable
  NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK), 19, "There should be 19 marked packets."
                        "As there is no CoDel minBytes parameter so all the packets apart from the first one gets marked. As q3 and q4 have"
                        "NotEct packets and the queue delay is much higher than 5ms so the queue gets empty pretty quickly so more"
                        "packets from q0 can be dequeued.");
  NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
  NS_TEST_EXPECT_MSG_EQ (q1->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK), 16, "There should be 16 marked packets"
                        "As there is no CoDel minBytes parameter so all the packets apart from the first one until no more packets are dequeued"
                        "are marked.");
  NS_TEST_EXPECT_MSG_EQ (q1->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
  NS_TEST_EXPECT_MSG_EQ (q2->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK), 12, "There should be 12 marked packets"
                        "Each packet size is 120 bytes and the quantum is 1500 bytes so in the first turn (1514/120 = 12.61) 13 packets are"
                        "dequeued and apart from the first one, all the packets are marked.");
  NS_TEST_EXPECT_MSG_EQ (q2->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
  
  // As packets in flow queues are not ECN capable
  NS_TEST_EXPECT_MSG_EQ (q3->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), m_dropNextCount, "The number of drops should"
                        "be equal to the number of times m_dropNext is updated");
  NS_TEST_EXPECT_MSG_EQ (q3->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK), 0, "There should not be any marked packets");
  NS_TEST_EXPECT_MSG_EQ (q4->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), m_dropNextCount, "The number of drops should"
                        "be equal to the number of times m_dropNext is updated");
  NS_TEST_EXPECT_MSG_EQ (q4->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK), 0, "There should not be any marked packets");

  Simulator::Destroy ();

  // Test case 2, CeThreshold set to 2ms
  queueDisc = CreateObjectWithAttributes<FqCobaltQueueDisc> ("MaxSize", StringValue ("10240p"), "UseEcn", BooleanValue (true),
                                                                                   "CeThreshold", TimeValue (MilliSeconds (2)),
                                                                                   "BlueThreshold", TimeValue (Time::Max ()));
  queueDisc->SetQuantum (1514);
  queueDisc->Initialize ();
  
  // Add 20 ECT0 (ECN capable) packets from first flow
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetEcn (Ipv4Header::ECN_ECT0);
  Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscEcnMarking::AddPacket, this, queueDisc, hdr, 20, 20, 1);

  // Add 20 ECT0 (ECN capable) packets from second flow
  hdr.SetDestination (Ipv4Address ("10.10.1.10"));
  Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscEcnMarking::AddPacket, this, queueDisc, hdr, 20, 40, 2);

  // Add 20 ECT0 (ECN capable) packets from third flow
  hdr.SetDestination (Ipv4Address ("10.10.1.20"));
  Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscEcnMarking::AddPacket, this, queueDisc, hdr, 20, 60, 3);

  // Add 20 NotECT packets from fourth flow
  hdr.SetDestination (Ipv4Address ("10.10.1.30"));
  hdr.SetEcn (Ipv4Header::ECN_NotECT);
  Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscEcnMarking::AddPacket, this, queueDisc, hdr, 20, 80, 4);

  // Add 20 NotECT packets from fifth flow
  hdr.SetDestination (Ipv4Address ("10.10.1.40"));
  Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscEcnMarking::AddPacket, this, queueDisc, hdr, 20, 100, 5);

  //Dequeue 60 packets with delay 0.1ms to induce packet drops and keep some remaining packets in each queue
  DequeueWithDelay (queueDisc, 0.0001, 60);
  Simulator::Run ();
  Simulator::Stop (Seconds (8.0));
  q0 = queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
  q1 = queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
  q2 = queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
  q3 = queueDisc->GetQueueDiscClass (3)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
  q4 = queueDisc->GetQueueDiscClass (4)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();

  // As packets in flow queues are ECN capable
  NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
  NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK), 0, "There should not be any marked packets"
                        "with quantum of 1514, 13 packets of size 120 bytes can be dequeued. sojourn time of 13th packet is 1.3ms which is"
                        "less than CE threshold");
  NS_TEST_EXPECT_MSG_EQ (q1->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
  NS_TEST_EXPECT_MSG_EQ (q1->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK), 6, "There should be 6 marked packets"
                        "with quantum of 1514, 13 packets of size 120 bytes can be dequeued. sojourn time of 8th packet is 2.1ms which is greater"
                        "than CE threshold and subsequent packet also have sojourn time more 8th packet hence remaining packet are marked.");
  NS_TEST_EXPECT_MSG_EQ (q2->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
  NS_TEST_EXPECT_MSG_EQ (q2->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK), 13, "There should be 13 marked packets"
                        "with quantum of 1514, 13 packets of size 120 bytes can be dequeued and all of them have sojourn time more than CE threshold");

  // As packets in flow queues are not ECN capable
  NS_TEST_EXPECT_MSG_EQ (q3->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK), 0, "There should not be any marked packets");
  NS_TEST_EXPECT_MSG_EQ (q3->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
  NS_TEST_EXPECT_MSG_EQ (q4->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK), 0, "There should not be any marked packets");
  NS_TEST_EXPECT_MSG_EQ (q4->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 1, "There should 1 dropped packet. As the queue"
                        "delay for the first dequeue is greater than the target (5ms), Cobalt overloads the m_dropNext field as an activity timeout"
                        "and dropNext is to set to the current Time value so on the next dequeue a packet is dropped.");

  Simulator::Destroy ();

  // Test case 3, CeThreshold set to 2ms with higher queue delay. This test is mainly to check that the packets are not getting marked twice.
  queueDisc = CreateObjectWithAttributes<FqCobaltQueueDisc> ("MaxSize", StringValue ("10240p"), "UseEcn", BooleanValue (true),
                                                                                   "CeThreshold", TimeValue (MilliSeconds (2)),
                                                                                   "BlueThreshold", TimeValue (Time::Max ()));
  queueDisc->SetQuantum (1514);
  queueDisc->Initialize ();
  
  // Add 20 ECT0 (ECN capable) packets from first flow
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetEcn (Ipv4Header::ECN_ECT0);
  Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscEcnMarking::AddPacket, this, queueDisc, hdr, 20, 20, 1);

  // Add 20 ECT0 (ECN capable) packets from second flow
  hdr.SetDestination (Ipv4Address ("10.10.1.10"));
  Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscEcnMarking::AddPacket, this, queueDisc, hdr, 20, 40, 2);

  // Add 20 ECT0 (ECN capable) packets from third flow
  hdr.SetDestination (Ipv4Address ("10.10.1.20"));
  Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscEcnMarking::AddPacket, this, queueDisc, hdr, 20, 60, 3);

  // Add 20 NotECT packets from fourth flow
  hdr.SetDestination (Ipv4Address ("10.10.1.30"));
  hdr.SetEcn (Ipv4Header::ECN_NotECT);
  Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscEcnMarking::AddPacket, this, queueDisc, hdr, 20, 80, 4);

  // Add 20 NotECT packets from fifth flow
  hdr.SetDestination (Ipv4Address ("10.10.1.40"));
  Simulator::Schedule (Time (Seconds (0)), &FqCobaltQueueDiscEcnMarking::AddPacket, this, queueDisc, hdr, 20, 100, 5);

  // Reset m_dropNextCount value;
  m_dropNextCount = 0;

  //Dequeue 60 packets with delay 110ms to induce packet drops and keep some remaining packets in each queue
  DequeueWithDelay (queueDisc, 0.110, 60);
  Simulator::Run ();
  Simulator::Stop (Seconds (8.0));
  q0 = queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
  q1 = queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
  q2 = queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
  q3 = queueDisc->GetQueueDiscClass (3)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();
  q4 = queueDisc->GetQueueDiscClass (4)->GetQueueDisc ()->GetObject <CobaltQueueDisc> ();

  // As packets in flow queues are ECN capable
  NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
  NS_TEST_EXPECT_MSG_EQ (q0->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK) + 
                         q0->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK), 20 - q0->GetNPackets (), "Number of CE threshold"
                        " exceeded marks plus Number of Target exceeded marks should be equal to total number of packets dequeued");
  NS_TEST_EXPECT_MSG_EQ (q1->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
  NS_TEST_EXPECT_MSG_EQ (q1->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK) + 
                         q1->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK), 20 - q1->GetNPackets (), "Number of CE threshold"
                        " exceeded marks plus Number of Target exceeded marks should be equal to total number of packets dequeued");
  NS_TEST_EXPECT_MSG_EQ (q2->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), 0, "There should not be any dropped packets");
  NS_TEST_EXPECT_MSG_EQ (q2->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK) + 
                         q2->GetStats ().GetNMarkedPackets (CobaltQueueDisc::FORCED_MARK), 20 - q2->GetNPackets (), "Number of CE threshold"
                        " exceeded marks plus Number of Target exceeded marks should be equal to total number of packets dequeued");

  // As packets in flow queues are not ECN capable
  NS_TEST_EXPECT_MSG_EQ (q3->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK), 0, "There should not be any marked packets");
  NS_TEST_EXPECT_MSG_EQ (q3->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), m_dropNextCount, "The number of drops should"
                        "be equal to the number of times m_dropNext is updated");
  NS_TEST_EXPECT_MSG_EQ (q4->GetStats ().GetNMarkedPackets (CobaltQueueDisc::CE_THRESHOLD_EXCEEDED_MARK), 0, "There should not be any marked packets");
  NS_TEST_EXPECT_MSG_EQ (q4->GetStats ().GetNDroppedPackets (CobaltQueueDisc::TARGET_EXCEEDED_DROP), m_dropNextCount, "The number of drops should"
                        "be equal to the number of times m_dropNext is updated");

  Simulator::Destroy ();
}


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
                                                                                  "CeThreshold", TimeValue (MilliSeconds (2)),
                                                                                  "BlueThreshold", TimeValue (Time::Max ()));

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
  AddTestCase (new FqCobaltQueueDiscEcnMarking, TestCase::QUICK);
  AddTestCase (new FqCobaltQueueDiscL4sMode, TestCase::QUICK);
}

static FqCobaltQueueDiscTestSuite fqCobaltQueueDiscTestSuite;
