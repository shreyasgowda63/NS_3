/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) Liangcheng Yu <liangcheng.yu46@gmail.com>
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
 * Authors: Liangcheng Yu <liangcheng.yu46@gmail.com>
 * GSoC 2019 project Mentors:
 *          Dizhi Zhou <dizhizhou@hotmail.com>
 *          Mohit P. Tahiliani <tahiliani.nitk@gmail.com>
 *          Tom Henderson <tomh@tomh.org>
*/

#include "ns3/fifo-queue-disc.h"
#include "ns3/log.h"
#include "ns3/mlfq-queue-disc.h"
#include "ns3/packet.h"
#include "ns3/prio-queue-disc.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * \ingroup traffic-control-test
 * \ingroup tests
 *
 * \brief Mlfq Queue Disc Test Item
 */
class MlfqQueueDiscTestItem : public QueueDiscItem
{
public:
  /**
   * Constructor
   *
   * \param p the packet
   * \param addr the address
   * \param flowHashValue the hash value for the naive Hash function implementation
   */
  MlfqQueueDiscTestItem (Ptr<Packet> p, const Address & addr, uint8_t flowHashValue);
  virtual ~MlfqQueueDiscTestItem ();
  virtual void AddHeader (void);
  virtual bool Mark (void);
  virtual uint32_t Hash (uint32_t perturbation) const;

private:
  uint32_t m_flowHashValue;
};

MlfqQueueDiscTestItem::MlfqQueueDiscTestItem (Ptr<Packet> p, const Address & addr, uint8_t flowHashValue)
  : QueueDiscItem (p, addr, 0)
{
  m_flowHashValue = flowHashValue;
}

MlfqQueueDiscTestItem::~MlfqQueueDiscTestItem ()
{
}

void
MlfqQueueDiscTestItem::AddHeader (void)
{
}

bool
MlfqQueueDiscTestItem::Mark (void)
{
  return false;
}

uint32_t
MlfqQueueDiscTestItem::Hash (uint32_t perturbation) const
{
  return m_flowHashValue;
}

/**
 * \ingroup traffic-control-test
 * \ingroup tests
 *
 * \brief Mlfq Queue Disc Test Case
 */
class MlfqQueueDiscTestCase : public TestCase
{
public:
  MlfqQueueDiscTestCase ();
  virtual void DoRun (void);
};

MlfqQueueDiscTestCase::MlfqQueueDiscTestCase ()
  : TestCase ("Sanity check on the mlfq queue disc implementation")
{
}

