/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/propagation-environment.h"
#include "ns3/applications-module.h"
#include "ns3/multicast-flow-monitor-helper.h"

using namespace ns3;

// Remove unnecessary ARP traffic
void
PopulateArpCache ()
{
  Ptr<ArpCache> arp = CreateObject<ArpCache> ();
  arp->SetAliveTimeout (Seconds (3600 * 24 * 365));
  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i)
    {
      Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
      NS_ASSERT (ip != 0);
      ObjectVectorValue interfaces;
      ip->GetAttribute ("InterfaceList", interfaces);

      for (ObjectVectorValue::Iterator j = interfaces.Begin (); j !=
           interfaces.End (); j++)
        {
          Ptr<Ipv4Interface> ipIface = (*j).second->GetObject<Ipv4Interface> ();
          NS_ASSERT (ipIface != 0);
          Ptr<NetDevice> device = ipIface->GetDevice ();
          NS_ASSERT (device != 0);
          Mac48Address addr = Mac48Address::ConvertFrom (device->GetAddress ());
          for (uint32_t k = 0; k < ipIface->GetNAddresses (); k++)
            {
              Ipv4Address ipAddr = ipIface->GetAddress (k).GetLocal ();
              if (ipAddr == Ipv4Address::GetLoopback ())
                {
                  continue;
                }
              ArpCache::Entry * entry = arp->Add (ipAddr);
              Ptr<Packet> dummy = Create<Packet>();
              Ipv4Header ipHeader;
              entry->MarkWaitReply (std::pair<Ptr<Packet>, Ipv4Header> (dummy, ipHeader));
              entry->MarkAlive (addr);
              entry->ClearPendingPacket ();
              entry->MarkPermanent ();
            }
        }
    }
  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i)
    {
      Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
      NS_ASSERT (ip != 0);
      ObjectVectorValue interfaces;
      ip->GetAttribute ("InterfaceList", interfaces);
      for (ObjectVectorValue::Iterator j = interfaces.Begin (); j !=
           interfaces.End (); j++)
        {
          Ptr<Ipv4Interface> ipIface = (*j).second->GetObject<Ipv4Interface> ();
          ipIface->SetAttribute ("ArpCache", PointerValue (arp));
        }
    }
}


