/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 NITK Surathkal
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
 * Authors: Vivek Jain <jain.vivek.anand@gmail.com>
 *          Sandeep Singh <hisandeepsingh@hotmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "ns3/abort.h"
#include "blue-queue-disc.h"
#include "ns3/drop-tail-queue.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BlueQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (BlueQueueDisc);

TypeId BlueQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BlueQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<BlueQueueDisc> ()
    .AddAttribute ("MaxSize",
                   "The maximum number of packets accepted by this queue disc",
                   QueueSizeValue (QueueSize ("25p")),
                   MakeQueueSizeAccessor (&QueueDisc::SetMaxSize, &QueueDisc::GetMaxSize),
                   MakeQueueSizeChecker ())
    .AddAttribute ("PMark",
                   "Marking Probabilty",
                   DoubleValue (0),
                   MakeDoubleAccessor (&BlueQueueDisc::m_Pmark),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("MeanPktSize",
                   "Average of packet size",
                   UintegerValue (1000),
                   MakeUintegerAccessor (&BlueQueueDisc::m_meanPktSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Increment",
                   "Pmark increment value",
                   DoubleValue (0.0025),
                   MakeDoubleAccessor (&BlueQueueDisc::m_increment),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Decrement",
                   "Pmark decrement Value",
                   DoubleValue (0.00025),
                   MakeDoubleAccessor (&BlueQueueDisc::m_decrement),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("FreezeTime",
                   "Time interval during which Pmark cannot be updated",
                   TimeValue (Seconds (0.1)),
                   MakeTimeAccessor (&BlueQueueDisc::m_freezeTime),
                   MakeTimeChecker ())
  ;

  return tid;
}

BlueQueueDisc::BlueQueueDisc () :
  QueueDisc (QueueDiscSizePolicy::SINGLE_INTERNAL_QUEUE)
{
  NS_LOG_FUNCTION (this);
  m_uv = CreateObject<UniformRandomVariable> ();
}

BlueQueueDisc::~BlueQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

void
BlueQueueDisc::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_uv = 0;
  QueueDisc::DoDispose ();
}

int64_t
BlueQueueDisc::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_uv->SetStream (stream);
  return 1;
}

bool
BlueQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);

  QueueSize nQueued = GetCurrentSize ();

  if (m_isIdle)
    {
      DecrementPmark ();
      m_isIdle = false; // not idle anymore
    }
  
  if (nQueued + item > GetMaxSize ())
    {
      // Increment the Pmark
      IncrementPmark ();
      DropBeforeEnqueue (item, FORCED_DROP);
      return false;
    }
  else if (DropEarly ())
    {
      // Early probability drop: proactive
      DropBeforeEnqueue (item, UNFORCED_DROP);
      return false;
    }

  // No drop
  bool isEnqueued = GetInternalQueue (0)->Enqueue (item);

  NS_LOG_LOGIC ("\t bytesInQueue  " << GetInternalQueue (0)->GetNBytes ());
  NS_LOG_LOGIC ("\t packetsInQueue  " << GetInternalQueue (0)->GetNPackets ());

  return isEnqueued;
}

void
BlueQueueDisc::InitializeParams (void)
{
  m_lastUpdateTime = Time (Seconds (0.0));
  m_idleStartTime = Time (Seconds (0.0));
  m_isIdle = true;
}

bool BlueQueueDisc::DropEarly (void)
{
  NS_LOG_FUNCTION (this);
  double u =  m_uv->GetValue ();
  if (u <= m_Pmark)
    {
      return true;
    }
  return false;
}

void BlueQueueDisc::IncrementPmark (void)
{
  NS_LOG_FUNCTION (this);
  Time now = Simulator::Now ();
  if (now - m_lastUpdateTime > m_freezeTime)
    {
      m_Pmark += m_increment;
      m_lastUpdateTime = now;
      if (m_Pmark > 1.0)
        {
          m_Pmark = 1.0;
        }
    }
}

void BlueQueueDisc::DecrementPmark (void)
{
  NS_LOG_FUNCTION (this);
  Time now = Simulator::Now ();
  if (m_isIdle)
    {
      uint32_t m = 0; // stores the number of times Pmark should be decremented
      m = ((now.GetMilliSeconds () - m_idleStartTime.GetMilliSeconds ()) / m_freezeTime.GetMilliSeconds ());
      m_Pmark -= (m_decrement * m);
      m_lastUpdateTime = now;
      if (m_Pmark < 0.0)
        {
          m_Pmark = 0.0;
        }
    }
  else if (now - m_lastUpdateTime > m_freezeTime)
    {
      m_Pmark -= m_decrement;
      m_lastUpdateTime = now;
      if (m_Pmark < 0.0)
        {
          m_Pmark = 0.0;
        }
    }
}

Ptr<QueueDiscItem>
BlueQueueDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem> (GetInternalQueue (0)->Dequeue ());

  NS_LOG_LOGIC ("Popped " << item);

  NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
  NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

  if (GetInternalQueue (0)->IsEmpty () && !m_isIdle)
    {
      NS_LOG_LOGIC ("Queue empty");

      DecrementPmark ();
      
      m_idleStartTime = Simulator::Now ();
      m_isIdle = true;
    }

  return item;
}

Ptr<const QueueDiscItem>
BlueQueueDisc::DoPeek ()
{
  NS_LOG_FUNCTION (this);
  if (GetInternalQueue (0)->IsEmpty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  Ptr<const QueueDiscItem> item = StaticCast<const QueueDiscItem> (GetInternalQueue (0)->Peek ());

  NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
  NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

  return item;
}

bool
BlueQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("BlueQueueDisc cannot have classes");
      return false;
    }

  if (GetNPacketFilters () > 0)
    {
      NS_LOG_ERROR ("BlueQueueDisc cannot have packet filters");
      return false;
    }

  if (GetNInternalQueues () == 0)
    {
      AddInternalQueue (CreateObjectWithAttributes<DropTailQueue<QueueDiscItem > >
                          ("MaxSize", QueueSizeValue (GetMaxSize ())));
    }

  if (GetNInternalQueues () != 1)
    {
      NS_LOG_ERROR ("BlueQueueDisc needs 1 internal queue");
      return false;
    }

  return true;
}

} //namespace ns3

