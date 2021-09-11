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
#ifndef SEMI_RANDOM_CIRCULAR_MOBILITY_MODEL_H
#define SEMI_RANDOM_CIRCULAR_MOBILITY_MODEL_H

#include <ns3/object.h>
#include <ns3/ptr.h>
#include <ns3/nstime.h>
#include <ns3/event-id.h>
#include <ns3/random-variable-stream.h>
#include "mobility-model.h"
#include "constant-velocity-helper.h"

namespace ns3

{
/**
 * \ingroup mobility
 * \brief Semi random circular mobility model.
 *
 * Each node selects a turning radius from the beginning,
 * uses the radius as the circular orbit, selects a rotation angle
 * and movement speed, after reaching the destination, pauses
 * for a random time, and continues to select new rotation angle and speed.
 * After a round of movement, select new turning radius, flying height and Speed,
 * move to the new position in a uniform linear motion, switch to the new track,
 * and continue to move according to the original rules.
 *
 * The SRCM model is suitable for simulating UAVs hovering over a specific location gathering information.
 *
 */
class SemiRandomCircularMobilityModel : public MobilityModel
{
public:
  /**
   * Register this type with the TypeId system.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  SemiRandomCircularMobilityModel (void);

private:
  //virtual void DoDispose (void);
  virtual void DoInitialize (void);
  void DoInitializePrivate (void);

  /**
   * After pause or after init, select a new turing angle and begin walk.
   */
  void DoWalk ();
  /**
   * Within a truning angle, move the default interval, maybe finish moving a circle or finish moving a turning angle.
   */
  void MoveInterval (double lastTimeValue, double moveTime, bool finishCircle);
  /**
   * After moving a turning angle, pause and select a new turning angle, continue to move.
   */
  void PauseAndDowalk ();
  /**
   * After finish moving a circle, pause and reset a new turning radius.
   */
  void PauseAndResetTurnRadius ();
  /**
   * Do reset turning radius and flying height, then move to the new track with the default interval.
   */
  void ResetTurnRadiusAndHeight (double distance, bool beginReset);
  /**
   * Within a turning angle, calculate actual move interval for the next step, and check if the node has finished move a circle.
   */
  double CalculateMoveTime (double& lastTimeValue, bool& finishCircle);

  virtual Vector DoGetPosition (void) const;
  virtual void DoSetPosition (const Vector &position);
  virtual Vector DoGetVelocity (void) const;
  virtual int64_t DoAssignStreams (int64_t stream);

  Ptr<RandomVariableStream> m_angle;   //!< random variable to generate turning angle(degree)
  Ptr<RandomVariableStream> m_speed;   //!< random variable to generate speeds
  Ptr<RandomVariableStream> m_pause;   //!< random variable to generate pauses
  Ptr<RandomVariableStream> m_radius;   //!< random variable to generate truning radius
  Ptr<RandomVariableStream> m_height;   //!< random variable to generate flying height
  EventId m_event;   //!< event ID of next scheduled event
  ConstantVelocityHelper m_helper;   //!< helper for velocity computations
  double m_r;   //!< actual turning radius
  double m_a;   //!< actual turing angle(radian)
  double m_interval;   //!< move interval
  double m_s;   //!< actual moving speed
  Vector m_startVector;   //!< Initial turning radius vector on current track
};

}

#endif