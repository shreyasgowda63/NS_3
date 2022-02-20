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

#ifndef POSITION_AWARE_H
#define POSITION_AWARE_H

#include "ns3/event-id.h"
#include "ns3/mobility-model.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/timer.h"
#include "ns3/traced-callback.h"
#include "ns3/vector.h"

namespace ns3 {
/** 
 * @ingroup mobility
 * @brief Buffer position updates with space and time thresholds
 *
 * Instances of this class register with the CourseChange trace of
 * a mobility model.  When the mobility model has moved more than a specified 
 * distance from a specified reference position, or a specified amount of
 * time has passed since the last PositionChange, a PositionChange is
 * reported through the trace source.
 * 
 * By default, the PositionChange trace source is effectively disabled.
 * It can be enabled by setting * the DeltaPosition and ReferencePosition attributes
 * for distance-based tracing, and/or the Timeout attribute for time-based
 * tracing.
 */
class PositionAware : public Object
{
public:
  /// \copydoc Object::GetTypeId(void)
  static TypeId GetTypeId (void);
  /// Constructor
  PositionAware ();
  /// Destructor
  virtual ~PositionAware ();

  /** Get DeltaPosition value
   * @returns DeltaPosition value */
  double GetDeltaPosition () const;
  /** Set DeltaPosition value
   * @param deltaPosition The new DeltaPosition value */
  void SetDeltaPosition (const double& deltaPosition);
  /** Get threshold time
   * @returns threshold time */
  Time GetTimeout () const;
  /** set threshold timeout value
   * @param timeout threshold timeout value */
  void SetTimeout (const Time& timeout);
  /** Get reference position
   * @returns reference position */
  Vector GetReferencePosition () const;
  /** Set reference position
   * @param position Reference position */
  void SetReferencePosition (const Vector& position);

protected:
  /** Overriden so we can automatically add callback to MobilityModel for
   * course change notification */
  void NotifyNewAggregate (void) override;
  void DoInitialize() override;
  

private:
  /** Used for MobilityModel callback
   * @param mobilityModel Pointer to the mobility model that trigger the callback */
  void CourseChange (Ptr<const MobilityModel> mobilityModel);
  /** Called by timer when a timeout occurs */
  void HandleTimeout ();
  /** Events are scheduled to call this method when DeltaPosition is crossed */
  void HandlePositionChange ();
  /** Determines if a timeout or position change should be scheduled based on
   * current speed and timeout */
  void ScheduleNext ();
  /** Cancel a position change event in the case of a course change */
  void CancelPositionChange ();
  /** Cancels timer in the case of a course change */
  void CancelTimeout ();
  /** Schedule a new position change event
   * @param t time that m_deltaPosition will be reached */
  void SchedulePositionChange (const Time& t);
  /** Start the timer */
  void ScheduleTimeout ();

private:
  /// Threshold distance to trigger a position change at
  double m_deltaPosition;
  /// Time to wait for position change before timing out
  Time m_timeout;
  /// Current m_speed as determined at last course change
  double m_speed;
  /// Position used as reference for position change
  Vector m_referencePosition;
  /// Time of last position change/timeout
  Time m_lastEvent;
  /// Timer object used to schedule timeouts
  Timer m_timeoutTimer;
  /// Indicates a PositionChange event is currently scheduled
  bool m_positionChangeScheduled;
  /// Indicates a Timeout event is currently scheduled
  bool m_timeoutScheduled;
  /// Pointer to mobility object for easy access
  Ptr<MobilityModel> m_mobilityPtr;
  /// Callback stack for position change events
  TracedCallback<Ptr<const PositionAware> > m_positionChangeTrace;
  /// Callback stack for m_timeout events
  TracedCallback<Ptr<const PositionAware> > m_timeoutTrace;
  /// Event of scheduled position change
  EventId m_scheduledEvent;
  /// Flag indicating whether this object has been aggregated
  bool m_aggregated;
};
} //namespace ns3

#endif //POSITION_AWARE_H
