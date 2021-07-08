/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 NITK Surathkal
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
 * Author: Ameya Deshpande <ameyanrd@outlook.com>
 */

#include "ns3/test.h"
#include "ns3/socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"

#include "ns3/internet-stack-helper.h"
#include "ns3/ipv6-address-helper.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/icmpv6-l4-protocol.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/nix-vector-helper.h"

using namespace ns3;
/**
 * \defgroup nix-vector-routing-test Nix-Vector Routing Tests
 */

/**
 * \ingroup nix-vector-routing-test
 * \ingroup tests
 *
 * The topology is of the form:
 * \verbatim
              __________
             /          \
    nSrc -- nA -- nB -- nC -- nDst
   \endverbatim
 *
 * Following are the tests in this test case:
 * - Test the routing from nSrc to nDst.
 * - Test if the path taken is the shortest path.
 * (Set down the interface of nA on nA-nC channel.)
 * - Test if the NixCache and Ipv6RouteCache are empty.
 * - Test the routing from nSrc to nDst again.
 * - Test if the new shortest path is taken.
 * (Set down the interface of nC on nB-nC channel.)
 * - Test that routing is not possible from nSrc to nDst.
 *
 * \brief IPv6 Nix-Vector Routing Test
 */
class Ipv6NixVectorRoutingTest : public TestCase
{
  Ptr<Packet> m_receivedPacket; //!< Received packet

  /**
   * \brief Send data immediately after being called.
   * \param socket The sending socket.
   * \param to Destination address.
   */
  void DoSendData (Ptr<Socket> socket, std::string to);
  /**
   * \brief Schedules the DoSendData () function to send the data.
   * \param delay The scheduled time to send data.
   * \param socket The sending socket.
   * \param to Destination address.
   */
  void SendData (Time delay, Ptr<Socket> socket, std::string to);

public:
  virtual void DoRun (void);
  Ipv6NixVectorRoutingTest ();

  /**
   * \brief Receive data.
   * \param socket The receiving socket.
   */
  void ReceivePkt (Ptr<Socket> socket);

  std::vector<uint32_t> m_receivedPacketSizes; //!< Received packet sizes
};

Ipv6NixVectorRoutingTest::Ipv6NixVectorRoutingTest ()
  : TestCase ("Nix-Vector Routing")
{
}

void Ipv6NixVectorRoutingTest::ReceivePkt (Ptr<Socket> socket)
{
  uint32_t availableData;
  availableData = socket->GetRxAvailable ();
  m_receivedPacket = socket->Recv (std::numeric_limits<uint32_t>::max (), 0);
  NS_TEST_ASSERT_MSG_EQ (availableData, m_receivedPacket->GetSize (),
                         "availableData should be equal to the size of packet received.");
  NS_UNUSED (availableData);
  m_receivedPacketSizes.push_back (m_receivedPacket->GetSize ());
}

void
Ipv6NixVectorRoutingTest::DoSendData (Ptr<Socket> socket, std::string to)
{
  std::cout << "Now....\n";
  Address realTo = Inet6SocketAddress (Ipv6Address (to.c_str ()), 1234);
  socket->SendTo (Create<Packet> (123), 0, realTo);
}

void
Ipv6NixVectorRoutingTest::SendData (Time delay, Ptr<Socket> socket, std::string to)
{
  m_receivedPacket = Create<Packet> ();
  Simulator::ScheduleWithContext (socket->GetNode ()->GetId (), delay,
                                  &Ipv6NixVectorRoutingTest::DoSendData, this, socket, to);
}

