/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 Strasbourg University
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
 *
 */

// Network topology

//    n0 (2001:2::/64)
//     |
//     |
//      ----------[R]----------------n2  (2001:2::/64)
//     |
//     |
//   n1 (2002:2::/64)


// //   Router [R] disseminating Prefixes to all nodes (n0, n1 and n2).
// // - n0 and n1 are on the same link but with different addresses.
// // - n0 and n2 belongs to same prefix but not on the same link
// // - n0 ping6 n1.
// //
// // - Tracing of queues and packet receptions to file "On-link-example-radvd-two-prefix.tr"
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"

#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/radvd.h"
#include "ns3/radvd-interface.h"
#include "ns3/radvd-prefix.h"
#include "ns3/ipv6-static-routing-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("OnLinkExampleRadvdTwoPrefix");

/**
 * \class IpAddressHelper
 * \brief Helper to print a node's IP addresses.
 */
//class IpAddressHelper
//{
//public:
//  /**
//   * \brief Print the node's IP addresses.
//   * \param n the node
//   */
//  inline void PrintIpAddresses (Ptr<Node>& n)
//  {
//    Ptr<Ipv6> ipv6 = n->GetObject<Ipv6> ();
//    uint32_t nInterfaces = ipv6->GetNInterfaces();
//
//    std::cout << "Node: " << ipv6->GetObject<Node> ()->GetId ()
//        << " Time: " << Simulator::Now ().GetSeconds () << "s "
//        << "IPv6 addresses" << std::endl;
//    std::cout << "(Interface index, Address index)\t" << "IPv6 Address" << std::endl;
//
//    for (uint32_t i = 0; i < nInterfaces; i++)
//      {
//        for (uint32_t j = 0; j < ipv6->GetNAddresses(i); j++)
//          {
//            std::cout << "(" << int(i) << "," << int(j) << ")\t" << ipv6->GetAddress(i,j) << std::endl;
//          }
//      }
//    std::cout << std::endl;
//  }
//};

