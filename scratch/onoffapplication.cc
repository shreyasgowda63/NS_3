
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
#include <map>
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
uint32_t pktCount = 0;
uint64_t pktTotalSize = 0;
uint32_t ackCount = 0;
uint64_t ackTotalSize = 0;
uint32_t unkCount = 0;
uint64_t unkTotalSize = 0;

std::map<uint8_t, uint32_t> icmpTypeCount;
uint32_t udpCount = 0;
uint32_t otherL4Count = 0;

uint32_t unicastcount = 0;
uint32_t multicastcount = 0;

void
PrintResults (Time interval)
{
	std::cout << Now().GetSeconds ()<< "\t";
	std::cout << pktCount << "\t" << pktTotalSize << "\t";
	std::cout << ackCount << "\t" << ackTotalSize << "\t";
	std::cout << unkCount << "\t" << unkTotalSize << "\t";
	std::cout << unicastcount << "\t" <<multicastcount << std::endl;
//	std::cout << udpCount << "\t" <<otherL4Count << std::endl;

	pktCount = 0;
	pktTotalSize = 0;
	ackCount = 0;
	ackTotalSize = 0;
	unkCount = 0;
	unkTotalSize = 0;
	Simulator::Schedule (interval, &PrintResults, interval);
}

void
PhyCallback (std::string path, Ptr<const Packet> packet)
{
  LrWpanMacHeader lrWpanHdr;
  Ptr<Packet> pktCpy = packet->Copy ();
  pktCpy->RemoveHeader (lrWpanHdr);

  if (lrWpanHdr.IsAcknowledgment ())
    {
      ackCount ++;
      ackTotalSize += packet->GetSize ();
    }
  else if (lrWpanHdr.IsData ())
    {
      pktCount ++;
      pktTotalSize += packet->GetSize ();
    }
  else
    {
      unkCount ++;
      unkTotalSize += packet->GetSize ();
      std::cout << *packet << std::endl;
    }
}

// Here you see only the packets that IP did send to 6LoWPAN.
// It can NOT tell you shit about the real packet size after compression, if there has been a fragmentation, etc.
// for that you need the function above.

void
SixLowCallback (std::string path, Ptr<const Packet> packet, Ptr<SixLowPanNetDevice> netDev, uint32_t index)
{
	Ipv6Header ipv6Hdr;
	Ptr<Packet> pktCpy = packet->Copy ();
	pktCpy->RemoveHeader (ipv6Hdr);
	if (ipv6Hdr.GetNextHeader () == UdpL4Protocol::PROT_NUMBER)
	{
		udpCount ++;
	}
	else if (ipv6Hdr.GetNextHeader () == Icmpv6L4Protocol::PROT_NUMBER)
	{
		Icmpv6Header icmpHdr;
		pktCpy->RemoveHeader (icmpHdr);
		icmpTypeCount[icmpHdr.GetType ()] ++;
	}
	else
	{
		otherL4Count ++;
	}

	if (ipv6Hdr.GetDestinationAddress()==Ipv6Address::GetAllRoutersMulticast())
	{
		multicastcount++;
	}
	else
	{
		unicastcount++;
	}
//std::cout << Now ().As (Time::S) << " Tx something of size (Packets that IP did send to 6LoWPAN) " << packet->GetSize () << " - " << *packet << std::endl;
}

