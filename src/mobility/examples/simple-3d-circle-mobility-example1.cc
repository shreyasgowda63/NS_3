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
This Example "Simple3DCircleMobilityExample1.cc" will generate a 5 UAV node topology and simulate CircleMobilityModel in them.
This simulation will create a NetAnim Trace file called "Simple3DCircleMobilityExample1.xml" as an output.
*/

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h" 

using namespace ns3;

int main (int argc, char *argv[])
{

  //Create 5 UAV nodes with default parameters [ origin=(x=100 m, y=100 m, z=100 m), radius=100 m, startAngle=0 Degree, speed=30 m/s	]
  // NumOfUAVs can be 1 to a high value
  int NumOfUAVs=5;

  NodeContainer CirclesUAV;
  CirclesUAV.Create (NumOfUAVs);
	
  MobilityHelper CircleMobility;
	
  CircleMobility.SetMobilityModel ("ns3::CircleMobilityModel");
  CircleMobility.Install (CirclesUAV);
  
  //Configure UAVs with new Circle Mobility Parameters in a random manner
  Ptr<UniformRandomVariable> rn = CreateObject<UniformRandomVariable> ();
  for(int i=0;i<NumOfUAVs;i++) {
     //set UAVs at same initial x,y but different initial altitudes (z) between 100m to 300m  [can be set according to the topographical area of interest]
     CirclesUAV.Get (i)->GetObject<CircleMobilityModel> ()->SetAttribute ("origin" , Vector3DValue(Vector (100, 100,rn->GetValue(100,300))));	
	  
     //set different random initial radius for circulating UAVs between 10m to 100m [Allowed min/max limit value: 1 m to 10000 m]
     CirclesUAV.Get (i)->GetObject<CircleMobilityModel> ()->SetAttribute ("radius" , DoubleValue(rn->GetValue(10,100)));

     //set different random initial startAngle for circulating UAVs between 0 degree to 360 degree [Allowed min/max limit value: 0 deg to 360deg]
     CirclesUAV.Get (i)->GetObject<CircleMobilityModel> ()->SetAttribute ("startAngle" ,  DoubleValue(rn->GetValue(0,360)));
	  
     //set different random initial speed for circulating UAVs between 10m/s to 30m/s [Allowed min/max limit value: 1 m/s to 100 m/s]
     CirclesUAV.Get (i)->GetObject<CircleMobilityModel> ()->SetAttribute ("speed" ,   DoubleValue(rn->GetValue(10,30)));
  }
	
  //Configure NetAnim
  AnimationInterface anim ("Simple3DCircleMobilityExample1.xml"); 
  //Set node size as 5m so that make it visible in NetAnim
  for(int i=0;i<NumOfUAVs;i++)	
      anim.UpdateNodeSize (i, 5, 5);
	
  //Stop the Simulation and Run it
  Simulator::Stop (Seconds (200.0));
  Simulator::Run ();
  Simulator::Destroy ();
  
  return 0;
}
