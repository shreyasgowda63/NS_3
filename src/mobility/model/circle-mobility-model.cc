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

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CircleMobilityModel");
NS_OBJECT_ENSURE_REGISTERED (CircleMobilityModel);

TypeId
CircleMobilityModel::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::CircleMobilityModel")
          .SetParent<MobilityModel> ()
          .SetGroupName ("Mobility")
          .AddConstructor<CircleMobilityModel> ()
          .AddAttribute ("Mode",
                         "The mode affects how the model is initialized",
                         EnumValue (CircleMobilityModel::INITIALIZE_RANDOM),
                         MakeEnumAccessor (&CircleMobilityModel::m_mode),
                         MakeEnumChecker (CircleMobilityModel::INITIALIZE_ATTRIBUTE, "Attribute",
                                          CircleMobilityModel::INITIALIZE_RANDOM, "Random"))

          .AddAttribute ("OriginConfigMode",
                         "The origin config mode affects how the origin is initialized",
                         EnumValue (CircleMobilityModel::RADIUS_AWAY_FROM_POSITION),
                         MakeEnumAccessor (&CircleMobilityModel::m_OriginConfigMode),
                         MakeEnumChecker (CircleMobilityModel::ORIGIN_FROM_ATTRIBUTE,"OFA",
                                          CircleMobilityModel::RADIUS_AWAY_FROM_POSITION,"RAP",
                                          CircleMobilityModel::POSITION_AS_ORIGIN,"PAO"))
                                          
         .AddAttribute ("Origin", "Origin for circular motion",
                         VectorValue (Vector (0, 0, 0)),
                         MakeVectorAccessor (&CircleMobilityModel::SetOrigin,
                                             &CircleMobilityModel::GetOrigin),
                         MakeVectorChecker ())

         .AddAttribute ("Radius", "Radius (m) for circular motion",
                         DoubleValue (0),
                         MakeDoubleAccessor (&CircleMobilityModel::SetRadius,
                                             &CircleMobilityModel::GetRadius),
                         MakeDoubleChecker <double>(0))
          .AddAttribute ("StartAngle", "Start angle (degrees) for circular motion",
                         DoubleValue (0),
                         MakeDoubleAccessor (&CircleMobilityModel::SetStartAngle,
                                             &CircleMobilityModel::GetStartAngle),
                         MakeDoubleChecker <double>(0, 360))
          .AddAttribute ("Speed", "Speed (m/s) for circular motion",
                         DoubleValue (0),
                         MakeDoubleAccessor (&CircleMobilityModel::SetSpeed,
                                             &CircleMobilityModel::GetSpeed),
                         MakeDoubleChecker<double> (0))
          .AddAttribute ("Clockwise", "The direction of circular movement.", 
                         BooleanValue (false),
                         MakeBooleanAccessor (&CircleMobilityModel::SetClockwise,
                                              &CircleMobilityModel::GetClockwise),
                         MakeBooleanChecker ())
         .AddAttribute ("RandomOriginX",
                         "A random variable used to pick the origin x-axis coordinate (m).",
                         StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"),
                         MakePointerAccessor (&CircleMobilityModel::m_randomOriginX),
                         MakePointerChecker<RandomVariableStream> ())
          .AddAttribute ("RandomOriginY",
                         "A random variable used to pick the origin y-axis coordinate (m).",
                         StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"),
                         MakePointerAccessor (&CircleMobilityModel::m_randomOriginY),
                         MakePointerChecker<RandomVariableStream> ())
          .AddAttribute ("RandomOriginZ",
                         "A random variable used to pick the origin z-axis coordinate (m).",
                         StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"),
                         MakePointerAccessor (&CircleMobilityModel::m_randomOriginZ),
                         MakePointerChecker<RandomVariableStream> ())
          .AddAttribute ("RandomRadius",
                         "A random variable used to pick the radius (m).",
                         StringValue ("ns3::UniformRandomVariable[Min=100.0|Max=1500.0]"),
                         MakePointerAccessor (&CircleMobilityModel::m_randomRadius),
                         MakePointerChecker<RandomVariableStream> ())
          .AddAttribute ("RandomStartAngle",
                         "A random variable used to pick the start angle (degrees).",
                         StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=360.0]"),
                         MakePointerAccessor (&CircleMobilityModel::m_randomStartAngle),
                         MakePointerChecker<RandomVariableStream> ())
          .AddAttribute ("RandomSpeed",
                         "A random variable used to pick the speed (m/s).",
                         StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=100.0]"),
                         MakePointerAccessor (&CircleMobilityModel::m_randomSpeed),
                         MakePointerChecker<RandomVariableStream> ()) 
  /*       .AddAttribute ("RandomClockwise",
                         "A random variable used to select clockwise (true) or counter-clockwise (false) direction. " 
                         StringValue ("ns3::BernoulliRandomVariable[Mean=0.5]"),
                         MakePointerAccessor (&CircleMobilityModel::m_randomClockwise),
                         MakePointerChecker<BernoulliRandomVariable> ());*/
      ;
  return tid;
}

