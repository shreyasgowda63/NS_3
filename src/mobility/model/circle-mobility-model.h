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
#include "ns3/vector.h"
#include "ns3/enum.h"
#include "ns3/pointer.h"
#include "ns3/string.h"
#include "ns3/double.h"
#include <ns3/boolean.h>
#include <ns3/random-variable-stream.h>
#include "constant-velocity-helper.h"

namespace ns3 {

/**
 * \ingroup mobility
 * \brief 3D Circle mobility model.
 *
 * The movement of the object will be controlled by parameters Origin, Radius, StartAngle, Speed and Direction
 * This mobility model enforces no bounding box by itself. 
 * 
 * The mobility model parameters/attributes can be set during initialization of the mobility Model
 * 
 * Even after initialization, if the user want to change the Mobility Parameter of one particular node,
 * or group of nodes, that can be only done through a custom SetAttributes method of the model.
 * 
 * The implementation of this model is not 2d-specific. i.e. if you provide
 * z-value greater than 0, then you may use it in 3d scenarios
 * It is possible to use this model as  child in a hierarchical/group mobility 
 * and create more practical 3d mobility scenarios
 * 
 * The following are different ways in which we can initialize and use the model:
 * All the example codes will set the CircleMobilityModel in all the nodes in the 
 * NodeContainer but move them differently according to settings
 * 
 * Caveat of this model:
 * 
 * If no further changes are made to the model's parameters after initialization, 
 * the model will not call NotifyCourseChange again.  If, however, the user changes 
 * any parameters after initialization, a course change will be notified.  
 * Changing of parameters may, in some cases, cause the node to change position 
 * instantaneously in a discrete jump (such as a change in altitude or radius) 
 * 
 * while running group mobility simulation, the nodes gets placed at 0,0,0 and 
 * starts to move from the configured locations while starts playing the simulation in NetAnim.
 * This problem will be resolved in future version
 * 
 * Example 1:
 * In this all the nodes start the movement at (0,0,0) but will have different 
 * origins derived from the default random value of radius, start angle 
 * and will have random speed and direction. So, all the nodes will circulate 
 * in different circular paths but the nodes will pass the point (0,0,0)
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
 * of radius, start angle and will have random speed and direction.
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
 * So, all the nodes will circulate in different circular planes perpendicular to the z-axis
 * \code
    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::CircleMobilityModel", 
                          "Mode", EnumValue (CircleMobilityModel::INITIALIZE_NONRANDOM),
                          "OriginConfigMode", EnumValue (CircleMobilityModel::ORIGIN_FROM_ATTRIBUTE));
    mobility.Install (UAVs);
 * \endcode
 *
 * Example 4:
 * In this, all the nodes will start the movement with respect to different 
 * origins derived from the user provided range of random value of radius, start angle 
 * and will have random speed and direction.
 * So, all the nodes will circulate in different circular x-y planes perpendicular to the z-axis
 * \code
    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::CircleMobilityModel", 
                          "Mode", EnumValue (CircleMobilityModel::INITIALIZE_RANDOM),
                          "OriginConfigMode",EnumValue (CircleMobilityModel::ORIGIN_FROM_ATTRIBUTE),
                          "RandomOriginX", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=2000.0]"),
                          "RandomOriginY", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=2000.0]"),
                          "RandomOriginZ", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"),
                          "Radius", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=500.0]"));
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
    mobility.SetMobilityModel ("ns3::CircleMobilityModel");
    mobility.Install (UAVs);
    UAVs.Get (0)->GetObject<CircleMobilityModel> ()->SetAttribute (
                        "Origin", VectorValue (Vector (1800, 1800, 1800))); 
    UAVs.Get (0)->GetObject<CircleMobilityModel> ()->SetAttribute (
                        "Radius", DoubleValue (200));               
    break; * \endcode
 * 
 * Example 6:
 * If the user choose to use the initial position of the node (provided by PositionAllocator) as origin,
 * they can do it as follows:
 * \code 
      mobility.SetMobilityModel ("ns3::CircleMobilityModel", 
                    "OriginConfigMode", EnumValue (CircleMobilityModel::POSITION_AS_ORIGIN));
      mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator", 
          "X", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"), 
          "Y", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"), 
          "Z", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"));
      mobility.Install (UAVs);
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
        group.SetMemberMobilityModel ("ns3::CircleMobilityModel", 
                                 "OriginConfigMode", EnumValue (CircleMobilityModel::ORIGIN_FROM_ATTRIBUTE));
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
   * @brief A function to set the radius
   * @param radius is a radius of the circle in meters
   */
   void SetRadius(const double radius);
   
