/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *  Copyright 2020. Lawrence Livermore National Security, LLC.
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

#include "ns3/test.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/nstime.h"
#include "ns3/traced-callback.h"
#include "ns3/packet.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-server.h"
#include "ns3/simple-distributed-net-device.h"
#include "ns3/simple-distributed-channel.h"
#include "ns3/channel-delay-model.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"


using namespace ns3;

class ConstantDelayModel : public ChannelDelayModel
{
public:
  ConstantDelayModel ()
  {
  }

  ~ConstantDelayModel ()
  {
  }


  uint32_t GetSrcId ()
  {
    return m_srcId;
  }

  uint32_t GetDstId ()
  {
    return m_dstId;
  }

  Vector GetSrcPosition ()
  {
    return m_srcPosition;
  }

private:

  Time DoComputeDelay (Ptr<Packet> pkt, uint32_t srcId, Vector srcPosition, Ptr<NetDevice> dst) const
  {
    // Record for later checking
    m_srcId = srcId;
    m_srcPosition = srcPosition;
    auto dstNode = dst->GetNode ();
    NS_ASSERT(dstNode);
    m_dstId = dstNode -> GetId ();
    
    return Time("20ms");
  }

  Time DoGetMinimumDelay (void) const
  {
    return Time("20ms");
  }
  
  void DoReset (void)
  {
  }

  /* State variables for checking arguments passed into ComputeDelay */
  mutable uint32_t m_srcId;
  mutable uint32_t m_dstId;
  mutable Vector m_srcPosition;
};

/**
 * Test delay model, slow 13 ms / m packet travel time.
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

    return MilliSeconds(distanceToSrc * 13);
  }

  Time DoGetMinimumDelay (void) const
  {
    // Assume minimum distance of 1m between ns-3 nodes.
    return MilliSeconds(1 * 13);
  }

  void DoReset (void)
  {
  }
};


/**
 * \ingroup simple-distributed
 * \ingroup tests
 *
 * \brief SimpleDistributedNetDevice Unit Test
 */
class SimpleDistributedTest : public TestCase
{
public:
  SimpleDistributedTest (std::string name) : TestCase ("simple-distributed-" + name),
                             m_receivedPacketSize(0),
                             m_receivedPacketNumber(0),
                             m_averageTime(0),
                             m_channelDistance(-1.0),
                             m_expectedDelay(0)
  {
  }
  
  virtual void DoRun (void)
  {
    // Create topology
    
    NodeContainer nodes;
    nodes.Create (2);
    
    PacketSocketHelper packetSocket;
    
    // give packet socket powers to nodes.
    packetSocket.Install (nodes);
    
    Ptr<SimpleDistributedNetDevice> txDev;
    txDev = CreateObject<SimpleDistributedNetDevice> ();
    txDev->SetAddress (Mac48Address ("00:00:00:00:00:01"));
    txDev -> SetAttribute("Delay", TimeValue(m_netDeviceDelay));
    txDev -> SetAttribute("DataRate", DataRateValue(m_netDeviceDataRate));
    txDev -> SetAttribute("InterframeGap", TimeValue(m_netDeviceGap));
    nodes.Get (0)->AddDevice (txDev);
    
    Ptr<SimpleDistributedNetDevice> rxDev;
    rxDev = CreateObject<SimpleDistributedNetDevice> ();
    rxDev->SetAddress (Mac48Address ("00:00:00:00:00:02"));
    rxDev -> SetAttribute("Delay", TimeValue(m_netDeviceDelay));
    rxDev -> SetAttribute("DataRate", DataRateValue(m_netDeviceDataRate));
    rxDev -> SetAttribute("InterframeGap", TimeValue(m_netDeviceGap));
    nodes.Get (1)->AddDevice (rxDev);
    
    Ptr<SimpleDistributedChannel> channel = CreateObject<SimpleDistributedChannel> ();
    channel -> SetAttribute("Delay", TimeValue(m_channelDelay));
    channel -> SetAttribute("DataRate", DataRateValue(m_channelDataRate));
    channel -> SetAttribute("Distance", DoubleValue(m_channelDistance));
    channel -> SetDelayModel (m_channelDelayModel);
    
    txDev->SetChannel (channel);
    rxDev->SetChannel (channel);
    txDev->SetNode (nodes.Get (0));
    rxDev->SetNode (nodes.Get (1));
    
    PacketSocketAddress socketAddr;
    socketAddr.SetSingleDevice (txDev->GetIfIndex ());
    socketAddr.SetPhysicalAddress (rxDev->GetAddress ());
    socketAddr.SetProtocol (1);
    
    Ptr<PacketSocketClient> client = CreateObject<PacketSocketClient> ();
    client->TraceConnectWithoutContext ("Tx", MakeCallback (&SimpleDistributedTest::SendPkt, this));
    client->SetRemote (socketAddr);
    client->SetAttribute ("PacketSize", UintegerValue (1000));
    client->SetAttribute ("MaxPackets", UintegerValue (3));
    nodes.Get (0)->AddApplication (client);
    
    Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer> ();
    server->TraceConnectWithoutContext ("Rx", MakeCallback (&SimpleDistributedTest::ReceivePkt, this));
    server->SetLocal (socketAddr);
    nodes.Get (1)->AddApplication (server);

    // Set nodes on a grid with spacing of 3.0 m
    MobilityHelper mobility;
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (0.0),
                                   "MinY", DoubleValue (0.0),
                                   "DeltaX", DoubleValue (3.0),
                                   "DeltaY", DoubleValue (3.0),
                                   "GridWidth", UintegerValue (10),
                                   "LayoutType", StringValue ("RowFirst"));
      
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (nodes);
    
