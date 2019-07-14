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

#include "mlfq-queue-disc.h"
#include "ns3/log.h"
#include "ns3/object-factory.h"
#include "ns3/pointer.h"
#include "ns3/socket.h"
#include <stdint.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MlfqQueueDisc");

FlowPriorityTag::FlowPriorityTag ()
{
}

void
FlowPriorityTag::SetPriority (uint8_t priority)
{
  m_priority = priority;
}

uint8_t
FlowPriorityTag::GetPriority (void) const
{
  return m_priority;
}

TypeId
FlowPriorityTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FlowPriorityTag")
    .SetParent<Tag> ()
    .SetGroupName("TrafficControl")
    .AddConstructor<FlowPriorityTag> ()
    ;
  return tid;
}

TypeId
FlowPriorityTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
FlowPriorityTag::GetSerializedSize (void) const
{
  return sizeof (uint8_t);
}

void
FlowPriorityTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_priority);
}

void
FlowPriorityTag::Deserialize (TagBuffer i)
{
  m_priority = i.ReadU8();
}

void
FlowPriorityTag::Print (std::ostream &os) const
{
  os << "FLOW_PRIORITY = " << m_priority;
}

NS_OBJECT_ENSURE_REGISTERED (FlowPrioPacketFilter);

TypeId 
FlowPrioPacketFilter::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FlowPrioPacketFilter")
    .SetParent<PacketFilter> ()
    // Need to register the constructor callback for ObjectFactory
    .AddConstructor<FlowPrioPacketFilter> ()
    .SetGroupName ("TrafficControl")
  ;
  return tid;
}

FlowPrioPacketFilter::FlowPrioPacketFilter ()
{
  NS_LOG_FUNCTION (this);
}

FlowPrioPacketFilter::~FlowPrioPacketFilter()
{
  NS_LOG_FUNCTION (this);
}

bool
FlowPrioPacketFilter::CheckProtocol (Ptr<QueueDiscItem> item) const
{
  NS_LOG_FUNCTION (this << item);
  // Allow all packets to be processed by DoClassify
  return true;
}

int32_t
FlowPrioPacketFilter::DoClassify (Ptr<QueueDiscItem> item) const
{
  FlowPriorityTag priorityTag;
  if (!item->GetPacket()->PeekPacketTag(priorityTag))
    {
      NS_LOG_DEBUG ("FlowPriorityTag not found.");
      return -1;
    }
  NS_LOG_DEBUG("Flow priority value of the packet: "<< priorityTag.GetPriority() << ".");
  // Convert uint8_t priority value (0~15) to int32_t
  return static_cast<int32_t>(priorityTag.GetPriority());
}

NS_OBJECT_ENSURE_REGISTERED (MlfqQueueDisc);

ATTRIBUTE_HELPER_CPP (ThresholdVector);

std::ostream &
operator << (std::ostream &os, const ThresholdVector &threshold)
{
  std::copy (threshold.begin (), threshold.end ()-1, std::ostream_iterator<uint32_t>(os, " "));
  os << threshold.back ();
  return os;
}

std::istream &operator >> (std::istream &is, ThresholdVector &threshold)
{
  for (int i = 0; i < 15; i++)  // Maximum 16 priorities, i.e., 15 threshold values
    {
      if (!(is >> threshold[i]))
          break;
    }
  return is;
}

TypeId MlfqQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MlfqQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<MlfqQueueDisc> ()
    .AddAttribute ("ThresholdVector", 
                   "Flow size threshold vector (in Bytes) configured for the priority set.",
                   ThresholdVectorValue (ThresholdVector{20000}),  // Source: PIAS
                   MakeThresholdVectorAccessor (&MlfqQueueDisc::m_thresholdVector),
                   MakeThresholdVectorChecker ())    
    .AddAttribute ("NumPriority",
                   "Number of priorities supported (max 16).",
                   UintegerValue (2),
                   MakeUintegerAccessor (&MlfqQueueDisc::m_numPriority),
                   MakeUintegerChecker <uint8_t> (1, 15))  
    .AddAttribute ("ResetThreshold", 
                   "Flow size threshold (in Bytes) to reset the transmitted bytes to prevent the starvation of long flows.",
                   UintegerValue (15000000),
                   MakeUintegerAccessor (&MlfqQueueDisc::m_resetThreshold),
                   MakeUintegerChecker<uint32_t> ())        
    .AddAttribute ("Perturbation",
                   "The salt used as an additional input to the hash function used to classify packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&MlfqQueueDisc::m_perturbation),
                   MakeUintegerChecker<uint32_t> ())                                                                      
    .AddAttribute ("HeaderBytesInclude",
                   "Whether or to include the header bytes when counting",
                   BooleanValue (true),
                   MakeBooleanAccessor (&MlfqQueueDisc::m_headerBytesInclude),
                   MakeBooleanChecker ())                     
  ;
  return tid;
}

MlfqQueueDisc::MlfqQueueDisc ()
  : QueueDisc (QueueDiscSizePolicy::NO_LIMITS)
{
  NS_LOG_FUNCTION (this);
}

MlfqQueueDisc::~MlfqQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

