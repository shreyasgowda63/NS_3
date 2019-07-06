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
#include "ns3/object-factory.h"
#include "priority-queue.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PriorityQueue");

NS_OBJECT_ENSURE_REGISTERED (FlowSizeTag);

FlowSizeTag::FlowSizeTag ()
{
}

void
FlowSizeTag::SetFlowSize (uint64_t flowSize)
{
  m_flowSize = flowSize;
}

uint64_t
FlowSizeTag::GetFlowSize (void) const
{
  return m_flowSize;
}

TypeId
FlowSizeTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FlowSizeTag")
    .SetParent<Tag> ()
    .SetGroupName("TrafficControl")
    .AddConstructor<FlowSizeTag> ()
    ;
  return tid;
}

TypeId
FlowSizeTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
FlowSizeTag::GetSerializedSize (void) const
{
  return sizeof (uint64_t);
}

void
FlowSizeTag::Serialize (TagBuffer i) const
{
  i.WriteU64 (m_flowSize);
}

void
FlowSizeTag::Deserialize (TagBuffer i)
{
  m_flowSize = i.ReadU64();
}

void
FlowSizeTag::Print (std::ostream &os) const
{
  os << "FLOW_SIZE = " << m_flowSize;
}

NS_OBJECT_ENSURE_REGISTERED (PriorityQueue);
NS_OBJECT_TEMPLATE_CLASS_DEFINE (Queue, QueueDiscItem);

TypeId
PriorityQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PriorityQueue")
    .SetParent<Queue<QueueDiscItem> > ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<PriorityQueue> ()
  ;
  return tid;
}

PriorityQueue::PriorityQueue ()
  : NS_LOG_TEMPLATE_DEFINE ("PriorityQueue")
{
}

PriorityQueue::~PriorityQueue ()
{
}

bool
PriorityQueue::Enqueue (Ptr<QueueDiscItem> item)
{
  // Get the flow size priority value
  FlowSizeTag flowSizeTag;
  uint64_t newFlowSize, tmpFlowSize;
  if (item->GetPacket ()->PeekPacketTag (flowSizeTag)){
    newFlowSize = flowSizeTag.GetFlowSize();
    NS_LOG_INFO ("Flow size priority tag for the enqueue packet:" << newFlowSize);
  }
  else {
    // Some packets originate directly from L3 & L4 rather than application layer, these packets
    // are signal packets and are prioritized with the top priority.
    newFlowSize = 0;
    NS_LOG_INFO ("FlowSizeTag not found. Signal packet detected.");
  }

  // If empty, directly enqueue the item
  if (IsEmpty()) {
    m_flowSizePriorities.insert(m_flowSizePriorities.begin(), newFlowSize);
    return DoEnqueue (end (), item);
  }

  // Otherwise, insert the QueueDiscItem and the corresponding flow size tag value with insertion sort
  auto itemIterator = begin ();
  for (auto priorityIterator = m_flowSizePriorities.cbegin(); priorityIterator != m_flowSizePriorities.cend(); ++priorityIterator)
  {
    tmpFlowSize = *priorityIterator;
    if (tmpFlowSize > newFlowSize){
      m_flowSizePriorities.insert(priorityIterator, newFlowSize);
      return DoEnqueue (itemIterator, item);
    }
    itemIterator++;
  }
  
  // If the flow size tag value is not smaller than any others, insert the item and the tag value at the end
  m_flowSizePriorities.insert(m_flowSizePriorities.end(), newFlowSize);
  return DoEnqueue (end (), item);
}

Ptr<QueueDiscItem>
PriorityQueue::Dequeue (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<QueueDiscItem> item = DoDequeue (begin ());

  if (item)
    {
      m_flowSizePriorities.erase (m_flowSizePriorities.begin());
    }
  NS_LOG_LOGIC ("Popped " << item);

  return item;
}

Ptr<const QueueDiscItem>
PriorityQueue::Peek (void) const
{
  NS_LOG_FUNCTION (this);
  return DoPeek (begin ());
}

Ptr<QueueDiscItem>
PriorityQueue::Remove (void)
{
  NS_LOG_FUNCTION (this);
  Ptr<QueueDiscItem> item = DoRemove (begin ());
  return item;
}

}