int main (int argc, char** argv)
{
  bool useMeshUnder = false;
  bool printNodesAddresses = false;
  bool printNeighborCache = false;
  bool useLLA = false;
  bool useGUA = false;
  std::string useUdpFrom = "";
  std::string usePingOn = "";
  double stopTime;
  Time interval = Seconds (1);

  CommandLine cmd;
  cmd.AddValue ("Mesh", "Use mesh-under in the network", useMeshUnder);
  cmd.AddValue ("Addresses", "Print the addresses of the nodes", printNodesAddresses);
  cmd.AddValue ("NeighborCache", "Print the neighbor cache entries", printNeighborCache);
  cmd.AddValue ("Udp", "Send one UDP packet from (6LBR, 6LN, nothing)", useUdpFrom);
  cmd.AddValue ("Ping", "Install Ping app on (6LBR, 6LN, nothing)", usePingOn);
  cmd.AddValue ("LLA", "Use link-local addresses for the communication", useLLA);
  cmd.AddValue ("GUA", "Use global addresses for the communication", useGUA);
  cmd.AddValue ("StopTime", "Simulation stop time (seconds)", stopTime);
  cmd.AddValue ("Interval", "Sampling interval", interval);
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

  NodeContainer nodes;
  nodes.Create(2);

  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (60),
                                 "DeltaY", DoubleValue (60),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  // this means that the registration is valid for 2 days (and the re-registration is performed after 1 day).
  Config::SetDefault ("ns3::SixLowPanNdProtocol::RegistrationLifeTime", UintegerValue (2880));
  // Config::SetDefault ("ns3::Icmpv6L4Protocol::SolicitationJitter", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));


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


  //*********************************OnOff Application testing*********************************
//  ApplicationContainer cbrApps;
//  uint16_t cbrPort = 12345;
//  uint32_t packetSize = 10;
//  uint32_t maxPacketCount = 10;
//  Time interPacketInterval = Seconds (1.);
//  OnOffHelper onOffHelper ("ns3::UdpSocketFactory", Inet6SocketAddress (Ipv6Address ("fe80::ff:fe00:1"), cbrPort));
//  onOffHelper.SetAttribute ("PacketSize", UintegerValue (1400));
//  onOffHelper.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
//  onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=10]"));
//
//  // flow 1:  node 1 -> node 0
//  onOffHelper.SetAttribute ("DataRate", StringValue ("3000bps"));
//  onOffHelper.SetAttribute ("StartTime", TimeValue (Seconds (1.000000)));
//  cbrApps.Add (onOffHelper.Install (nodes.Get (1)));
//*******************************************************************************************
  // Create a packet sink on the star "hub" to receive these packets
  uint16_t port = 50000;
  Address sinkLocalAddress (Inet6SocketAddress (Ipv6Address ("fe80::ff:fe00:1"), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get(0));
  sinkApp.Start (Seconds (1.0));
  sinkApp.Stop (Seconds (10.0));

  // Create the OnOff applications to send TCP to the server
  OnOffHelper clientHelper ("ns3::TcpSocketFactory", Address ());
  clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

  ApplicationContainer clientApps;
  AddressValue remoteAddress(Inet6SocketAddress(sinkLocalAddress, port));
  clientHelper.SetAttribute ("Remote", remoteAddress);
  clientApps.Add (clientHelper.Install (nodes.Get (1)));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));


  //*********************************ICMPV6 Ping testing*********************************
