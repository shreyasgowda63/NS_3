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
#ifndef CIRCLE_MOBILITY_MODEL_H
#define CIRCLE_MOBILITY_MODEL_H

#include "ns3/simulator.h"
#include "mobility-model.h"
#include <ns3/vector.h>
#include <ns3/random-variable-stream.h>
#include "constant-velocity-helper.h"

namespace ns3 {

/**
 * \ingroup mobility
 * \brief 3D Circle mobility model.
 *
 * The movement of the object will be controlled by parameters 
 * Origin, Radius, StartAngle, Speed and Direction
 * This mobility model enforces no bounding box by itself. 
 * 
 * The the mobility model Parameters/Attributes can be set during initialization of the Mobility Model
 * 
 * Even after initialization, if the user want to change the Mobility Parameter of one Particular Node,
 * or group of nodes, that can be only done through a custom SetAttributes method of the model.
 * 
 * The implementation of this model is not 2d-specific. i.e. if you provide
 * z value greater than 0, then you may use it in 3d scenarios
 * It is possible to use this model as  child in a hierarchical/group mobility 
 * and create more practical 3d mobility scenarios
 * 
 * The following are different ways in which we can initialize and use the model:
 * All the example codes will set the CircleMobilityModel in all the nodes in the 
 * NodeContainer but move them differently according to settings
 * 
 * Example 1:
 * In this all the nodes start the movement at (0,0,0) but will have different 
 * origins derived from the default random value of radius, start angle 
 * and will have random speed and direction. So, all the nodes will circulate 
 * in different circlular paths but the nodes will pass the point (0,0,0)
 * 
 * \code
    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::CircleMobilityModel");
    mobility.Install (UAVs);
 * \endcode
 *
 * Example 2:
 * In this, all the nodes will start the movement at initial position provided by the PositionAllocator
 * and calculate origins with respect to the positions and with respect to the default random value
 * of radius, strat angle and will have random speed and direction.
 * So, all the nodes will circulate in different circles but will pass the initial point provided by PositionAllocator
 * 
 * \code
    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::CircleMobilityModel");
    mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
                               "X", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"),
                               "Y", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"),
                               "Z", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"));
    mobility.Install (UAVs);
 * \endcode
 *
 * Example 3:
 * In this, all the nodes will start the movement at position with respect to different 
 * origins derived from the default random value of radius, start angle 
 * and will have random speed and direction.
 * So, all the nodes will circulate in different circlular planes perpendicular to the z axis
 * \code
    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::CircleMobilityModel", 
                               "UseConfiguredOrigin",BooleanValue(true));
    mobility.Install (UAVs);
 * \endcode
 *
 * Example 4:
 * In this, all the nodes will start the movement with respect to different 
 * origins derived from the user provided range of random value of radius, start angle 
 * and will have random speed and direction.
 * So, all the nodes will circulate in different circlular planes perpendicular to the z axis
 * \code
    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::CircleMobilityModel", 
                               "UseConfiguredOrigin",BooleanValue(true),
                               "MinOrigin",Vector3DValue(Vector3D(0,0,0)),"MaxOrigin",Vector3DValue(Vector3D(500,500,500)),
                               "MinMaxRadius",Vector2DValue(Vector2D(500,500)),
                               "MinMaxStartAngle",Vector2DValue(Vector2D(0,0)),
                               "MinMaxSpeed",Vector2DValue(Vector2D(30,60)),
                               "RandomizeDirection",BooleanValue(false),
                               "Clockwise",BooleanValue(true)
    mobility.Install (UAVs);
 * \endcode
 * 
 * 
 * Example 5:
 * What ever may be the way in which we initialize the mobility model, 
 * we can customize the path of any single node by 
 * class CircleMobilityModel : public MobilityModel
 * using the CircleMobilityModel::SetParameters function at any time.
 * 
 * \code 
   //void SetParameters(const Vector &Origin, const double Radius, const double StartAngle, const bool Clockwise, const double Speed);
  
   mobility.Get (0)->GetObject<CircleMobilityModel> ()->SetParameters(Vector (150, 150, 250), 150, 0, true, 20); 
 * \endcode
 * 
 * Example 6:
 * If the user choose to use the initial position of the node (provided by PositionAllocator) as origin,
 * they can do it as follows:
 * \code 
           mobility.SetMobilityModel ("ns3::CircleMobilityModel",
                                    "UseInitialPositionAsOrigin", BooleanValue(true),
                                    "MinMaxSpeed",Vector2DValue(Vector2D(10,10)),
                                    "RandomizeDirection",BooleanValue(false),
                                    "MinMaxRadius",Vector2DValue(Vector2D(300,300)));
          mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
                                    "X", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"),
                                    "Y", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"),
                                    "Z", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"));
 * \endcode
 * 
 * Example 7:
 * The CircleMobilityModel can be used in group mobility as shown below:
 *         
 * \code
        Ptr<WaypointMobilityModel> waypointMm = CreateObject<WaypointMobilityModel> ();
        waypointMm->AddWaypoint (Waypoint (Seconds (0), Vector (0, 0, 0)));
        waypointMm->AddWaypoint (Waypoint (Seconds (1000), Vector (5000, 0, 0)));
        waypointMm->AddWaypoint (Waypoint (Seconds (2000), Vector (0, 5000, 0)));
        GroupMobilityHelper group;
        group.SetReferenceMobilityModel (waypointMm);
        group.SetMemberMobilityModel ("ns3::CircleMobilityModel","UseConfiguredOrigin",BooleanValue(true));
        group.Install (UAVs);
 * \endcode
 */
class CircleMobilityModel : public MobilityModel
{
public:
  /**
   * Register this type with the TypeId system.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
  * SetParameters function may be called by user after the initialization of the MobilityModel
  * This will override the values set during the initialization of the MobilityModel
  * This function can be called during the simulation whenever needed to 
  * set the circle mobility models attributes of a particular node's MobilityModel  
  * This will set the mobility model patameters/attributes 
  *              Origin, Radius, StartAngle, Speed and Direction
  *
  * @param Origin is a Vector(x,y,z) - in meters
  * @param Radius is a radius of the circle in meters
  * @param StartAngle is startng angle from which the object starts to move - measured with reference to x axis in degrees
  * @param Clockwise is a boolean that will decide the direction of the object on the circular path
  * @param Speed is the moving speed of the object in meters
  */
  void SetParameters (const Vector &Origin, const double Radius, const double StartAngle,
                      const bool Clockwise, const double Speed);

private:
  virtual Vector DoGetPosition (void) const;
  virtual void DoSetPosition (const Vector &nitOrigin);
  virtual Vector DoGetVelocity (void) const;
  virtual void DoInitialize (void);

  mutable Time m_lastUpdate; //!< the  last upsate time

  double m_radius; //!< the  radius of the circle
  double m_startAngle; //!< the  start angle of the circle
  double m_speed; //!< the  speed of the object
  bool m_clockwise; //!< the  direction of circular movement
  bool m_randomizeDirection; //!< randomize the direction of circular movement
  bool m_useConfiguredOrigin; //!< use the origins configured through attribute
  bool
      m_useInitialPositionAsOrigin; //!< Use the initial position of the node (provided by PositionAllocator) as origin
  Vector m_origin; //!< the  origin of the circle

  Vector2D m_radiusMinMax; //!< minimum and maximum range of radius
  Vector2D m_startAngleMinMax; //!< minimum and maximum range of start angle
  Vector2D m_speedMinMax; //!< minimum and maximum range of start speed
  Vector3D m_position; //!< the positon of the object
  Vector3D m_originMin; //!< minimum range of origin
  Vector3D m_originMax; //!< maximum range of origin
  ConstantVelocityHelper m_helper;   //!< helper for velocity computations
  bool m_parametersInitialized=false; //!< to check whether the parameters are initialized or not
};

} // namespace ns3

#endif /* CIRCLE_MOBILITY_MODEL_H */
