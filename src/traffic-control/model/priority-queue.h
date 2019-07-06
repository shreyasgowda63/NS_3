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

#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "ns3/queue.h"
#include "ns3/tag.h"

namespace ns3 {

/**
 * \brief A custom tag to indicate the flow size value (uint64_t)
 */
class FlowSizeTag : public Tag
{
public:
  FlowSizeTag ();

  /**
   * \brief Set the tag's flow size
   *
   * \param flowSize the flow size value
   */
  void SetFlowSize (uint64_t flowSize);

  /**
   * \brief Get the tag's flow size
   *
   * \returns the flow size value
   */
  uint64_t GetFlowSize (void) const;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  virtual TypeId GetInstanceTypeId (void) const;

  virtual uint32_t GetSerializedSize (void) const;

  virtual void Serialize (TagBuffer i) const;

  virtual void Deserialize (TagBuffer i);

  virtual void Print (std::ostream &os) const;
private:
  uint64_t m_flowSize;  //!< the flow size value carried by the tag
};

/**
 * \brief This queue implements the custom priority queue to support SJF scheduling. 
 * Each QueueDiscItem is ranked based on the FlowSizeTag carried by the packet.
 * During Dequeue, the packet with the minimum flow size tag value is served first 
 * (i.e., the front of the queue). When there are multiple packets with the same minimal 
 * tag value, the earliest packet is served, i.e., FIFO policy is applied for the packets 
 * of the same rank. During Enqueue, the QueueDiscItem is inserted based on the flow size tag
 * so that the QueueDiscItems are sorted in non-decreasing order in terms of the flow size tag
 * value. If the queue is full, the enqueue packet will be dropped by default.
 */
class PriorityQueue : public Queue<QueueDiscItem>
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  PriorityQueue ();
  ~PriorityQueue ();

  /**
   * Enqueue the given QueueDiscItem with insertion sort based on the flow size tag value.
   *
   * \param item the QueueDiscItem to be enqueued
   * \return true if success, false if the packet has been dropped
   */
  bool Enqueue (Ptr<QueueDiscItem> item);

  /**
   * Dequeue the QueueDiscItem in the front of the queue.
   *
   * \return the QueueDiscItem
   */
  Ptr<QueueDiscItem> Dequeue (void);

  /**
   * Remove the QueueDiscItem in the front of the queue.
   *
   * \return the QueueDiscItem
   */
  Ptr<QueueDiscItem> Remove (void);

  /**
   * Peek the QueueDiscItem in the front of the queue. The QueueDiscItem is not removed.
   *
   * \return the QueueDiscItem
   */  
  Ptr<const QueueDiscItem> Peek (void) const;

private:
  using Queue<QueueDiscItem>::begin;
  using Queue<QueueDiscItem>::end;
  using Queue<QueueDiscItem>::DoEnqueue;
  using Queue<QueueDiscItem>::DoDequeue;
  using Queue<QueueDiscItem>::DoRemove;
  using Queue<QueueDiscItem>::DoPeek;

  std::list<uint64_t> m_flowSizePriorities;  //!< the meta list to store the flow size tag value 

  NS_LOG_TEMPLATE_DECLARE;                  //!< redefinition of the log component
};

} // namespace ns3

#endif /* PRIORITY_QUEUE_H */
