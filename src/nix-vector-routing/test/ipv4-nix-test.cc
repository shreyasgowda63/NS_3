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
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/icmpv4-l4-protocol.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/ipv4-nix-vector-helper.h"

using namespace ns3;
/**
 * \ingroup nix-vector-routing
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
 * \brief IPv4 Nix-Vector Routing Test
 */
class Ipv4NixVectorRoutingTest : public TestCase
{
  Ptr<Packet> m_receivedPacket; //!< Received packet

  /**
   * \brief Send data.
   * \param socket The sending socket.
   * \param to Destination address.
   */
  void DoSendData (Ptr<Socket> socket, std::string to);
  /**
   * \brief Send data.
   * \param socket The sending socket.
   * \param to Destination address.
   */
  void SendData (Time delay, Ptr<Socket> socket, std::string to);

public:
  virtual void DoRun (void);
  Ipv4NixVectorRoutingTest ();

  /**
   * \brief Receive data.
   * \param socket The receiving socket.
   */
  void ReceivePkt (Ptr<Socket> socket);

  std::vector<uint32_t> m_receivedPacketSizes; //!< Received packet sizes
};

Ipv4NixVectorRoutingTest::Ipv4NixVectorRoutingTest ()
  : TestCase ("Nix-Vector Routing")
{
}

void Ipv4NixVectorRoutingTest::ReceivePkt (Ptr<Socket> socket)
{
  uint32_t availableData;
  availableData = socket->GetRxAvailable ();
  m_receivedPacket = socket->Recv (std::numeric_limits<uint32_t>::max (), 0);
  NS_ASSERT (availableData == m_receivedPacket->GetSize ());
  //cast availableData to void, to suppress 'availableData' set but not used
  //compiler warning
  (void) availableData;
  m_receivedPacketSizes.push_back (m_receivedPacket->GetSize ());
}

void
Ipv4NixVectorRoutingTest::DoSendData (Ptr<Socket> socket, std::string to)
{
  Address realTo = InetSocketAddress (Ipv4Address (to.c_str ()), 1234);
  NS_TEST_EXPECT_MSG_EQ (socket->SendTo (Create<Packet> (123), 0, realTo),
                         123, "100");
}

void
Ipv4NixVectorRoutingTest::SendData (Time delay, Ptr<Socket> socket, std::string to)
{
  m_receivedPacket = Create<Packet> ();
  Simulator::ScheduleWithContext (socket->GetNode ()->GetId (), delay,
                                  &Ipv4NixVectorRoutingTest::DoSendData, this, socket, to);
}

