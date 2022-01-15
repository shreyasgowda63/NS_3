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
* Insprired from: ConstantPositionMobilityModel of ns-3 by Mathieu Lacage
*                  and circle function logic of circle mobility model of Omnet++ by Andras Varga
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
   .AddAttribute ("MinMaxRadius","The min and max of radius of the circle in meters.", 
                   Vector2DValue (Vector2D(100.0,1500.0)),
                   MakeVector2DAccessor (&CircleMobilityModel::m_radiusMinMax),
                   MakeVector2DChecker())
   .AddAttribute ("MinMaxStartAngle", "The min and max starting angle  if the circle in degree.", 
                   Vector2DValue (Vector2D(0.0,360.0)),
                   MakeVector2DAccessor (&CircleMobilityModel::m_startAngleMinMax),
                   MakeVector2DChecker())
   .AddAttribute ("MinMaxSpeed", "The min and max speed of the object (m/s).", 
                   Vector2DValue (Vector2D(0.0,100.0)),
                   MakeVector2DAccessor (&CircleMobilityModel::m_speedMinMax),
                   MakeVector2DChecker())

   .AddAttribute ("Clockwise", "The direction of circular movement", 
                   BooleanValue (false),
                   MakeBooleanAccessor (&CircleMobilityModel::m_clockwise),
                   MakeBooleanChecker ())
   .AddAttribute ("RandomizeDirection", "The direction of circular movement", 
                   BooleanValue (true),
                   MakeBooleanAccessor (&CircleMobilityModel::m_randomizeDirection),
                   MakeBooleanChecker ())                   

   .AddAttribute ("UseConfiguredOrigin", "Use the origins configured through attribute", 
                   BooleanValue (false),
                   MakeBooleanAccessor (&CircleMobilityModel::m_useConfiguredOrigin),
                   MakeBooleanChecker ())
   .AddAttribute ("UseInitialPositionAsOrigin", "Use the initial position of the node (provided by PositionAllocator) as origin", 
                   BooleanValue (false),
                   MakeBooleanAccessor (&CircleMobilityModel::m_useInitialPositionAsOrigin),
                   MakeBooleanChecker ())

   .AddAttribute ("MinOrigin", "Min origin of the circle in meters.", 
                   Vector3DValue (Vector3D(0.0,0.0,0.0)),
                   MakeVector3DAccessor (&CircleMobilityModel::m_originMin),
                   MakeVector3DChecker())
   .AddAttribute ("MaxOrigin", "Max origin of the circle in meters.", 
                   Vector3DValue (Vector3D(1000.0,1000.0,1000.0)),
                   MakeVector3DAccessor (&CircleMobilityModel::m_originMax),
                   MakeVector3DChecker())                 

  ;
  return tid;
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
   Ptr<UniformRandomVariable> rn = CreateObject<UniformRandomVariable> ();
   m_radius =  rn->GetValue(m_radiusMinMax.x, m_radiusMinMax.y);
   m_startAngle = rn->GetValue(m_startAngleMinMax.x, m_startAngleMinMax.x );
   m_speed =  rn->GetValue(m_speedMinMax.x, m_speedMinMax.y);
   if (m_randomizeDirection){
     m_clockwise = rn->GetValue(1,100)>50? false:true;
   } 
   if(m_useInitialPositionAsOrigin){
      m_origin = position;

   } else
   if(m_useConfiguredOrigin){
      m_origin=Vector (rn->GetValue(m_originMin.x,m_originMax.x), rn->GetValue(m_originMin.y,m_originMax.y),rn->GetValue(m_originMin.z,m_originMax.z));
   } else {
      // Set Origin of the Circle According to the initial position of the object passed by PositionAllocator or user
      // Usually the possition of the node will be passed by a PositionAllocator
      // calclulate the origin of the circle according to the initial position of the object passed by PositionAllocator or user
      double cosAngle = cos(m_startAngle);
      double sinAngle = sin(m_startAngle);
      m_origin = Vector( position.x - m_radius * cosAngle, position.y - m_radius * sinAngle, position.z);
   }
   m_lastUpdate = Simulator::Now ();
   NotifyCourseChange (); 	
}

/*
 * In the function SetParameters, the  initial parameters are set.
 */
void 
CircleMobilityModel::SetParameters(const Vector &Origin, const double Radius, const double StartAngle, const bool Clockwise, const double Speed)
{
   m_origin = Origin;
   m_radius = Radius;
   m_startAngle = StartAngle;
   m_clockwise = Clockwise;
   m_speed = Speed;	
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
   double direction = m_clockwise?1:-1;
   double angle = m_startAngle + ((direction*m_speed/m_radius) * now.GetSeconds());
   double cosAngle = cos(angle);
   double sinAngle = sin(angle);   
   return Vector ( -sinAngle * m_speed, cosAngle * m_speed, 0.0);
}

} // namespace ns3
