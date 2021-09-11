/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Buliao Chen
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
 * Author: Buliao Chen <220190896@seu.edu.cn>
 */
#include <ns3/simulator.h>
#include <algorithm>
#include <cmath>
#include <ns3/log.h>
#include <ns3/string.h>
#include <ns3/pointer.h>
#include "semi-random-circular-mobility-model.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SemiRandomCircularMobilityModel");

NS_OBJECT_ENSURE_REGISTERED (SemiRandomCircularMobilityModel);

SemiRandomCircularMobilityModel::SemiRandomCircularMobilityModel () :
  m_interval (0.1)
{
}

TypeId
SemiRandomCircularMobilityModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SemiRandomCircularMobilityModel")
    .SetParent<MobilityModel> ()
    .SetGroupName ("Mobility")
    .AddConstructor<SemiRandomCircularMobilityModel> ()
    .AddAttribute ("Speed", "A random variable to control the speed (m/s).",
                   StringValue ("ns3::UniformRandomVariable[Min=1.0|Max=2.0]"),
                   MakePointerAccessor (&SemiRandomCircularMobilityModel::m_speed),
                   MakePointerChecker<RandomVariableStream> ())
    .AddAttribute ("Pause", "A random variable to control the pause (s).",
                   StringValue ("ns3::ConstantRandomVariable[Constant=2.0]"),
                   MakePointerAccessor (&SemiRandomCircularMobilityModel::m_pause),
                   MakePointerChecker<RandomVariableStream> ())
    .AddAttribute ("Angle", "A random variable to control the angle (degree).",
                   StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=180.0]"),
                   MakePointerAccessor (&SemiRandomCircularMobilityModel::m_angle),
                   MakePointerChecker<RandomVariableStream> ())
    .AddAttribute ("TuringRadius", "A random variable to control the radius (m).",
                   StringValue ("ns3::UniformRandomVariable[Min=0.01|Max=200.0]"),
                   MakePointerAccessor (&SemiRandomCircularMobilityModel::m_radius),
                   MakePointerChecker<RandomVariableStream> ())
    .AddAttribute ("FlyingHeight", "A random variable to control the flying height (m).",
                   StringValue ("ns3::UniformRandomVariable[Min=80.0|Max=100.0]"),
                   MakePointerAccessor (&SemiRandomCircularMobilityModel::m_height),
                   MakePointerChecker<RandomVariableStream> ())
  ;
  return tid;
}

int64_t
SemiRandomCircularMobilityModel::DoAssignStreams (int64_t stream)
{
  m_angle->SetStream (stream);
  m_speed->SetStream (stream + 1);
  m_pause->SetStream (stream + 2);
  m_radius->SetStream (stream + 3);
  return 4;
}

void
SemiRandomCircularMobilityModel::DoInitialize (void)
{
  DoInitializePrivate ();
  MobilityModel::DoInitialize ();
}

void
SemiRandomCircularMobilityModel::DoInitializePrivate (void)
{
  Vector initPos = m_helper.GetCurrentPosition ();
  m_startVector.x = initPos.x;
  m_startVector.y = initPos.y;
  m_startVector.z = 0.0;
  m_r = m_startVector.GetLength ();
  m_startVector.z = initPos.z;
  m_a = 0.0;
  DoWalk ();
}

void
SemiRandomCircularMobilityModel::DoWalk ()
{
  m_helper.Update ();
  double nextAngle = m_angle->GetValue () / 180 * M_PI;
  m_s = m_speed->GetValue ();
  double lastTimeValue = m_r * nextAngle / m_s;
  bool finishCircle = false;

  double moveTime = CalculateMoveTime (lastTimeValue, finishCircle);

  Time delay = Seconds (moveTime);
  m_event.Cancel ();
  m_event = Simulator::Schedule (delay, &SemiRandomCircularMobilityModel::MoveInterval, this, lastTimeValue, moveTime, finishCircle);
  //NotifyCourseChange ();
}

