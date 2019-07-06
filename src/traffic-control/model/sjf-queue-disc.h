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

#ifndef SJF_QUEUE_DISC_H
#define SJF_QUEUE_DISC_H

#include "ns3/queue-disc.h"

namespace ns3 {

/**
 * \ingroup traffic-control
 *
 * A queue disc implementing the SJF (Shortest Job First) policy. The ideal scheduling
 * policy assumes that the flow size is known a priori. The packets are tagged with the 
 * meta data of the flow size in bytes. The internal queue structure is PriorityQueue
 * and during DoEnqueue, packets are sorted based on the flow size tag and the packet 
 * belonging to the shortest flow is served first. If several packets are with the same 
 * shortest tag value, then they are served with FIFO (First In First Out) policy. 
 * During DoDequeue, the priority drop policy is applied so that the packet with the largest 
 * tag value is dropped. Similarly, if several packets are with the largest tag value, 
 * the last coming packet is dropped.
 * 
 * Notice that the end hosts need to tag the packets with the corresponding flow size information.
 * It assumes that the flow size information is known exactly by the application or estimated roughly
 * before the packet transmission. Hence, we extend the source applications with the optional 
 * attributes FlowSizeTagInclude to tag the packet with the flow size information. To enforce SJF policy
 * over the network, the attribute FlowSizeTagInclude should be set true for the source applications.
 * 
 */
class SjfQueueDisc : public QueueDisc {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief SjfQueueDisc constructor
   */
  SjfQueueDisc ();

  virtual ~SjfQueueDisc();

  // Reasons for dropping packets
  static constexpr const char* LIMIT_EXCEEDED_DROP = "Queue disc limit exceeded";  //!< Packet dropped due to queue disc limit exceeded

private:
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
  virtual Ptr<QueueDiscItem> DoDequeue (void);
  virtual Ptr<const QueueDiscItem> DoPeek (void);
  virtual bool CheckConfig (void);
  virtual void InitializeParams (void);
};

} // namespace ns3

#endif /* SJF_QUEUE_DISC_H */
