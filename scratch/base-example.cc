
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Universita' di Firenze, Italy
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
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
           Adnan Rashid <adnan.rashid@unifi.it>
// Network topology
//
//    n0(6LBR)
//  +---------+
//  | UDP     |         n1
//  +---------+    +---------+
//  | IPv6    |    | IPv6    |
//  +---------+    +---------+
//  | 6LoWPAN |    | 6LoWPAN |
//  +---------+    +---------+
//  | lr-wpan |    | lr-wpan |
//  +---------+    +---------+
//      ||             ||
//       ===============
//
// How to run 
//  ./waf --run "scratch/example-ping-lr-wpan-6lowNd.cc --Mesh --NeighborCache --Ping=6LN --GUA"
 */


#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/log.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/applications-module.h"
#include "ns3/address.h"

using namespace ns3;

int main (int argc, char** argv)
{
  bool useMeshUnder = false;
  bool printNodesAddresses = false;
  bool printNeighborCache = false;
  bool useLLA = false;
  bool useGUA = false;
  std::string useUdpFrom = "";
  std::string usePingOn = "";
  double stopTime = 40;

  CommandLine cmd;
  cmd.AddValue ("Mesh", "Use mesh-under in the network", useMeshUnder);
  cmd.AddValue ("Addresses", "Print the addresses of the nodes", printNodesAddresses);
  cmd.AddValue ("NeighborCache", "Print the neighbor cache entries", printNeighborCache);
  cmd.AddValue ("Udp", "Send one UDP packet from (6LBR, 6LN, nothing)", useUdpFrom);
  cmd.AddValue ("Ping", "Install Ping app on (6LBR, 6LN, nothing)", usePingOn);
  cmd.AddValue ("LLA", "Use link-local addresses for the communication", useLLA);
  cmd.AddValue ("GUA", "Use global addresses for the communication", useGUA);

  cmd.AddValue ("StopTime", "Simulation stop time (seconds)", stopTime);
  cmd.Parse (argc, argv);

  if (useMeshUnder)
    {
      Config::SetDefault ("ns3::SixLowPanNetDevice::UseMeshUnder", BooleanValue (true));
    }

  Packet::EnablePrinting ();

#if 0
  LogComponentEnable ("Ping6Application", LOG_LEVEL_ALL);
  LogComponentEnable ("LrWpanMac",LOG_LEVEL_ALL);
  LogComponentEnable ("LrWpanPhy",LOG_LEVEL_ALL);
  LogComponentEnable ("LrWpanNetDevice", LOG_LEVEL_ALL);
  LogComponentEnable ("SixLowPanNetDevice", LOG_LEVEL_ALL);
#endif

  Time SimulationEnd = Seconds (stopTime);

  NodeContainer nodes;
  nodes.Create(2);

  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (20),
                                 "DeltaY", DoubleValue (20),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  LrWpanHelper lrWpanHelper;
  // Add and install the LrWpanNetDevice for each node
  NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(nodes);

  // Fake PAN association and short address assignment.
  lrWpanHelper.AssociateToPan (lrwpanDevices, 0);

  InternetStackHelper internetv6;
  internetv6.Install (nodes);

  SixLowPanHelper sixlowpan;
  NetDeviceContainer devices = sixlowpan.Install (lrwpanDevices);

  sixlowpan.InstallSixLowPanNdBorderRouter (devices.Get (0), "2001::");
  sixlowpan.InstallSixLowPanNdNode (devices.Get (1));

//  sixlowpan.Set6LowPanBorderRouter (devices.Get (0));
  sixlowpan.SetAdvertisedPrefix (devices.Get (0), Ipv6Prefix ("2001::", 64));
  //  sixlowpan.AddAdvertisedContext (devices.Get (0), Ipv6Prefix ("2002::", 64));
  sixlowpan.AddAdvertisedContext (devices.Get (0), Ipv6Prefix ("2001::", 64));


  if (printNodesAddresses)
    {
      std::cout<< "\n 6LoWPAN Boarder Router Addresses"<<std::endl;

      Ptr<Ipv6L3Protocol> ipv6l3;

      ipv6l3 = nodes.Get(0)->GetObject<Ipv6L3Protocol> ();
      for (uint32_t index=0; index<ipv6l3->GetNInterfaces (); index++)
        {
          for (uint32_t i=0; i<ipv6l3->GetNAddresses (index); i++)
            {
              std::cout << "interface " << index << ", " << i << " - " << ipv6l3->GetAddress (index, i);
              std::cout << " MAC: " << ipv6l3->GetInterface(index)->GetDevice()->GetAddress() << std::endl;
            }
        }

      std::cout<< "\n 6LoWPAN Node Addresses"<<std::endl;
      ipv6l3 = nodes.Get(1)->GetObject<Ipv6L3Protocol> ();
      for (uint32_t index=0; index<ipv6l3->GetNInterfaces (); index++)
        {
          for (uint32_t i=0; i<ipv6l3->GetNAddresses (index); i++)
            {
              std::cout << "interface " << index << ", " << i << " - " << ipv6l3->GetAddress (index, i);
              std::cout << " MAC: " << ipv6l3->GetInterface(index)->GetDevice()->GetAddress() << std::endl;
            }
        }
    }


  //*********************************ICMPV6 Ping testing*********************************
  if (usePingOn != "")
    {
      uint32_t packetSize = 10;
      uint32_t maxPacketCount = 2;
      Time interPacketInterval = Seconds (1.);
      Ping6Helper ping6;

      ping6.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      ping6.SetAttribute ("Interval", TimeValue (interPacketInterval));
      ping6.SetAttribute ("PacketSize", UintegerValue (packetSize));
      ApplicationContainer apps;

      // 6LBR addresses: "2001::ff:fe00:1" - "fe80::ff:fe00:1"
      // 6LN addresses: "2001::ff:fe00:2" - "fe80::ff:fe00:2"

      if (usePingOn == "6LBR")
        {
          if (useGUA!=false)
            {
              ping6.SetLocal ("2001::ff:fe00:1");
              ping6.SetRemote ("2001::ff:fe00:2");
              apps.Add (ping6.Install (nodes.Get (0)));
            }
          else
            {
              ping6.SetLocal ("fe80::ff:fe00:1");
              ping6.SetRemote ("fe80::ff:fe00:2");
              apps.Add (ping6.Install (nodes.Get (0)));
            }
        }
      else if (usePingOn == "6LN")
            {
              if(useGUA!=false)
                {
                  ping6.SetLocal ("2001::ff:fe00:2");
                  ping6.SetRemote ("2001::ff:fe00:1");
                  apps.Add (ping6.Install (nodes.Get (1)));
                }
              else
                {
                  ping6.SetLocal ("fe80::ff:fe00:2");
                  ping6.SetRemote ("fe80::ff:fe00:1");
                  apps.Add (ping6.Install (nodes.Get (1)));
                }
            }
      else
        {
          std::cout << "PING: invalid option\n";
          exit (0);
        }
      apps.Start (Seconds (35.0));
      apps.Stop (SimulationEnd - Seconds (1));
    }

  //*********************************UDP testing*********************************
  if (useUdpFrom != "")
    {
      uint16_t port = 4000;
      UdpServerHelper server (port);
      ApplicationContainer udpServerApps = server.Install (nodes);
      udpServerApps.Start (Seconds (0.0));
      udpServerApps.Stop (SimulationEnd);

      uint32_t MaxPacketSize = 12;
      Time interPacketInterval = Seconds (0.05);
      uint32_t maxPacketCount = 2;

      // Server IP and port number
      UdpClientHelper client;
      client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      client.SetAttribute ("Interval", TimeValue (interPacketInterval));
      client.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
      ApplicationContainer udpClientApps;

      if (useUdpFrom == "6LBR")
        {
          if(useGUA==false)
            {
              client.SetAttribute ("RemoteAddress", AddressValue (Ipv6Address ("2001::ff:fe00:2")));
              client.SetAttribute ("RemotePort", UintegerValue (port));
              udpClientApps.Add (client.Install (nodes.Get (0)));
            }
          else
            {
              client.SetAttribute ("RemoteAddress", AddressValue (Ipv6Address ("fe80::ff:fe00:2")));
              client.SetAttribute ("RemotePort", UintegerValue (port));
              udpClientApps.Add (client.Install (nodes.Get (0)));
            }
        }
      else if (useUdpFrom == "6LN")
        {
          if(useGUA==false)
            {
              client.SetAttribute ("RemoteAddress", AddressValue (Ipv6Address ("2001::ff:fe00:1")));
              client.SetAttribute ("RemotePort", UintegerValue (port));
              udpClientApps.Add (client.Install (nodes.Get (1)));
            }
          else
            {
              client.SetAttribute ("RemoteAddress", AddressValue (Ipv6Address ("fe80::ff:fe00:1")));
              client.SetAttribute ("RemotePort", UintegerValue (port));
              udpClientApps.Add (client.Install (nodes.Get (1)));
            }
        }
      else
        {
          std::cout << "UDP app: invalid option\n";
          exit (0);
        }
      udpClientApps.Start (Seconds (35.0));
      udpClientApps.Stop (SimulationEnd - Seconds (1));
    }


  if (useUdpFrom != "" && usePingOn != "")
    {
      std::cout<< "****------------------Ping or UDP Applications are not running------------------****"<<std::endl;
    }
  AsciiTraceHelper ascii;
  lrWpanHelper.EnableAsciiAll (ascii.CreateFileStream ("Ping-6LoW-lr-wpan.tr"));
  lrWpanHelper.EnablePcapAll (std::string ("Ping-6LoW-lr-wpan"), true);

  if (printNeighborCache)
    {
      Ptr<OutputStreamWrapper> neighborStream = Create<OutputStreamWrapper> (&std::cout);
      Ipv6RoutingHelper::PrintNeighborCacheAllEvery (Seconds (1), neighborStream);
    }

  Simulator::Stop (SimulationEnd);
  Simulator::Run ();
  Simulator::Destroy ();

};
