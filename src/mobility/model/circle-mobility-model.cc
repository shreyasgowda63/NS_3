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
#include <ns3/double.h>
#include <ns3/boolean.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CircleMobilityModel");
NS_OBJECT_ENSURE_REGISTERED (CircleMobilityModel);

TypeId
CircleMobilityModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CircleMobilityModel")
   .SetParent<MobilityModel> ()
   .SetGroupName ("Mobility")
   .AddConstructor<CircleMobilityModel> ()
   .AddAttribute ("origin", "origin of the circle (x,y,z) in meters.", 
                   Vector3DValue (Vector (100, 100, 100)),
                   MakeVector3DAccessor (&CircleMobilityModel::m_origin),
                   MakeVector3DChecker ())
   .AddAttribute ("radius","The radius of the circle in meters.", 
                   DoubleValue (100),
                   MakeDoubleAccessor (&CircleMobilityModel::m_radius),
		   MakeDoubleChecker<double> (1,10000))
   .AddAttribute ("startAngle", "The starting angle  if the circle in degree.", 
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&CircleMobilityModel::m_startAngle),
		   MakeDoubleChecker<double> (0,360))
   .AddAttribute ("speed", "The speed of the object (m/s).", 
	           DoubleValue (30.0),
                   MakeDoubleAccessor (&CircleMobilityModel::m_speed),
                   MakeDoubleChecker<double> (0,100))
   .AddAttribute ("clockwise", "The direction of movement", 
	           BooleanValue (false),
                   MakeBooleanAccessor (&CircleMobilityModel::m_clockwise),
                   MakeBooleanChecker ())
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
   double direction = m_clockwise?1:-1;
   double angle = m_startAngle+ ((direction*m_speed/m_radius) * now.GetSeconds());
   double cosAngle = cos(angle);
   double sinAngle = sin(angle);
   return Vector( m_origin.x + m_radius * cosAngle, m_origin.y + m_radius * sinAngle, m_origin.z);
}

void
CircleMobilityModel::DoSetPosition (const Vector &position)
{
   //Set Origin of the Circle According to the initial position of the object passed by PositionAllocator or user
   // Usually the possition of the node will be passed by PositionAllocator
   double cosAngle = cos(m_startAngle);
   double sinAngle = sin(m_startAngle);
   m_origin = Vector( position.x - m_radius * cosAngle, position.y - m_radius * sinAngle, position.z);
   m_lastUpdate = Simulator::Now ();
   NotifyCourseChange ();
}

void
CircleMobilityModel::DoInitialize (void)
{
   //~ Vector position = MobilityModel::GetPosition();
	

  //~ // calclulate the origin of the circle cccording to the initial position of the object passed by PositionAllocator or user
   //~ double cosAngle = cos(m_startAngle);
   //~ double sinAngle = sin(m_startAngle);
   //~ Vector t_origin = Vector( position.x - m_radius * cosAngle, position.y - m_radius * sinAngle, position.z);
    //~ // checks that the initial positions are Radius away from the configured origin,
    //~ // by comparing the given origin and the calculated origin with respect to the initial position of the node
    //~ //if not use  calculated origin w.r.t the initial positions	as new origin
   //~ if(t_origin.x != m_origin.x || t_origin.y != m_origin.y || t_origin.z != m_origin.z ){
    //~ m_origin = t_origin;
    //~ m_lastUpdate = Simulator::Now ();
    //~ NotifyCourseChange ();
   //~ }
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
   double direction = m_clockwise?1:-1;
   double angle = m_startAngle + ((direction*m_speed/m_radius) * now.GetSeconds());
   double cosAngle = cos(angle);
   double sinAngle = sin(angle);   
   return Vector ( -sinAngle * m_speed, cosAngle * m_speed, 0.0);
}

} // namespace ns3
