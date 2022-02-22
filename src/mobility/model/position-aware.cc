/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2020 National Technology & Engineering Solutions of Sandia,
 * LLC (NTESS). Under the terms of Contract DE-NA0003525 with NTESS,
 * the U.S. Government retains certain rights in this software.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 *
 */

#define NS_LOG_APPEND_CONTEXT                                           \
  if (m_aggregated) { std::clog << "[node " << GetObject<Node> ()->GetId ()  << "] "; }

#include "position-aware.h"
#include <ns3/double.h>
#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/trace-source-accessor.h>
#include <ns3/node.h>

NS_LOG_COMPONENT_DEFINE ("PositionAware");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (PositionAware);

PositionAware::PositionAware ()
  : m_aggregated (false)
{
  NS_LOG_FUNCTION (this);
  m_timeoutTimer.SetFunction (&PositionAware::HandleTimeout, this);
}

PositionAware::~PositionAware ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
PositionAware::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PositionAware")
    .SetParent<Object> ()
    .AddConstructor<PositionAware> ()
    .AddAttribute ("ReferencePosition",
                   "The Reference position.",
                   TypeId::ATTR_SET | TypeId::ATTR_GET,
                   VectorValue (Vector (0.0, 0.0, 0.0)),
                   MakeVectorAccessor (&PositionAware::SetReferencePosition,
                                       &PositionAware::GetReferencePosition),
                   MakeVectorChecker ())
    .AddAttribute ("DeltaPosition",
                   "The minimum position change from reference to trigger PositionChange",
                   TypeId::ATTR_SET | TypeId::ATTR_GET,
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&PositionAware::SetDeltaPosition,
                                       &PositionAware::GetDeltaPosition),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Timeout",
                   "The duration before reporting Timeout if the position delta is not met",
                   TypeId::ATTR_SET | TypeId::ATTR_GET,
                   TimeValue (Seconds (0.0)),
                   MakeTimeAccessor (&PositionAware::SetTimeout,
                                     &PositionAware::GetTimeout),
                   MakeTimeChecker ())
    .AddTraceSource ("Timeout",
                     "The Timeout duration was reached",
                     MakeTraceSourceAccessor (&PositionAware::m_timeoutTrace),
                     "ns3::PositionAware::TimeoutCallback")
    .AddTraceSource ("PositionChange",
                     "The Position has changed by DeltaPosition",
                     MakeTraceSourceAccessor (&PositionAware::m_positionChangeTrace),
                     "ns3::PositionAware::PositionChangeCallback")
  ;
  return tid;
}


double
PositionAware::GetDeltaPosition () const
{
  return m_deltaPosition;
}

void
PositionAware::SetDeltaPosition (const double& delta_position)
{
  m_deltaPosition = delta_position;
  if (m_positionChangeScheduled || m_timeoutScheduled)
    {
      ScheduleNext ();
    }
}

Time
PositionAware::GetTimeout () const
{
  return m_timeout;
}

void
PositionAware::SetTimeout (const Time& timeout)
{
  m_timeoutTimer.SetDelay (timeout);
  m_timeout = timeout;
  if (m_positionChangeScheduled || m_timeoutScheduled)
    {
      ScheduleNext ();
    }
}

Vector
PositionAware::GetReferencePosition () const
{
  return m_referencePosition;
}

void
PositionAware::SetReferencePosition (const Vector& position)
{
  m_referencePosition = position;
}