    Simulator::Run ();
    Simulator::Destroy ();

    if (m_expectedDelay == Time("-1s"))
      {
        // No packet delivery expected 
        NS_TEST_EXPECT_MSG_EQ (m_receivedPacketNumber, 0, "Number of packet received");
        NS_TEST_EXPECT_MSG_EQ (m_receivedPacketSize, 0, "Size of packet received");
      }
      else
      {
        m_averageTime = Time(m_averageTime.GetTimeStep () /  m_receivedPacketNumber);

        NS_TEST_EXPECT_MSG_EQ (m_receivedPacketNumber, 3, "Number of packet received");
        NS_TEST_EXPECT_MSG_EQ (m_receivedPacketSize, 1000, "Size of packet received");
        NS_TEST_EXPECT_MSG_EQ (m_averageTime, m_expectedDelay, "Average transmit time");
      }

    // Check some parameters provided to the delay model.
    Ptr<ConstantDelayModel> constantDelayModel = DynamicCast<ConstantDelayModel>(m_channelDelayModel);
    if (constantDelayModel)
      {
        // Check parameters; these checks are dependent on the test scenario
        NS_TEST_EXPECT_MSG_EQ (constantDelayModel -> GetSrcId (), 0, "Incorrect source ID provided to ComputeDelay");
        NS_TEST_EXPECT_MSG_EQ (constantDelayModel -> GetDstId (), 1, "Incorrect dst provided to ComputeDelay");
        NS_TEST_EXPECT_MSG_EQ (constantDelayModel -> GetSrcPosition (), Vector (0,0,0),
                               "Incorrect source position provided to ComputeDelay");
      }

  }

  /**
   * Receive a packet callback
   * \param packet The packet
   * \param from Address of the sender
   */
  void ReceivePkt (Ptr<const Packet> packet, const Address &from)
  {
    if (packet)
    {
      m_receivedPacketSize = packet->GetSize ();
      m_receivedPacketNumber++;
      m_averageTime += Simulator::Now () - m_sentTime;
    }
  }

  /**
   * Send a packet callback
   * \param packet The packet
   * \param from Address of the sender
   */
  void SendPkt (Ptr<const Packet> packet, const Address &from)
  {
    if (packet)
      {
        m_sentTime = Simulator::Now ();
      }
  }

  void SetChannelDelay(Time delay)
  {
    m_channelDelay = delay;
  }

  void SetChannelDataRate(DataRate rate)
  {
    m_channelDataRate = rate;
  }

  void SetChannelDistance(double distance)
  {
    m_channelDistance = distance;
  }

  void SetExpectedDelay(Time delay)
  {
    m_expectedDelay = delay;
  }

  void SetChannelDelayModel(Ptr<ChannelDelayModel> delayModel)
  {
    m_channelDelayModel = delayModel;
  }

  void SetNetDeviceDelay(Time delay)
  {
    m_netDeviceDelay = delay;
  }

  void SetNetDeviceDataRate(DataRate rate)
  {
    m_netDeviceDataRate = rate;
  }

  void SetNetDeviceInterframeGap(Time gap)
  {
    m_netDeviceGap = gap;
  }
  
private:
  uint32_t m_receivedPacketSize;    //!< Received packet size
  uint32_t m_receivedPacketNumber;  //!< Number of received packets

  Time m_averageTime; //!< Average delay time from send to recv.
  Time m_sentTime; //!< Time packet was sent.

  Time m_channelDelay;
  DataRate m_channelDataRate;
  double m_channelDistance;

  Time m_netDeviceDelay;
  DataRate m_netDeviceDataRate;
  Time  m_netDeviceGap;

  Time m_expectedDelay;

  Ptr<ChannelDelayModel> m_channelDelayModel;
};