int64_t
CircleMobilityModel::DoAssignStreams (int64_t stream)
{
  m_randomOriginX->SetStream (stream);
  m_randomOriginY->SetStream (stream + 1);
  m_randomOriginZ->SetStream (stream + 2);
  m_randomRadius->SetStream (stream + 3);
  m_randomStartAngle->SetStream (stream + 4);
  m_randomSpeed->SetStream (stream + 5);
  m_randomRadius->SetStream (stream + 6);
  //m_randomClockwise->SetStream (stream + 3);
  return 4;
}

const Vector3D &
CircleMobilityModel::GetOrigin () const
{
  NS_LOG_FUNCTION (this);
  return m_origin;
}

void
CircleMobilityModel::SetOrigin(const Vector3D &origin)
{
  NS_LOG_FUNCTION (this << origin);
  m_origin = origin;
}


double
CircleMobilityModel::GetRadius () const
{
  NS_LOG_FUNCTION (this);
  return m_radius;
}


void 
CircleMobilityModel::SetRadius(const double radius)
{
  NS_LOG_FUNCTION (this << radius);  
  //NS_LOG_DEBUG ("value error");
  NS_ASSERT (radius > 0);
  m_radius = radius;
}

void 
CircleMobilityModel::SetStartAngle(const double startAngle)
{
  NS_LOG_FUNCTION (this << startAngle);  
  //NS_LOG_DEBUG ("value error");
  NS_ASSERT ((startAngle < 0||startAngle >360));
  m_startAngle = startAngle;
}

double 
CircleMobilityModel::GetStartAngle() const
{
  NS_LOG_FUNCTION (this);
  return m_startAngle;
}

void 
CircleMobilityModel::SetSpeed(const double speed)
{
  NS_LOG_FUNCTION (this << speed);  
  //NS_LOG_DEBUG ("value error");
  NS_ASSERT (speed < 0);
  m_speed = speed;
}

double 
CircleMobilityModel::GetSpeed() const
{
  NS_LOG_FUNCTION (this);
  return m_speed;
}

void 
CircleMobilityModel::SetClockwise(const bool clockwise)
{
  NS_LOG_FUNCTION (this << clockwise);  
  m_clockwise = clockwise;
}

bool 
CircleMobilityModel::GetClockwise() const
{
  NS_LOG_FUNCTION (this);
  return m_clockwise;
}

void
CircleMobilityModel::DoInitialize (void)
{
if(!m_parametersInitialized){ // in case it was already called from somewhere else
  InitializePrivate();
}
}

/**
 * @brief In the function InitializePrivate, the  initial variables are set according to configuration settings.
 */