void
Ipv6NixVectorRoutingTest::DoRun (void)
{
  // Create topology
  NodeContainer nSrcnA;
  NodeContainer nAnB;
  NodeContainer nBnC;
  NodeContainer nCnDst;
  NodeContainer nAnC;

  nSrcnA.Create (2);

  nAnB.Add (nSrcnA.Get (1));
  nAnB.Create (1);

  nBnC.Add (nAnB.Get (1));
  nBnC.Create (1);

  nCnDst.Add (nBnC.Get (1));
  nCnDst.Create (1);

  nAnC.Add (nAnB.Get (0));
  nAnC.Add (nCnDst.Get (0));

  SimpleNetDeviceHelper devHelper;
  devHelper.SetNetDevicePointToPointMode (true);

  NodeContainer allNodes = NodeContainer (nSrcnA, nBnC, nCnDst.Get (1));

  // NixHelper to install nix-vector routing on all nodes
  Ipv6NixVectorHelper nixRouting;
  InternetStackHelper stack;
  stack.SetRoutingHelper (nixRouting); // has effect on the next Install ()
  stack.Install (allNodes);

  NetDeviceContainer dSrcdA;
  NetDeviceContainer dAdB;
  NetDeviceContainer dBdC;
  NetDeviceContainer dCdDst;
  NetDeviceContainer dAdC;
  dSrcdA = devHelper.Install (nSrcnA);
  dAdB = devHelper.Install (nAnB);
  dBdC = devHelper.Install (nBnC);
  dCdDst = devHelper.Install (nCnDst);
  dAdC = devHelper.Install (nAnC);

  Ipv6AddressHelper aSrcaA;
  aSrcaA.SetBase (Ipv6Address ("2001:0::"), Ipv6Prefix (64));
  Ipv6AddressHelper aAaB;
  aAaB.SetBase (Ipv6Address ("2001:1::"), Ipv6Prefix (64));
  Ipv6AddressHelper aBaC;
  aBaC.SetBase (Ipv6Address ("2001:2::"), Ipv6Prefix (64));
  Ipv6AddressHelper aCaDst;
  aCaDst.SetBase (Ipv6Address ("2001:3::"), Ipv6Prefix (64));
  Ipv6AddressHelper aAaC;
  aAaC.SetBase (Ipv6Address ("2001:4::"), Ipv6Prefix (64));

  aSrcaA.Assign (dSrcdA);
  aAaB.Assign (dAdB);
  aBaC.Assign (dBdC);
  Ipv6InterfaceContainer iCiDst = aCaDst.Assign (dCdDst);
  Ipv6InterfaceContainer iAiC = aAaC.Assign (dAdC);

  // Create the UDP sockets
  Ptr<SocketFactory> rxSocketFactory = nCnDst.Get (1)->GetObject<UdpSocketFactory> ();
  Ptr<Socket> rxSocket = rxSocketFactory->CreateSocket ();
  NS_TEST_EXPECT_MSG_EQ (rxSocket->Bind (Inet6SocketAddress (iCiDst.GetAddress (1, 1), 1234)), 0, "trivial");
  rxSocket->SetRecvCallback (MakeCallback (&Ipv6NixVectorRoutingTest::ReceivePkt, this));

  Ptr<SocketFactory> txSocketFactory = nSrcnA.Get (0)->GetObject<UdpSocketFactory> ();
  Ptr<Socket> txSocket = txSocketFactory->CreateSocket ();
  txSocket->SetAllowBroadcast (true);

  SendData (Seconds (2), txSocket, "2001:3::200:ff:fe00:8");

  std::ostringstream stringStream1;
  Ptr<OutputStreamWrapper> routingStream1 = Create<OutputStreamWrapper> (&stringStream1);
  nixRouting.PrintRoutingPathAt (Seconds (3), nSrcnA.Get (0), iCiDst.GetAddress (1, 1), routingStream1);

  // Set the nA interface on nA - nC channel down.
  Ptr<Ipv6> ipv6 = nAnC.Get (0)->GetObject<Ipv6> ();
  int32_t ifIndex = ipv6->GetInterfaceForDevice (dAdC.Get (0));
  Simulator::Schedule (Seconds (5), &Ipv6::SetDown, ipv6, ifIndex);

  std::ostringstream stringStream2;
  Ptr<OutputStreamWrapper> cacheStream = Create<OutputStreamWrapper> (&stringStream2);
  nixRouting.PrintRoutingTableAllAt (Seconds (7), cacheStream);

  SendData (Seconds (8), txSocket, "2001:3::200:ff:fe00:8");

  std::ostringstream stringStream3;
  Ptr<OutputStreamWrapper> routingStream3 = Create<OutputStreamWrapper> (&stringStream3);
  nixRouting.PrintRoutingPathAt (Seconds (9), nSrcnA.Get (0), iCiDst.GetAddress (1, 1), routingStream3);

  // Set the nC interface on nB - nC channel down.
  ipv6 = nBnC.Get (1)->GetObject<Ipv6> ();
  ifIndex = ipv6->GetInterfaceForDevice (dBdC.Get (1));
  Simulator::Schedule (Seconds (10), &Ipv6::SetDown, ipv6, ifIndex);
  // This is the 3rd routing of the test and should not work.
  SendData (Seconds (11), txSocket, "2001:3::200:ff:fe00:8");

  Simulator::Stop (Seconds (66));
  Simulator::Run ();

  // ------ Now the tests ------------

  // Test the Routing
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacketSizes[0], 123, "IPv6 Nix-Vector Routing should work.");
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacketSizes.size (), 2, "IPv6 Nix-Vector Routing should have received only 1 packet.");

  // Test the Path
  const std::string path_nSrc_nA_nC_nDst = "Time: +3s, Nix Routing\n"
                                           "Route Path: (Node 0 to Node 4, Nix Vector: 01001)\n"
                                           "2001::200:ff:fe00:1      (Node 0)  ---->   fe80::200:ff:fe00:2      (Node 1)\n"
                                           "fe80::200:ff:fe00:9      (Node 1)  ---->   fe80::200:ff:fe00:a      (Node 3)\n"
                                           "fe80::200:ff:fe00:7      (Node 3)  ---->   2001:3::200:ff:fe00:8    (Node 4)\n\n";
  NS_TEST_EXPECT_MSG_EQ (stringStream1.str (), path_nSrc_nA_nC_nDst, "Routing Path is incorrect.");
  const std::string path_nSrc_nA_nB_nC_nDst = "Time: +9s, Nix Routing\n"
                                              "Route Path: (Node 0 to Node 4, Nix Vector: 001101)\n"
                                              "2001::200:ff:fe00:1      (Node 0)  ---->   fe80::200:ff:fe00:2      (Node 1)\n"
                                              "fe80::200:ff:fe00:3      (Node 1)  ---->   fe80::200:ff:fe00:4      (Node 2)\n"
                                              "fe80::200:ff:fe00:5      (Node 2)  ---->   fe80::200:ff:fe00:6      (Node 3)\n"
                                              "fe80::200:ff:fe00:7      (Node 3)  ---->   2001:3::200:ff:fe00:8    (Node 4)\n\n";
  NS_TEST_EXPECT_MSG_EQ (stringStream3.str (), path_nSrc_nA_nB_nC_nDst, "Routing Path is incorrect.");

  const std::string emptyCaches = "Node: 0, Time: +7s, Local time: +7s, Nix Routing\n"
                                  "NixCache:\n"
                                  "IpRouteCache:\n\n"
                                  "Node: 1, Time: +7s, Local time: +7s, Nix Routing\n"
                                  "NixCache:\n"
                                  "IpRouteCache:\n\n"
                                  "Node: 2, Time: +7s, Local time: +7s, Nix Routing\n"
                                  "NixCache:\n"
                                  "IpRouteCache:\n\n"
                                  "Node: 3, Time: +7s, Local time: +7s, Nix Routing\n"
                                  "NixCache:\n"
                                  "IpRouteCache:\n\n"
                                  "Node: 4, Time: +7s, Local time: +7s, Nix Routing\n"
                                  "NixCache:\n"
                                  "IpRouteCache:\n\n";
  NS_TEST_EXPECT_MSG_EQ (stringStream2.str (), emptyCaches, "The caches should have been empty.");

  Simulator::Destroy ();
}

/**
 * \ingroup nix-vector-routing-test
 * \ingroup tests
 *
 * \brief IPv6 Nix-Vector Routing TestSuite
 */
class Ipv6NixVectorRoutingTestSuite : public TestSuite
{
public:
  Ipv6NixVectorRoutingTestSuite () : TestSuite ("ipv6-nix-vector-routing", UNIT)
  {
    AddTestCase (new Ipv6NixVectorRoutingTest, TestCase::QUICK);
  }
};

/// Static variable for test initialization
static Ipv6NixVectorRoutingTestSuite g_Ipv6NixVectorRoutingTestSuite;