/**
 * \ingroup network-test
 * \ingroup tests
 *
 * \brief Sequential SimpleDistributedNetDevice TestSuite
 */
class SimpleDistributedTestSuite : public TestSuite
{
public:
  SimpleDistributedTestSuite () : TestSuite ("simple-distributed", UNIT)
  {
    // Check channel delay
    auto testCase = new SimpleDistributedTest("channel-1");
    testCase -> SetChannelDelay(Time("100ms"));
    testCase -> SetExpectedDelay(Time("100ms"));
    AddTestCase (testCase, TestCase::QUICK);

    // Check channel delay
    testCase = new SimpleDistributedTest("channel-2");
    testCase -> SetChannelDelay(Time("10ms"));
    testCase -> SetExpectedDelay(Time("10ms"));
    AddTestCase (testCase, TestCase::QUICK);

    // Check channel data rate
    testCase = new SimpleDistributedTest("channel-3");
    testCase -> SetChannelDataRate(DataRate("1000B/s"));
    testCase -> SetExpectedDelay(Time("1s"));
    AddTestCase (testCase, TestCase::QUICK);

    // Check channel data rate
    testCase = new SimpleDistributedTest("channel-4");
    testCase -> SetChannelDataRate(DataRate("10000B/s"));
    testCase -> SetExpectedDelay(Time("0.1s"));
    AddTestCase (testCase, TestCase::QUICK);

    // Check channel data rate and delay
    testCase = new SimpleDistributedTest("channel-5");
    testCase -> SetChannelDelay(Time("10ms"));
    testCase -> SetChannelDataRate(DataRate("10000B/s"));
    testCase -> SetExpectedDelay(Time("10ms") + Time("0.1s"));
    AddTestCase (testCase, TestCase::QUICK);

    // Check with ConstantDelayModel
    testCase = new SimpleDistributedTest("channel-7");
    Ptr<ChannelDelayModel> constantDelayModel = CreateObject<ConstantDelayModel>();
    testCase -> SetChannelDelayModel (constantDelayModel);
    testCase -> SetExpectedDelay(Time("20ms"));
    AddTestCase (testCase, TestCase::QUICK);

    // Check with DistanceDelayModel
    testCase = new SimpleDistributedTest("channel-8");
    Ptr<DistanceDelayModel> distanceDelayModel = CreateObject<DistanceDelayModel>();
    testCase -> SetChannelDelayModel (distanceDelayModel);
    testCase -> SetExpectedDelay(Time("39ms"));  // 13 ms / m * 3 m
    AddTestCase (testCase, TestCase::QUICK);

    // Check with Distance cutoff; no packets expected
    testCase = new SimpleDistributedTest("channel-9");
    testCase -> SetChannelDelayModel (distanceDelayModel);
    testCase -> SetChannelDistance(2.0);
    testCase -> SetExpectedDelay(Time("-1s"));
    AddTestCase (testCase, TestCase::QUICK);

    // Check netdevice delay
    testCase = new SimpleDistributedTest("netdevice-1");
    testCase -> SetNetDeviceDelay(Time("100ms"));
    testCase -> SetExpectedDelay(Time("100ms"));
    AddTestCase (testCase, TestCase::QUICK);

    // Check netdevice data rate
    testCase = new SimpleDistributedTest("netdevice-2");
    testCase -> SetNetDeviceDataRate(DataRate("1000B/s"));
    testCase -> SetExpectedDelay(Time("1s"));
    AddTestCase (testCase, TestCase::QUICK);

    // Check netdevice data rate and delay
    testCase = new SimpleDistributedTest("netdevice-3");
    testCase -> SetNetDeviceDelay(Time("10ms"));
    testCase -> SetNetDeviceDataRate(DataRate("10000B/s"));
    testCase -> SetExpectedDelay(Time("10ms") + Time("0.1s"));
    AddTestCase (testCase, TestCase::QUICK);

    // Check interframe gap
    testCase = new SimpleDistributedTest("netdevice-4");
    testCase -> SetNetDeviceInterframeGap(Time("2s"));
    // First packet has no delay, second/third packet are delayed
    // 1s by interframe gap.  Average delay is expected to be 2/3 s
    // Note application send interval is 1s
    testCase -> SetExpectedDelay(Time("666666666ns"));
    AddTestCase (testCase, TestCase::QUICK);
  }
};

static SimpleDistributedTestSuite g_simpleDistributedTestSuite; //!< Static variable for test initialization
