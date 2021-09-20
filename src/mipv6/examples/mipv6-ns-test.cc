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
#include "ns3/log.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>

using namespace ns3;

// This program demonstrates home agent and mobile node behaviour
// with regards to Proxy Neighbour Discovery Protocol in HA respectively
// in the following topology
//
//     mn ---- ar ---- ha ---- dn
//
// dn tries to configure an address on its interface with same address as mobile agent
// home agent defends the address and sends a Neighbour Advertisement in response
// (advertisement not processed in dn since no address matches in dn with destination of NA)

NS_LOG_COMPONENT_DEFINE ("mipv6-ns-test");

class Mipv6NSTest
{
  public:
    Mipv6NSTest ();
    void SendNS (Ptr<Node> node, Ptr<NetDevice> dev1, Ptr<NetDevice> dev2);
    void TestReceived (Ptr<const Packet> p, Ptr<Ipv6> ipv6, uint32_t interface);
    bool sentNS;
    bool receivedNA;
  private:

};

Mipv6NSTest::Mipv6NSTest () {
  sentNS = false;
  receivedNA = false;
}

void Mipv6NSTest::SendNS (Ptr<Node> node, Ptr<NetDevice> dev1, Ptr<NetDevice> dev2) {
  Ptr<Icmpv6L4Protocol> icmp = node->GetObject<Icmpv6L4Protocol>();
  Ptr<Ipv6L3Protocol> ipv6 = node->GetObject<Ipv6L3Protocol>();

  Address hardwareAddress = dev1->GetAddress ();

  icmp->SendNS ("3001:db80::200:ff:fe00:3", "ff02::1:ff00:2", "3001:db80::200:ff:fe00:4", hardwareAddress);

  sentNS = true;

}

void Mipv6NSTest::TestReceived (Ptr<const Packet> p, Ptr<Ipv6> ipv6, uint32_t interface)
{
  Ipv6Header ipv6Header;
  Ptr<Packet> pkt = p->Copy ();
  pkt->Print (std::cout);
  pkt->RemoveHeader (ipv6Header);

  if (ipv6Header.GetNextHeader () == Icmpv6L4Protocol::PROT_NUMBER) {
    Icmpv6Header icmpHeader;
    pkt->PeekHeader (icmpHeader);

    if (icmpHeader.GetType () == Icmpv6Header::ICMPV6_ND_NEIGHBOR_ADVERTISEMENT) {
      if (!sentNS) {
        NS_LOG_ERROR ("Neighbour Solicitation was not sent");
        exit (1);
      }
      Icmpv6NA naHeader;
      pkt->RemoveHeader (naHeader);
      if (!(naHeader.GetIpv6Target () == "3001:db80::200:ff:fe00:4")) {
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

  NodeContainer arhadn;
  arhadn.Add (ar);
  arhadn.Add (ha);
  arhadn.Add (dn);

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

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  NetDeviceContainer dAdHdN = csma.Install (arhadn);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (5000000)));
  p2p.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  NetDeviceContainer dMdA = p2p.Install (mnar);
  NetDeviceContainer dA (dMdA.Get (1));
  NetDeviceContainer dM (dMdA.Get (0));

  Ipv6InterfaceContainer iA;
  Ipv6InterfaceContainer iM;
  Ipv6InterfaceContainer iAiHiC;
  Ipv6AddressHelper ipv6;

  ipv6.SetBase (Ipv6Address ("3001:db80::"), Ipv6Prefix (64));
  iAiHiC = ipv6.Assign (dAdHdN);
  iAiHiC.SetForwarding (0, true);
  iAiHiC.SetForwarding (1, true);
  iAiHiC.SetForwarding (2, true);

  iAiHiC.SetDefaultRouteInAllNodes (0);
  iAiHiC.SetDefaultRouteInAllNodes (1);
  iAiHiC.SetDefaultRouteInAllNodes (2);
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
  rttop->AddHostRouteTo(Ipv6Address ("3001:db80::200:ff:fe00:4"),  Ipv6Address ("fe80::200:ff:fe00:4"), 2);
  ipv692 = ha.Get (0)->GetObject<Ipv6> ();
  rttop = routingHelper.GetStaticRouting (ipv692);
  rttop->AddNetworkRouteTo(Ipv6Address ("1001:db80::"), Ipv6Prefix (64), Ipv6Address("fe80::200:ff:fe00:1"), 1, 0);

  //Installing MIPv6
  Mipv6HaHelper hahelper;
  hahelper.Install (ha.Get (0));
  Mipv6MnHelper mnhelper (hahelper.GetHomeAgentAddressList (),false); 
  mnhelper.Install (mn.Get (0));

  Mipv6NSTest mi;

  Simulator::Schedule (Seconds(5), &Mipv6NSTest::SendNS, &mi, dn.Get (0), dAdHdN.Get (2), dAdHdN.Get (0));

  Ptr<Ipv6L3Protocol> ipL3 = dn.Get (0)->GetObject<Ipv6L3Protocol> ();
  ipL3->TraceConnectWithoutContext ("Rx", MakeCallback(&Mipv6NSTest::TestReceived, &mi));
  
  Simulator::Stop (Seconds (100.0));
  Simulator::Run ();

  if (!mi.sentNS) {
    NS_LOG_ERROR ("NS packet was not sent");
    exit (1);
  }
  else if (!mi.receivedNA) {
    NS_LOG_ERROR ("NA packet was not received");
    exit (1);
  }

  Simulator::Destroy ();
}