void
MlfqQueueDiscTestCase::DoRun (void)
{

  Ptr<MlfqQueueDisc> qdiscCustom;
  Ptr<MlfqQueueDisc> qdiscDefault0;
  Ptr<MlfqQueueDisc> qdiscDefault1;
  Ptr<PrioQueueDisc> qdiscPrio;
  Ptr<QueueDiscItem> item;
  uint32_t flowHashValueTest = 1;
  Address dest;

  /*
   * Test 1: The custom ThresholdVector attribute could be set correctly.
   */
  qdiscCustom = CreateObjectWithAttributes<MlfqQueueDisc> ("NumPriority", UintegerValue(4),
                                                           "ResetThreshold", UintegerValue(15000000),
                                                           "HeaderBytesInclude", BooleanValue (false));

  ThresholdVector thVec = {10000, 20000, 30000};
  NS_TEST_EXPECT_MSG_EQ (qdiscCustom->SetAttributeFailSafe ("ThresholdVector", ThresholdVectorValue(thVec)),
                         true, "Verify that we can actually set the attribute ThresholdVector");
  ThresholdVectorValue thVecValue;
  NS_TEST_EXPECT_MSG_EQ (qdiscCustom->GetAttributeFailSafe ("ThresholdVector", thVecValue),
                         true, "Verify that we can actually get the attribute ThresholdVector");
  NS_TEST_EXPECT_MSG_EQ (thVecValue.Get (), thVec, "Verify that the attribute ThresholdVector has been correctly set");

  // Initialize the configured queueDisc, 4 child fifo queue discs will be added during CheckConfig
  qdiscCustom->Initialize ();
  NS_TEST_EXPECT_MSG_EQ (qdiscCustom->GetNQueueDiscClasses (), 4, "Verify that the queue disc has 4 child queue discs");
  qdiscCustom->Dispose ();

  /*
   * Test 2: Packets of the same flow are tagged and enqueued correctly based on the ThresholdVector.
   * We initialize a default MlfqQueueDisc with 2 priorities and 1 threshold value 20000 bytes.
   * A simulated flow with 30000 bytes (20 packets each of 1500 bytes) will pass through the MlfqQueueDisc.
   * With 20000 byte threshold, 13 packets (13*1500=19500 bytes) will be tagged with priority 0 (top priority)
   * and the remaining 7 packets will be tagged with priority 1.
   */
  // Initialize a default MlfqQueueDisc with 2 priorities for simplicity
  qdiscDefault0 = CreateObject<MlfqQueueDisc> ();
  qdiscDefault0->Initialize ();
  NS_TEST_EXPECT_MSG_EQ (qdiscDefault0->GetNQueueDiscClasses (), 2, "Verify that the queue disc has 2 child queue discs");

  NS_TEST_EXPECT_MSG_EQ (qdiscDefault0->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (),
                               0, "There should be no packets in the child queue disc " << 0);  
  NS_TEST_EXPECT_MSG_EQ (qdiscDefault0->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (),
                               0, "There should be no packets in the child queue disc " << 1);                               
  // Create a flow with 30000 Bytes and each packet of size 1500 Bytes
  for (uint16_t i = 0; i < 20; i++)
    {
      item = Create<MlfqQueueDiscTestItem> (Create<Packet> (1500), dest, flowHashValueTest);
      qdiscDefault0->Enqueue (item);
    }
  // With default threshold 20000, check if the number of packets match the theoretical value
  NS_TEST_EXPECT_MSG_EQ (qdiscDefault0->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (),
                                13, "There should be 13 packets in the child queue disc " << 0);    
  NS_TEST_EXPECT_MSG_EQ (qdiscDefault0->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (),
                                7, "There should be 7 packets in the child queue disc " << 1);

  /*
   * Test 3: ResetThreshold is working correctly for a simulated long flow.
   * Similar to test 2, we set a ResetThreshold value (30000 bytes) and create an extra packet of 1500 bytes. 
   * Since the flow entry will be reset once the flow size reaches 30000 bytes, the last packet will be tagged with top priority.
   * Theoretically, there should be 14 packets of priority 0 and 7 packets of priority 1.
   */
  qdiscDefault1 = CreateObject<MlfqQueueDisc> ();
  // Set a small ResetThreshold value for testing
  qdiscDefault1->SetAttribute ("ResetThreshold", UintegerValue(30000));
  qdiscDefault1->Initialize ();
  // Simulate a "long" flow with 21 packets each of 1500 Bytes for testing
  NS_TEST_EXPECT_MSG_EQ (qdiscDefault1->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (),
                               0, "There should be no packets in the child queue disc " << 0);  
  NS_TEST_EXPECT_MSG_EQ (qdiscDefault1->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (),
                               0, "There should be no packets in the child queue disc " << 1);                               
  for (uint16_t i = 0; i < 21; i++)
    {
      item = Create<MlfqQueueDiscTestItem> (Create<Packet> (1500), dest, flowHashValueTest);
      qdiscDefault1->Enqueue (item);
    }
  // Check if the number of packets match the theoretical value
  NS_TEST_EXPECT_MSG_EQ (qdiscDefault1->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (),
                                14, "There should be 14 packets in the child queue disc " << 0);    
  NS_TEST_EXPECT_MSG_EQ (qdiscDefault1->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (),
                                7, "There should be 7 packets in the child queue disc " << 1);

  /*
   * Test 4: Strict priority policy is correctly enforced during DoDequeue.
   * We dequeue each packet sequentially and check if the first 13 dequeued packets come 
   * from band 0 and the last 7 packets from band 1.
   */
  // Reuse the qdiscDefault0 for checking the DoDequeue
  uint8_t packetIndex = 0;
  while ((item = qdiscDefault0->Dequeue ()))
  {
    if (packetIndex < 13)
      {
        NS_TEST_EXPECT_MSG_EQ (qdiscDefault0->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (),
                                  13-packetIndex-1, "There should be " << 13-packetIndex-1 << 
                                  " packet in the child queue disc " << 0);
        NS_TEST_EXPECT_MSG_EQ (qdiscDefault0->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (),
                                  7, "There should be 7 packets in the child queue disc " << 1);                           
      }
    if (packetIndex >= 13 && packetIndex < 20)
      {
        NS_TEST_EXPECT_MSG_EQ (qdiscDefault0->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (),
                                  0, "There should be 0 packets in the child queue disc " << 0);                           
        NS_TEST_EXPECT_MSG_EQ (qdiscDefault0->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (),
                                  13-packetIndex-1+7, "There should be " << 19-packetIndex << 
                                  " packet in the child queue disc " << 1);                                  
    }    
    packetIndex += 1;
  }
  NS_TEST_EXPECT_MSG_EQ (packetIndex, 20, "Make sure exactly 20 packets are dequeued");
  qdiscDefault0->Dispose ();

  /*
   * Test 5: Combined with PrioQueueDisc, the tagged priority could be leveraged for classification.
   * Check if the 14 packets previously tagged with priority 0 are enqueued at band 0 and
   * the rest 7 packets tagged with priority 1 are enqueued at band 1 at PrioQueueDisc.
   */
  // Create an accompanying PrioQueueDisc with 2 bands rather than the default 3 bands
  qdiscPrio = CreateObject<PrioQueueDisc> ();
  std::string priomap ("0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1");
  qdiscPrio->SetAttribute ("Priomap", StringValue (priomap));
  for (uint8_t i = 0; i < 2; i++)
    {
      Ptr<FifoQueueDisc> child = CreateObject<FifoQueueDisc> ();
      child->Initialize ();
      Ptr<QueueDiscClass> c = CreateObject<QueueDiscClass> ();
      c->SetQueueDisc (child);
      qdiscPrio->AddQueueDiscClass (c);
    }
  qdiscPrio->Initialize ();
  
  // Create the FlowPrioPacketFilter for classification
  Ptr<FlowPrioPacketFilter> pf = CreateObject<FlowPrioPacketFilter> ();
  qdiscPrio->AddPacketFilter (pf);
  // Get the dequeued packets from qdiscDefault1 and enqueue them on qdiscPrio
  while ((item = qdiscDefault1->Dequeue ()))
    {
      qdiscPrio->Enqueue (item);
    }
  // Validate if the number of packets on each child queue disc matches the theoretical number
  NS_TEST_EXPECT_MSG_EQ (qdiscPrio->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (),
                          14, "There should be 14 packets in the child queue disc " << 0);
  NS_TEST_EXPECT_MSG_EQ (qdiscPrio->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (),
                          7, "There should be 7 packets in the child queue disc " << 1);
  qdiscDefault1->Dispose ();
  qdiscPrio->Dispose ();

  Simulator::Destroy ();
}

/**
 * \ingroup traffic-control-test
 * \ingroup tests
 *
 * \brief Mlfq Queue Disc Test Suite
 */
static class MlfqQueueDiscTestSuite : public TestSuite
{
public:
  MlfqQueueDiscTestSuite ()
    : TestSuite ("mlfq-queue-disc", UNIT)
  {
    AddTestCase (new MlfqQueueDiscTestCase (), TestCase::QUICK);
  }
} g_mlfqQueueTestSuite; ///< the test suite