void
PositionAware::CourseChange (Ptr<const MobilityModel> mobilityModel)
{
  NS_LOG_FUNCTION (this << mobilityModel);
  double distance = CalculateDistance (mobilityModel->GetPosition (),
                                       m_referencePosition);
  NS_ABORT_MSG_IF (distance >= m_deltaPosition, "Distance " << distance
    << " met or exceeded threshold " << m_deltaPosition << " unexpectedly");
  Time delayLeft;
  if (m_timeoutScheduled)
    {
      delayLeft = m_timeoutTimer.GetDelayLeft ();
      NS_LOG_DEBUG ("Timeout already scheduled, delay left=" << delayLeft.As (Time::S));
    }
  else
    {
      NS_LOG_DEBUG ("Timeout not scheduled");
    }
  if (m_positionChangeScheduled)
    {
      CancelPositionChange ();
    }
  // Speed can be calculated as the cartesian distance of velocity from origin
  m_speed = CalculateDistance (mobilityModel->GetVelocity (), Vector (0, 0, 0));
  NS_LOG_DEBUG ("New Speed is: " << m_speed << " m/s");
  if (0.0 == m_speed)
    {
      NS_LOG_DEBUG ("Not moving");
      if (!m_timeoutScheduled)
        {
          ScheduleTimeout ();
        }
    }
  else if (0.0 == distance)
    {
      double t = m_deltaPosition / m_speed;
      NS_LOG_DEBUG ("Wasn't moving, but is now: t = " << t << " delayLeft= " << delayLeft.As (Time::S));
      if (t < delayLeft.GetSeconds ())
        {
          NS_LOG_DEBUG ("Position change threshold occurs before m_timeout");
          SchedulePositionChange (Seconds (t)); //schedule positionchange
        }
      else
        {
          if (!m_timeoutScheduled) //if there is no m_timeout scheduled
            {
              NS_LOG_DEBUG ("Position change occurs after m_timeout");
              ScheduleTimeout (); //schedule m_timeout
            }
        }
    }
  else
    {
      NS_LOG_DEBUG ("Was moving and is moving a different direction now");
      Vector deltaP;
      deltaP.x = mobilityModel->GetPosition ().x - m_referencePosition.x;
      deltaP.y = mobilityModel->GetPosition ().y - m_referencePosition.y;
      deltaP.z = mobilityModel->GetPosition ().z - m_referencePosition.z;
      Vector V = mobilityModel->GetVelocity ();
      //common values for both test and calculation
      double dotProduct = deltaP.x * V.x + deltaP.y * V.y + deltaP.z * V.z;
      double PV = distance * m_speed;
      double DD = m_deltaPosition * m_deltaPosition;
      double PP = distance * distance;
      double a = m_speed * m_speed;
      //No divisions, no sqrt for test;
      double c = PP - DD;                         //1-
      double b = 2.0 * dotProduct;               //1*
      double t = delayLeft.GetSeconds ();
      double TT = t * t;                          //1*
      double BB = b * b;                          //1*
      double A4 = 4.0 * a;                        //1*
      double LHS = BB - A4 * c;                   //1*1-
      double RHS = A4 * a * TT + BB + A4 * b * t; //4*2+
      if (LHS < RHS)
        {
          NS_LOG_DEBUG ("We cross m_deltaPosition before we cross m_timeout");
          double DP = dotProduct * dotProduct;
          t = (sqrt (DD * a + DP - PV * PV) + dotProduct) / a; //2*1/3 + 1sqrt
          SchedulePositionChange (Seconds (t));
          if (m_timeoutScheduled)
            {
              CancelTimeout ();
            }
        }
      else
        {
          NS_LOG_DEBUG ("We cross timeout threshold before we cross DeltaPosition");
          if (!m_timeoutScheduled)
            {
              ScheduleTimeout ();
            }
        }
    }
}

void
PositionAware::HandleTimeout ()
{
  NS_LOG_FUNCTION (this);
  m_timeoutTrace (this);
  ScheduleNext ();
}

void
PositionAware::HandlePositionChange ()
{
  NS_LOG_FUNCTION (this);
  m_positionChangeTrace (this);
  ScheduleNext ();
}