//  if (usePingOn != "")
//    {
//      uint32_t packetSize = 10;
//      uint32_t maxPacketCount = 2;
//      Time interPacketInterval = Seconds (1.);
//      Ping6Helper ping6;
//
//      ping6.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
//      ping6.SetAttribute ("Interval", TimeValue (interPacketInterval));
//      ping6.SetAttribute ("PacketSize", UintegerValue (packetSize));
//      ApplicationContainer apps;
//
//      // 6LBR addresses: "2001::ff:fe00:1" - "fe80::ff:fe00:1"
//      // 6LN addresses: "2001::ff:fe00:2" - "fe80::ff:fe00:2"
//
//      if (usePingOn == "6LBR")
//        {
//          if (useGUA!=false)
//            {
//              ping6.SetLocal ("2001::ff:fe00:1");
//              ping6.SetRemote ("2001::ff:fe00:2");
//              apps.Add (ping6.Install (nodes.Get (0)));
//            }
//          else
//            {
//              ping6.SetLocal ("fe80::ff:fe00:1");
//              ping6.SetRemote ("fe80::ff:fe00:2");
//              apps.Add (ping6.Install (nodes.Get (0)));
//            }
//        }
//      else if (usePingOn == "6LN")
//            {
//              if(useGUA!=false)
//                {
//                  ping6.SetLocal ("2001::ff:fe00:2");
//                  ping6.SetRemote ("2001::ff:fe00:1");
//                  apps.Add (ping6.Install (nodes.Get (1)));
//                }
//              else
//                {
//                  ping6.SetLocal ("fe80::ff:fe00:2");
//                  ping6.SetRemote ("fe80::ff:fe00:1");
//                  apps.Add (ping6.Install (nodes.Get (1)));
//                }
//            }
//      else
//        {
//          std::cout << "PING: invalid option\n";
//          exit (0);
//        }
//      apps.Start (Seconds (0.5));
//      apps.Stop (Seconds(stopTime - 1));
//    }
//
//  //*********************************UDP testing*********************************
//  if (useUdpFrom != "")
//    {
//      uint16_t port = 4000;
//      UdpServerHelper server (port);
//      ApplicationContainer udpServerApps = server.Install (nodes);
//      udpServerApps.Start (Seconds (0.0));
//      udpServerApps.Stop (Seconds(stopTime-1));
//
//      uint32_t MaxPacketSize = 12;
//      Time interPacketInterval = Seconds (0.05);
//      uint32_t maxPacketCount = 2;
//
//      // Server IP and port number
//      UdpClientHelper client;
//      client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
//      client.SetAttribute ("Interval", TimeValue (interPacketInterval));
//      client.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
//      ApplicationContainer udpClientApps;
//
//      if (useUdpFrom == "6LBR")
//        {
//          if(useGUA==false)
//            {
//              client.SetAttribute ("RemoteAddress", AddressValue (Ipv6Address ("2001::ff:fe00:2")));
//              client.SetAttribute ("RemotePort", UintegerValue (port));
//              udpClientApps.Add (client.Install (nodes.Get (0)));
//            }
//          else
//            {
//              client.SetAttribute ("RemoteAddress", AddressValue (Ipv6Address ("fe80::ff:fe00:2")));
//              client.SetAttribute ("RemotePort", UintegerValue (port));
//              udpClientApps.Add (client.Install (nodes.Get (0)));
//            }
//        }
//      else if (useUdpFrom == "6LN")
//        {
//          if(useGUA==false)
//            {
//              client.SetAttribute ("RemoteAddress", AddressValue (Ipv6Address ("2001::ff:fe00:1")));
//              client.SetAttribute ("RemotePort", UintegerValue (port));
//              udpClientApps.Add (client.Install (nodes.Get (1)));
//            }
//          else
//            {
//              client.SetAttribute ("RemoteAddress", AddressValue (Ipv6Address ("fe80::ff:fe00:1")));
//              client.SetAttribute ("RemotePort", UintegerValue (port));
//              udpClientApps.Add (client.Install (nodes.Get (1)));
//            }
//        }
//      else
//        {
//          std::cout << "UDP app: invalid option\n";
//          exit (0);
//        }
//      udpClientApps.Start (Seconds (35.0));
//      udpClientApps.Stop (Seconds (stopTime-1));
//    }
//
//
//  if (useUdpFrom != "" && usePingOn != "")
//    {
//      std::cout<< "****------------------Ping or UDP Applications are not running------------------****"<<std::endl;
//    }
  AsciiTraceHelper ascii;
  lrWpanHelper.EnableAsciiAll (ascii.CreateFileStream ("onoffapplication.tr"));
  lrWpanHelper.EnablePcapAll (std::string ("onoffapplication"), true);

  if (printNeighborCache)
    {
      Ptr<OutputStreamWrapper> neighborStream = Create<OutputStreamWrapper> (&std::cout);
      Ipv6RoutingHelper::PrintNeighborCacheAllEvery (Seconds (1), neighborStream);
    }

  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::LrWpanNetDevice/Phy/PhyTxBegin",
                    MakeCallback (&PhyCallback));
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::SixLowPanNetDevice/TxPre",
                    MakeCallback (&SixLowCallback));

  Simulator::Schedule (interval, &PrintResults, interval);

  Simulator::Stop (Seconds (stopTime));
  Simulator::Run ();
  Simulator::Destroy ();

};
