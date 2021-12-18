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
*                  and Circle Function Logic of Circle Mobility Model of Omnet++ by Andras Varga
*/
#include "circle-mobility-model.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (CircleMobilityModel);

TypeId
CircleMobilityModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CircleMobilityModel")
   .SetParent<MobilityModel> ()
   .SetGroupName ("Mobility")
   .AddConstructor<CircleMobilityModel> ()
  ;
  return tid;
}

CircleMobilityModel::CircleMobilityModel ()
{
}

CircleMobilityModel::~CircleMobilityModel ()
{
}

/*
 * In the function DoGetPosition, the position of the object at the circle 
 * will be calculated by the circle function
*
* This calculations are inspired from and Circle Function Logic of Circle Mobility Model of Omnet++ by Andras Varga
* In omnet++, they did radian-degree conversions
* But here we are doing the angle math in degrees only
 */

Vector
CircleMobilityModel::DoGetPosition (void) const
{
   Time now = Simulator::Now ();
   NS_ASSERT (m_lastUpdate <= now);
   m_lastUpdate = now;
   double angle = m_StartAngle+ ((m_Speed/m_Radius) * now.GetSeconds());
   double cosAngle = cos(angle);
   double sinAngle = sin(angle);
   return Vector( m_Origin.x + m_Radius * cosAngle,m_Origin.y + m_Radius * sinAngle,m_Origin.z);
}

void
CircleMobilityModel::DoSetPosition (const Vector &position)
{
   m_position = position;
   m_lastUpdate = Simulator::Now ();
   NotifyCourseChange ();
}

/*
 * In the function DoGetVelocity, the velosity of the object at the circle 
 * will be calculated 
*
* This calculations are inspired from  Mobility Model of Omnet++ by Andras Varga
* In omnet++, they did radian-degree conversions 
* But here we are doing the angle math in degrees only
 */
Vector
CircleMobilityModel::DoGetVelocity (void) const
{
   Time now = Simulator::Now ();
   NS_ASSERT (m_lastUpdate <= now);
    m_lastUpdate = now;
   double angle = m_StartAngle+ ((m_Speed/m_Radius) * now.GetSeconds());
   double cosAngle = cos(angle);
   double sinAngle = sin(angle);   
    return Vector ( -sinAngle * m_Speed, cosAngle * m_Speed, 0.0);
}

void
CircleMobilityModel::UpdateCourseChange (void)
{
   Time next = Simulator::Now ()+Seconds(1);
   NotifyCourseChange ();
   m_event = Simulator::Schedule (Seconds(0.1), &CircleMobilityModel::UpdateCourseChange,this);
}

/*
 * In the function SetParameters, the  initial parameters are set.
 */
void 
CircleMobilityModel::SetParameters(const Vector &Origin, const double Radius, const double StartAngle, const double Speed)
{
   DoSetPosition(Origin);
   m_Origin=Origin;
   m_Radius=Radius;
   m_StartAngle=StartAngle;
   m_Speed=Speed;	
   NotifyCourseChange ();
}

} // namespace ns3
