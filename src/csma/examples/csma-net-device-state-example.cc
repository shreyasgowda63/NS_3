/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 NITK Surathkal
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
 * Author: Ananthakrishnan S <ananthakrishnan190@gmail.com>
 */

// Network topology
//
//       n0    n1   n2   n3
//       |     |    |    |
//     =====================
// Traffic flows from n0 to n3.


#include <fstream>
#include <string>


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/packet-sink.h"
#include "ns3/internet-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CsmaNetDeviceStateExample");

void
StateChangeListener (bool isUp, NetDeviceState::OperationalState opState)
{
  static bool lastReportedAdminState = false;

  if (lastReportedAdminState != isUp)
  {
    NS_LOG_UNCOND ("At time " << Simulator::Now().GetSeconds () << "s, device goes administratively " <<
    (isUp? "UP" : "DOWN") << ", Operational state: " << opState);
    lastReportedAdminState = isUp; 
  }
  else
  {
    NS_LOG_UNCOND ("At time " << Simulator::Now().GetSeconds () << "s, Operational state changed to " << opState);
  }
}


int
main (int argc, char *argv[])
{

  CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv);

  // Here, we will explicitly create four nodes.
  NS_LOG_INFO ("Create nodes.");
  NodeContainer c;
  c.Create (4);

  // connect all our nodes to a shared channel.
  NS_LOG_INFO ("Build Topology.");
  CsmaHelper csmaHelper;
  csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate (10000000)));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (1)));
  NetDeviceContainer devs = csmaHelper.Install (c);

  Ptr<CsmaNetDeviceState> state = devs.Get (0)->GetObject<CsmaNetDeviceState> ();
  state->TraceConnectWithoutContext ("StateChange", MakeCallback (&StateChangeListener));

  // add an ip stack to all nodes.
  NS_LOG_INFO ("Add ip stack.");
  InternetStackHelper ipStack;
  ipStack.Install (c);

  // assign ip addresses
  NS_LOG_INFO ("Assign ip addresses.");
  Ipv4AddressHelper ip;
  ip.SetBase ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer addresses = ip.Assign (devs);

  //
  // Create a BulkSendApplication and install it on node 0
  //
  uint16_t port = 9;  // well-known echo port number


  BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (addresses.GetAddress (3), port));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (0));
  ApplicationContainer sourceApps = source.Install (c.Get (0));
  sourceApps.Start (Seconds (0.0));
  sourceApps.Stop (Seconds (50.0));

//
// Create a PacketSinkApplication and install it on node 3
//
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (c.Get (3));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (50.0));

  // Bring down 0th NetDevice which is in node 0 at 25th 
  // second into the simulation.
  csmaHelper.SetDeviceDown (Seconds (25.0), devs.Get (0)); 

  // Bring up the device that was brought down at 30th
  // second into the simulation.
  csmaHelper.SetDeviceUp (Seconds (30.0), devs.Get (0));

  // Detach channel from NetDevice 0 at 40th second into
  // the simulation.
  csmaHelper.DetachChannel (Seconds (40.0), devs.Get (0));

  // Reattach the channel to NetDevice 0 at 45th second.
  csmaHelper.ReattachChannel (Seconds (45.0), devs.Get (0));

  // Enable pcap on NetDevice 0.
  csmaHelper.EnablePcap ("csma-device-state", devs.Get(0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (50.0));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
