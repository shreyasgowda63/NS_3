/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *  Copyright 2018. Lawrence Livermore National Security, LLC.
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
 * Author: Steven Smith <smith84@llnl.gov>
 */

#include "ns3/arp-cache.h"
#include "ns3/channel-delay-model.h"
#include "ns3/channel-error-model.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/core-module.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/node-list.h"
#include "ns3/object-vector.h"
#include "ns3/object.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/simple-distributed-helper.h"

#ifdef NS3_MPI
#include "ns3/mpi-interface.h"
#include <mpi.h>
#endif

#include <cmath>
#include <limits>

typedef std::numeric_limits< double > dbl;

/*
 * \file
 * \ingroup simple-distributed
 * \ingroup simple-distributed-examples
 *
 * Parallel example/test case for SimpleDistributedNetDevice.
 *
 * This example shows how a simple distributed channel is setup in a
 * parallel simulation.  The example is designed to scale in size as
 * based on the number of processors used.  This example is used as a
 * test case for SimpleDistributedNetDevice.
 *
 * A single channel is created on each processor.  The single instance
 * is 'shared' across the processors.  UPD messages are exchanged
 * between the nodes.
 *
 * The entire topology is represented on all ranks but only owning
 * rank installs the applications and mobility models.
 *
 * The node positioning is a square grid of nodes.  The size of the
 * grid is specified via the --grid-size=N command line argument.
 * Nodes are distributed across ranks using block distribution (rank 0
 * owns nodes 0 - N / # ranks).  The number of nodes must be evenly
 * divisible by number of ranks.  The grids are 1m apart.
 *
 * The UDP communication pattern is specified via the --communication-pattern=C argument.
 * C is one of:
 *    - 0  Ring communication.  nodeID i sends to nodeID i+1
 *    - 1  Send to node 0. Nodes 1-N send to 0.
 *    - 2  Node 0 broadcasts.  Node 0 broadcasts to channel.
 *
 * Each sending node sends 4 UDP packets. An ARP cache is manually
 * created to avoid ARP messages causing extra delays, this makes
 * verifying send/receive times easier.   The channel is configured 
 * with a delay model of 10ms delay per packet per meter.
 * 
 * The SimpleDistributedChannel can limit communication range, this 
 * is specified via the --distance=<distance> command line argument.
 *
 * The number of packets and delay times are checked.
 */

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("SimpleDistributedChannelScalingExample");

/**
 * Create an ARP cache on each node based on the current
 * nodes/netdevices.
 *
 * A single ARP cache is static and shared across all nodes for better
 * scaling so one should not use this in situations where nodes may be
 * updating the ARP cache.
 */
void
PopulateArpCache () 
{
  Ptr<ArpCache> arp = CreateObject<ArpCache> ();
  arp->SetAliveTimeout (Seconds(3600 * 24 * 365));
  for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
  {
    Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
    NS_ASSERT(ip !=0);
    ObjectVectorValue interfaces;
    ip->GetAttribute("InterfaceList", interfaces);
    for(ObjectVectorValue::Iterator j = interfaces.Begin(); j !=
	  interfaces.End (); j ++)
    {
      Ptr<Ipv4Interface> ipIface = (*j).second->GetObject<Ipv4Interface> ();
      NS_ASSERT(ipIface != 0);
      Ptr<NetDevice> device = ipIface->GetDevice();
      NS_ASSERT(device != 0);
      Mac48Address addr = Mac48Address::ConvertFrom(device->GetAddress ());
      for(uint32_t k = 0; k < ipIface->GetNAddresses (); k ++)
      {
        Ipv4Address ipAddr = ipIface->GetAddress (k).GetLocal();
        if(ipAddr == Ipv4Address::GetLoopback())
          continue;
        ArpCache::Entry * entry = arp->Add(ipAddr);
	entry->SetMacAddress(addr);
	entry->MarkPermanent();
      }
    }
  }
  for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
  {
    Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
    NS_ASSERT(ip !=0);
    ObjectVectorValue interfaces;
    ip->GetAttribute("InterfaceList", interfaces);
    for(ObjectVectorValue::Iterator j = interfaces.Begin(); j !=
	  interfaces.End (); j ++)
    {
      Ptr<Ipv4Interface> ipIface = (*j).second->GetObject<Ipv4Interface> ();
      ipIface->SetAttribute("ArpCache", PointerValue(arp));
    }
  }
}

