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

#include "ns3/log.h"
#include "fq-cobalt-queue-disc.h"
#include "ns3/queue.h"
#include "cobalt-queue-disc.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FqCobaltQueueDisc");
NS_OBJECT_ENSURE_REGISTERED (FqCobaltQueueDisc);

TypeId FqCobaltQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FqCobaltQueueDisc")
    .SetParent<FqQueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<FqCobaltQueueDisc> ()
    .AddAttribute ("Interval",
                   "The CoDel algorithm interval for each FqCobalt queue",
                   StringValue ("100ms"),
                   MakeStringAccessor (&FqCobaltQueueDisc::m_interval),
                   MakeStringChecker ())
    .AddAttribute ("Target",
                   "The CoDel algorithm target queue delay for each FqCobalt queue",
                   StringValue ("5ms"),
                   MakeStringAccessor (&FqCobaltQueueDisc::m_target),
                   MakeStringChecker ())
    .AddAttribute ("Pdrop",
                   "Marking Probability",
                   DoubleValue (0),
                   MakeDoubleAccessor (&FqCobaltQueueDisc::m_Pdrop),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Increment",
                   "Pdrop increment value",
                   DoubleValue (1. / 256),
                   MakeDoubleAccessor (&FqCobaltQueueDisc::m_increment),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Decrement",
                   "Pdrop decrement Value",
                   DoubleValue (1. / 4096),
                   MakeDoubleAccessor (&FqCobaltQueueDisc::m_decrement),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("BlueThreshold",
                   "The Threshold after which Blue is enabled",
                   TimeValue (MilliSeconds (400)),
                   MakeTimeAccessor (&FqCobaltQueueDisc::m_blueThreshold),
                   MakeTimeChecker ())
  ;
  return tid;
}

FqCobaltQueueDisc::FqCobaltQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

FqCobaltQueueDisc::~FqCobaltQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

void
FqCobaltQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);

  m_flowFactory.SetTypeId ("ns3::FqFlow");

  m_queueDiscFactory.SetTypeId ("ns3::CobaltQueueDisc");
  m_queueDiscFactory.Set ("MaxSize", QueueSizeValue (GetMaxSize ()));
  m_queueDiscFactory.Set ("Interval", StringValue (m_interval));
  m_queueDiscFactory.Set ("Target", StringValue (m_target));
  m_queueDiscFactory.Set ("UseEcn", BooleanValue (m_useEcn));
  m_queueDiscFactory.Set ("CeThreshold", TimeValue (m_ceThreshold));
  m_queueDiscFactory.Set ("UseL4s", BooleanValue (m_useL4s));
  m_queueDiscFactory.Set ("Pdrop", DoubleValue (m_Pdrop));
  m_queueDiscFactory.Set ("Increment", DoubleValue (m_increment));
  m_queueDiscFactory.Set ("Decrement", DoubleValue (m_decrement));
  m_queueDiscFactory.Set ("BlueThreshold", TimeValue (m_blueThreshold));
}

} // namespace ns3