void
SemiRandomCircularMobilityModel::MoveInterval (double lastTimeValue, double moveTime, bool finishCircle)
{
  if (lastTimeValue > 0)
    {
      NS_ASSERT (moveTime == m_interval);
    }

  const Vector curPos (std::cos (m_a) * m_startVector.x - std::sin (m_a) * m_startVector.y,
                       std::sin (m_a) * m_startVector.x + std::cos (m_a) * m_startVector.y,
                       0.0);

  double moveAngle = moveTime * m_s / m_r;
  m_a += moveAngle;

  if (finishCircle)
    {
      NS_ASSERT ((m_a - 2 * M_PI) < 0.001 && (m_a - 2 * M_PI) > -0.001);
      const Vector constVelocity ( (m_startVector.x - curPos.x) / moveTime,
                                   (m_startVector.y - curPos.y) / moveTime,
                                   0.0);
      m_helper.SetVelocityOnly (constVelocity);
      m_helper.Unpause ();
      m_helper.Update ();
      NotifyCourseChange ();
      m_event.Cancel ();
      m_event = Simulator::ScheduleNow (&SemiRandomCircularMobilityModel::PauseAndResetTurnRadius, this);
      return;
    }

  const Vector nextPos (std::cos (m_a) * m_startVector.x - std::sin (m_a) * m_startVector.y,
                        std::sin (m_a) * m_startVector.x + std::cos (m_a) * m_startVector.y,
                        0.0);
  const Vector constVelocity ( (nextPos.x - curPos.x) / moveTime,
                               (nextPos.y - curPos.y) / moveTime,
                               0.0);
  m_helper.SetVelocityOnly (constVelocity);
  m_helper.Unpause ();
  m_helper.Update ();
  NotifyCourseChange ();

  if (lastTimeValue == 0.0)
    {
      m_event.Cancel ();
      m_event = Simulator::ScheduleNow (&SemiRandomCircularMobilityModel::PauseAndDowalk, this);
    }
  else
    {
      moveTime = CalculateMoveTime (lastTimeValue, finishCircle);
      Time delay = Seconds (moveTime);
      m_event.Cancel ();
      m_event = Simulator::Schedule (delay, &SemiRandomCircularMobilityModel::MoveInterval, this, lastTimeValue, moveTime, finishCircle);
    }
}

double
SemiRandomCircularMobilityModel::CalculateMoveTime (double& lastTimeValue, bool& finishCircle)
{
  double moveTime;
  if (lastTimeValue > m_interval)
    {
      lastTimeValue -= m_interval;
      moveTime =  m_interval;
    }
  else
    {
      moveTime = lastTimeValue;
      lastTimeValue = 0.0;
    }

  double moveAngle = moveTime * m_s / m_r;
  if (moveAngle + m_a > 2 * M_PI)
    {
      moveTime = (2 * M_PI - m_a) / moveAngle * moveTime;
      lastTimeValue = 0.0;
      finishCircle = true;
    }
  return moveTime;
}

void
SemiRandomCircularMobilityModel::PauseAndDowalk ()
{
  m_helper.Pause ();
  Time pause = Seconds (m_pause->GetValue ());
  m_event.Cancel ();
  m_event = Simulator::Schedule (pause, &SemiRandomCircularMobilityModel::DoWalk, this);
}

void
SemiRandomCircularMobilityModel::PauseAndResetTurnRadius ()
{
  m_helper.Pause ();
  Time pause = Seconds (m_pause->GetValue ());
  m_event.Cancel ();
  m_event = Simulator::Schedule (pause, &SemiRandomCircularMobilityModel::ResetTurnRadiusAndHeight, this, 0.0, true);
}

void
SemiRandomCircularMobilityModel::ResetTurnRadiusAndHeight (double distance, bool beginReset)
{
  //Pause:update lasttime but not notify
  m_helper.Update ();
  if (beginReset)
    {
      m_a = 0.0;
      double newRadius = m_radius->GetValue ();
      double newHeight = m_height->GetValue ();
      const Vector newStartPos (m_startVector.x / m_r * newRadius, m_startVector.y / m_r * newRadius, newHeight);
      const Vector oldToNew (newStartPos.x - m_startVector.x, newStartPos.y - m_startVector.y, newHeight - m_startVector.z);

      m_s = m_speed->GetValue ();
      const Vector velocity (oldToNew.x / oldToNew.GetLength () * m_s,
                             oldToNew.y / oldToNew.GetLength () * m_s,
                             oldToNew.z / oldToNew.GetLength () * m_s);
      m_helper.SetVelocityOnly (velocity);

      m_helper.Unpause ();
      //NotifyCourseChange ();
      distance = CalculateDistance (m_startVector, newStartPos);
      m_r = newRadius;
      m_startVector = newStartPos;
    }
  else
    {
      NotifyCourseChange ();
    }

  if (distance == 0.0)
    {
      NS_ASSERT (CalculateDistance (m_helper.GetCurrentPosition (), m_startVector) < 0.001 && CalculateDistance (m_helper.GetCurrentPosition (), m_startVector) > -0.001);
      DoWalk ();
    }
  else
    {
      double moveTime = m_interval;
      if (distance < m_interval * m_s)
        {
          moveTime = distance / m_s;
          distance = 0.0;
        }
      else
        {
          distance -= m_interval * m_s;
        }
      m_event.Cancel ();
      m_event = Simulator::Schedule (Seconds (moveTime), &SemiRandomCircularMobilityModel::ResetTurnRadiusAndHeight, this, distance, false);
    }
}

Vector
SemiRandomCircularMobilityModel::DoGetPosition (void) const
{
  return m_helper.GetCurrentPosition ();
}

void
SemiRandomCircularMobilityModel::DoSetPosition (const Vector &position)
{
  m_helper.SetPosition (position);
  Simulator::Remove (m_event);
  m_event.Cancel ();
  m_event = Simulator::ScheduleNow (&SemiRandomCircularMobilityModel::DoInitializePrivate, this);
}

Vector
SemiRandomCircularMobilityModel::DoGetVelocity (void) const
{
  return m_helper.GetVelocity ();
}

} // namespace ns3