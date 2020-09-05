/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Institute for the Wireless Internet of Things, Northeastern University, Boston, MA
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
 * Author: Michele Polese <michele.polese@gmail.com>
 */

#include "ns3/core-module.h"
#include <ns3/buildings-module.h>
#include <ns3/mobility-module.h>
#include "ns3/network-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ScratchSimulator");

void
PrintPosition(Ptr<Node> node)
{
  Ptr<MobilityModel> model = node->GetObject<MobilityModel> ();
  NS_LOG_UNCOND(node->GetId() << " Position +****************************** " << model->GetPosition() << " at time " << Simulator::Now().GetSeconds());
}


int 
main (int argc, char *argv[])
{
  // LogComponentEnable("GroupSecondaryMobilityModel", LOG_LEVEL_LOGIC);
  LogComponentEnable("GroupMobilityHelper", LOG_LEVEL_LOGIC);
  LogComponentEnable("MobilityHelper", LOG_LEVEL_LOGIC);

  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (0, 100, -75, 75)));
  Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator> ();
  position->Add (Vector (50, 73, 1));
  mobility.SetPositionAllocator(position);

  MobilityHelper mobility2;
  mobility2.SetMobilityModel ("ns3::GaussMarkovMobilityModel",
                             "Bounds", BoxValue (Box (0, 100, -75, 75, 0.4, 1.7)));
  Ptr<ListPositionAllocator> position2 = CreateObject<ListPositionAllocator> ();
  position2->Add (Vector (0, 10, 1.5));
  mobility2.SetPositionAllocator(position2);

  NodeContainer group1;
  group1.Create(10);

  NodeContainer group2;
  group2.Create(4);

  Ptr<GroupMobilityHelper> groupMobility = CreateObject<GroupMobilityHelper>();
  groupMobility->SetAttribute("PathDeviationRandomVariable", 
    StringValue("ns3::NormalRandomVariable[Mean=0.0|Variance=1|Bound=20]"));

  groupMobility->SetMobilityHelper(&mobility);
  NodeContainer allNodes1 = groupMobility->InstallGroupMobility(group1);

  groupMobility->SetMobilityHelper(&mobility2);
  NodeContainer allNodes2 = groupMobility->InstallGroupMobility(group2);

  AsciiTraceHelper ascii;
  MobilityHelper::EnableAsciiAll (ascii.CreateFileStream ("mobility-trace-example.mob"));

  double simTimeSeconds = 2000;

  double numPrints = 1000;
  for(int i = 0; i < numPrints; i++)
  {
    for(auto nodeIt = group1.Begin(); nodeIt != group1.End(); ++nodeIt)
    {
      Simulator::Schedule(Seconds(i*simTimeSeconds/numPrints), &PrintPosition, (*nodeIt));
    }
   Simulator::Schedule(Seconds(i*simTimeSeconds/numPrints), &PrintPosition, allNodes1.Get(0));
    for(auto nodeIt = group2.Begin(); nodeIt != group2.End(); ++nodeIt)
    {
      Simulator::Schedule(Seconds(i*simTimeSeconds/numPrints), &PrintPosition, (*nodeIt));
    }
    Simulator::Schedule(Seconds(i*simTimeSeconds/numPrints), &PrintPosition, allNodes2.Get(0));
  }

  Simulator::Stop(Seconds(simTimeSeconds));
  Simulator::Run ();
  Simulator::Destroy ();
}
