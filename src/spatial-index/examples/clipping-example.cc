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

#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel-si.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/node-container.h"
#include "ns3/position-aware-helper.h"
#include "ns3/wifi-module.h"
#include <chrono> //for timer

using namespace ns3;

void output_received(std::vector<unsigned int> received, unsigned int width)
{
  for(unsigned int i=0; i<received.size(); i++)
    {
      std::cout << received[i];
      if((i+1)%width==0)
        {
          std::cout << std::endl;
        }
    }
}

std::pair<double, std::vector<unsigned int> > run(unsigned int width,
                                                  double total_time,
                                                  bool clipping_enabled,
                                                  bool verbose)
{
  //Create width^2 nodes on a grid
  NodeContainer nodes;
  nodes.Create(width*width);

  // for ns-3 versions 3.29 and earlier
  // use 757 for node_separation and 1070 for clip_range below.
  double node_separation = 367.0;
  double clip_range = 519.0;

  MobilityHelper mobility;
  mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                "MinX"       , DoubleValue  ( 0.0 ),
                                "MinY"       , DoubleValue  ( 0.0 ),
                                "DeltaX"     , DoubleValue  ( node_separation),
                                "DeltaY"     , DoubleValue  ( node_separation),
                                "GridWidth"  , UintegerValue( width  ), //Nodes per row
                                "LayoutType" , StringValue  ( "RowFirst"));
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(nodes);

  if(verbose)
    {
      std::cout << "Created " << nodes.GetN() << " nodes on a grid." << std::endl;
    }

  //---------
  //Create Mac, Channel and Phy

  if(clipping_enabled)
    {
      //Add Position Aware (needed for clipping)
      PositionAwareHelper pos_aware;
      pos_aware.Install(nodes);
    }

  WifiMacHelper wifiMac = WifiMacHelper();
  wifiMac.SetType("ns3::AdhocWifiMac");

  SpectrumWifiPhyHelper spectrumPhy;
  spectrumPhy.SetErrorRateModel ("ns3::NistErrorRateModel");
  
  Ptr<MultiModelSpectrumChannel> spectrumChannel = nullptr;
  if (!clipping_enabled)
  {
    spectrumChannel = CreateObject<MultiModelSpectrumChannel> ();
  }
  else
  {
    spectrumChannel = CreateObject<MultiModelSpectrumChannelSpatialIndex> ();
    spectrumChannel->SetAttribute ("EnableSpatialIndexing", BooleanValue (true));
    spectrumChannel->SetAttribute ("ReceiveClipRange", DoubleValue (clip_range));
  }
  spectrumChannel->AddPropagationLossModel(CreateObject<FriisPropagationLossModel>());
  Ptr<ConstantSpeedPropagationDelayModel> delayModel
    = CreateObject<ConstantSpeedPropagationDelayModel> ();
  spectrumChannel->SetPropagationDelayModel (delayModel);

  spectrumPhy.SetChannel (spectrumChannel);


  //Create Devices
  WifiHelper wifi;
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode",
                               StringValue("OfdmRate6Mbps"),
                               "RtsCtsThreshold", UintegerValue(0));
  NetDeviceContainer devices = wifi.Install(spectrumPhy, wifiMac, nodes);

  //Install Internet Stack
  InternetStackHelper stack;
  stack.Install(nodes);
  Ipv4AddressHelper address;
  address.SetBase("10.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer interface_pointer;
  interface_pointer.Add(address.Assign(devices));

  //Install UDP client/server Applications
  ApplicationContainer p;
  ApplicationContainer s;
  int port = 100;
  UdpClientHelper ping_app(Ipv4Address("255.255.255.255"),port); //broadcast
  UdpServerHelper serv_app(port);
  ping_app.SetAttribute("Interval",TimeValue(Seconds(100)));
  ping_app.SetAttribute("MaxPackets",UintegerValue(1000000));
  p.Add(ping_app.Install(nodes));
  for(unsigned int i=0; i<p.GetN(); i++)
    {
      //offset start times slightly to avoid collisions
      p.Get(i)->SetStartTime(Seconds(i*.01));
    }
  s.Add(serv_app.Install(nodes));

  s.Start (Seconds (0));
  p.Stop (Seconds(total_time));
  s.Stop (Seconds(total_time));

  //------
  if(verbose)
    {
      std::cout << "Sending packets..." << std::endl;
    }
  Simulator::Stop(Seconds(total_time));
  auto start = std::chrono::high_resolution_clock::now();
  Simulator::Run();
  auto finish = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = finish-start;
  std::cout << "Run time = " << elapsed.count() << " seconds" << std::endl;
  std::vector<unsigned int> received;
  for(unsigned int i=0; i<p.GetN(); i++)
    {
      received.push_back(DynamicCast<UdpServer>(s.Get(i))->GetReceived());
    }
  Simulator::Destroy ();
  return std::make_pair(elapsed.count(), received);
}

int
main (int argc, char *argv[])
{
  bool verbose = false;

  unsigned int width = 32;
  double total_time = 100;

  CommandLine cmd;
  cmd.AddValue ("verbose", "Tell application to log if true", verbose);
  cmd.AddValue ("width", "width and height of grid of nodes", width);
  cmd.Parse (argc,argv);

  std::cout << "In the following simulation a grid of " << width*width <<
    " nodes will be created, and each node will send a UDP packet to the" <<
    " broadcast address over a Spectrum Wifi Channel.  However the" <<
    " distances between nodes have been strategically set such that the" <<
    " wifi packets will only successfully propagate to direct neighbors" <<
    " (not diagonal).  With clipping simulation time is drastically reduced" <<
    " as receive events are only placed on the queue for nodes within the" <<
    " chosen clipping range, yielding the same results in much less time." <<
    std::endl << std::endl;
  std::cout << "Simulating with clipping enabled..." << std::endl;
  std::pair<double, std::vector<unsigned int> > vals1 = run(width, total_time, true, verbose);
  double dur_with_clip = vals1.first;
  std::vector<unsigned int> received_with_clip = vals1.second;
  std::cout << std::endl <<
    "Simulating with clipping disabled..." << std::endl;
  std::pair<double, std::vector<unsigned int> > vals2 = run(width, total_time, false, verbose);
  double dur_no_clip = vals2.first;
  std::vector<unsigned int> received_no_clip = vals2.second;
  unsigned int num_same = 0;
  for(unsigned int i=0; i<received_with_clip.size(); i++)
    {
      if(received_with_clip[i] == received_no_clip[i])
        {
          num_same++;
        }
    }
  if(verbose)
    {
      std::cout << "Number of packets received for each node (clipped):" << std::endl;
      output_received(received_with_clip, width);
      std::cout << "Number of packets received for each node (not clipped):" << std::endl;
      output_received(received_no_clip, width);
    }
  std::cout << "Speedup = " << dur_no_clip/dur_with_clip << std::endl;
  std::cout << "Fidelity:" << (num_same / double(received_with_clip.size()))*100 << "%" << std::endl;
  std::cout << "(above is percentage of nodes receiving the same number of packets with clipping enabled as they do otherwise)" << std::endl;
  //std::cout <<      "Incorrectly clipped % = " << std::endl;
  return 0;
}




