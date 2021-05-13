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
 *               ================   ===============
 *
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
#include "ns3/energy-module.h"
#include "ns3/wifi-module.h"
#include "ns3/gnuplot.h"

using namespace ns3;
using namespace std;


uint32_t pktCount = 0;
uint64_t pktTotalSize = 0;
uint32_t cicle = 10;
uint32_t nowMax = 1;

void
PhyCallback (std::string path, Ptr<const Packet> packet)
{

  uint32_t nowMaxLocal = std::floor (Now().GetSeconds () / 10) + 1;

  if ( nowMaxLocal != nowMax)
    {
      if (pktCount)
//        std::cout << "Time " << "\t" << "Packet Count " << "\t" << " Total Size "<< std::endl;
      std::cout << (nowMax)*10 << "\t" << pktCount << "\t" << pktTotalSize << std::endl;
      nowMax = nowMaxLocal;

      pktCount = 0;
      pktTotalSize = 0;
    }
  pktCount ++;
  pktTotalSize += packet->GetSize ();

//  if (Now().GetSeconds () > 10)
//    std::cout << Now ().As (Time::S) << " Tx packet of size " << packet->GetSize () << " - " << *packet << std::endl;

//  std::cout << Now ().As (Time::S) << " - " << packet->GetSize () << std::endl;

//  LrWpanMacHeader lrWpanHdr;
//  Ptr<Packet> pktCpy = packet->Copy ();
//  pktCpy->RemoveHeader (lrWpanHdr);
//
//  if (lrWpanHdr.IsAcknowledgment ())
//    std::cout << Now ().As (Time::S)<< " Tx ACK of size " << packet->GetSize () << std::endl;
//  else if (lrWpanHdr.IsData ())
//    std::cout << Now ().As (Time::S) << " Tx packet of size " << packet->GetSize () << " - " << *pktCpy << std::endl;
//  else
//    std::cout << Now ().As (Time::S) << " Tx OTHER of size " << packet->GetSize () << std::endl;
}

// Here you see only the packets that IP did send to 6LoWPAN.
// It can NOT tell you shit about the real packet size after compression, if there has been a fragmentation, etc.
// for that you need the function above.

void
SixLowCallback (std::string path, Ptr<const Packet> packet, Ptr<SixLowPanNetDevice> netDev, uint32_t index)
{

//  std::cout << "adnan" <<path << std::endl;
//  std::cout << Now ().As (Time::S) << " Tx something of size (Packets that IP did send to 6LoWPAN) " << packet->GetSize () << " - " << *packet << std::endl;
}

int main (int argc, char** argv)
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  Packet::EnablePrinting ();

#if 0
  LogComponentEnable ("Ping6Application", LOG_LEVEL_ALL);
  LogComponentEnable ("LrWpanMac",LOG_LEVEL_ALL);
  LogComponentEnable ("LrWpanPhy",LOG_LEVEL_ALL);
  LogComponentEnable ("LrWpanNetDevice", LOG_LEVEL_ALL);
  LogComponentEnable ("SixLowPanNetDevice", LOG_LEVEL_ALL);
#endif

  NodeContainer lo_nodes;
  lo_nodes.Create (9);

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
  mobility.Install (lo_nodes);

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


  for (int var = 0; var < 9; var++)
    {
      if (var == 4)
        {
          sixlowpan.InstallSixLowPanNdBorderRouter (devices.Get (var), "2001::");
          sixlowpan.SetAdvertisedPrefix (devices.Get (var), Ipv6Prefix ("2001::", 64));
          sixlowpan.AddAdvertisedContext (devices.Get (var), Ipv6Prefix ("2002::", 64));
        }
      else
        sixlowpan.InstallSixLowPanNdNode (devices.Get (var));
    }


//  uint32_t packetSize = 10;
//  uint32_t maxPacketCount = 500;
//  Time interPacketInterval = Seconds (1.);
//  Ping6Helper ping6;
//
//  //************Communication from 6LN(n1)---to---6LN(n2)************
//
//  //GUA-to-GUA---------------------------------working
//    ping6.SetLocal ("2001::ff:fe00:2");
//    ping6.SetRemote ("2001::ff:fe00:3");
//
//  //LLA-to-LLA---------------------------------Working
////    ping6.SetLocal ("fe80::ff:fe00:2");
////    ping6.SetRemote ("fe80::ff:fe00:3");
//
//  //GUA-to-LLA---------------------------------working
////    ping6.SetLocal ("2001::ff:fe00:2");
////    ping6.SetRemote ("fe80::ff:fe00:3");
//
//  //LLA-to-GUA---------------------------------Working
////    ping6.SetLocal ("fe80::ff:fe00:2");
////    ping6.SetRemote ("2001::ff:fe00:3");
//
//  //************Communication from 6LN(n2)---to---6LN(n1)************
//
//  // GUA-to-GUA---------------------------------Working
////    ping6.SetLocal ("2001::ff:fe00:3");
////    ping6.SetRemote ("2001::ff:fe00:2");
//
//  //LLA-to-LLA---------------------------------Working
////    ping6.SetLocal ("fe80::ff:fe00:3");
////    ping6.SetRemote ("fe80::ff:fe00:2");
//
//  //GUA-to-LLA---------------------------------Working
////    ping6.SetLocal ("2001::ff:fe00:3");
////    ping6.SetRemote ("fe80::ff:fe00:2");
//
//  //LLA-to-GUA---------------------------------Working
////    ping6.SetLocal ("fe80::ff:fe00:3");
////    ping6.SetRemote ("2001::ff:fe00:2");
//
//
//  ping6.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
//  ping6.SetAttribute ("Interval", TimeValue (interPacketInterval));
//  ping6.SetAttribute ("PacketSize", UintegerValue (packetSize));
//  ApplicationContainer apps = ping6.Install (lo_nodes.Get (1)); // always the SetLocal node index
//
//  apps.Start (Seconds (5.0));
//  apps.Stop (Seconds (100.0));

  AsciiTraceHelper ascii;
  lrWpanHelper.EnableAsciiAll (ascii.CreateFileStream ("sixlowpan-mesh-example.tr"));
  lrWpanHelper.EnablePcapAll (std::string ("sixlowpan-mesh-example"), true);

//  Ptr<OutputStreamWrapper> neighborStream = Create<OutputStreamWrapper> (&std::cout);
//  for (int var = 0; var < 4; ++var)
//  {
//	  Ipv6RoutingHelper::PrintNeighborCacheAllAt (Seconds (var), neighborStream);
//    Ipv6RoutingHelper::PrintRoutingTableAllAt(Seconds (var), neighborStream);
//  }

  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::LrWpanNetDevice/Phy/PhyTxBegin",
                    MakeCallback (&PhyCallback));
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::SixLowPanNetDevice/TxPre",
                    MakeCallback (&SixLowCallback));

  Simulator::Stop (Seconds (50000));
  Simulator::Run ();

  Ptr<Packet> foo = Create<Packet> ();
  PhyCallback ("foobar", foo);
  std::cout << "End of simulation at " << Now ().As (Time::S) << std::endl;

  Simulator::Destroy ();

};