   /**
   * @brief A function to get the radius
   * \return a double value
   */                  
   double GetRadius () const;

   /**
   * @brief A function to set the origin of the circle
   * @param origin is a Vector(x,y,z) - in meters
   */
   void SetOrigin(const Vector3D &origin);

   /**
   * @brief A function to get the origin
   * \return pointer to Vector(x,y,z)
   */    
   const Vector3D & GetOrigin () const;


   /**
   * @brief A function to set the starting angle
   * @param startAngle is a starting angle of the circle - in meters
   */  
   void SetStartAngle(const double startAngle);

   /**
   * @brief A function to get the starting angle
   * \return a double value
   */  
   double GetStartAngle() const;

   /**
   * @brief A function to set the speed of the node
   * @param speed is a moving speed of the   node - in meters/second
   */  
   void SetSpeed(const double speed);

   /**
   * @brief A function to get the speed of the node
   * \return a double value 
   */  
   double GetSpeed() const;

   /**
   * @brief A function to set the direction of rotation of the node
   * @param clockwise true sets it in clockwise; false sets it in anticlockwise
   */  
   void SetClockwise(const bool clockwise);

   /**
   * @brief A function to get the direction of rotation of the node
   * \return a boolean value 
   */     
   bool GetClockwise() const;

   enum model_mode {
   // The mode affects how the model is initialized
      INITIALIZE_RANDOM,
      INITIALIZE_NONRANDOM
   };

//OriginConfigMode is a important parametre that will 
//decided the way in which origin of the circle will be derived
   enum origin_mode {
   // An enum representing the different origin configuration modes
      ORIGIN_FROM_ATTRIBUTE,
      RADIUS_AWAY_FROM_POSITION,
      POSITION_AS_ORIGIN
   };

private:
  virtual Vector DoGetPosition (void) const;
  virtual void DoSetPosition (const Vector &nitOrigin);
  virtual Vector DoGetVelocity (void) const;
  virtual void DoInitialize (void);
  virtual int64_t DoAssignStreams (int64_t stream);
  /**
   * @brief Initializes the parameters of the circle mobility model
   * 
   */
  void InitializePrivate(void);
   
  enum model_mode m_mode; //!< this decids the way in which origin of the circle will be derived
  enum origin_mode m_OriginConfigMode; //!< this decids the way in which origin of the circle will be derived
  mutable Time m_lastUpdate; //!< the  last upsate time
  Vector3D m_position; //!< the position of the node/object
  bool m_parametersInitialized=false; //!< to check whether the parameters are initialized or not

//the five main parameters of the model
  Vector m_origin; //!< the  origin of the circle
  double m_radius; //!< the  radius of the circle
  double m_startAngle; //!< the  start angle of the circle
  double m_speed; //!< the  speed of the object
  bool m_clockwise;       //!< The  direction of circular movement.


//the parameters that will control the randomness in circular orbit creation
  Ptr<RandomVariableStream> m_randomOriginX; //!< A random variable used to pick the origin y-axis coordinate (m).
  Ptr<RandomVariableStream> m_randomOriginY; //!< A random variable used to pick the origin y-axis coordinate (m).
  Ptr<RandomVariableStream> m_randomOriginZ; //!< A random variable used to pick the origin z-axis coordinate (m).
  Ptr<RandomVariableStream> m_randomRadius;  //!< A random variable used to pick the radius (m).
  Ptr<RandomVariableStream> m_randomStartAngle;//!< A random variable used to pick the start angle (degrees).
  Ptr<RandomVariableStream> m_randomSpeed    ;//!< A random variable used to pick the speed (m/s).
  Ptr<RandomVariableStream> m_randomClockwise;//!< A random variable used to select clockwise (true) or counter-clockwise (false) direction. 

 };

} // namespace ns3

#endif /* CIRCLE_MOBILITY_MODEL_H */