/**
 * Test delay model
 * 
 * Models a slow 10 ms / m packet travel time.
 */
class DistanceDelayModel : public ChannelDelayModel
{
public:
  DistanceDelayModel ()
  {
  }

  ~DistanceDelayModel ()
  {
  }

private:

  Time DoComputeDelay (Ptr<Packet> pkt, uint32_t srcId, Vector srcPosition, Ptr<NetDevice> dst) const
  {
    auto dstNode = dst->GetNode ();
    auto dstMobilityModel = dstNode->GetObject<MobilityModel> ();
    NS_ASSERT(dstMobilityModel != 0);

    auto distanceToSrc = CalculateDistance (srcPosition, dstMobilityModel -> GetPosition ());

    return NanoSeconds(round(distanceToSrc * m_timePerPacket));
  }

  Time DoGetMinimumDelay (void) const
  {
    // Assume minimum of 1 m distance between nodes.
    return NanoSeconds(round(1 * m_timePerPacket));
  }

  void DoReset (void)
  {
  }

  /** Packet delay time, 10ms as ns */
  const double m_timePerPacket = 10.0 * 1000000.0; 
};


/**
 * Distance based error model.
 *
 * Corrupts packets when distance between sender and receivers is >
 * than provided distance.  This mimics the distance limit in
 * SimpleDistributedChannel.
 */
class DistanceErrorModel : public ChannelErrorModel
{
public:
  DistanceErrorModel (double distance) 
    : m_distance(distance)
  {
  }

  ~DistanceErrorModel ()
  {
  }

private:

  bool DoIsCorrupt (Ptr<Packet> pkt, uint32_t srcId, Vector srcPosition, Ptr<NetDevice> dst) const
  {
    auto dstNode = dst->GetNode ();
    auto dstMobilityModel = dstNode->GetObject<MobilityModel> ();
    NS_ASSERT(dstMobilityModel != 0);

    auto distanceToSrc = CalculateDistance (srcPosition, dstMobilityModel -> GetPosition ());

    bool isCorrupt = distanceToSrc > m_distance;

    return isCorrupt;
  }

  void DoReset (void)
  {
  }

  const double m_distance; /**< Corruption distance */
};


/**
 *  Sent time as a packet tag.
 *
 *  Used to compute total transmission delay from OnOff application to packet sink.
 */
class SimpleDistributedTestTag : public Tag
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  virtual TypeId GetInstanceTypeId (void) const;

  SimpleDistributedTestTag();
    
  SimpleDistributedTestTag(Time time);
  
  virtual uint32_t GetSerializedSize (void) const;
  
  virtual void Serialize (TagBuffer i) const;
  
  virtual void Deserialize (TagBuffer i);

  virtual void Print(std::ostream&) const;

  Time m_time;
};

NS_OBJECT_ENSURE_REGISTERED (SimpleDistributedTestTag);

TypeId SimpleDistributedTestTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimpleDistributedTestTag")
    .SetParent<Tag> ()
    .SetGroupName ("Network")
    .AddConstructor<SimpleDistributedTestTag> ()
    ;
  return tid;
}

TypeId SimpleDistributedTestTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

SimpleDistributedTestTag::SimpleDistributedTestTag()
{
}
    
SimpleDistributedTestTag::SimpleDistributedTestTag(Time time) :
  m_time(time)
{
}
  
uint32_t SimpleDistributedTestTag::GetSerializedSize (void) const
{
  return 8;
}
  
void SimpleDistributedTestTag::Serialize (TagBuffer i) const
{
  i.WriteU64(m_time.GetTimeStep ());
}
  
void SimpleDistributedTestTag::Deserialize (TagBuffer i)
{
  m_time = Time(i.ReadU64 ());
}

void SimpleDistributedTestTag::Print(std::ostream&) const
{
}

