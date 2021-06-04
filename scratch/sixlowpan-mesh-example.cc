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
 * Author: Adnan Rashid <adnan.rashid@unifi.it>
 *         Tommaso Pecorella <tommaso.pecorella@unifi.it>
 *
 *        Sixlowpan Mesh-under network topology
 *
 *                             n0(6LBR)
 *                           +---------+
 *            n1(6LN)        | UDP     |        n2(6LN)
 *          +---------+      +---------+      +---------+
 *          | IPv6    |      | IPv6    |      | IPv6    |
 *          +---------+      +---------+      +---------+
 *   ...    | 6LoWPAN |      | 6LoWPAN |      | 6LoWPAN |    ...
 *          +---------+      +---------+      +---------+
 *          | lr-wpan |      | lr-wpan |      | lr-wpan |
 *          +---------+      +---------+      +---------+
 *              ||               ||               ||
 *                ===============  ===============
 *
 * ./waf --run "scratch/sixlowpan-mesh-example.cc --Mesh --Ping=6LN --LLA --StopTime=2000 --Interval=100"
 *We are in backoff mode.
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
#include "ns3/energy-module.h"
#include "ns3/wifi-module.h"
#include "ns3/gnuplot.h"
#include "ns3/applications-module.h"

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
	bool useLLA = false;
	bool useGUA = false;
	bool printNeighborCache = false;
	std::string useUdpFrom = "";
	std::string usePingOn = "";
	double stopTime = 100;
	Time interval = Seconds (1);

	CommandLine cmd;
	cmd.AddValue ("Mesh", "Use mesh-under in the network", useMeshUnder);
	cmd.AddValue ("Udp", "Send one UDP packet from (6LBR, 6LN, nothing)", useUdpFrom);
	cmd.AddValue ("Ping", "Install Ping app on (6LBR, 6LN, nothing)", usePingOn);
	cmd.AddValue ("NeighborCache", "Print the neighbor cache entries", printNeighborCache);
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

  NodeContainer lo_nodes;
  lo_nodes.Create (40);

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (30),
                                 "DeltaY", DoubleValue (30),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (lo_nodes);

  // this means that the registration is valid for 2 days (and the re-registration is performed after 1 day).
  Config::SetDefault ("ns3::SixLowPanNdProtocol::RegistrationLifeTime", UintegerValue (2880));
  // Config::SetDefault ("ns3::Icmpv6L4Protocol::SolicitationJitter", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));


  LrWpanHelper lrWpanHelper;
  // Add and install the LrWpanNetDevice for each node
  NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(lo_nodes);

  // Fake PAN association and short address assignment.
  lrWpanHelper.AssociateToPan (lrwpanDevices, 0);

  InternetStackHelper internetv6;
  internetv6.Install (lo_nodes);

  // installing 6LoWPAN stack on nodes
  SixLowPanHelper sixlowpan;
  NetDeviceContainer devices = sixlowpan.Install (lrwpanDevices);


  for (int var = 0; var <40; var++)
    {
      if (var == 20)
        {
          sixlowpan.InstallSixLowPanNdBorderRouter (devices.Get (var), "2001::");
          sixlowpan.SetAdvertisedPrefix (devices.Get (var), Ipv6Prefix ("2001::", 64));
          sixlowpan.AddAdvertisedContext (devices.Get (var), Ipv6Prefix ("2002::", 64));
        }
      else
        sixlowpan.InstallSixLowPanNdNode (devices.Get (var));
    }

 //*********************************ICMPV6 Ping testing*********************************
  if (usePingOn != "")
    {
      uint32_t packetSize = 10;
      uint32_t maxPacketCount = 100;
      Time interPacketInterval = Seconds (1.);
      Ping6Helper ping6;

      ping6.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      ping6.SetAttribute ("Interval", TimeValue (interPacketInterval));
      ping6.SetAttribute ("PacketSize", UintegerValue (packetSize));
      ApplicationContainer apps;

      // 6LBR addresses: "2001::ff:fe00:5" - "fe80::ff:fe00:5"
      // 6LN addresses: "2001::ff:fe00:1" - "fe80::ff:fe00:1"

      if (usePingOn == "6LBR")
        {
          if (useGUA!=false)
            {
              ping6.SetLocal ("2001::ff:fe00:5");
              ping6.SetRemote ("2001::ff:fe00:1");
              apps.Add (ping6.Install (lo_nodes.Get (4)));
            }
          else
            {
              ping6.SetLocal ("fe80::ff:fe00:5");
              ping6.SetRemote ("fe80::ff:fe00:1");
              apps.Add (ping6.Install (lo_nodes.Get (4)));
            }
        }
      else if (usePingOn == "6LN")
            {
              if(useGUA!=false)
                {
                  ping6.SetLocal ("2001::ff:fe00:5");
                  ping6.SetRemote ("2001::ff:fe00:1");
                  apps.Add (ping6.Install (lo_nodes.Get (4)));
                }
              else
                {
                  ping6.SetLocal ("fe80::ff:fe00:5");
                  ping6.SetRemote ("fe80::ff:fe00:1");
                  apps.Add (ping6.Install (lo_nodes.Get (4)));
                }
            }
      else
        {
          std::cout << "PING: invalid option\n";
          exit (0);
        }
      apps.Start (Seconds(1.5));
      apps.Stop (Seconds (stopTime-1));
    }

  //*********************************UDP testing*********************************
  if (useUdpFrom != "")
    {
      uint16_t port = 4000;
      UdpServerHelper server (port);
      ApplicationContainer udpServerApps = server.Install (lo_nodes);
      udpServerApps.Start (Seconds (0.0));
      udpServerApps.Stop (Seconds(stopTime - 1));

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
              udpClientApps.Add (client.Install (lo_nodes.Get (0)));
            }
          else
            {
              client.SetAttribute ("RemoteAddress", AddressValue (Ipv6Address ("fe80::ff:fe00:2")));
              client.SetAttribute ("RemotePort", UintegerValue (port));
              udpClientApps.Add (client.Install (lo_nodes.Get (0)));
            }
        }
      else if (useUdpFrom == "6LN")
        {
          if(useGUA==false)
            {
              client.SetAttribute ("RemoteAddress", AddressValue (Ipv6Address ("2001::ff:fe00:1")));
              client.SetAttribute ("RemotePort", UintegerValue (port));
              udpClientApps.Add (client.Install (lo_nodes.Get (1)));
            }
          else
            {
              client.SetAttribute ("RemoteAddress", AddressValue (Ipv6Address ("fe80::ff:fe00:1")));
              client.SetAttribute ("RemotePort", UintegerValue (port));
              udpClientApps.Add (client.Install (lo_nodes.Get (1)));
            }
        }
      else
        {
          std::cout << "UDP app: invalid option\n";
          exit (0);
        }
      udpClientApps.Start (Seconds (5.0));
      udpClientApps.Stop (Seconds(stopTime - 1));
    }
  if (useUdpFrom != "" && usePingOn != "")
    {
      std::cout<< "****------------------Ping or UDP Applications are not running------------------****"<<std::endl;
    }

