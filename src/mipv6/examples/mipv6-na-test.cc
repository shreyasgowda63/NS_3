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
// dn sends a neighbour advertisement to ha with target as home address of mn
// home agent sets entry for that address in its cache as invalid. 

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
#include "ns3/log.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("mipv6-na-test");

class Mipv6NATest
{
  public: 
    Mipv6NATest();
    void SendNA (Ptr<Node> node, Ptr<NetDevice> dev1, Ptr<NetDevice> dev2);
    void TestReceived (Ptr<const Packet> p, Ptr<Ipv6> ipv6, uint32_t interface);
    bool sentNA;
    bool receivedNA;
  private:

};

Mipv6NATest::Mipv6NATest () {
  sentNA = false;
  receivedNA = false;
}

void Mipv6NATest::SendNA (Ptr<Node> node, Ptr<NetDevice> dev1, Ptr<NetDevice> dev2) {
  Ptr<Icmpv6L4Protocol> icmp = node->GetObject<Icmpv6L4Protocol>();
  Ptr<Ipv6L3Protocol> ipv6 = node->GetObject<Ipv6L3Protocol>();

  Address hardwareAddress = dev1->GetAddress ();

  NdiscCache::Ipv6PayloadHeaderPair p = icmp->ForgeNA ("3001:db80::200:ff:fe00:3", "fe80::200:ff:fe00:1", &hardwareAddress, 3);

  Address replyMacAddress = dev1->GetMulticast (Ipv6Address("fe80:200:ff:fe00:1"));

  Ptr<Packet> pkt = p.first;
  pkt->AddHeader (p.second);
  sentNA = dev1->Send (pkt, replyMacAddress, Ipv6L3Protocol::PROT_NUMBER);

  if (!sentNA) {
    NS_LOG_ERROR ("Failed to send NA");
    exit (1);
  }
}

void Mipv6NATest::TestReceived (Ptr<const Packet> p, Ptr<Ipv6> ipv6, uint32_t interface)
{
  Ipv6Header ipv6Header;
  Ptr<Packet> pkt = p->Copy ();
  pkt->Print (std::cout);
  pkt->RemoveHeader (ipv6Header);

  if (ipv6Header.GetNextHeader () == Icmpv6L4Protocol::PROT_NUMBER) {
    Icmpv6Header icmpHeader;
    pkt->PeekHeader (icmpHeader);

    if (icmpHeader.GetType () == Icmpv6Header::ICMPV6_ND_NEIGHBOR_ADVERTISEMENT) {
      if (!sentNA) {
        NS_LOG_ERROR ("Neighbour Advertisement was not sent");
        exit (1);
      }
      Icmpv6NA naHeader;
      pkt->RemoveHeader (naHeader);
      if (!(naHeader.GetIpv6Target () == "3001:db80::200:ff:fe00:3")) {
        NS_LOG_ERROR ("Target address does not match");
        exit (1);
      }
      receivedNA = true;
    }
  }
}

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

  NodeContainer mn (nodes.Get(0));
  NodeContainer ar (nodes.Get(1));
  NodeContainer ha (nodes.Get(2));
  NodeContainer dn (nodes.Get(3));

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
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));  //mn
  positionAlloc->Add (Vector (20.0, 0.0, 0.0)); //ar
  positionAlloc->Add (Vector (40.0, 0.0, 0.0));  //ha
  positionAlloc->Add (Vector (60.0, 0.0, 0.0));  //dn

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
  NetDeviceContainer dAdH = p2p.Install (arha);

  Ipv6InterfaceContainer iA;
  Ipv6InterfaceContainer iM;
  Ipv6InterfaceContainer iAiH;
  Ipv6InterfaceContainer iHiC;
  Ipv6AddressHelper ipv6;

  ipv6.SetBase (Ipv6Address ("3001:db80::"), Ipv6Prefix (64));
  iAiH = ipv6.Assign (dAdH);
  iAiH.SetForwarding (0, true);
  iAiH.SetDefaultRouteInAllNodes (1);
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

  Ipv6StaticRoutingHelper routingHelper;

  Ptr<Ipv6> ipv692 = ar.Get(0)->GetObject<Ipv6> ();
  Ptr<Ipv6StaticRouting> rttop = routingHelper.GetStaticRouting (ipv692);
  rttop->AddNetworkRouteTo(Ipv6Address ("2001:db80::"), Ipv6Prefix (64), Ipv6Address ("3001:db80::200:ff:fe00:6"), 1, 0);
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

  Mipv6NATest mi;

  Simulator::Schedule (Seconds(5), &Mipv6NATest::SendNA, &mi, dn.Get (0), dHdC.Get (1), dHdC.Get (0));

  Ptr<Ipv6L3Protocol> ipL3 = ha.Get (0)->GetObject<Ipv6L3Protocol> ();
  ipL3->TraceConnectWithoutContext ("Rx", MakeCallback(&Mipv6NATest::TestReceived, &mi));
  
  Simulator::Stop (Seconds (100.0));
  Simulator::Run ();

  if (!mi.sentNA) {
    NS_LOG_ERROR ("NS packet was not sent");
    exit (1);
  }
  else if (!mi.receivedNA) {
    NS_LOG_ERROR ("NA packet was not received");
    exit (1);
  }
  else {
    Ptr<Mipv6Ha> agent = ha.Get (0)->GetObject<Mipv6Ha> ();
    if (agent) {
      if (agent->IsAddress ("3001:db80::200:ff:fe00:3")) {
        NS_LOG_ERROR ("Duplicate address not handled properly");
        exit (1);
      }
    }
  }

  Simulator::Destroy ();
}