int
main (int argc, char *argv[])
{

  CommandLine cmd;
  uint32_t numNodesRows = 3;
  cmd.AddValue ("numNodesRows", "Number of nodes", numNodesRows);

  uint32_t numRows = 3;
  cmd.AddValue ("numRows", "Number of rows in grid", numRows);

  uint32_t distance = 20;
  cmd.AddValue ("distance", "Distance between nodes in a row and column", distance);

  uint32_t endTime = 60.0;
  cmd.AddValue ("endTime", "Time to end simulation", endTime);

  cmd.Parse (argc,argv);

  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (125.0));

  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("1kb/s"));

  Config::SetDefault ("ns3::Ipv4L3Protocol::EnableDuplicatePacketDetection", BooleanValue (true));
  Config::SetDefault ("ns3::Ipv4L3Protocol::DuplicateExpire", TimeValue (Seconds (endTime)));

  std::cout << "Running Ipv4 Duplicate detection test" << std::endl;

  uint32_t start_time = 1.0;

  NodeContainer wifi_nodes;
  uint32_t numNodes = numNodesRows * numRows;
  wifi_nodes.Create (numNodes);

  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("DsssRate1Mbps"),
                                "ControlMode", StringValue ("DsssRate1Mbps"),
                                "RtsCtsThreshold", UintegerValue (500), // Not really necessary since multicast
                                "NonUnicastMode", StringValue ("DsssRate1Mbps"),
                                "DefaultTxPowerLevel", UintegerValue (1));

  YansWifiPhyHelper phy;
  phy.SetErrorRateModel ("ns3::TableBasedErrorRateModel");
  phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
  phy.DisablePreambleDetectionModel (); // Prevents multicast from working as it should

  Ptr<YansWifiChannel> channel = CreateObject<YansWifiChannel> ();
  Ptr<FriisPropagationLossModel> loss_model = CreateObject<FriisPropagationLossModel> ();
  loss_model->SetSystemLoss (1.0); // Default
  loss_model->SetFrequency (2.4e9);
  channel->SetPropagationDelayModel (CreateObject<ConstantSpeedPropagationDelayModel> ());
  channel->SetPropagationLossModel (loss_model);

  phy.SetChannel (channel);

  WifiMacHelper mac;
  mac.SetType ("ns3::AdhocWifiMac",
         "QosSupported", BooleanValue (false), // Default, just exposing attributes.
         "CtsToSelfSupported", BooleanValue (false)); // Default, just exposing attributes.

  NetDeviceContainer n_devices;
  n_devices = wifi.Install (phy, mac, wifi_nodes);

  InternetStackHelper stack;
  stack.Install (wifi_nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (n_devices);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  // Set up routes
  Ipv4Address group_dest_addr ("225.1.2.1");
  for (uint32_t i = 0; i < interfaces.GetN (); i++)
  {
    std::pair<Ptr<Ipv4>, uint32_t> ipv4_if = interfaces.Get (i);
    Ptr<Ipv4StaticRouting> static_router = ipv4RoutingHelper.GetStaticRouting (ipv4_if.first);
    ipv4RoutingHelper.SetDefaultMulticastRoute (wifi_nodes.Get (i), n_devices.Get (i));
    // Host route for multicast
    // route for host
    // Use host routing entry according to note in Ipv4StaticRouting::RouteOutput:
    //// Note:  Multicast routes for outbound packets are stored in the
    //// normal unicast table.  An implication of this is that it is not
    //// possible to source multicast datagrams on multiple interfaces.
    //// This is a well-known property of sockets implementation on
    //// many Unix variants.
    //// So, we just log it and fall through to LookupStatic ()
    static_router->AddHostRouteTo (group_dest_addr, ipv4_if.second, 0);
  }

  // Flood across whole network
  for (uint32_t i = 0; i < wifi_nodes.GetN (); i++)
    {
      for (uint32_t j = 0; j < wifi_nodes.GetN (); j++)
        {
          if (j == i)
            {
              continue;
            }
          ipv4RoutingHelper.AddMulticastRoute (wifi_nodes.Get (i), interfaces.GetAddress (j),
                                               group_dest_addr, n_devices.Get (i), NetDeviceContainer (n_devices.Get (i)));
        }
    }
  stack.SetRoutingHelper (ipv4RoutingHelper);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (-100.0),
                                 "MinY", DoubleValue (-100.0),
                                 "DeltaX", DoubleValue (distance),
                                 "DeltaY", DoubleValue (distance),
                                 "GridWidth", UintegerValue (numNodesRows),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  mobility.Install (wifi_nodes);

  std::map<Ipv4Address, std::vector<uint32_t> > mcast_groups;

  NodeContainer sinkNodes;

  for (uint32_t i = 0; i < wifi_nodes.GetN (); i++)
    {
      if (i == 0)
        {
          continue;
        }
      else
        {
          mcast_groups[group_dest_addr].push_back (i);
          sinkNodes.Add (wifi_nodes.Get (i));
        }
    }

  PopulateArpCache ();

  uint32_t port = 10001;

  ApplicationContainer sinkApps, srcApps;

  OnOffHelper onOffHelper ("ns3::UdpSocketFactory", Address (InetSocketAddress (group_dest_addr, port)));
  onOffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  srcApps.Add (onOffHelper.Install (wifi_nodes.Get (0)));

  PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", Address (InetSocketAddress (group_dest_addr, port)));
  sinkApps.Add (packetSinkHelper.Install (sinkNodes));

  sinkApps.Start (Seconds (start_time));
  sinkApps.Stop (Seconds (endTime - 1.0));

  srcApps.Start (Seconds (start_time));
  srcApps.Stop (Seconds (endTime - 1.0));

  MulticastFlowMonitorHelper flowmon;
  // flowmon.SetMulticastMonitorAttribute("MaxPerHopDelay", TimeValue(Seconds(end_time)));
  Ptr<MulticastFlowMonitor> monitor = flowmon.InstallAll (mcast_groups);

  Simulator::Stop (Seconds (endTime + 1));
  Simulator::Run ();

  Ptr<Ipv4MulticastFlowClassifier> classifier = DynamicCast<Ipv4MulticastFlowClassifier> (flowmon.GetClassifier ());
  MulticastFlowMonitor::MulticastFlowStatsContainer stats = monitor->GetMulticastFlowStats ();
  std::ofstream flow_out ("FlowStats.csv");
  flow_out << "flow_id,flow_src,flow_dst,flow_dst_addr,avg_hop_count,tx_packets," <<
    "tx_bytes,tx_rate_kbps,rx_packets,rx_bytes,rx_tput_kbps," <<
    "dups_dropped,times_fwded,lost_packets,avg_delay\n";

  std::ofstream pack_out ("PacketStats.csv");
  pack_out << "flow_id,flow_src,flow_dst,flow_dst_addr,packetSeq,delay,hop_count\n";

  for (std::map<MulticastFlowId, MulticastFlowMonitor::MulticastFlowStats>::iterator i = stats.begin (); i != stats.end (); ++i)
    {
      Ipv4MulticastFlowClassifier::FiveTuple t = classifier->FindMulticastFlow (i->first);
      // std::vector<uint32_t> groupNodeIds
      std::vector<uint32_t>::const_iterator it;
      for (it = i->second.groupNodeIds.begin (); it != i->second.groupNodeIds.end (); it++)
        {
          uint32_t node = *(it);
          flow_out << i->first << ","; //flow_id
          flow_out << t.sourceAddress << ","; // flow_src node
          flow_out << node << ","; // flow_dst node
          flow_out << t.destinationAddress << ","; //flow_dst addr

          std::map<uint32_t, uint32_t>::iterator h_it;
          uint32_t tot_hops = 0;
          for (h_it = i->second.numHops[node].begin (); h_it != i->second.numHops[node].end (); h_it++)
            {
              tot_hops = tot_hops + h_it->second;
            }
          flow_out << static_cast<double> (tot_hops) / static_cast<double> (i->second.numHops[node].size ()) << ","; // avg_hop_count
          flow_out << i->second.txPackets << ","; // num_tx_packets
          flow_out << i->second.txBytes << ","; // tx_bytes
          flow_out << i->second.txBytes * 8.0 / (endTime - start_time) / 1000.0 << ","; //tx_throughput_kbps
          flow_out << i->second.rxPackets[node] << ","; //num_rx_packets
          flow_out << i->second.rxBytes[node] << ","; //rx_bytes
          flow_out << i->second.rxBytes[node] * 8.0 / (endTime - start_time) / 1000.0 << ","; //rx_throughput_kbps
          flow_out << i->second.dupsDropped[node] << ",";
          flow_out << i->second.timesForwarded[node] << ","; // times_forwarded
          flow_out << i->second.lostPackets[node] << ",";
          flow_out << i->second.delaySum[node].GetSeconds () / i->second.txPackets << "\n";

          std::map<uint32_t, Time>::iterator p_it;
          for (p_it = i->second.packetDelay[node].begin (); p_it != i->second.packetDelay[node].end (); p_it++)
            {
              uint32_t pack_id = p_it->first;
              Time pack_delay = p_it->second;
              pack_out << i->first << ","; //flow_id
              pack_out << t.sourceAddress << ","; // flow_src node
              pack_out << node << ","; // flow_dst node
              pack_out << t.destinationAddress << ","; //flow_dst addr
              pack_out << pack_id << ","; // packet id/seq
              pack_out << pack_delay.GetSeconds () << ",";
              pack_out << i->second.numHops[node][pack_id] << "\n";
            }
        }
    }
  return 0;
}


