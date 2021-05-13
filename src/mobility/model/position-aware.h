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
/** @ingroup PositionAware 
 * @brief Buffer position updates with space and time thresholds
 *
 * Instances of this class trace the CourseChanges of a node's mobility model.
 * When a node has moved more than a given distance from a given reference
 * position, or a given amount of time has passed since the last
 * PositionChange, a PositionChange is announced.
 */
class PositionAware : public Object
{
 protected:
  typedef ns3::PositionAware  ThisType; ///< Type of this
  typedef ns3::Object         BaseType; ///< Type of parent
 public:
  /// \copydoc Object::GetTypeId(void)
  static TypeId GetTypeId (void);
  /// Constructor
  PositionAware ();
  /// Destructor
  virtual ~PositionAware ();

  /** Get dL (threshold m_distance)
   * @returns dL (threshold m_distance) */
  double GetDistance () const;
  /** Set dL (threshold m_distance)
   * @param _m_distance The new threshold m_distance */
  void SetDistance (const double& _m_distance);
  /** Get dT (threshold time)
   * @returns dT (threshold time) */
  Time GetTimeout () const;
  /** set dT (threshold time)
   * @param _m_timeout Time until m_timeout */
  void SetTimeout (const Time& _m_timeout);
  /** Get reference position
   * @returns reference position */
  Vector GetPosition () const;
  /** Set reference position
   * @param _position Reference position */
  void SetPosition (const Vector& _position);
  /** Return a pointer to the mobility model associated with this instance of position-aware
   * @returns Pointer to a mobility model */
  inline Ptr<MobilityModel> GetMobilityModelPointer () const {return m_mobility_ptr;}
 protected:
  /** Overriden so we can automatically add callback to MobilityModel for
   * course change notification */
  virtual void NotifyNewAggregate (void);
 private:
  /** Used for MobilityModel callback 
   * @param _mobility_model Pointer to the mobility model that trigger the callback */
  void CourseChange (Ptr<const MobilityModel> _mobility_model);
  /** Called by timer when a m_timeout occurs */
  void HandleTimeout ();
  /** Events are scheduled to call this method with m_distance is crossed */
  void HandlePositionChange ();
  /** Determines if a m_timeout or position change should be scheduled based on
   * current m_speed and m_timeout */
  void ScheduleNext ();
  /** Unschedule a position change event in the case of a course change */
  void CancelPositionChange ();
  /** Cancels timer in the case of a course change */
  void CancelTimeout ();
  /** Schedule a new position change event
   * @param _t time that m_distance will be reached */
  void SchedulePositionChange (const Time& _t);
  /** Start the m_timeout timer from 'now' */
  void ScheduleTimeout ();
 private:
  /// Threshold m_distance dL to trigger a position change at
  double m_distance;
  /// Time to wait for position change before timing out
  Time m_timeout;
  /// Current m_speed as determined at last course change
  double m_speed;
  /// Position determined at last event, used as reference for position change
  Vector m_last_position;
  /// Time of last position change/timeout
  Time m_last_event;
  /// Timer object used to schedule m_timeouts
  Timer m_timeout_timer;
  /// Indicates a m_distance event is currently scheduled
  bool m_distance_scheduled;
  /// Indicates a m_timeout event is currently scheduled
  bool m_timeout_scheduled;
  /// Pointer to mobility object for easy access
  Ptr<MobilityModel> m_mobility_ptr;
  /// Callback stack for position change events
  TracedCallback<Ptr<const PositionAware> > m_position_change_trace;
  /// Callback stack for m_timeout events
  TracedCallback<Ptr<const PositionAware> > m_timeout_trace;
  /// Event of scheduled position change
  EventId m_scheduled_event;
  /// Flag indicated that this position aware has been aggregated and does not need to be aggregated again.
  bool m_aggregated;
};
}//namespace ns3

#endif//POSITION_AWARE_H
