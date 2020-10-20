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
 */

#include "ns3/log.h"
#include "fq-pie-queue-disc.h"
#include "ns3/queue.h"
#include "pie-queue-disc.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FqPieQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (FqPieQueueDisc);

TypeId FqPieQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FqPieQueueDisc")
    .SetParent<FqQueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<FqPieQueueDisc> ()
    .AddAttribute ("MarkEcnThreshold",
                   "ECN marking threshold (RFC 8033 suggests 0.1 (i.e., 10%) default)",
                   DoubleValue (0.1),
                   MakeDoubleAccessor (&FqPieQueueDisc::m_markEcnTh),
                   MakeDoubleChecker<double> (0, 1))
    .AddAttribute ("ActiveThreshold",
                   "Threshold for activating PIE (disabled by default)",
                   TimeValue (Time::Max ()),
                   MakeTimeAccessor (&FqPieQueueDisc::m_activeThreshold),
                   MakeTimeChecker ())
    .AddAttribute ("MeanPktSize",
                   "Average of packet size",
                   UintegerValue (1000),
                   MakeUintegerAccessor (&FqPieQueueDisc::m_meanPktSize),
                   MakeUintegerChecker<uint32_t> ()) 
    .AddAttribute ("A",
                   "Value of alpha",
                   DoubleValue (0.125),
                   MakeDoubleAccessor (&FqPieQueueDisc::m_a),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("B",
                   "Value of beta",
                   DoubleValue (1.25),
                   MakeDoubleAccessor (&FqPieQueueDisc::m_b),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Tupdate",
                   "Time period to calculate drop probability",
                   TimeValue (Seconds (0.015)),
                   MakeTimeAccessor (&FqPieQueueDisc::m_tUpdate),
                   MakeTimeChecker ())
    .AddAttribute ("Supdate",
                   "Start time of the update timer",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&FqPieQueueDisc::m_sUpdate),
                   MakeTimeChecker ())
    .AddAttribute ("DequeueThreshold",
                   "Minimum queue size in bytes before dequeue rate is measured",
                   UintegerValue (16384),
                   MakeUintegerAccessor (&FqPieQueueDisc::m_dqThreshold),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("QueueDelayReference",
                   "Desired queue delay",
                   TimeValue (Seconds (0.015)),
                   MakeTimeAccessor (&FqPieQueueDisc::m_qDelayRef),
                   MakeTimeChecker ())
    .AddAttribute ("MaxBurstAllowance",
                   "Current max burst allowance before random drop",
                   TimeValue (Seconds (0.15)),
                   MakeTimeAccessor (&FqPieQueueDisc::m_maxBurst),
                   MakeTimeChecker ())
    .AddAttribute ("UseDequeueRateEstimator",
                   "Enable/Disable usage of Dequeue Rate Estimator",
                   BooleanValue (false),
                   MakeBooleanAccessor (&FqPieQueueDisc::m_useDqRateEstimator),
                   MakeBooleanChecker ())
    .AddAttribute ("UseCapDropAdjustment",
                   "Enable/Disable Cap Drop Adjustment feature mentioned in RFC 8033",
                   BooleanValue (true),
                   MakeBooleanAccessor (&FqPieQueueDisc::m_isCapDropAdjustment),
                   MakeBooleanChecker ())
    .AddAttribute ("UseDerandomization",
                   "Enable/Disable Derandomization feature mentioned in RFC 8033",
                   BooleanValue (false),
                   MakeBooleanAccessor (&FqPieQueueDisc::m_useDerandomization),
                   MakeBooleanChecker ())
  ;
  return tid;
}

FqPieQueueDisc::FqPieQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

FqPieQueueDisc::~FqPieQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

void
FqPieQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);

  m_flowFactory.SetTypeId ("ns3::FqFlow");

  m_queueDiscFactory.SetTypeId ("ns3::PieQueueDisc");
  m_queueDiscFactory.Set ("MaxSize", QueueSizeValue (GetMaxSize ()));
  m_queueDiscFactory.Set ("UseEcn", BooleanValue (m_useEcn));
  m_queueDiscFactory.Set ("CeThreshold", TimeValue (m_ceThreshold));
  m_queueDiscFactory.Set ("UseL4s", BooleanValue (m_useL4s));
  m_queueDiscFactory.Set ("ActiveThreshold", TimeValue (m_activeThreshold));
  m_queueDiscFactory.Set ("MeanPktSize", UintegerValue (m_meanPktSize));
  m_queueDiscFactory.Set ("A", DoubleValue (m_a));
  m_queueDiscFactory.Set ("B", DoubleValue (m_b));
  m_queueDiscFactory.Set ("Tupdate", TimeValue (m_tUpdate));
  m_queueDiscFactory.Set ("Supdate", TimeValue (m_sUpdate));
  m_queueDiscFactory.Set ("DequeueThreshold", UintegerValue (m_dqThreshold));
  m_queueDiscFactory.Set ("QueueDelayReference", TimeValue (m_qDelayRef));
  m_queueDiscFactory.Set ("MaxBurstAllowance", TimeValue (m_maxBurst));
  m_queueDiscFactory.Set ("UseDequeueRateEstimator", BooleanValue (m_useDqRateEstimator));
  m_queueDiscFactory.Set ("UseCapDropAdjustment", BooleanValue (m_isCapDropAdjustment));
  m_queueDiscFactory.Set ("UseDerandomization", BooleanValue (m_useDerandomization));
}

} // namespace ns3

