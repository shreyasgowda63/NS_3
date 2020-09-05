/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 * Copyright (c) 2019, University of Padova, Dep. of Information Engineering, SIGNET lab
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
 * Author: Nicola Baldo <nbaldo@cttc.es> for the code adapted from the lena-dual-stripe.cc example
 * Author: Michele Polese <michele.polese@gmail.com> for this version
 */

#include "ns3/buildings-module.h"
#include "ns3/mobility-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("GroupMobilityOutdoorExample");

void
PrintGnuplottableBuildingListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  uint32_t index = 0;
  for (BuildingList::Iterator it = BuildingList::Begin (); it != BuildingList::End (); ++it)
    {
      ++index;
      Box box = (*it)->GetBoundaries ();
      outFile << "set object " << index
              << " rect from " << box.xMin  << "," << box.yMin
              << " to "   << box.xMax  << "," << box.yMax
              << std::endl;
    }
}


/**
 * This is an example on how to use the GroupSecondaryOutdoorMobilityModel class.
 */
int
main (int argc, char *argv[])
{
  LogComponentEnable ("GroupMobilityOutdoorExample", LOG_LEVEL_LOGIC);
  CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv);

  // create a grid of buildings
  double buildingSizeX = 100;       // m
  double buildingSizeY = 50; // m
  double streetWidth = 25; // m
  double buildingHeight = 10; // m
  uint32_t numBuildingsX = 10;
  uint32_t numBuildingsY = 10;
  double maxAxisX = (buildingSizeX + streetWidth) * numBuildingsX;
  double maxAxisY = (buildingSizeY + streetWidth) * numBuildingsY;

  std::vector<Ptr<Building> > buildingVector;
  for (uint32_t buildingIdX = 0; buildingIdX < numBuildingsX; ++buildingIdX)
    {
      for (uint32_t buildingIdY = 0; buildingIdY < numBuildingsY; ++buildingIdY)
        {
          Ptr < Building > building;
          building = CreateObject<Building> ();

          building->SetBoundaries (Box (buildingIdX * (buildingSizeX + streetWidth),
                                        buildingIdX * (buildingSizeX + streetWidth) + buildingSizeX,
                                        buildingIdY * (buildingSizeY + streetWidth),
                                        buildingIdY * (buildingSizeY + streetWidth) + buildingSizeY,
                                        0.0, buildingHeight));
          building->SetNRoomsX (1);
          building->SetNRoomsY (1);
          building->SetNFloors (1);
          buildingVector.push_back (building);
        }
    }

  // print the list of buildings to file
  PrintGnuplottableBuildingListToFile ("buildings.txt");

  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::RandomWalk2dOutdoorMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-10, maxAxisX+40, -10, maxAxisY+40)));
  Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator> ();
  position->Add (Vector (-1, -1, 1));
  mobility.SetPositionAllocator(position);

  MobilityHelper mobility2;
  mobility2.SetMobilityModel ("ns3::RandomWalk2dOutdoorMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-10, maxAxisX+40, -10, maxAxisY+40)));
  Ptr<ListPositionAllocator> position2 = CreateObject<ListPositionAllocator> ();
  position2->Add (Vector (-1, maxAxisY, 1));
  mobility2.SetPositionAllocator(position2);

  NodeContainer group1;
  group1.Create(10);

  NodeContainer group2;
  group2.Create(4);

  Ptr<GroupMobilityHelper> groupMobility = CreateObject<GroupMobilityHelper>();
  groupMobility->SetAttribute("GroupSecondaryMobilityModel", 
    StringValue("ns3::GroupSecondaryOutdoorMobilityModel"));
  groupMobility->SetAttribute("PathDeviationRandomVariable", 
    StringValue("ns3::NormalRandomVariable[Mean=0.0|Variance=1|Bound=20]"));

  groupMobility->SetMobilityHelper(&mobility);
  NodeContainer allNodes1 = groupMobility->InstallGroupMobility(group1);

  groupMobility->SetMobilityHelper(&mobility2);
  NodeContainer allNodes2 = groupMobility->InstallGroupMobility(group2);

  AsciiTraceHelper ascii;
  MobilityHelper::EnableAsciiAll (ascii.CreateFileStream ("mobility-trace-example.mob"));

  Simulator::Stop(Seconds(1e4));
  Simulator::Run ();
  Simulator::Destroy ();
}