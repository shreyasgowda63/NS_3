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
 *          Dizhi Zhou, Mohit P. Tahiliani, Tom Henderson
*/

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/flow-size-prio-queue.h"
#include "ns3/sjf-queue-disc.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * \ingroup traffic-control-test
 * \ingroup tests
 *
 * \brief Sjf Queue Disc Test Item
 */
class SjfQueueDiscTestItem : public QueueDiscItem
{
public:
  /**
   * Constructor
   *
   * \param p the packet
   * \param addr the address
   * \param flowSizePriority the flow size priority value
   * \param itemId the unique id for the queue disc item
   */
  SjfQueueDiscTestItem (Ptr<Packet> p, const Address & addr, uint64_t flowSizePriority, uint32_t itemId);
  virtual ~SjfQueueDiscTestItem ();
  virtual void AddHeader (void);
  virtual bool Mark (void);
  uint32_t GetItemId (void);
  uint64_t GetFlowSizePriority (void);

private:
  uint64_t m_flowSizePriority;
  uint32_t m_itemId;
};

SjfQueueDiscTestItem::SjfQueueDiscTestItem (Ptr<Packet> p, const Address & addr, uint64_t flowSizePriority, uint32_t itemId)
  : QueueDiscItem (p, addr, 0)
{
  m_flowSizePriority = flowSizePriority;
  m_itemId = itemId;
}

SjfQueueDiscTestItem::~SjfQueueDiscTestItem ()
{
}

void
SjfQueueDiscTestItem::AddHeader (void)
{
}

bool
SjfQueueDiscTestItem::Mark (void)
{
  return false;
}

uint32_t
SjfQueueDiscTestItem::GetItemId (void)
{
  return m_itemId;
}

uint64_t
SjfQueueDiscTestItem::GetFlowSizePriority (void)
{
  return m_flowSizePriority;
}

/**
 * \ingroup traffic-control-test
 * \ingroup tests
 *
 * \brief Sjf Queue Disc Test Case
 */
class SjfQueueDiscTestCase : public TestCase
{
public:
  SjfQueueDiscTestCase ();
  virtual void DoRun (void);
};

SjfQueueDiscTestCase::SjfQueueDiscTestCase ()
  : TestCase ("Sanity check on the sjf queue disc implementation")
{
}

