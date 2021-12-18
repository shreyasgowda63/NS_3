/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
* Copyright (c) 2021 Charles Pandian, ProjectGuideline.com
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
* Author: Charles Pandian<igs3000@gmail.com>
* Insprired from: ConstantPositionMobilityModel of ns-3 by Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
*                          and Circle Function Logic of Circle Mobility Model of Omnet++ by Andras Varga
*/
#ifndef CIRCLE_MOBILITY_MODEL_H
#define CIRCLE_MOBILITY_MODEL_H

#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/nstime.h"

#include "mobility-model.h"


namespace ns3 {

/**
 * \ingroup mobility
 * \brief 3D Circle mobility model.
 *
 * The Movement of the Object will be controlled by patameters Origin, Radius, StartAngle and Speed
 * This mobility model enforces no bounding box by itself; 
 * The implementation of this model is not 2d-specific. i.e. if you provide
 * z value greater than 0, then you may use it in 3d scenarios
 */
class CircleMobilityModel : public MobilityModel 
{
public:
  /**
   * Register this type with the TypeId system.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  CircleMobilityModel ();
  virtual ~CircleMobilityModel ();
  /**
   * Sets patameters Origin, Radius, StartAngle and Speed
   */
    void SetParameters(const Vector &Origin, const double Radius, const double StartAngle, const double Speed);
private:
  virtual Vector DoGetPosition (void) const;
  virtual void DoSetPosition (const Vector &position);
  virtual Vector DoGetVelocity (void) const;
  void UpdateCourseChange (void);
  Vector m_position; //!< the  position
  Vector m_Origin;   //!< the  origin of the circle
  EventId m_event; //!< event ID of next scheduled event
  double m_Radius;//!< the  radius of the circle
  double m_StartAngle;//!< the  start angle of the circle
  double m_Speed;//!< the  speed of the object
  mutable Time m_lastUpdate;//!< the  last upsate time

};

} // namespace ns3

#endif /* CIRCLE_MOBILITY_MODEL_H */