void
CircleMobilityModel::InitializePrivate(void)
{
  //set  radius, start angle and speed according to default or selected range
 
 switch (m_mode)
 {
 case INITIALIZE_RANDOM:
    //set the parameters after checking them in setters
    // it will override the value that may already set through setters
    SetRadius(m_randomRadius->GetValue ());
    SetStartAngle(m_randomStartAngle->GetValue ());
    SetSpeed(m_randomSpeed->GetValue ());
    SetRadius(m_randomRadius->GetValue ());
    //SetClockwise(m_randomClockwise->GetValue()); //completed after the MR of BernoulliRandomVariable
   break;
 case INITIALIZE_ATTRIBUTE:
   /* In this case the value may be already set by setters*/
   break; 
 default:
   break;
 }

  double cosAngle, sinAngle;
  switch (m_OriginConfigMode)
  {
  case ORIGIN_FROM_ATTRIBUTE:
        //set origin randomly according to default or selected /randomly selected range
      m_origin=Vector(m_randomOriginX->GetValue (),m_randomOriginY->GetValue (),m_randomOriginZ->GetValue ());
    break;

  case RADIUS_AWAY_FROM_POSITION:
      // Set Origin of the Circle According to the initial position of the object passed by PositionAllocator or user
      // Usually the possition of the node will be passed by a PositionAllocator
      // calculate the origin of the circle according to the initial position of the object passed by PositionAllocator or user
      cosAngle = cos (m_startAngle);
      sinAngle = sin (m_startAngle);
      m_origin = Vector (m_position.x - m_radius * cosAngle, m_position.y - m_radius * sinAngle, m_position.z);
    break;

  case POSITION_AS_ORIGIN:
      //set position as origin according to choice
      m_origin = m_position;
    break;
  default:
    break;
  }

  m_lastUpdate = Simulator::Now ();
  NotifyCourseChange ();
  m_parametersInitialized=true;
 
}

/*
 * In the function DoGetPosition, the position of the object at the circle 
 * will be calculated by the circle function
*
* This calculations are inspired from and circle function logic of circle mobility model of Omnet++ by Andras Varga
* In Omnet++, they did radian-degree conversions
* But here we are doing the angle math in degrees only
 */

Vector
CircleMobilityModel::DoGetPosition (void) const
{ 
  if(!m_parametersInitialized){
    const_cast<CircleMobilityModel *>(this)->InitializePrivate();
  }
  Time now = Simulator::Now ();
  NS_ASSERT (m_lastUpdate <= now);
  m_lastUpdate = now;
  double direction = m_clockwise ? 1 : -1;
  double angle = m_startAngle + ((direction * m_speed / m_radius) * now.GetSeconds ());
  double cosAngle = cos (angle);
  double sinAngle = sin (angle);
  return Vector (m_origin.x + m_radius * cosAngle, m_origin.y + m_radius * sinAngle, m_origin.z);
}

void
CircleMobilityModel::DoSetPosition (const Vector &position)
{
  //if the PositionAllocator or user sets the position then initialize variables accordingly
  //this will have impact if UseInitialPositionAsOrigin=true
  //this will not have impact if UseConfiguredOrigin=true
  m_position=position;
  InitializePrivate();
}


/**
 * @brief In the function SetParameters, the variables are set.
 * 
 */
void
CircleMobilityModel::SetParameters (const Vector &Origin, const double Radius,
                                    const double StartAngle, const bool Clockwise,
                                    const double Speed)
{
  SetOrigin(Origin);
  SetRadius(Radius);
  SetStartAngle(StartAngle);
  SetClockwise(Clockwise);
  SetSpeed(Speed);
  NotifyCourseChange ();
}

/*
 * In the function DoGetVelocity, the velocity of the object at the circle 
 * will be calculated 
*
* This calculations are inspired from  mobility Model of Omnet++ by Andras Varga
* In Omnet++, they did radian-degree conversions 
* But here we are doing the angle math in degrees only
 */
Vector
CircleMobilityModel::DoGetVelocity (void) const
{
  Time now = Simulator::Now ();
  NS_ASSERT (m_lastUpdate <= now);
  m_lastUpdate = now;
  double direction = m_clockwise ? 1 : -1;
  double angle = m_startAngle + ((direction * m_speed / m_radius) * now.GetSeconds ());
  double cosAngle = cos (angle);
  double sinAngle = sin (angle);
  return Vector (-sinAngle * m_speed, cosAngle * m_speed, 0.0);
}

} // namespace ns3