void
SjfQueueDiscTestCase::DoRun (void)
{

  Ptr<SjfQueueDisc> qdiscSjfDefault;
  Ptr<QueueDiscItem> qdiscItem;
  Ptr<SjfQueueDiscTestItem> sjfQdiscItem;
  Address dest;

  qdiscSjfDefault = CreateObjectWithAttributes<SjfQueueDisc> ("MaxSize", QueueSizeValue (QueueSize ("10p")));
  qdiscSjfDefault->Initialize ();

  /*
   * Test 1: Check that packets with different flow size tag values are dequeued in the non-decreasing order.
   */
  std::vector<uint64_t> flowSizeVec = {10000, 20000, 5000, 300000, 16000};
  // The expected flow size tag values for the dequeued items (refFlowSizeVec) is the sorted flowSizeVec in the non-decreasing order
  std::vector<uint64_t> refFlowSizeVec = {5000, 10000, 16000, 20000, 300000};
  
  // Generate items with different flow size tag values based on flowSizeVec
  for (auto itemId = 0; itemId < 5; itemId++)
    {
      Ptr<Packet> p = Create<Packet> (1500);
      FlowSizeTag flowSizeTag;
      flowSizeTag.SetFlowSize (flowSizeVec[itemId]);      
      p->AddPacketTag (flowSizeTag);
      qdiscItem = Create<SjfQueueDiscTestItem> (p, dest, flowSizeVec[itemId], itemId);
      qdiscSjfDefault->Enqueue (qdiscItem);
    }
  // Check that the dequeue order is correct based on refFlowSizeVec
  for (auto i = 0; i < 5; i++)
    {
      qdiscItem = qdiscSjfDefault->Dequeue ();
      sjfQdiscItem = DynamicCast<SjfQueueDiscTestItem> (qdiscItem);
      NS_TEST_EXPECT_MSG_EQ (sjfQdiscItem->GetFlowSizePriority (), refFlowSizeVec[i], 
        "The flow size tag value of the dequeued packet vs the expected value: " << sjfQdiscItem->GetFlowSizePriority () << " " << refFlowSizeVec[i]);  
    }

  /*
   * Test 2: Check that packets with the same flow size tag values are dequeued based on the FIFO policy.
   */
  NS_TEST_EXPECT_MSG_EQ (qdiscSjfDefault->GetInternalQueue (0)->GetNPackets (), 0, "The queue disc should be empty currently.");
  // Generate items with the flow size tag values
  uint64_t flowSizeValue = 10000;
  for (auto itemId = 5; itemId < 10; itemId++)
    {
      Ptr<Packet> p = Create<Packet> (1500);
      FlowSizeTag flowSizeTag;
      flowSizeTag.SetFlowSize (flowSizeValue);      
      p->AddPacketTag (flowSizeTag);
      qdiscItem = Create<SjfQueueDiscTestItem> (p, dest, flowSizeVec[itemId], itemId);
      qdiscSjfDefault->Enqueue (qdiscItem);      
    }
  // With the same flow size tag value, check that the dequeue order is correct based on the FIFO policy
  for (auto itemId = 5; itemId < 10; itemId++)
    {
      qdiscItem = qdiscSjfDefault->Dequeue ();
      sjfQdiscItem = DynamicCast<SjfQueueDiscTestItem> (qdiscItem);
      NS_TEST_EXPECT_MSG_EQ (sjfQdiscItem->GetItemId (), itemId, 
        "The itemId of the dequeued packet vs the expected itemId: " << sjfQdiscItem->GetItemId () << " " << itemId);  
    }
    
  /*
   * Test 3: Check that when the queue disc size is full, the default drop tail policy is applied.
   * That is, any new incoming packet will be dropped regardless of its tag value.
   */
  NS_TEST_EXPECT_MSG_EQ (qdiscSjfDefault->GetInternalQueue (0)->GetNPackets (), 0, "The queue disc should be empty currently.");
  // Fill the qdisc with 10p (max size) with itemId 0~9 and flow size tag specificed in  flowSizeVecFull
  std::vector<uint64_t> flowSizeVecFull = {10000000, 10000, 20000, 5000, 300000, 16000, 30000, 25000, 160000, 25000};
  for (auto itemId = 0; itemId <10; itemId++)
    {
      Ptr<Packet> p = Create<Packet> (1500);
      FlowSizeTag flowSizeTag;
      flowSizeTag.SetFlowSize (flowSizeVecFull[itemId]);      
      p->AddPacketTag (flowSizeTag);
      qdiscItem = Create<SjfQueueDiscTestItem> (p, dest, flowSizeVecFull[itemId], itemId);
      qdiscSjfDefault->Enqueue (qdiscItem);
    }
  // Specified the itemId and the flow size tag of the packet to be enqueued
  uint64_t flowSizeValueNew = 10000;
  uint32_t itemIdNew = 10;
  Ptr<Packet> p = Create<Packet> (1500);
  qdiscItem = Create<SjfQueueDiscTestItem> (p, dest, flowSizeValueNew, itemIdNew);
  // Check that with the default drop tail policy, the new packet would be dropped, i.e., not in the queue disc
  qdiscSjfDefault->Enqueue (qdiscItem);
  for (auto i = 0; i < 10; i++)
    {
      qdiscItem = qdiscSjfDefault->Dequeue ();
      sjfQdiscItem = DynamicCast<SjfQueueDiscTestItem> (qdiscItem);
      NS_TEST_EXPECT_MSG_LT_OR_EQ (sjfQdiscItem->GetItemId (), 9, 
        "The itemId of the dequeued packet (should be less than or equal to 9): " << sjfQdiscItem->GetItemId());  
    }

  qdiscSjfDefault->Dispose ();
  Simulator::Destroy ();
}

/**
 * \ingroup traffic-control-test
 * \ingroup tests
 *
 * \brief Sjf Queue Disc Test Suite
 */
static class SjfQueueDiscTestSuite : public TestSuite
{
public:
  SjfQueueDiscTestSuite ()
    : TestSuite ("sjf-queue-disc", UNIT)
  {
    AddTestCase (new SjfQueueDiscTestCase (), TestCase::QUICK);
  }
} g_sjfQueueTestSuite; ///< the test suite
