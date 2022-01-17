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
*/

/**
 * \ingroup mobility
 * \brief 3D Circle mobility model example.
 * 
 * This Example "Simple3DCircleMobilityExample1.cc" will generate a 5 UAV node topology and simulate CircleMobilityModel in them.
 * This simulation will create a NetAnim Trace file as an output.
 * 
 * The movement of the object will be controlled by parameters 
 * Origin, Radius, StartAngle, Speed and Direction
 * This mobility model enforces no bounding box by itself. 
 * 
 * The the mobility model parameters/attributes can be set during initialization of the mobility model
 * 
 *  During initialization itself, we can configure different  Mobility Parameter.
 * 
 * Even after initialization, if the user want to change the mobility parameter of one particular node,
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
                               "UseConfiguredOrigin",BooleanValue(true));
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
                               "UseConfiguredOrigin",BooleanValue(true),
                               "MinOrigin",Vector3DValue(Vector3D(0,0,0)),"MaxOrigin",Vector3DValue(Vector3D(500,500,500)),
                               "MinMaxRadius",Vector2DValue(Vector2D(500,500)),
                               "MinMaxStartAngle",Vector2DValue(Vector2D(0,0)),
                               "MinMaxSpeed",Vector2DValue(Vector2D(30,60)),
                               "RandomizeDirection",BooleanValue(false),
                               "Clockwise",BooleanValue(true)
    mobility.Install (UAVs);
 * \endcode
 * What ever may be the way in which we initialize the mobility model, 
 * we can customize the path of any single node by 
 * class CircleMobilityModel : public MobilityModel
 * using the CircleMobilityModel::SetParameters function at any time.
 * 
 * Example 5:
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
"" * 
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

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include <sstream>

using namespace ns3;

int
main (int argc, char *argv[])
{
  int NumOfUAVs = 5;
  int example = 1;
  bool useHelper=false;
  CommandLine cmd;
  cmd.AddValue("NumOfUAVs", "Number of UAVs to Simulate", NumOfUAVs);
  cmd.AddValue("example", "Number of example scenario to Simulate", example);
  cmd.AddValue("useHelper", "helper", useHelper);
  cmd.Parse (argc, argv);

  // if (useHelper){
  //   std::cout<<"All the examples in this script are using helpers\n";
  //   std::cout<<"You can run the code with different parameters as follows:\n\n";
  //   std::cout<<"    ./waf --run \"simple-3d-circle-mobility-example1\"\n";
  //   std::cout<<"    ./waf --run \"simple-3d-circle-mobility-example1 --NumOfUAVs=5\"\n";
  //   std::cout<<"    ./waf --run \"simple-3d-circle-mobility-example1 --example=2\"\n";
  //   std::cout<<"\nRunning the example with helpers\n";
  // } else {
  //   std::cout<<"Sorry. No examples provided without helpers\n";
  //   std::cout<<"Running the example with helpers\n";
  // }

  NodeContainer UAVs;
  UAVs.Create (NumOfUAVs);

  MobilityHelper mobility;

  //run the example as per selection
  switch (example)
    {
    case 1:
      mobility.SetMobilityModel ("ns3::CircleMobilityModel");
      mobility.Install (UAVs);
      break;
    case 2:
      mobility.SetMobilityModel ("ns3::CircleMobilityModel");
      mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator", 
          "X", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"), 
          "Y", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"), 
          "Z", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"));
      mobility.Install (UAVs);
      break;

    case 3:
      mobility.SetMobilityModel ("ns3::CircleMobilityModel", "UseConfiguredOrigin",
                                 BooleanValue (true));
      mobility.Install (UAVs);
      break;
    case 4:
      mobility.SetMobilityModel ("ns3::CircleMobilityModel", 
          "UseConfiguredOrigin", BooleanValue (true), 
          "MinOrigin", Vector3DValue (Vector3D (0, 0, 0)), "MaxOrigin", Vector3DValue (Vector3D (500, 500, 500)),
          "MinMaxRadius", Vector2DValue (Vector2D (500, 500)), 
          "MinMaxStartAngle", Vector2DValue (Vector2D (0, 0)), 
          "MinMaxSpeed", Vector2DValue (Vector2D (30, 60)),
          "RandomizeDirection", BooleanValue (false), 
          "Clockwise", BooleanValue (true));
      mobility.Install (UAVs);
      break;
    case 5:
      mobility.SetMobilityModel ("ns3::CircleMobilityModel");
      mobility.Install (UAVs);
      UAVs.Get (0)->GetObject<CircleMobilityModel> ()->SetParameters (Vector (1000, 1000, 1000),
                                                                      200, 0, true, 20);     
      break;
    case 6:
      mobility.SetMobilityModel ("ns3::CircleMobilityModel", 
          "UseInitialPositionAsOrigin", BooleanValue (true),
          "MinMaxSpeed", Vector2DValue (Vector2D (10, 10)), 
          "RandomizeDirection", BooleanValue (false), 
          "MinMaxRadius", Vector2DValue (Vector2D (300, 300)));
      mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator", 
          "X", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"), 
          "Y", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"), 
          "Z", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"));
      mobility.Install (UAVs);
      break;
    case 7:
        break;
    default:
      std::cout << "Sorry wrong example number\n";
      return 1;
    }

  //The special group mobility scenario example
    if (example==7){
        Ptr<WaypointMobilityModel> waypointMm = CreateObject<WaypointMobilityModel> ();
        waypointMm->AddWaypoint (Waypoint (Seconds (0), Vector (0, 0, 0)));
        waypointMm->AddWaypoint (Waypoint (Seconds (1000), Vector (5000, 0, 0)));
        waypointMm->AddWaypoint (Waypoint (Seconds (2000), Vector (0, 5000, 0)));
        GroupMobilityHelper group;
        group.SetReferenceMobilityModel (waypointMm);
        group.SetMemberMobilityModel ("ns3::CircleMobilityModel","UseConfiguredOrigin",BooleanValue(true));
        group.Install (UAVs);
    }

  //Setting file name suffix
  std::stringstream sstm;
  sstm << "Example-" << example << ".xml";

  //Configure NetAnim
  AnimationInterface anim ("Simple3DCircleMobility" + sstm.str ());
  //Set node size as 5m so that make it visible in NetAnim
  for (int i = 0; i < NumOfUAVs; i++)
    anim.UpdateNodeSize (i, 20, 20);

  //Stop the simulation and run it
  Simulator::Stop (Seconds (5000.0));
  Simulator::Run ();
  Simulator::Destroy ();
  
  return 0;
}
