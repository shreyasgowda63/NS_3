/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008-2009 Strasbourg University
 * Copyright (c) 2022 Jadavpur University
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
 * Author: David Gross <gdavid.devel@gmail.com>
 *         Sebastien Vincent <vincent@clarinet.u-strasbg.fr>
 * Modified by Akash Mondal <a98mondal@gmail.com>
 */

// Network topology
// //
// //             n0     n1
// //             |       |
// //             =========
// //
// // - Tracing of queues and packet receptions to file "fragmentation-ipv6-three.tr"

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/ipv6-static-routing-helper.h"

#include "ns3/ipv6-routing-table-entry.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FragmentationIpv6ExampleThree");

int main (int argc, char** argv)
{
  bool verbose = false;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("verbose", "turn on log components", verbose);
  cmd.Parse (argc, argv);

  if (verbose)
    {
      LogComponentEnable ("Ipv6L3Protocol", LOG_LEVEL_ALL);
      LogComponentEnable ("Icmpv6L4Protocol", LOG_LEVEL_ALL);
      LogComponentEnable ("Ipv6StaticRouting", LOG_LEVEL_ALL);
      LogComponentEnable ("Ipv6Interface", LOG_LEVEL_ALL);
      LogComponentEnable ("Ping6Application", LOG_LEVEL_ALL);
    }

  NS_LOG_INFO ("Create nodes.");
  Ptr<Node> n0 = CreateObject<Node> ();
  Ptr<Node> n1 = CreateObject<Node> ();

  NodeContainer net (n0, n1);

  NS_LOG_INFO ("Create IPv6 Internet Stack");
  InternetStackHelper internetv6;
  internetv6.Install (net);

  NS_LOG_INFO ("Create channels.");
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  // CSMA Network has a default MTU of 1500 bytes
  NetDeviceContainer dev = csma.Install (net);

  NS_LOG_INFO ("Create networks and assign IPv6 Addresses.");
  Ipv6AddressHelper ipv6;
  ipv6.SetBase (Ipv6Address ("2001:1::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer interface = ipv6.Assign (dev);

  /* Create a Ping6 application to send ICMPv6 echo request from n0 to n1 via r */
  uint32_t packetSize = 1500 - 40 - 8; // MTU - IPv6 header - ICMPv6 header
  uint32_t maxPacketCount = 5;
  Time interPacketInterval = Seconds (1.0);
  Ping6Helper ping6;

  ping6.SetLocal (interface.GetAddress (0, 1));
  ping6.SetRemote (interface.GetAddress (1, 1)); 

  // Ping application sending packets of size = MTU
  ping6.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  ping6.SetAttribute ("Interval", TimeValue (interPacketInterval));
  ping6.SetAttribute ("PacketSize", UintegerValue (packetSize));
  ApplicationContainer apps = ping6.Install (net.Get (0));
  apps.Start (Seconds (2.0));
  apps.Stop (Seconds (10.0));

  // Ping application sending packets of size > MTU
  packetSize = 1500 + 30;    // MTU + 30
  ping6.SetAttribute ("PacketSize", UintegerValue (packetSize));
  apps = ping6.Install (net.Get (0));
  apps.Start (Seconds (12.0));
  apps.Stop (Seconds (20.0));

  AsciiTraceHelper ascii;
  csma.EnableAsciiAll (ascii.CreateFileStream ("fragmentation-ipv6-three.tr"));
  csma.EnablePcapAll (std::string ("fragmentation-ipv6-three"), true);

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
