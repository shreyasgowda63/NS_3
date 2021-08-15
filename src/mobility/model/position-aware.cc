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

#define NS_LOG_APPEND_CONTEXT						\
    if ( m_aggregated) { std::clog << "[node " << GetObject<Node> ( )->GetId ( )  << "] "; }

#include "position-aware.h"
#include <ns3/double.h>
#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/trace-source-accessor.h>
#include <ns3/node.h>

NS_LOG_COMPONENT_DEFINE ( "PositionAware");
namespace ns3{
NS_OBJECT_ENSURE_REGISTERED ( PositionAware);

PositionAware::PositionAware ( ):
    m_aggregated ( false)
{
  NS_LOG_FUNCTION ( this);
  m_timeout_timer.SetFunction ( &PositionAware::HandleTimeout,
                             this);
}

PositionAware::~PositionAware ( )
{
  NS_LOG_FUNCTION_NOARGS ( );
}

TypeId
PositionAware::GetTypeId ( void)
{
  static TypeId tid = TypeId ( "ns3::PositionAware")
      .SetParent<Object> ( )
      .AddConstructor<PositionAware> ( )
      .AddAttribute ( "LastPosition","The Reference position.",
                     TypeId::ATTR_SET | TypeId::ATTR_GET,
                     VectorValue ( Vector ( 0.0, 0.0, 0.0)),
                     MakeVectorAccessor ( &PositionAware::SetPosition,
                                         &PositionAware::GetPosition),
                     MakeVectorChecker ( ))
      .AddAttribute ( "PositionDelta","The minimum m_distance from reference to trigger a position change",
                     TypeId::ATTR_SET | TypeId::ATTR_GET,
                     DoubleValue (100),
                     MakeDoubleAccessor ( &PositionAware::SetDistance,
                                         &PositionAware::GetDistance),
                     MakeDoubleChecker<double> ( ))
      .AddAttribute ( "Timeout","The time it takes before we give up on a position change",
                     TypeId::ATTR_SET | TypeId::ATTR_GET,
                     TimeValue ( Seconds ( 10)),
                     MakeTimeAccessor ( &PositionAware::SetTimeout,
                                       &PositionAware::GetTimeout),
                     MakeTimeChecker ( ))
      .AddTraceSource ( "Timeout",
                       "The m_timeout was reached",
                       MakeTraceSourceAccessor ( &PositionAware::m_timeout_trace),
                       "ns3::PositionAware::TimeoutCallback")
      .AddTraceSource ( "PositionChange",
                       "The Position has changed by 'm_distance'",
                       MakeTraceSourceAccessor ( &PositionAware::m_position_change_trace),
                       "ns3::PositionAware::PositionChangeCallback")
      ;
  return tid;
}


double
PositionAware::GetDistance ( ) const
{
  NS_LOG_DEBUG ( __FUNCTION__);
  return m_distance;
}

void
PositionAware::SetDistance ( const double& _m_distance)
{
  NS_LOG_DEBUG ( __FUNCTION__);
  m_distance = _m_distance;
}

Time
PositionAware::GetTimeout ( ) const
{
  NS_LOG_DEBUG ( __FUNCTION__);
  return m_timeout;
}

void
PositionAware::SetTimeout ( const Time& _m_timeout)
{
  NS_LOG_DEBUG ( __FUNCTION__);
  m_timeout_timer.SetDelay ( _m_timeout);
  m_timeout = _m_timeout;
}

Vector
PositionAware::GetPosition ( ) const
{
  NS_LOG_DEBUG ( __FUNCTION__);
  return m_last_position;
}

void
PositionAware::SetPosition ( const Vector& _position)
{
  NS_LOG_DEBUG ( __FUNCTION__);
  m_last_position = _position;
}

void
PositionAware::CourseChange ( Ptr<const MobilityModel> _mobility_model)
{
  NS_LOG_FUNCTION ( this);
  NS_LOG_DEBUG(Simulator::Now().GetSeconds() <<": Course Change");
  double deltaP_mag = CalculateDistance ( _mobility_model->GetPosition ( ),
                                       m_last_position);
  Time threshold = m_timeout - (Simulator::Now()-m_last_event);
  if ( m_timeout_scheduled){//Currently going to timeout first
    threshold = m_timeout_timer.GetDelayLeft ( );
    NS_LOG_DEBUG("Timout already scheduled, delay left="<<threshold);
  }
  else{
    NS_LOG_DEBUG("Timout not scheduled, delay left="<<threshold);
  }
  if ( deltaP_mag >= m_distance) {//shouldn't happen
    HandlePositionChange ( );
    return;
  }
  if ( m_distance_scheduled){
    CancelPositionChange ( );//cancel scheduled event;
  }
  m_speed = CalculateDistance ( _mobility_model->GetVelocity ( ),Vector ( ));
  NS_LOG_DEBUG("New Speed is: "<<m_speed);
  // FIXME: we may not need to worry about small m_speed and just simply
  //        schedule the "escape time" regardless.  However, there is a
  //        concern that if we implement a timeout-based mechanism, there
  //        may be an issue ... so please re-evaluate before removing
  if ( m_speed < .001){    //Not Moving
    NS_LOG_DEBUG("Not moving");
    if ( !m_timeout_scheduled)
      ScheduleTimeout ( );//Schedule m_timeout
  }
  else if ( deltaP_mag < .001) { //Wasn't Moving, but is now,
    //which means a m_timeout should be scheduled.
    double t = m_distance/m_speed;
    NS_LOG_DEBUG("Wasn't moving, but is now: t = "<<t<<" threshold= "<<threshold.GetSeconds());
    if ( t < threshold.GetSeconds()){ //if position change occurs before m_timeout
      SchedulePositionChange (Seconds ( t));//schedule positionchange
      if ( m_timeout_scheduled){
        CancelTimeout ( );//cancel timer
      }
    }
    else{ //if position change is too slow
      if ( !m_timeout_scheduled){ //if there is no m_timeout scheduled
        ScheduleTimeout ( );//schedule m_timeout
      }
    }
  }
  else{
    NS_LOG_DEBUG("Was moving and is moving");
    //Was moving and is moving a different direction now.
    Vector deltaP;
    deltaP.x = _mobility_model->GetPosition ( ).x - m_last_position.x;
    deltaP.y = _mobility_model->GetPosition ( ).y - m_last_position.y;
    deltaP.z = _mobility_model->GetPosition ( ).z - m_last_position.z;
    Vector V = _mobility_model->GetVelocity ( );
    //common values for both test and calculation
    double dot_product = deltaP.x*V.x + deltaP.y*V.y + deltaP.z*V.z;
    double PV = deltaP_mag*m_speed;
    double DD = m_distance*m_distance;
    double PP = deltaP_mag*deltaP_mag;
    double a = m_speed*m_speed;
    //No divisions, no sqrt for test;
    double c = PP - DD;                           //1-
    double b = 2.0*dot_product;                   //1*
    double t = threshold.GetSeconds ( );
    double TT = t*t;                              //1*
    double BB = b*b;                              //1*
    double A4 = 4.0*a;                            //1*
    double LHS = BB - A4*c;                       //1*1-
    double RHS = A4*a*TT + BB + A4*b*t;           //4*2+
    if ( LHS < RHS){//We cross m_distance before we cross m_timeout
      double DP = dot_product*dot_product;
      t = (sqrt ( DD*a + DP - PV*PV) + dot_product)/a;//2*1/3 + 1sqrt
      SchedulePositionChange (Seconds ( t));
      if ( m_timeout_scheduled){
        CancelTimeout ( );
      }
    }
    else { //We cross m_timeout before we cross m_distance
      if ( !m_timeout_scheduled){
        ScheduleTimeout ( );
      }
    }
  }
}

void
PositionAware::HandleTimeout ( )
{
  NS_LOG_FUNCTION ( this);
  m_timeout_trace ( this);
  ScheduleNext ( );
}

void
PositionAware::HandlePositionChange ( )
{
  NS_LOG_FUNCTION ( this);
  m_position_change_trace ( this);
  ScheduleNext ( );
}

void
PositionAware::ScheduleNext ( )
{
  NS_LOG_FUNCTION ( this);
  //update position, m_speed should be same as set by changecourse
  m_last_position = m_mobility_ptr->GetPosition ( );
  m_last_event = Simulator::Now();
  //Assume that this is called by HandleTimeout or HandlePosition
  //Don't assume timer or position change is repeated
  //assume we are starting fresh
  if ( m_timeout_scheduled)
    CancelTimeout ( );
  if ( m_timeout.GetSeconds()*m_speed >= m_distance){ //if before next timeout could cross threshold   //m_timeout m_distance exceeds threshold
    double t = m_distance/m_speed;
    SchedulePositionChange (Seconds ( t));
  }
  else{ //m_timeout m_distance is shorter than threshold.
    ScheduleTimeout ( );
    if ( m_distance_scheduled)
      CancelPositionChange ( );
  }
}

void
PositionAware::CancelPositionChange ( )
{
  NS_LOG_DEBUG ( __FUNCTION__);
  Simulator::Cancel ( m_scheduled_event);
  m_distance_scheduled = false;
}

void
PositionAware::CancelTimeout ( )
{
  NS_LOG_DEBUG ( __FUNCTION__);
  if ( m_timeout_scheduled){
    NS_LOG_DEBUG("There is a timer to cancel");
    m_timeout_timer.Cancel ( );
    m_timeout_scheduled = false;
  }
}

void
PositionAware::ScheduleTimeout ( )
{
  NS_LOG_DEBUG (Simulator::Now().GetSeconds() << ": timeout scheduled for " << Simulator::Now() + (m_timeout-(Simulator::Now()-m_last_event)));
  NS_LOG_DEBUG ( __FUNCTION__);
  if ( m_timeout > ns3::Time(0)){
    if ( m_timeout_scheduled)
      CancelTimeout ( );
    m_timeout_timer.Schedule (m_timeout-(Simulator::Now()-m_last_event) ); //remaining time in timeout threshold since last event
    m_timeout_scheduled = true;
  }
}

void
PositionAware::SchedulePositionChange ( const Time& _t)
{
  NS_LOG_DEBUG (Simulator::Now().GetSeconds() << ": Position change scheduled for " << Simulator::Now() + _t);
  NS_LOG_DEBUG ( __FUNCTION__ << " t = " << _t);
  m_scheduled_event =  Simulator::Schedule ( _t,&PositionAware::HandlePositionChange, this);
  m_distance_scheduled = true;
}

void
PositionAware::NotifyNewAggregate ( )
{
  NS_LOG_FUNCTION ( this);
  if ( false == m_aggregated){
    Ptr<Node> node = GetObject<Node> ( );
    NS_LOG_DEBUG ( "Node: " << node->GetId ( ));
    m_mobility_ptr = GetObject<MobilityModel> ( );
    NS_ASSERT_MSG ( 0 != m_mobility_ptr,
                   "Must install Mobility before PostionAware");
    m_mobility_ptr->TraceConnectWithoutContext ( "CourseChange",
                                              MakeCallback ( &PositionAware::CourseChange,
                                                            this)
                                             );
    m_speed = CalculateDistance ( Vector ( ),m_mobility_ptr->GetVelocity ( ));
    ScheduleNext ( );
    m_aggregated = true;
  }
  else{
    NS_LOG_DEBUG("Already Aggregated");
  }
  BaseType::NotifyNewAggregate ( );
}

}//namespace ns3
