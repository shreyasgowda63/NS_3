/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 Jadavpur University, India
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
 * Author: Zakaria Helal Arzoo <arzoozakaria@gmail.com>
 */


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/csma-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/bridge-module.h"
#include "ns3/ipv6-static-routing.h"
#include "ns3/ipv6-list-routing-helper.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/mipv6-module.h"
#include "ns3/internet-trace-helper.h"
#include "ns3/trace-helper.h"
#include "ns3/internet-apps-module.h"
#include "ns3/radvd.h"
#include "ns3/radvd-interface.h"
#include "ns3/radvd-prefix.h"
#include "ns3/ssid.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>

using namespace ns3;

// This program demonstrates home agent and a single mobile node behaviour
// with regards to binding updates and binding acknowledgement respectively
// in the following topology
//
// Time: 0s-35s
//     ha --------- ar
//     |             |
//     *             *
//
//     * ---->
//     |
//     mn
//
//  The distance between the two ARs is 100 and they are both connected to 
// the Home agent via a PointToPoint link.
//
// Time: 35s onwards
//     ha --------- ar
//     |             |
//     *             *
//
//             <---- * 
//                   |
//                   mn
//
// It aims to show behaviour when mobile node disconnects from home agent 
// and joins another ar and reconnects with ha

int main (int argc, char** argv)
{
  bool verbose = false;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("verbose", "turn on log components", verbose);

  cmd.Parse (argc, argv);
  
  if (verbose)
    {
      LogComponentEnable ("Mipv6Mn", LOG_LEVEL_ALL);
      LogComponentEnable ("Mipv6Ha", LOG_LEVEL_ALL);
      LogComponentEnable ("Mipv6Agent", LOG_LEVEL_ALL);
    }

  NodeContainer nodes;
  nodes.Create(3);

  NodeContainer mn (nodes.Get(0));
  NodeContainer ar;
  NodeContainer ha (nodes.Get(1));

  ar.Add (nodes.Get(1));
  ar.Add (nodes.Get(2));

  InternetStackHelper internet;
  internet.Install (nodes);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc;
  positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (-50.0, 20.0, 0.0)); //AR1
  positionAlloc->Add (Vector (50.0, 20.0, 0.0)); //AR2
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (ar);

  positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (-50.0, 50.0, 0.0)); //MN
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");  
  mobility.Install(mn);

  Ptr<ConstantVelocityMobilityModel> cvm = mn.Get (0)->GetObject<ConstantVelocityMobilityModel> ();
  cvm->SetVelocity (Vector (3, 0, 0));

  Simulator::Schedule (Seconds (35.0), &ConstantVelocityMobilityModel::SetVelocity, cvm, Vector (-3, 0, 0));

  NetDeviceContainer mnDev;
  NetDeviceContainer ar1Devs;
  NetDeviceContainer ar2Devs;

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy;
  phy.SetChannel (channel.Create ());
  
  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));


  mnDev.Add (wifi.Install (phy, mac, mn.Get (0)));
  

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));
               
  ar1Devs = wifi.Install (phy, mac, ar.Get (0));
  ar2Devs = wifi.Install (phy, mac, ar.Get (1));

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (5000000)));
  p2p.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  NetDeviceContainer dAdH = p2p.Install (ar);

  Ipv6InterfaceContainer iA;
  Ipv6InterfaceContainer iM;
  Ipv6InterfaceContainer iH;
  Ipv6InterfaceContainer iAiH;

  Ipv6AddressHelper ipv6;

  ipv6.SetBase (Ipv6Address ("1001:db80::"), Ipv6Prefix (64));
  iA = ipv6.Assign (ar1Devs);
  iA.SetForwarding (0, true);
  iA.SetDefaultRouteInAllNodes (0);

  ipv6.SetBase (Ipv6Address ("2001:db80::"), Ipv6Prefix (64));
  iH = ipv6.Assign (ar2Devs);
  iH.SetForwarding (0, true);
  iH.SetDefaultRouteInAllNodes (0);

  ipv6.SetBase (Ipv6Address ("3001:db80::"), Ipv6Prefix (64));
  iAiH = ipv6.Assign (dAdH);
  iAiH.SetForwarding (0, true);
  iAiH.SetForwarding (1, true);
  iAiH.SetDefaultRouteInAllNodes (0);
  iAiH.SetDefaultRouteInAllNodes (1);

  iM = ipv6.AssignWithoutAddress (mnDev);


  Ipv6Address prefix ("1001:db80::");  //create the prefix 

  uint32_t indexRouter = iA.GetInterfaceIndex (0);  //AR interface (mn-AR) 

  Ptr<Radvd> radvd = CreateObject<Radvd> ();
  Ptr<RadvdInterface> routerInterface = Create<RadvdInterface> (indexRouter, 1500, 50);
  Ptr<RadvdPrefix> routerPrefix = Create<RadvdPrefix> (prefix, 64, 1.5, 2.0);

  routerInterface->AddPrefix (routerPrefix);

  radvd->AddConfiguration (routerInterface);

  ar.Get(0)->AddApplication (radvd);
  radvd->SetStartTime(Seconds (1.0));
  radvd->SetStopTime(Seconds (100.0));

  Ipv6Address prefix2 ("2001:db80::");  //create the prefix 

  uint32_t indexRouter2 = iH.GetInterfaceIndex (0);  //HA interface (mn-HA) 

  Ptr<Radvd> radvd2 = CreateObject<Radvd> ();
  Ptr<RadvdInterface> routerInterface2 = Create<RadvdInterface> (indexRouter2, 1500, 50);
  Ptr<RadvdPrefix> routerPrefix2 = Create<RadvdPrefix> (prefix2, 64, 1.5, 2.0);

  routerInterface2->AddPrefix (routerPrefix2);

  radvd2->AddConfiguration (routerInterface2);

  ar.Get(1)->AddApplication (radvd2);
  radvd2->SetStartTime(Seconds (1.0));
  radvd2->SetStopTime(Seconds (100.0));

  //Installing MIPv6
  Mipv6HaHelper hahelper;
  hahelper.Install (ha.Get (0));
  Mipv6MnHelper mnhelper (hahelper.GetHomeAgentAddressList (), false); 
  mnhelper.Install (mn.Get (0));

  Simulator::Stop (Seconds (100.0));
  Simulator::Run ();
  Simulator::Destroy ();

}