/**
 * Compute number of lattice points and compute sum of all nodes
 * distances to origin (0,0) in single quadrant constrained by
 * distance and size of grid.
 *
 * Computed using a modified Gauss Circle Algorithm.
 * Grid spacing is assumed to be 1.0.
 *
 * \returns number of lattice points
 * \param distance Communication distance
 * \param gridLength total length of square grid along x/y axis
 */
std::tuple<unsigned int, double> GaussCircleCount (const double distance, const double gridLength)
{
  double sumDistance = 0;
  unsigned int count=0;
  unsigned int x_max = std::min(floor(distance), floor(gridLength));
  for(unsigned int x = 0; x <= x_max; ++x)
    {
      unsigned y_max= std::min(floor(sqrt(pow(distance,2) - pow(x,2))), floor(gridLength));
      for(unsigned y = 0; y <= y_max; ++y)
        {
          ++count;
          sumDistance += sqrt(pow(x,2) + pow(y,2));
        }
    }

  return std::make_tuple(count, sumDistance);
}

static Time g_receviedSumTime; /**< Running sum of all delay times, used for correctness check */
static unsigned long g_receivedPacketNumber; /**< Running sum of all delay times, used for correctness check */

/**
 * Callback to update the running sum of packets received and delay times.
 *
 * \param packet The packet
 */
static void ReceivePkt (Ptr<const Packet> packet)
{
  if (packet)
    {
      SimpleDistributedTestTag tag;
      if (packet->PeekPacketTag (tag))
        {
          g_receviedSumTime += Simulator::Now () - tag.m_time;
          g_receivedPacketNumber++;
        }
      else
        {
          // SGS TODO why was this comment out?
          // NS_ASSERT_MSG(false, "Failed to find packet tag");
        }
    }
}

/**
 * Callback to add packet tag with sending time.
 *
 * \param packet The packet
 * \param from Address of the sender
 */
void SendPkt (Ptr<const Packet> packet)
{
  if (packet)
    {
      SimpleDistributedTestTag tag(Simulator::Now ());
      packet -> AddPacketTag(tag);
    }
}

enum CommunicationPattern { RING = 0, GATHER = 1, SCATTER = 2 };