//  AsciiTraceHelper ascii;
//  lrWpanHelper.EnableAsciiAll (ascii.CreateFileStream ("sixlowpan-mesh-example.tr"));
//  lrWpanHelper.EnablePcapAll (std::string ("sixlowpan-mesh-example"), true);
//
//  Ptr<OutputStreamWrapper> neighborStream = Create<OutputStreamWrapper> (&std::cout);
//  for (int var = 0; var < stopTime; ++var)
//  {
//	  Ipv6RoutingHelper::PrintNeighborCacheAllAt (Seconds (var), neighborStream);
////	  Ipv6RoutingHelper::PrintRoutingTableAllAt(Seconds (var), neighborStream);
//  }

  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::LrWpanNetDevice/Phy/PhyTxBegin",
                    MakeCallback (&PhyCallback));
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::SixLowPanNetDevice/TxPre",
                    MakeCallback (&SixLowCallback));

  Simulator::Schedule (interval, &PrintResults, interval);

  Simulator::Stop (Seconds (stopTime));
  Simulator::Run ();

//  Ptr<Packet> foo = Create<Packet> ();
//  PhyCallback ("foobar", foo);
//  std::cout << "End of simulation at " << Now ().As (Time::S) << std::endl;

  Simulator::Destroy ();

//  std::cout << "Udp: " << udpCount << std::endl;
//  std::cout << "Other L4: " << otherL4Count << std::endl;
//  for (auto const& x : icmpTypeCount)
//  {
//      std::cout << +x.first
//                << ':'
//                << x.second
//                << std::endl;
//  }

};

