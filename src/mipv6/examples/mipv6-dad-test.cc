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

// This program demonstrates home agent and mobile node behaviour
// with regards to Proxy Neighbour Discovery Protocol in HA respectively
// in the following topology
//
//     mn ---- ar ---- ha ---- dn
//
//  dn is configured such that it contains an interface with address same as what mn tries to configure its home address
// therefore DAD in HA for home address of mn fails.

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
  nodes.Create(4);

  NodeContainer mn (nodes.Get (0));
  NodeContainer ar (nodes.Get (1));
  NodeContainer ha (nodes.Get (2));
  NodeContainer dn (nodes.Get (3));

  NodeContainer hadn;
  hadn.Add (ha);
  hadn.Add(dn);

  NodeContainer arha;
  arha.Add (ar);
  arha.Add (ha);

  NodeContainer mnar;
  mnar.Add (mn);
  mnar.Add (ar);

  InternetStackHelper internet;
  internet.Install (nodes);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc;
  positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));  // mn
  positionAlloc->Add (Vector (20.0, 0.0, 0.0)); // ar
  positionAlloc->Add (Vector (40.0, 0.0, 0.0)); // ha
  positionAlloc->Add (Vector (60.0, 0.0, 0.0)); // dn

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (5000000)));
  p2p.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  NetDeviceContainer dHdC = p2p.Install (hadn);
  NetDeviceContainer dMdA = p2p.Install (mnar);
  NetDeviceContainer dA (dMdA.Get (1));
  NetDeviceContainer dM (dMdA.Get (0));
  NetDeviceContainer dC (dHdC.Get (1));
  NetDeviceContainer dAdH = p2p.Install (arha);

  Ipv6InterfaceContainer iA;
  Ipv6InterfaceContainer iM;
  Ipv6InterfaceContainer iC;
  Ipv6InterfaceContainer iAiH;
  Ipv6InterfaceContainer iHiC;
  Ipv6AddressHelper ipv6;

  ipv6.SetBase (Ipv6Address ("2001:db80::"), Ipv6Prefix (64));
  iHiC = ipv6.Assign (dHdC);
  iHiC.SetForwarding (0, true);
  iHiC.SetForwarding (1, true);
  iHiC.SetDefaultRouteInAllNodes (0);
  iHiC.SetDefaultRouteInAllNodes (1);
  ipv6.SetBase (Ipv6Address ("1001:db80::"), Ipv6Prefix (64));
  iA = ipv6.Assign (dA);
  iA.SetForwarding (0, true);
  iA.SetDefaultRouteInAllNodes (0);
  iM = ipv6.AssignWithoutAddress (dM);
  ipv6.SetBase (Ipv6Address ("3001:db80::"), Ipv6Prefix (64));
  iAiH = ipv6.Assign (dAdH);
  iAiH.SetForwarding (0, true);
  iAiH.SetForwarding (1, true);
  iAiH.SetDefaultRouteInAllNodes (0);
  iAiH.SetDefaultRouteInAllNodes (1);

  dC.Get(0) ->SetAddress (dM.Get(0)->GetAddress ());
  ipv6.SetBase (Ipv6Address ("2001:db80::"), Ipv6Prefix (64));
  iC = ipv6.Assign (dC);
  iC.SetForwarding (0, true);
  iC.SetDefaultRouteInAllNodes (0);

  Ipv6Address prefix ("1001:db80::");

  uint32_t indexRouter = iA.GetInterfaceIndex (0);  // AR interface (mn-AR) 

  Ptr<Radvd> radvd = CreateObject<Radvd> ();
  Ptr<RadvdInterface> routerInterface = Create<RadvdInterface> (indexRouter, 1500, 50);
  Ptr<RadvdPrefix> routerPrefix = Create<RadvdPrefix> (prefix, 64, 1.5, 2.0);

  routerInterface->AddPrefix (routerPrefix);

  radvd->AddConfiguration (routerInterface);

  ar.Get(0)->AddApplication (radvd);
  radvd->SetStartTime(Seconds (1.0));
  radvd->SetStopTime(Seconds (10.0));

  Ipv6StaticRoutingHelper routingHelper;

  Ptr<Ipv6> ipv692 = ar.Get (0)->GetObject<Ipv6> ();
  Ptr<Ipv6StaticRouting> rttop = routingHelper.GetStaticRouting (ipv692);
  rttop->AddNetworkRouteTo(Ipv6Address ("2001:db80::"), Ipv6Prefix (64), Ipv6Address ("3001:db80::200:ff:fe00:6"), 2, 0);
  ipv692 = ha.Get (0)->GetObject<Ipv6> ();
  rttop = routingHelper.GetStaticRouting (ipv692);
  rttop->AddNetworkRouteTo(Ipv6Address ("1001:db80::"), Ipv6Prefix (64), Ipv6Address("3001:db80::200:ff:fe00:5"), 1, 0);
  ipv692 = dn.Get (0)->GetObject<Ipv6> ();
  rttop = routingHelper.GetStaticRouting (ipv692);
  rttop->AddNetworkRouteTo (Ipv6Address ("1001:db80::"), Ipv6Prefix (64), Ipv6Address ("2001:db80::200:ff:fe00:1"), 1, 0);


  //Installing MIPv6
  Mipv6HaHelper hahelper;
  hahelper.Install (ha.Get (0));
  Mipv6MnHelper mnhelper (hahelper.GetHomeAgentAddressList (),false); 
  mnhelper.Install (mn.Get (0));

  Simulator::Run ();
  Simulator::Destroy ();

}