int
main (int argc, char *argv[])
{
  bool tracing = false;
  uint32_t gridSize = 10;
  uint32_t communicationPattern = RING;
  double distance = -1.0;
  bool verbose=false;
  bool timing=false;
  std::string time = "400s";

  double gridSpacing = 1.0;

  double corruptionDistance = std::numeric_limits<double>::max ();

  SystemWallClockMs clock;
  clock.Start ();

  /*
   * Checksums enabled to make packets digestible by wireshark and other tools.
   */
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  // Parse command line
  CommandLine cmd;
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
  cmd.AddValue ("grid-size", "Number of nodes in x/y (default = 10", gridSize);
  cmd.AddValue ("communication-pattern", "Communication pattern 0 = ring, 1 = gather, 2 = scatter", communicationPattern);
  cmd.AddValue ("distance", "Communication distance ", distance);
  cmd.AddValue ("corruption-distance", "Corruption distance ", corruptionDistance);
  cmd.AddValue ("time", "Simulation runtime ", time);
  cmd.AddValue ("verbose", "Verbose mode", verbose);
  cmd.AddValue ("timing", "Timing output", timing);
  cmd.Parse (argc, argv);

  // Example runs in parallel or sequentially depending on availability of MPI.
#ifdef NS3_MPI
  // Use parallel granted time window algorithm.
  GlobalValue::Bind ("SimulatorImplementationType",
                     StringValue ("ns3::DistributedSimulatorImpl"));

  // Enable parallel simulator with the command line arguments
  MpiInterface::Enable (&argc, &argv);

  uint32_t systemCount = MpiInterface::GetSize ();
#else
  uint32_t systemCount = 1;
#endif

  uint32_t systemId = Simulator::GetSystemId ();

  if(verbose)
    {
      LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
    }

  if (gridSize * gridSize < systemCount)
    {
      std::cout << "Simulation requires number of nodes >= ranks. Increase gridSize (currently = " << gridSize << ")\n";
      return 1;
    }

  // Topology is square grid of nodes.
  uint32_t numberOfNodes = gridSize * gridSize;

  if ( numberOfNodes % systemCount )
    {
      std::cout << "Simulation requires number of ns-3 nodes to be evenly divisible by ranks.\n";
    }

  uint32_t nodesPerRank = numberOfNodes / systemCount;
  
  // Set application traffic parameters
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (512));
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("512b/s"));
  Config::SetDefault ("ns3::OnOffApplication::MaxBytes", UintegerValue (2048));

  // Create nodes on ranks using block distribution.
  NodeContainer leafNodes;
  for(uint32_t i = 0; i < numberOfNodes; ++i)
    {
      uint32_t ownerSystemId = i / nodesPerRank;
      Ptr<Node> node  = CreateObject<Node> (ownerSystemId);
      leafNodes.Add(node);

      auto mobilityModel = CreateObject<ConstantPositionMobilityModel> ();
      
      Vector position( (i % gridSize) * gridSpacing, (i / gridSize) * gridSpacing, 0);
      mobilityModel -> SetPosition (position);
      
      node->AggregateObject (mobilityModel);
    }

  SimpleDistributedHelper link;

  // Limits distance of communications if requested for the run.
  link.SetChannelAttribute ("Distance", DoubleValue(distance));
  
  Ptr<DistanceDelayModel> distanceDelayModel = CreateObject<DistanceDelayModel>();
  link.SetChannelAttribute ("DelayModel", PointerValue(distanceDelayModel));


  if (corruptionDistance < std::numeric_limits<double>::max ())
    {
      Ptr<DistanceErrorModel> distanceErrorModel = CreateObject<DistanceErrorModel> (corruptionDistance);
      link.SetChannelAttribute ("ErrorModel", PointerValue (distanceErrorModel));
    }
  
  NetDeviceContainer devices;
  devices = link.Install (leafNodes);

  InternetStackHelper stack;

  Ipv4StaticRoutingHelper staticRouting;
  stack.SetRoutingHelper (staticRouting);
  stack.InstallAll ();

  Ipv4InterfaceContainer leafInterfaces;

  Ipv4AddressHelper addresses;
  addresses.SetBase ("10.0.0.0", "255.0.0.0");

  leafInterfaces = addresses.Assign (devices);

  // Turn on PCAP captures if requested
  if (tracing == true)
    {
      for(uint32_t i = 0; i < numberOfNodes; ++i)
        {
          uint32_t ownerSystemId = i / nodesPerRank;
          if (ownerSystemId == systemId)
            {
              link.EnablePcap("node", devices.Get (i), true);
            }
        }
    }

  // Create a packet sink on all nodes
  uint16_t port = 50000;

  for(uint32_t i = 0; i < numberOfNodes; ++i)
  {
    // Install applications only on nodes the rank owns.
    uint32_t ownerSystemId = i / nodesPerRank;
    if (ownerSystemId == systemId)
      {
        Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
        PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);
        ApplicationContainer sinkApp;
        sinkApp.Add (sinkHelper.Install (leafNodes.Get (i)));
        sinkApp.Start (Seconds (1.0));
        sinkApp.Stop (Time (time));
      }
  }

  // Setup communication pattern specified by user
  switch (communicationPattern)
    {
    case RING:
      {
        // Ring comm pattern.  All nodes send to node + 1; ring
        for(uint32_t i = 0; i < numberOfNodes; ++i)
          {
            uint32_t ownerSystemId = i / nodesPerRank;
            if (ownerSystemId == systemId)
              {
                OnOffHelper clientHelper ("ns3::UdpSocketFactory", Address ());
                clientHelper.SetAttribute
                  ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
                clientHelper.SetAttribute
                  ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

                ApplicationContainer clientApps;
                AddressValue remoteAddress(InetSocketAddress
                                           (leafInterfaces.GetAddress ((i+1) % numberOfNodes), port));
                clientHelper.SetAttribute ("Remote", remoteAddress);
                clientApps.Add (clientHelper.Install (leafNodes.Get (i)));
                clientApps.Start (Seconds (1.0));
                clientApps.Stop (Time (time));
              }
          }
      }
      break;
    case GATHER:
      {
        // All nodes send to node 0
        for(uint32_t i = 1; i < numberOfNodes; ++i)
          {
            uint32_t ownerSystemId = i / nodesPerRank;
            if (ownerSystemId == systemId)
              {
                OnOffHelper clientHelper ("ns3::UdpSocketFactory", Address ());
                clientHelper.SetAttribute
                  ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
                clientHelper.SetAttribute
                  ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
                
                ApplicationContainer clientApps;
                AddressValue remoteAddress
                  (InetSocketAddress (leafInterfaces.GetAddress (0), port));

                clientHelper.SetAttribute ("Remote", remoteAddress);
                clientApps.Add (clientHelper.Install (leafNodes.Get (i)));
                clientApps.Start (Seconds (1.0));
                clientApps.Stop (Time (time));
              }
          }
      }
      break;
    case SCATTER:
      {
        // Node 0 broadcasts to all other nodes
        int i = 0;
        
        uint32_t ownerSystemId = i / nodesPerRank;
        if (ownerSystemId == systemId)
          {
            OnOffHelper clientHelper ("ns3::UdpSocketFactory", Address ());
            clientHelper.SetAttribute
              ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
            clientHelper.SetAttribute
              ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
                  
            ApplicationContainer clientApps;
            AddressValue remoteAddress (InetSocketAddress ("255.255.255.255", port));
            
            clientHelper.SetAttribute ("Remote", remoteAddress);
            clientApps.Add (clientHelper.Install (leafNodes.Get (i)));
            clientApps.Start (Seconds (1.0));
            clientApps.Stop (Time (time));
          }
      }
      break;
    default:
      {
        NS_FATAL_ERROR ("Invalid communication pattern selected : " << communicationPattern);
                exit(1);
      }
    }

  // Callbacks track number of packets received and sum of delay times
  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::SimpleDistributedNetDevice/MacTx",
                                 MakeCallback (&SendPkt));

  Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/$ns3::SimpleDistributedNetDevice/MacRx",
                                 MakeCallback (&ReceivePkt));

  // Populate ARP caches on nodes, this avoids ARP traffic which
  // simplifies the checks used on measured traffic delay times.
  PopulateArpCache ();

  Simulator::Stop (Time (time));

  clock.End ();
  uint64_t setupReal = clock.GetElapsedReal ();
  clock.Start ();

  Simulator::Run ();

  clock.End ();
  uint64_t runReal = clock.GetElapsedReal ();

  // Packet transmission distance limit in the tests is minimum of the
  // distance limit and error model corruption distance.
  double combinedDistanceLimit;
  if (distance < 0)
    {
      combinedDistanceLimit = corruptionDistance;
    }
  else
    {
      combinedDistanceLimit = std::min(distance, corruptionDistance);
    }

  // Check number of packets received and sum of delay times to validate the model is working
  unsigned int expectedNumberOfPacketsRecvd = 0;
  Time expectedSumTxTime;
  switch (communicationPattern)
    {
    case RING:
      {
        // All nodes sending to neighbor.
        if ( combinedDistanceLimit < gridSpacing)
          {
            // combinedDistanceLimit to small; no communication is possible.
            expectedNumberOfPacketsRecvd = 0;
            expectedSumTxTime = Time(0);
          }
        else if ( combinedDistanceLimit < gridSize * gridSpacing)
          {
            // Edges can't wrap
            expectedNumberOfPacketsRecvd = 4 * (gridSize * (gridSize - 1));
            // 4 comm * combinedDistanceLimit (m) * 10 m/ms *  n/ms
            expectedSumTxTime = NanoSeconds(4 * (gridSize * (gridSize - 1)) * gridSpacing * 10.0 * 1000000.0);
          }
        else if ( combinedDistanceLimit < sqrt(pow(gridSize * gridSpacing, 2) + pow(gridSize * gridSpacing, 2)))
          {
            // Edges can wrap but corner can't
            expectedNumberOfPacketsRecvd = 4 * (gridSize * gridSize - 1);

            // interior 
            expectedSumTxTime = NanoSeconds(4 * (gridSize * (gridSize - 1)) * gridSpacing * 10.0 * 1000000.0);
            // edge wrap
            expectedSumTxTime += NanoSeconds(4 * (gridSize - 1) * (sqrt(pow(gridSpacing,2) + pow(gridSpacing*(gridSize-1),2))) * 10.0 * 1000000.0);
          }
        else 
          {
            // Everyone can comm with +1 node index neighbor, including corner
            expectedNumberOfPacketsRecvd = 4 * (gridSize * gridSize);
            
            // interior 
            expectedSumTxTime = NanoSeconds(4 * (gridSize * (gridSize - 1)) * gridSpacing * 10.0 * 1000000.0);
            // edge wrap
            expectedSumTxTime += NanoSeconds(4 * (gridSize - 1) * (sqrt(pow(gridSpacing,2) + pow(gridSpacing*(gridSize-1),2))) * 10.0 * 1000000.0);
            // corner wrap
            expectedSumTxTime += NanoSeconds(4 * (1) * (sqrt(2*pow( (gridSize-1) * gridSpacing,2))) * 10.0 * 1000000.0);
          }
      }
      break;
    case GATHER:
      {
        // All nodes sending 4 pkts to node 0.
        int count;
        double sumDistance;
        std::tie(count, sumDistance) = GaussCircleCount(combinedDistanceLimit, (gridSize-1) * gridSpacing);
        expectedNumberOfPacketsRecvd = ( count - 1) * 4;
        // 4 comm * combinedDistanceLimit (m) * 10 m/ms *  n/ms
        expectedSumTxTime = NanoSeconds(4.0 * sumDistance * 10.0 * 1000000.0);
      }
      break;
    case SCATTER:
      {
        // Node 0 sending 4 pkts to other nodes within combinedDistanceLimit
        int count;
        double sumDistance;
        std::tie(count, sumDistance) = GaussCircleCount(combinedDistanceLimit, (gridSize-1) * gridSpacing);
        expectedNumberOfPacketsRecvd = (count - 1) * 4;
        // 4 comm * combinedDistanceLimit (m) * 10 m/ms *  n/ms
        expectedSumTxTime = NanoSeconds(4.0 * sumDistance * 10.0 * 1000000.0);
      }
      break;
    }


