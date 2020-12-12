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
 */


#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/propagation-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"

using namespace ns3;


void SendOnePacket (NetDeviceContainer devices, Ipv6InterfaceContainer deviceInterfaces, Ipv6Address from, Ipv6Address to)
{
  Ptr<Packet> pkt = Create<Packet> (10);
  Ipv6Header ipHdr;
  ipHdr.SetSourceAddress (from);
  ipHdr.SetDestinationAddress (to);
  ipHdr.SetHopLimit (64);
  ipHdr.SetPayloadLength (10);
  ipHdr.SetNextHeader (0xff);
  pkt->AddHeader (ipHdr);

  devices.Get (0)->Send (pkt, Mac48Address::ConvertFrom (devices.Get (1)->GetAddress ()), 0);
}

int main (int argc, char** argv)
{
  bool verbose = false;
  bool disablePcap = false;
  bool disableAsciiTrace = false;
  bool enableLSixlowLogLevelInfo = false;

//  Packet::EnablePrinting ();

  CommandLine cmd (__FILE__);
  cmd.AddValue ("verbose", "turn on log components", verbose);
  cmd.AddValue ("disable-pcap", "disable PCAP generation", disablePcap);
  cmd.AddValue ("disable-asciitrace", "disable ascii trace generation", disableAsciiTrace);
  cmd.AddValue ("enable-sixlowpan-loginfo", "enable sixlowpan LOG_LEVEL_INFO (used for tests)", enableLSixlowLogLevelInfo);
  cmd.Parse (argc, argv);
  
  if (verbose)
    {
      LogComponentEnable ("LrWpanMac", LOG_LEVEL_ALL);
      LogComponentEnable ("LrWpanPhy", LOG_LEVEL_ALL);
      LogComponentEnable ("LrWpanNetDevice", LOG_LEVEL_ALL);
      LogComponentEnable ("SixLowPanNetDevice", LOG_LEVEL_ALL);
    }
  if (enableLSixlowLogLevelInfo)
    {
      LogComponentEnable ("SixLowPanNetDevice", LOG_LEVEL_INFO);
    }

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
  NetDeviceContainer lrwpanDevices = lrWpanHelper.Install (nodes);

  // Fake PAN association and short address assignment.
  // This is needed because the lr-wpan module does not provide (yet)
  // a full PAN association procedure.
  lrWpanHelper.AssociateToPan (lrwpanDevices, 1);

  InternetStackHelper internetv6;
  internetv6.Install (nodes);

  SixLowPanHelper sixlowpan;
  NetDeviceContainer devices = sixlowpan.Install (lrwpanDevices); 
 
  Ipv6AddressHelper ipv6;
  ipv6.SetBase (Ipv6Address ("2001:2::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer deviceInterfaces;
  deviceInterfaces = ipv6.Assign (devices);

  if (enableLSixlowLogLevelInfo)
    {
      std::cout << "Device 0: address 0 " << Mac48Address::ConvertFrom (devices.Get (0)->GetAddress ()) << " -> "  << deviceInterfaces.GetAddress (0, 0) << std::endl;
      std::cout << "Device 0: address 1 " << Mac48Address::ConvertFrom (devices.Get (0)->GetAddress ()) << " -> "  << deviceInterfaces.GetAddress (0, 1) << std::endl;
      std::cout << "Device 1: address 0 " << Mac48Address::ConvertFrom (devices.Get (1)->GetAddress ()) << " -> "  << deviceInterfaces.GetAddress (1, 0) << std::endl;
      std::cout << "Device 1: address 1 " << Mac48Address::ConvertFrom (devices.Get (1)->GetAddress ()) << " -> "  << deviceInterfaces.GetAddress (1, 1) << std::endl;
    }
   


  // This is a hack to prevent Router Solicitations and Duplicate Address Detection being sent.
  for (auto i = nodes.Begin (); i != nodes.End (); i++)
    {
      Ptr<Node> node = *i;
      Ptr<Ipv6L3Protocol> ipv6L3 = (*i)->GetObject<Ipv6L3Protocol> ();
      if (ipv6L3)
        {
          ipv6L3->SetAttribute ("IpForward", BooleanValue (true));
          ipv6L3->SetAttribute ("SendIcmpv6Redirect", BooleanValue (false));
        }
      Ptr<Icmpv6L4Protocol> icmpv6 = (*i)->GetObject<Icmpv6L4Protocol> ();
      if (icmpv6)
        {
          icmpv6->SetAttribute ("DAD", BooleanValue (false));
        }
    }

  if (!disableAsciiTrace)
    {
      AsciiTraceHelper ascii;
      lrWpanHelper.EnableAsciiAll (ascii.CreateFileStream ("6LoW-lr-wpan-IPHC-stateful.tr"));
    }
  if (!disablePcap)
    {
      lrWpanHelper.EnablePcapAll (std::string ("6LoW-lr-wpan-IPHC-stateful"), true);
    }

  if (enableLSixlowLogLevelInfo)
    {
      Ipv6Prefix contextprefix = Ipv6Prefix ("2001:2::", 64);
      std::cout << Ipv6Address::GetOnes ().CombinePrefix (contextprefix) << contextprefix << std::endl;
    }

  sixlowpan.AddContext (devices, 0, Ipv6Prefix ("2001:2::", 64), Time (Minutes (30)));
  sixlowpan.AddContext (devices, 1, Ipv6Prefix ("2001:1::", 64), Time (Minutes (30)));

  // This is another hack - pre-set all the NDISC cache entries
  internetv6.AddPermanentNdiscEntry (devices.Get (0), deviceInterfaces.GetAddress (1, 0), devices.Get (1)->GetAddress ());
  internetv6.AddPermanentNdiscEntry (devices.Get (0), deviceInterfaces.GetAddress (1, 1), devices.Get (1)->GetAddress ());
  internetv6.AddPermanentNdiscEntry (devices.Get (1), deviceInterfaces.GetAddress (0, 0), devices.Get (0)->GetAddress ());
  internetv6.AddPermanentNdiscEntry (devices.Get (1), deviceInterfaces.GetAddress (0, 1), devices.Get (0)->GetAddress ());


  Simulator::Schedule (Seconds (1), SendOnePacket, devices, deviceInterfaces,
                       Ipv6Address::GetAny (),
                       deviceInterfaces.GetAddress (1, 1));

  Simulator::Schedule (Seconds (2), SendOnePacket, devices, deviceInterfaces,
                       deviceInterfaces.GetAddress (0, 1),
                       Ipv6Address ("2001:1::0000:00ff:fe00:cafe"));

  Simulator::Schedule (Seconds (3), SendOnePacket, devices, deviceInterfaces,
                       deviceInterfaces.GetAddress (0, 1),
                       Ipv6Address ("2001:1::f00d:f00d:cafe:cafe"));

  // 64-bit inline source address test is not possible because LrWpanNetDevice can not send packets using the 64-bit address.

  Simulator::Stop (Seconds (10));
  
  Simulator::Run ();
  Simulator::Destroy ();

}