void
PositionAware::ScheduleNext ()
{
  NS_LOG_FUNCTION (this);
  // update position, m_speed should be same as set by CourseChange()
  m_referencePosition = m_mobilityPtr->GetPosition ();
  m_lastEvent = Simulator::Now ();
  //Assume that this is called by HandleTimeout or HandlePosition
  //Don't assume timer or position change is repeated
  //assume we are starting fresh
  if (m_timeoutScheduled)
    {
      CancelTimeout ();
    }
  if (m_timeout.GetSeconds () * m_speed >= m_deltaPosition)
    {
      double t = m_deltaPosition / m_speed;
      NS_LOG_DEBUG ("Scheduling position change in " << t << " sec");
      SchedulePositionChange (Seconds (t));
    }
  else
    {
      NS_LOG_DEBUG ("Scheduling timeout change in " << m_timeout.GetSeconds () << " sec");
      ScheduleTimeout ();
      if (m_positionChangeScheduled)
        {
          CancelPositionChange ();
        }
    }
}

void
PositionAware::CancelPositionChange ()
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_scheduledEvent);
  m_positionChangeScheduled = false;
}

void
PositionAware::CancelTimeout ()
{
  NS_LOG_FUNCTION (this);
  if (m_timeoutScheduled)
    {
      NS_LOG_DEBUG ("There is a timer to cancel");
      m_timeoutTimer.Cancel ();
      m_timeoutScheduled = false;
    }
}

void
PositionAware::ScheduleTimeout ()
{
  NS_LOG_FUNCTION (this);
  if (m_timeoutScheduled)
    {
      CancelTimeout ();
    }
  if (Seconds (0) == m_timeout)
    {
      NS_LOG_DEBUG ("Timeout set to 0, timeout notifications disabled");
    }
  if (m_timeout > (Simulator::Now () - m_lastEvent))
    {
      m_timeoutTimer.Schedule (m_timeout - (Simulator::Now () - m_lastEvent) );     //remaining time in timeout threshold since last event
      NS_LOG_DEBUG ("Timeout scheduled for " << (Simulator::Now () + (m_timeout - (Simulator::Now () - m_lastEvent))).As (Time::S));
      m_timeoutScheduled = true;
    }
  else
    {
      NS_LOG_ERROR ("Timeout not set because m_timeout <= Now()- m_lastEvent");
    }
}

void
PositionAware::SchedulePositionChange (const Time& t)
{
  NS_LOG_FUNCTION (this);
  if (0.0 == m_deltaPosition)
    {
      NS_LOG_DEBUG ("Delta Position is 0; position notification disabled");
      return;
    }
  m_scheduledEvent =  Simulator::Schedule (t, &PositionAware::HandlePositionChange, this);
  NS_LOG_DEBUG ("Position change scheduled for " << (Simulator::Now () + t).As (Time::S));
  m_positionChangeScheduled = true;
}

void
PositionAware::NotifyNewAggregate ()
{
  NS_LOG_FUNCTION (this);
  if ((0.0 == m_deltaPosition) && (Seconds (0) == m_timeout))
    {
      NS_LOG_WARN ("Both delta_position and timeout set to 0, PositionChange is effectively disabled");
    }

  if (false == m_aggregated)
    {
      if (GetObject<Node> ())
        {
          NS_LOG_DEBUG ("Node: " << GetObject<Node> ()->GetId ());
        }
      m_mobilityPtr = GetObject<MobilityModel> ();
      if (nullptr == m_mobilityPtr)
        {
          NS_FATAL_ERROR ("Must install Mobility before PostionAware");
        }
      m_mobilityPtr->TraceConnectWithoutContext ("CourseChange",
                                                   MakeCallback (&PositionAware::CourseChange,
                                                                  this)
                                                   );
      m_speed = CalculateDistance (Vector (), m_mobilityPtr->GetVelocity ());
      ScheduleNext ();
      m_aggregated = true;
    }
  else
    {
      NS_LOG_DEBUG ("Already Aggregated");
    }
  Object::NotifyNewAggregate ();
}

} //namespace ns3
