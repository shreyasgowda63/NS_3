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
 * \ingroup internet-test
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
  void SendData (Ptr<Socket> socket, std::string to);

public:
  virtual void DoRun (void);
  Ipv4NixVectorRoutingTest ();

  /**
   * \brief Receive data.
   * \param socket The receiving socket.
   */
  void ReceivePkt (Ptr<Socket> socket);
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
}

void
Ipv4NixVectorRoutingTest::DoSendData (Ptr<Socket> socket, std::string to)
{
  Address realTo = InetSocketAddress (Ipv4Address (to.c_str ()), 1234);
  NS_TEST_EXPECT_MSG_EQ (socket->SendTo (Create<Packet> (123), 0, realTo),
                         123, "100");
}

void
Ipv4NixVectorRoutingTest::SendData (Ptr<Socket> socket, std::string to)
{
  m_receivedPacket = Create<Packet> ();
  Simulator::ScheduleWithContext (socket->GetNode ()->GetId (), Seconds (60),
                                  &Ipv4NixVectorRoutingTest::DoSendData, this, socket, to);
  Simulator::Stop (Seconds (66));
  Simulator::Run ();
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

  // NixHelper to install nix-vector routing
  // on all nodes
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

  SendData (txSocket, "10.1.3.2");
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket->GetSize (), 123, "IPv4 Nix-Vector Routing should work.");

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