bool
MlfqQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);

  /* 
    Users should use a QueueDiscItem that provides an overloaded Hash() method to identify different flows via 
    hashing the packet's 5-tuple, as in Ipv4QueueDiscItem and Ipv6QueueDiscItem. Otherwise, the base hash function 
    will return 0 always and all packets will share the same priority.
  */
  uint32_t h = 0;
  h = item->Hash (m_perturbation);
  NS_LOG_LOGIC ("Hash value of the item to be enqueued: " << h);

  /*
    The priority tagging is protocol agnostic. Signal packets (e.g., SYN, FIN/RST, ACK packets in TCP connection) 
    are also tagged based on the bytes sent as well. When counting the bytes sent, we could either only count the 
    payload bytes at the application layer or count the raw bytes without eliminating the header bytes from L3/L4 upper 
    layers (network transmission costs).
   */
  if (item->GetPacket() == 0)
    {
      NS_LOG_DEBUG ("Null packet in the queue disc item.");
    }
  uint32_t packetSize = item->GetPacket()->GetSize();
  NS_LOG_LOGIC ("Size of the packet be enqueued: " << packetSize);
  uint32_t payloadSize;
  if (m_headerBytesInclude)
    {
      NS_LOG_INFO ("Traffic control layer is protocol agnostic, we include the header bytes by default.");
      payloadSize = packetSize;
    }
  else
    {
      NS_LOG_INFO ("Only the payload size is considered when counting.");
      uint32_t bytesHeaderSum = item->GetHeaderBytes ();
      payloadSize = packetSize - bytesHeaderSum;
    }
  NS_LOG_LOGIC ("Payload size : " << payloadSize);
  
  // Update flow bytes
  auto it = m_hashToBytes.find(h);
  if (it != m_hashToBytes.end())
    {
      it->second += payloadSize;
      NS_LOG_LOGIC("Update the transmitted flow size in bytes for the flow entry to be " << it->second << ".");
    }
  else
    {
      m_hashToBytes.insert({h, payloadSize});
      NS_LOG_LOGIC ("Insert a new flow entry.");
    }
  // Decide on the packet priority
  uint8_t prio;
  for (prio = 0; prio<m_numPriority-1; prio++)
    {
      if (m_hashToBytes.at(h) <= m_thresholdVector[prio])
        {
          break;
        }
    }
  NS_LOG_LOGIC ("Packet priority value to be tagged: " << prio);
  // Reset flow entry
  if (m_hashToBytes.at(h) >= m_resetThreshold)
    {
      m_hashToBytes.at(h) = 0;
    }
  // \todo In the case of large scale simulation (where large number of flows are present), if the flow finishes, 
  // the corresponding flow entry should be removed from the table. This would require the upper layer protocol 
  // to notify TC layer about the flow completion event or adding a timer to delete the entry when timeout.

  /*
    We use the custom PacketTag to store the priority value (max 16) since the tagging method for MLFQ
    is not standardized currently.
  */
  FlowPriorityTag priorityTag;
  priorityTag.SetPriority(prio);
  item->GetPacket()->AddPacketTag(priorityTag);
  bool retval = GetQueueDiscClass (prio)->GetQueueDisc ()->Enqueue (item);

  // If Queue::Enqueue fails, QueueDisc::Drop is called by the child queue disc
  // because QueueDisc::AddQueueDiscClass sets the drop callback

  return retval;
}

Ptr<QueueDiscItem>
MlfqQueueDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<QueueDiscItem> item;

  for (uint32_t i = 0; i < GetNQueueDiscClasses (); i++)
    {
      if ((item = GetQueueDiscClass (i)->GetQueueDisc ()->Dequeue ()) != 0)
        {
          NS_LOG_LOGIC ("Popped from band " << i << ": " << item);
          NS_LOG_LOGIC ("Number packets band " << i << ": " << GetQueueDiscClass (i)->GetQueueDisc ()->GetNPackets ());
          return item;
        }
    }
  
  NS_LOG_LOGIC ("Queue empty");
  return item;
}

Ptr<const QueueDiscItem>
MlfqQueueDisc::DoPeek (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<const QueueDiscItem> item;

  for (uint32_t i = 0; i < GetNQueueDiscClasses (); i++)
    {
      if ((item = GetQueueDiscClass (i)->GetQueueDisc ()->Peek ()) != 0)
        {
          NS_LOG_LOGIC ("Peeked from band " << i << ": " << item);
          NS_LOG_LOGIC ("Number packets band " << i << ": " << GetQueueDiscClass (i)->GetQueueDisc ()->GetNPackets ());
          return item;
        }
    }

  NS_LOG_LOGIC ("Queue empty");
  return item;
}

bool
MlfqQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNInternalQueues () > 0)
    {
      NS_LOG_ERROR ("MlfqQueueDisc cannot have internal queues");
      return false;
    }

  if (GetNPacketFilters () > 0)
    {
      NS_LOG_ERROR ("MlfqQueueDisc cannot have packet filters");
      return false;
    }

  if (GetNQueueDiscClasses () == 0)
    {
      // Create m_numPriority fifo queue discs by default
      ObjectFactory factory;
      factory.SetTypeId ("ns3::FifoQueueDisc");
      for (uint8_t i = 0; i < m_numPriority; i++)
        {
          Ptr<QueueDisc> qd = factory.Create<QueueDisc> ();
          qd->Initialize ();
          Ptr<QueueDiscClass> c = CreateObject<QueueDiscClass> ();
          c->SetQueueDisc (qd);
          AddQueueDiscClass (c);
        }
    }

  // If the user already configured ChildQueueDisc
  if (GetNQueueDiscClasses () < 2)
    {
      NS_LOG_ERROR ("MlfqQueueDisc needs at least 2 classes");
      return false;
    }
  
  if (GetNQueueDiscClasses () != m_numPriority)
    {
      NS_LOG_ERROR ("QueueDiscClasses number should equal to numPriority");
      return false;
    }
  
  if (m_thresholdVector.size () != (m_numPriority-1))
    {
      NS_LOG_ERROR ("Threshold configuration not compatible with numPriority");
      return false;
    }

  return true;
}

void
MlfqQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);
}

} // namespace ns3
