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
#include "ns3/rectangle.h"
#include "ns3/wifi-module.h"
#include "ns3/single-model-spectrum-channel.h"
#include <chrono> //for timer
#include <iomanip>

using namespace ns3;

void output_received(std::vector<unsigned int> received, unsigned int width)
{
  for(unsigned int i=0; i<received.size(); i++)
    {
      std::cout << std::setw(3) << received[i];
      if((i+1)%width==0)
        {
          std::cout << std::endl;
        }
    }
}

std::pair<double, std::vector<unsigned int> > run(unsigned int width,
                                                  double total_time,
                                                  bool clipping_enabled,
                                                  double clip_range,
                                                  std::string wifi_type,
                                                  std::string loss_model,
                                                  bool verbose)
{
  RngSeedManager::SetSeed(1);
  RngSeedManager::SetRun(1);
  Ptr<UniformRandomVariable> rndDir = CreateObject<UniformRandomVariable> ();
  rndDir->SetAttribute ("Min", DoubleValue (0));
  rndDir->SetAttribute ("Max", DoubleValue (6.283184)); // 2*pi radians
  rndDir->SetStream(1);

  //Create width^2 nodes on a grid
  NodeContainer nodes;
  nodes.Create(width*width);

  //for ns-3 versions 3.29 and earlier use 757 for node_separation
  double node_separation = 367.0; //before moving

  MobilityHelper mobility;
  mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                "MinX"       , DoubleValue  ( 0.0 ),
                                "MinY"       , DoubleValue  ( 0.0 ),
                                "DeltaX"     , DoubleValue  ( node_separation),
                                "DeltaY"     , DoubleValue  ( node_separation),
                                "GridWidth"  , UintegerValue( width  ), //Nodes per row
                                "LayoutType" , StringValue  ( "RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),
                             "Time", StringValue ("15s"),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"),
                             "Direction", PointerValue(rndDir),
                             "Bounds", RectangleValue(Rectangle(-node_separation, width*node_separation, -node_separation, width*node_separation)));
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

  YansWifiPhyHelper wifiPhy;
  wifiPhy.SetErrorRateModel ("ns3::NistErrorRateModel");
  SpectrumWifiPhyHelper spectrumPhy;
  spectrumPhy.SetErrorRateModel ("ns3::NistErrorRateModel");
  

  Ptr<PropagationLossModel> lossModel;
  if(loss_model == "range") {
    lossModel = CreateObject<RangePropagationLossModel>();
    lossModel->SetAttribute("MaxRange", DoubleValue(518.0));
  }
  else if(loss_model == "friis"){
    lossModel = CreateObject<FriisPropagationLossModel>();
  }
  else {
    NS_FATAL_ERROR("Unsupported propagation loss model:" << loss_model);
  }

  if (wifi_type == "ns3::YansWifiPhy")
    {
      YansWifiChannelHelper wifiChannel;
      wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
      if (clipping_enabled)
      {
        wifiChannel.EnableClipping ();
      }
      Ptr<YansWifiChannel> channel = wifiChannel.Create ();
      channel->SetPropagationLossModel(lossModel);
      if (clipping_enabled)
      {
        channel->SetAttribute ("ReceiveClipRange", DoubleValue (clip_range));
      }
      wifiPhy.SetChannel (channel);
    }
  else if (wifi_type == "ns3::SpectrumWifiPhy")
    {
      Ptr<MultiModelSpectrumChannel> spectrumChannel = nullptr;   //multi model
      if(!clipping_enabled) {
        spectrumChannel = CreateObject<MultiModelSpectrumChannel> ();
      }
      else {
        spectrumChannel = CreateObject<MultiModelSpectrumChannelSpatialIndex> () ;
        spectrumChannel->SetAttribute ("EnableSpatialIndexing", BooleanValue (true));
        spectrumChannel->SetAttribute ("ReceiveClipRange", DoubleValue (clip_range));
      }
      spectrumChannel->AddPropagationLossModel (lossModel);

      Ptr<ConstantSpeedPropagationDelayModel> delayModel
        = CreateObject<ConstantSpeedPropagationDelayModel> ();
      spectrumChannel->SetPropagationDelayModel (delayModel);

      spectrumPhy.SetChannel (spectrumChannel);
    }
  else
    {
      NS_FATAL_ERROR ("Unsupported WiFi type " << wifi_type);
    }
  //Create Devices
  WifiHelper wifi;
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode",
                               StringValue("OfdmRate6Mbps"),
                               "RtsCtsThreshold", UintegerValue(0));

  NetDeviceContainer devices;
  if (wifi_type == "ns3::YansWifiPhy")
    {
      devices = wifi.Install (wifiPhy, wifiMac, nodes);
    }
  else if(wifi_type == "ns3::SpectrumWifiPhy")
    {
      devices = wifi.Install (spectrumPhy, wifiMac, nodes);
    }

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
  UdpClientHelper ping_app(Ipv4Address("255.255.255.255"),port); //ping all
  UdpServerHelper serv_app(port);
  ping_app.SetAttribute("Interval",TimeValue(Seconds(100)));
  ping_app.SetAttribute("MaxPackets",UintegerValue(1000000));
  p.Add(ping_app.Install(nodes));
  for(unsigned int i=0; i<p.GetN(); i++)
    {
      //offset start times slightly to avoid collisions
      p.Get(i)->SetStartTime(Seconds(10+(i*.01)));
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
  //for ns-3 version 3.29 and earlier use 1070.0 for default clip_range
  double clip_range = 519.0;
  std::string wifi_type  = "ns3::SpectrumWifiPhy";
  std::string loss_model = "friis";

  CommandLine cmd;
  cmd.AddValue ("verbose", "Tell application to log if true", verbose);
  cmd.AddValue ("width", "width and height of grid of nodes", width);
  cmd.AddValue ("clip_range", "distance within which to attempt to send packets", clip_range);
  cmd.AddValue ("wifi_type", "select ns3::SpectrumWifiPhy or ns3::YansWifiPhy", wifi_type);
  cmd.AddValue ("loss_model", "model to use for packet loss. range or friis", loss_model);
  cmd.Parse (argc,argv);

  LogComponentEnable ("SpatialIndexing", LOG_LEVEL_WARN);
  LogComponentEnable ("KDTree", LOG_LEVEL_WARN);
  std::cout << "In the following simulation " << width*width <<
    " nodes will be created, with their starting positions on a grid." <<
    " Nodes will select a random direction of" <<
    " travel every 15 seconds and travel in that direction at 5 m/s." <<
    " During the simulation Each node will send a UDP packet to the" <<
    " broadcast address over a" <<
    " Wifi Channel.  The identical motions are repeated with and without" <<
    " clipping enabled, and the speedup using clipping and fidelity are" <<
    " computed.  With clipping enabled the simulation time is drastically" <<
    " reduced as receive events are only placed on the queue for nodes" <<
    " within the clipping range, yielding nearly the same, if not"
    " identical results in much less time." <<
    std::endl << std::endl;
  std::cout << "Wifi Type: " << wifi_type << std::endl;
  std::cout << "Clip Range: " << clip_range << std::endl;
  std::cout << "Loss Model: " << loss_model << std::endl;
  std::cout << "Simulating " << width*width << " mobile nodes." << std::endl;
  std::cout << "Simulating with clipping enabled..." << std::endl;
  std::pair<double, std::vector<unsigned int> > vals1 = run(width, total_time, true, clip_range, wifi_type, loss_model, verbose);
  double dur_with_clip = vals1.first;
  std::vector<unsigned int> received_with_clip = vals1.second;
  std::cout << std::endl <<
    "Simulating with clipping disabled..." << std::endl;
  std::pair<double, std::vector<unsigned int> > vals2 = run(width, total_time, false, clip_range, wifi_type, loss_model, verbose);
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
  std::cout << std::endl;
  std::cout << "Speedup = " << dur_no_clip/dur_with_clip << std::endl;
  std::cout << "Fidelity:" << (num_same / double(received_with_clip.size()))*100 << "%" << std::endl;
  std::cout << "(above is percentage of nodes receiving the same number of packets with clipping enabled as they do otherwise)" << std::endl;
  return 0;
}




