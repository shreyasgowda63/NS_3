/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Universita' degli Studi di Napoli Federico II
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
 * Authors: Pasquale Imputato <p.imputato@gmail.com>
 *          Stefano Avallone <stefano.avallone@unina.it>
*/

#include "ns3/log.h"
#include "fq-codel-queue-disc.h"
#include "ns3/queue.h"
#include "codel-queue-disc.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("FqCoDelQueueDisc");
NS_OBJECT_ENSURE_REGISTERED (FqCoDelQueueDisc);

TypeId FqCoDelQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FqCoDelQueueDisc")
    .SetParent<FqQueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<FqCoDelQueueDisc> ()
    .AddAttribute ("Interval",
                   "The CoDel algorithm interval for each FqCoDel queue",
                   StringValue ("100ms"),
                   MakeStringAccessor (&FqCoDelQueueDisc::m_interval),
                   MakeStringChecker ())
    .AddAttribute ("Target",
                   "The CoDel algorithm target queue delay for each FqCoDel queue",
                   StringValue ("5ms"),
                   MakeStringAccessor (&FqCoDelQueueDisc::m_target),
                   MakeStringChecker ())
  ;
  return tid;
}
FqCoDelQueueDisc::FqCoDelQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

FqCoDelQueueDisc::~FqCoDelQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

void
FqCoDelQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);
  m_flowFactory.SetTypeId ("ns3::FqFlow");

  m_queueDiscFactory.SetTypeId ("ns3::CoDelQueueDisc");
  m_queueDiscFactory.Set ("MaxSize", QueueSizeValue (GetMaxSize ()));
  m_queueDiscFactory.Set ("Interval", StringValue (m_interval));
  m_queueDiscFactory.Set ("Target", StringValue (m_target));
  m_queueDiscFactory.Set ("UseEcn", BooleanValue (m_useEcn));
  m_queueDiscFactory.Set ("CeThreshold", TimeValue (m_ceThreshold));
  m_queueDiscFactory.Set ("UseL4s", BooleanValue (m_useL4s));
}
} // namespace ns3