void
Ipv4NixVectorRoutingTest::DoRun (void)
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
  Ipv4NixVectorHelper nixRouting;
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

  Ipv4AddressHelper aSrcaA;
  aSrcaA.SetBase ("10.1.0.0", "255.255.255.0");
  Ipv4AddressHelper aAaB;
  aAaB.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4AddressHelper aBaC;
  aBaC.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4AddressHelper aCaDst;
  aCaDst.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4AddressHelper aAaC;
  aAaC.SetBase ("10.1.4.0", "255.255.255.0");

  aSrcaA.Assign (dSrcdA);
  aAaB.Assign (dAdB);
  aBaC.Assign (dBdC);
  Ipv4InterfaceContainer iCiDst = aCaDst.Assign (dCdDst);
  Ipv4InterfaceContainer iAiC = aAaC.Assign (dAdC);

  // Create the UDP sockets
  Ptr<SocketFactory> rxSocketFactory = nCnDst.Get (1)->GetObject<UdpSocketFactory> ();
  Ptr<Socket> rxSocket = rxSocketFactory->CreateSocket ();
  NS_TEST_EXPECT_MSG_EQ (rxSocket->Bind (InetSocketAddress (iCiDst.GetAddress (1), 1234)), 0, "trivial");
  rxSocket->SetRecvCallback (MakeCallback (&Ipv4NixVectorRoutingTest::ReceivePkt, this));

  Ptr<SocketFactory> txSocketFactory = nSrcnA.Get (0)->GetObject<UdpSocketFactory> ();
  Ptr<Socket> txSocket = txSocketFactory->CreateSocket ();
  txSocket->SetAllowBroadcast (true);

  SendData (Seconds (2), txSocket, "10.1.3.2");

  std::ostringstream stringStream1;
  Ptr<OutputStreamWrapper> routingStream1 = Create<OutputStreamWrapper> (&stringStream1);
  nixRouting.PrintRoutingPathAt (Seconds (3), nSrcnA.Get (0), iCiDst.GetAddress (1), routingStream1);

  // Set the nA interface on nA - nC channel down.
  Ptr<Ipv4> ipv4 = nAnC.Get (0)->GetObject<Ipv4> ();
  int32_t ifIndex = ipv4->GetInterfaceForDevice (dAdC.Get (0));
  Simulator::Schedule (Seconds (5), &Ipv4::SetDown, ipv4, ifIndex);

  std::ostringstream stringStream2;
  Ptr<OutputStreamWrapper> cacheStream = Create<OutputStreamWrapper> (&stringStream2);
  nixRouting.PrintRoutingTableAllAt (Seconds (7), cacheStream);

  SendData (Seconds (8), txSocket, "10.1.3.2");

  std::ostringstream stringStream3;
  Ptr<OutputStreamWrapper> routingStream3 = Create<OutputStreamWrapper> (&stringStream3);
  nixRouting.PrintRoutingPathAt (Seconds (9), nSrcnA.Get (0), iCiDst.GetAddress (1), routingStream3);

  // Set the nC interface on nB - nC channel down.
  ipv4 = nBnC.Get (1)->GetObject<Ipv4> ();
  ifIndex = ipv4->GetInterfaceForDevice (dBdC.Get (1));
  Simulator::Schedule (Seconds (10), &Ipv4::SetDown, ipv4, ifIndex);
  // This is the 3rd routing of the test and should not work.
  SendData (Seconds (11), txSocket, "10.1.3.2");

  Simulator::Stop (Seconds (66));
  Simulator::Run ();

  // ------ Now the tests ------------

  // Test the Routing
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacketSizes[0], 123, "IPv4 Nix-Vector Routing should work.");
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacketSizes.size (), 2, "IPv4 Nix-Vector Routing should have received only 1 packet.");

  // Test the Path
  const std::string path_nSrc_nA_nC_nDst = "Time: +3s, Nix Routing\n"
                                           "Route Path: (Node 0 to Node 4, Nix Vector: 01001)\n"
                                           "10.1.0.1 (Node 0)   ---->   10.1.0.2 (Node 1)\n"
                                           "10.1.4.1 (Node 1)   ---->   10.1.4.2 (Node 3)\n"
                                           "10.1.3.1 (Node 3)   ---->   10.1.3.2 (Node 4)\n\n";
  NS_TEST_EXPECT_MSG_EQ (stringStream1.str (), path_nSrc_nA_nC_nDst, "Routing Path is incorrect.");
  const std::string path_nSrc_nA_nB_nC_nDst = "Time: +9s, Nix Routing\n"
                                              "Route Path: (Node 0 to Node 4, Nix Vector: 001101)\n"
                                              "10.1.0.1 (Node 0)   ---->   10.1.0.2 (Node 1)\n"
                                              "10.1.1.1 (Node 1)   ---->   10.1.1.2 (Node 2)\n"
                                              "10.1.2.1 (Node 2)   ---->   10.1.2.2 (Node 3)\n"
                                              "10.1.3.1 (Node 3)   ---->   10.1.3.2 (Node 4)\n\n";
  NS_TEST_EXPECT_MSG_EQ (stringStream3.str (), path_nSrc_nA_nB_nC_nDst, "Routing Path is incorrect.");

  const std::string emptyCaches = "Node: 0, Time: +7s, Local time: +7s, Nix Routing\n"
                                  "NixCache:\n"
                                  "Ipv4RouteCache:\n\n"
                                  "Node: 1, Time: +7s, Local time: +7s, Nix Routing\n"
                                  "NixCache:\n"
                                  "Ipv4RouteCache:\n\n"
                                  "Node: 2, Time: +7s, Local time: +7s, Nix Routing\n"
                                  "NixCache:\n"
                                  "Ipv4RouteCache:\n\n"
                                  "Node: 3, Time: +7s, Local time: +7s, Nix Routing\n"
                                  "NixCache:\n"
                                  "Ipv4RouteCache:\n\n"
                                  "Node: 4, Time: +7s, Local time: +7s, Nix Routing\n"
                                  "NixCache:\n"
                                  "Ipv4RouteCache:\n\n";
  NS_TEST_EXPECT_MSG_EQ (stringStream2.str (), emptyCaches, "The caches should have been empty.");

  Simulator::Destroy ();
}

/**
 * \ingroup nix-vector-routing-test
 * \ingroup tests
 *
 * \brief IPv4 Nix-Vector Routing TestSuite
 */
class Ipv4NixVectorRoutingTestSuite : public TestSuite
{
public:
  Ipv4NixVectorRoutingTestSuite () : TestSuite ("ipv4-nix-vector-routing", UNIT)
  {
    AddTestCase (new Ipv4NixVectorRoutingTest, TestCase::QUICK);
  }
};

/// Static variable for test initialization
static Ipv4NixVectorRoutingTestSuite g_ipv4NixVectorRoutingTestSuite;