int main (int argc, char** argv)
{
  bool verbose = false;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("verbose", "turn on log components", verbose);
  cmd.Parse (argc, argv);

  if (verbose)
    {
      LogComponentEnable ("Ipv6L3Protocol", LOG_LEVEL_ALL);
      LogComponentEnable ("Ipv6RawSocketImpl", LOG_LEVEL_ALL);
      LogComponentEnable ("Icmpv6L4Protocol", LOG_LEVEL_ALL);
      LogComponentEnable ("Ipv6StaticRouting", LOG_LEVEL_ALL);
      LogComponentEnable ("Ipv6Interface", LOG_LEVEL_ALL);
      LogComponentEnable ("RadvdApplication", LOG_LEVEL_ALL);
      LogComponentEnable ("Ping6Application", LOG_LEVEL_ALL);
    }

  NS_LOG_INFO ("Create nodes.");
  Ptr<Node> n0 = CreateObject<Node> ();
  Ptr<Node> r = CreateObject<Node> ();
  Ptr<Node> n1 = CreateObject<Node> ();
  Ptr<Node> n2 = CreateObject<Node> ();

  NodeContainer net1 (r, n0);
  net1.Add(n2);
  NodeContainer net2 (r, n1);
  NodeContainer all (r, n0, n1, n2);

  NS_LOG_INFO ("Channel parameters Setup and applying on all nodes.");

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  NetDeviceContainer devices1 = csma.Install (net1);
  NetDeviceContainer devices2 = csma.Install (net2);

  NS_LOG_INFO ("Installation of IPv6 Stack on all nodes.");

  InternetStackHelper internetv6;
  internetv6.Install (all);

  NS_LOG_INFO ("Setting up the first subnet and Router interface.");

  Ipv6AddressHelper ipv6;
  ipv6.SetBase (Ipv6Address ("2001::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer iic1 = ipv6.Assign(devices1);
  iic1.SetForwarding (0, true);

  NS_LOG_INFO ("Setting up the second subnet and Router interface.");

  ipv6.SetBase (Ipv6Address ("2002::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer iic2 = ipv6.Assign(devices2);
  iic2.SetForwarding (0, true);

  std::cout<< "\n Router Addresses"<<std::endl;

  Ptr<Ipv6L3Protocol> ipv6l3;

  ipv6l3 = r->GetObject<Ipv6L3Protocol> ();
  for (uint32_t index=0; index<ipv6l3->GetNInterfaces (); index++)
    {
      for (uint32_t i=0; i<ipv6l3->GetNAddresses (index); i++)
        {
          std::cout << "interface " << index << ", " << i << " - " << ipv6l3->GetAddress (index, i);
          std::cout << " MAC: " << ipv6l3->GetInterface(index)->GetDevice()->GetAddress() << std::endl;
        }
    }

  std::cout<< "\n n0 Addresses"<<std::endl;
  ipv6l3 = n0->GetObject<Ipv6L3Protocol> ();
  for (uint32_t index=0; index<ipv6l3->GetNInterfaces (); index++)
    {
      for (uint32_t i=0; i<ipv6l3->GetNAddresses (index); i++)
        {
          std::cout << "interface " << index << ", " << i << " - " << ipv6l3->GetAddress (index, i);
          std::cout << " MAC: " << ipv6l3->GetInterface(index)->GetDevice()->GetAddress() << std::endl;
        }
    }

  std::cout<< "\n n2 Addresses"<<std::endl;
  ipv6l3 = n2->GetObject<Ipv6L3Protocol> ();
  for (uint32_t index=0; index<ipv6l3->GetNInterfaces (); index++)
    {
      for (uint32_t i=0; i<ipv6l3->GetNAddresses (index); i++)
        {
          std::cout << "interface " << index << ", " << i << " - " << ipv6l3->GetAddress (index, i);
          std::cout << " MAC: " << ipv6l3->GetInterface(index)->GetDevice()->GetAddress() << std::endl;
        }
    }

  std::cout<< "\n n1 Addresses"<<std::endl;
  ipv6l3 = n1->GetObject<Ipv6L3Protocol> ();
  for (uint32_t index=0; index<ipv6l3->GetNInterfaces (); index++)
    {
      for (uint32_t i=0; i<ipv6l3->GetNAddresses (index); i++)
        {
          std::cout << "interface " << index << ", " << i << " - " << ipv6l3->GetAddress (index, i);
          std::cout << " MAC: " << ipv6l3->GetInterface(index)->GetDevice()->GetAddress() << std::endl;
        }
    }

  NS_LOG_INFO ("Create a Ping6 application to send ICMPv6 echo request from n0 to n1 via R.");

  uint32_t packetSize = 1024;
  uint32_t maxPacketCount = 8;
  Time interPacketInterval = Seconds (1.);
  Ping6Helper ping6;


  ping6.SetLocal ("2001::200:ff:fe00:2");
  ping6.SetRemote ("2002::200:ff:fe00:5");

  ping6.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  ping6.SetAttribute ("Interval", TimeValue (interPacketInterval));
  ping6.SetAttribute ("PacketSize", UintegerValue (packetSize));
  ApplicationContainer apps = ping6.Install (net2.Get (1));
  apps.Start (Seconds (2.0));
  apps.Stop (Seconds (10.0));
//
//  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (&std::cout);
//  for (int var = 0; var < 4; ++var)
//  {
//	  Ipv6RoutingHelper::PrintNeighborCacheAllAt (Seconds (var), routingStream);
//	  Ipv6RoutingHelper::PrintRoutingTableAllAt(Seconds (var), routingStream);
//  }

  AsciiTraceHelper ascii;
  csma.EnableAsciiAll (ascii.CreateFileStream ("on-link-example-radvd-two-prefix.tr"));
  csma.EnablePcapAll (std::string ("on-link-example-radvd-two-prefix"), true);
//  csma.EnablePcap("on-link-example-radvd-two-prefix", devices1.Get(1), true);
//  csma.EnablePcap("on-link-example-radvd-two-prefix", devices2.Get(1), true);

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