#ifdef NS3_MPI
  // In parallel case sum received counts on all ranks
  unsigned long localSum [2];
  unsigned long globalSum [2];
  localSum[0] = g_receivedPacketNumber;
  localSum[1] = g_receviedSumTime.GetTimeStep ();

  MPI_Reduce(&localSum, &globalSum, 2, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  
  g_receivedPacketNumber = globalSum[0];
  g_receviedSumTime = Time(globalSum[1]);
#endif
  
  if(!systemId)
    {
      bool passed = true;

      if( Abs(g_receviedSumTime - expectedSumTxTime) < Time("100ns") )
        {
        }
      else
        {
          passed &= false;
          std::cout << "FAILED : transmission delays != expected; " << g_receviedSumTime << "!=" << expectedSumTxTime << "\n";
        }

      if( g_receivedPacketNumber == expectedNumberOfPacketsRecvd)
        {
        }
      else
        {
          passed &= false;
          std::cout << "FAILED : number of packets received != expected; " << g_receivedPacketNumber << "!=" << expectedNumberOfPacketsRecvd << "\n";
        }

      if(passed)
        {
          std::cout << "PASSED\n";
        }
    }

  if(!systemId && timing)
    {
      std::cout << "CSV," << "CommunicationPattern" << "," << "NumberOfNodes" << "," << "NumberOfPackets" << "," << "Setup Time (ms)" << "," << "Simulation Runtime (ms)" << "\n";
      std::cout << "CSV," << communicationPattern << "," << numberOfNodes << "," << g_receivedPacketNumber << "," << setupReal << "," << runReal << "\n";
    }

  Simulator::Destroy ();

#ifdef NS3_MPI
  MpiInterface::Disable ();
#endif

  return 0;
}
