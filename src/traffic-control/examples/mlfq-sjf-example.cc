/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) Liangcheng Yu <liangcheng.yu46@gmail.com>
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
 * Authors: Liangcheng Yu <liangcheng.yu46@gmail.com>
 * GSoC 2019 project Mentors:
 *          Dizhi Zhou <dizhizhou@hotmail.com>
 *          Mohit P. Tahiliani <tahiliani.nitk@gmail.com>
 *          Tom Henderson <tomh@tomh.org>
*/

/** Network topology for the experiments
*
*    <4Mbps, 1ms>                         <4Mbps, 1ms>
* s0--------------|                    |---------------d0
*                 |   <2Mbps, 10ms>    |
*                 r0------------------r1
*    <4Mbps, 1ms> |                    |  <4Mbps, 1ms>
* s1--------------|                    |---------------d1
* 
* This example shows how to use MlfqQueueDisc, SjfQueueDisc for scheduling.
* It also includes an experiment with FifoQueueDisc for comparison.
*/

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MlfqSjfExample");

int
main (int argc, char *argv[])
{
  LogComponentEnable ("MlfqSjfExample", LOG_LEVEL_INFO);

  std::string    expQueueDiscName = "MlfqQueueDisc";
  std::string    socketType = "TcpSocketFactory";
  std::string    socketTypeName;
  uint32_t       nLeaf = 2;  // Symmetric dumbbell topology (left leaves equal to right leaves)

  NS_LOG_INFO ("Configuration and command line parameter parsing.");
  CommandLine cmd;
  cmd.AddValue ("queueDiscName", "Run MlfqQueueDisc or FifoQueueDisc or SjfQueueDisc", expQueueDiscName);
  cmd.AddValue ("socketType", "Specify TcpSocketFactory or UdpSocketFactory", socketType);
  cmd.Parse (argc, argv);

  // Sanity check for expQueueDiscName
  NS_LOG_INFO ("Check experiment number.");
  if (!((expQueueDiscName.compare("MlfqQueueDisc") == 0)||(expQueueDiscName.compare("SjfQueueDisc") == 0)||(expQueueDiscName.compare("FifoQueueDisc") == 0)))
    {
      NS_LOG_INFO ("Invalid experiment number. expQueueDiscName should be MlfqQueueDisc, SjfQueueDisc or FifoQueueDisc.");
      return 0;
    }

  // Sanity check for socketType
  if (socketType.compare("UdpSocketFactory") == 0)
    {
      socketTypeName = "ns3::UdpSocketFactory";
    }
  else if (socketType.compare("TcpSocketFactory") == 0)
    {
      socketTypeName = "ns3::TcpSocketFactory";
      Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1448));
      // Wait 1 packet before sending a TCP ACK
      Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
    }
  else
    {
      NS_LOG_INFO ("Invalid socket type. Please specify tcp or udp.");
      return 0;
    }

  // No packet drop
  Config::SetDefault ("ns3::QueueBase::MaxSize", StringValue ("1000000p"));

  // Create point-to-point link helpers
  PointToPointHelper p2pBottleNeck;
  p2pBottleNeck.SetDeviceAttribute ("DataRate", StringValue ("2Mbps"));
  p2pBottleNeck.SetChannelAttribute ("Delay", StringValue ("10ms"));
  p2pBottleNeck.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2pBottleNeck.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));

  PointToPointHelper p2pLeaf;
  p2pLeaf.SetDeviceAttribute ("DataRate", StringValue ("4Mbps"));
  p2pLeaf.SetChannelAttribute ("Delay", StringValue ("1ms"));
  p2pLeaf.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2pLeaf.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));

  PointToPointDumbbellHelper p2pDumbbell (nLeaf, p2pLeaf,
                                          nLeaf, p2pLeaf,
                                          p2pBottleNeck);

  // Install Stack
  InternetStackHelper stack;
  p2pDumbbell.InstallStack(stack);

  // Install queue discs
  TrafficControlHelper tchBottleneck;
  TrafficControlHelper tchLeaf;
  if (expQueueDiscName.compare("MlfqQueueDisc") == 0)
    {
      // Use the default configuration: 2 priorities and default threshold 20000 bytes
      tchLeaf.SetRootQueueDisc ("ns3::MlfqQueueDisc");
      // Uncomment the line below to eliminate the header bytes when counting. Differences are trivial in this simple simulation setting.
      // Config::SetDefault ("ns3::MlfqQueueDisc::HeaderBytesInclude", BooleanValue (false));
      /*
      * PrioQueueDisc needs to be compliant with MlfqQueueDisc configurations,
      * i.e., the number of priorities supported should be 2 in this case.
      * Meanwhile, FlowPrioPacketFilter needs to be installed.
      */ 
      uint16_t handle = tchBottleneck.SetRootQueueDisc ("ns3::PrioQueueDisc", "Priomap",
                                        StringValue ("0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1"));
      tchBottleneck.AddPacketFilter (handle, "ns3::FlowPrioPacketFilter");
      TrafficControlHelper::ClassIdList cid = tchBottleneck.AddQueueDiscClasses (handle, 2, "ns3::QueueDiscClass");
      tchBottleneck.AddChildQueueDisc (handle, cid[0], "ns3::FifoQueueDisc");
      tchBottleneck.AddChildQueueDisc (handle, cid[1], "ns3::FifoQueueDisc");      
    }
  else if (expQueueDiscName.compare("SjfQueueDisc") == 0)
    {
      tchLeaf.SetRootQueueDisc ("ns3::SjfQueueDisc");
      tchBottleneck.SetRootQueueDisc ("ns3::SjfQueueDisc");
    }    
  else if (expQueueDiscName.compare("FifoQueueDisc") == 0)
    {
      tchLeaf.SetRootQueueDisc ("ns3::FifoQueueDisc");
      tchBottleneck.SetRootQueueDisc ("ns3::FifoQueueDisc");
    }

  p2pDumbbell.InstallTrafficControl(tchLeaf, tchBottleneck);

  // Assign IP Addresses
  p2pDumbbell.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"),
                         Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"),
                         Ipv4AddressHelper ("10.3.1.0", "255.255.255.0"));

  NS_LOG_INFO ("Configure traffic generation.");
  // As a simple example, we use OnOffApplication with zero OffTime to simulate the flows to compare the flow completion statistics
  // Configure applications at source nodes
  ApplicationContainer sourceAppTmp;
  Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), 5000));  
  OnOffHelper sourceAppHelper (socketTypeName, localAddress);  
  sourceAppHelper.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  sourceAppHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  sourceAppHelper.SetAttribute ("PacketSize", UintegerValue (1448));
  sourceAppHelper.SetAttribute ("DataRate", StringValue ("4Mbps"));
  // Create an on/off app sending packets to the same right side leaf
  for (uint32_t i = 0; i < p2pDumbbell.LeftCount (); ++i)
    {
      AddressValue remoteAddress (InetSocketAddress (p2pDumbbell.GetRightIpv4Address (i), 5000));
      sourceAppHelper.SetAttribute ("Remote", remoteAddress);
      // For SjfQueueDisc, it requires the FlowSizeTagInclude attribute to be true
      if (expQueueDiscName.compare("SjfQueueDisc") == 0)
        {
          sourceAppHelper.SetAttribute ("FlowSizeTagInclude", BooleanValue (true));
        }    
      // Configure a long flow and short flows during its transmission at each left nodes
      sourceAppHelper.SetAttribute ("MaxBytes", UintegerValue (10000));
      sourceAppTmp = sourceAppHelper.Install (p2pDumbbell.GetLeft (i));
      sourceAppTmp.Start (Seconds (0.05));
      sourceAppTmp.Stop (Seconds (50.0));

      sourceAppHelper.SetAttribute ("MaxBytes", UintegerValue (40000));     
      sourceAppTmp = sourceAppHelper.Install (p2pDumbbell.GetLeft (i));
      sourceAppTmp.Start (Seconds (0.0));
      sourceAppTmp.Stop (Seconds (50.0)); 
    }

  PacketSinkHelper packetSinkHelper (socketTypeName, localAddress);
  ApplicationContainer sinkApps;
  for (uint32_t i = 0; i < p2pDumbbell.RightCount (); ++i)
    {
      sinkApps.Add (packetSinkHelper.Install (p2pDumbbell.GetRight (i)));
    }
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (60.0));  // Stop after sourceApps

  // Populate routing configurations
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Install FlowMon
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  Simulator::Stop (Seconds (100));
  NS_LOG_INFO ("Start running simulation."); 
  Simulator::Run ();

  /* 
    Calculate the per-flow Flow Completion Time (FCT) and the average FCT.

    ===
    SjfQueueDisc: Average flow completion time: 0.347087s
    MlfqQueueDisc: Average flow completion time: 0.396029s
    FifoQueueDisc: Average flow completion time: 0.468533s
    ===

    This example demonstrates the benefits of using SjfQueueDisc or MlfqQueueDisc to reduce the average FCT.
    Compared with FIFO, MLFQ and SJF trades the FCT of the long flow for the FCT of short flows
    and obtains a smaller average FCT. SJF obtains a even smaller average FCT compared with MLFQ 
    since it offers more fined-grained priority differentiation with FlowSizePrioQueue rather than 
    a limited number of FIFO queues.
  */
  NS_LOG_INFO ("Calculate the flow completion time.");
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  uint32_t numFlows = 0;
  double sumFct = 0;
  double tmpFct;
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple tuple = classifier->FindFlow (i->first);
      if ((tuple.sourceAddress == "10.2.1.1") || (tuple.sourceAddress == "10.2.2.1"))
        {
          continue;
        }
      numFlows += 1;
      tmpFct = i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstTxPacket.GetSeconds ();
      sumFct += tmpFct;
      std::cout << "Flow completion time for the flow " << i->first << " (" << tuple.sourceAddress << " -> " << 
        tuple.destinationAddress << "): " << tmpFct << "s; receiving bytes: " << i->second.rxBytes << "; transmitted bytes: "
        << i->second.txBytes << "; time first packet transmitted: " << i->second.timeFirstTxPacket << "s; time last packet received: "
        << i->second.timeLastRxPacket << std::endl;
    }
  std::cout << "Average flow completion time: " << sumFct/numFlows << std::endl;

  Simulator::Destroy ();
  return 0;

}
