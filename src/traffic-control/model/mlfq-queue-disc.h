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

#ifndef MLFQ_QUEUE_DISC_H
#define MLFQ_QUEUE_DISC_H

#include <unordered_map>
#include "ns3/queue-disc.h"
#include "ns3/packet-filter.h"
#include "ns3/tag.h"

namespace ns3 {

typedef std::vector<uint32_t> ThresholdVector;

/**
 * \brief A custom tag to indicate the priority value (max 16) set by MlfqQueueDisc
 */
class FlowPriorityTag : public Tag
{
public:
  FlowPriorityTag ();

  /**
   * \brief Set the tag's priority
   *
   * \param priority the priority
   */
  void SetPriority (uint8_t priority);

  /**
   * \brief Get the tag's priority
   *
   * \returns the priority
   */
  uint8_t GetPriority (void) const;

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
  uint8_t m_priority;  //!< the priority carried by the tag
};

/**
 * \ingroup traffic-control
 *
 * FlowPrioPacketFilter is the PacketFilter to classify the packets
 * with FlowPriorityTag during PrioQueueDisc DoEnqueue. It requires 
 * packets to have a FlowPriorityTag tagged by MlfqQueueDisc. 
 */
class FlowPrioPacketFilter: public PacketFilter {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  FlowPrioPacketFilter ();
  virtual ~FlowPrioPacketFilter ();

private:
  virtual bool CheckProtocol (Ptr<QueueDiscItem> item) const;
  virtual int32_t DoClassify (Ptr<QueueDiscItem> item) const;
};

/**
 * \ingroup traffic-control
 *
 * The Mlfq qdisc (Multi-Level Feedback Queue) is a queueing discipline that prioritizes 
 * short flows over longer ones to mimic the shortest job first algorithm. Priority 
 * of the flows are decided based on the historically transmitted flow size and the 
 * configured ThresholdVector. Mlfq qdisc also contains an arbitrary number of Fifo queues 
 * corresponding to the number of priorities. Packets are tagged with the priority based 
 * on the flow they belong to and enqueued on one of the FIFO queues based on the priority 
 * value. By default, 2 Fifo queue discs are configured with the ThresholdVector with one
 * threshold flow size value in Bytes.
 * 
 * Notice that ideally, Mlfq qdisc should set the right priority value for each individual 
 * MTU-sized packet. There could be an interference on the effectiveness of Mlfq if the Mlfq 
 * tagged packet is chunked into smaller MTU-sized packet for NIC transmission. Check the 
 * paper Bai, Wei, et al. "Information-agnostic flow scheduling for commodity data centers." 
 * 12th {USENIX} Symposium on Networked Systems Design and Implementation ({NSDI} 15), a.k.a.
 * PIAS for more details on MLFQ for DCN.
 * 
 */

class MlfqQueueDisc : public QueueDisc {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief MlfqQueueDisc constructor
   */
  MlfqQueueDisc ();

  virtual ~MlfqQueueDisc();

private:
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
  virtual Ptr<QueueDiscItem> DoDequeue (void);
  virtual Ptr<const QueueDiscItem> DoPeek (void);
  virtual bool CheckConfig (void);
  virtual void InitializeParams (void);

  uint32_t m_perturbation;   //!< Hash perturbation value
  std::unordered_map<int32_t, int64_t> m_hashToBytes;  //!< Historical transmitted bytes for each flow
  ThresholdVector m_thresholdVector;  //!< Priority thresholds
  uint8_t m_numPriority;  //!< Number of priorities (max 16)
  uint32_t m_resetThreshold;  //!< Threshold to reset the flow priority
  bool m_headerBytesInclude;  //!< Include the header bytes or not
};

ATTRIBUTE_HELPER_HEADER (ThresholdVector);

/**
 * Serialize the ThresholdVector to the given ostream
 *
 * \param os
 * \param threshold
 *
 * \return std::ostream
 */
std::ostream &operator << (std::ostream &os, const ThresholdVector &threshold);

/**
 * Serialize from the given istream to this ThresholdVector.
 *
 * \param is
 * \param threshold 
 *
 * \return std::istream
 */
std::istream &operator >> (std::istream &is, ThresholdVector &threshold);

} // namespace ns3

#endif /* MLFQ_QUEUE_DISC_H */
