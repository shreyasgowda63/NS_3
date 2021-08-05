/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 INRIA
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
 * Author: Zakaria Helal Arzoo <arzoozakaria@gmail.com>
 */

#include "ns3/test.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/simulator.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/net-device-queue-interface.h"

#include "ns3/ipv6-extension-header.h"
#include "ns3/ipv6-option-header.h"

#include <string>

#include "ns3/ipv6-address-helper.h"
#include "ns3/simple-net-device.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/icmpv6-header.h"
#include "ns3/socket.h"
#include "ns3/socket-factory.h"
#include "ns3/uinteger.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv6-routing-helper.h"
#include "ns3/node.h"
#include "ns3/internet-stack-helper.h"

using namespace ns3;

/**
 * \brief Test class for Ipv6ExtensionType2RoutingHeader and Ipv6HomeAddressOptionHeader
 *
 * It tries to send one packet with Ipv6ExtensionType2RoutingHeader and Ipv6HomeAddressOptionHeader from one NetDevice to another and checks if proper headers and options are present
 */
class Ipv6HeaderOptionTest : public TestCase
{
public:
  /**
   * \brief Create the test
   */
  Ipv6HeaderOptionTest ();

  /**
   * \brief Run the test
   */
  virtual void DoRun (void);

private:
  
  Ptr<const Packet> m_recvdPacket; //!< received packet
  /**
   * \brief Send one packet with Ipv6ExtensionType2RoutingHeader and Ipv6HomeAddressOptionHeader to the device specified
   *
   * \param dev NetDevice to send to.
   */
  void SendOnePacket (Ptr<NetDevice> dev);

  /**
   * \brief Callback function which sets the recvdPacket parameter and tests if correct home address present
   *
   * \param dev The receiving device.
   * \param pkt The received packet.
   * \param mode The protocol mode used.
   * \param sender The sender address.
   * 
   * \return A boolean indicating packet handled properly.
   */
  bool RxPacket (Ptr<NetDevice> dev, Ptr<const Packet> pkt, uint16_t mode, const Address &sender);
};

Ipv6HeaderOptionTest::Ipv6HeaderOptionTest ()
  : TestCase ("Ipv6HeaderOptionTest")
{
}

void
Ipv6HeaderOptionTest::SendOnePacket (Ptr<NetDevice> dev)
{

  Ptr<Packet> p = Create<Packet> ();
  Ipv6ExtensionType2RoutingHeader type2extn;
  type2extn.SetHomeAddress ("2001:db80::1");
  p->AddHeader (type2extn);

  Ipv6ExtensionDestinationHeader destextnhdr;
  Ipv6HomeAddressOptionHeader homeopt;
  homeopt.SetHomeAddress ("2001:db80::1");
  destextnhdr.AddOption (homeopt);

  destextnhdr.SetNextHeader (59);
  p->AddHeader (destextnhdr);

  NS_TEST_EXPECT_MSG_EQ (dev->Send (p, dev->GetBroadcast(), 0x800), 1, "Sending failed");
}


bool
Ipv6HeaderOptionTest::RxPacket (Ptr<NetDevice> dev, Ptr<const Packet> pkt, uint16_t mode, const Address &sender)
{
  m_recvdPacket = pkt->Copy ();
  Ptr<Packet> p = pkt->Copy ();

  Ipv6ExtensionDestinationHeader dest;
  p->RemoveHeader (dest);

  Buffer buf;

  Buffer::Iterator start;
  buf = dest.GetOptionBuffer ();
  start = buf.Begin ();
  Ipv6HomeAddressOptionHeader homopt;
  homopt.Deserialize (start);

  NS_TEST_EXPECT_MSG_EQ (homopt.GetHomeAddress (), "2001:db80::1", "HomeAddressOption does not match");

  NS_TEST_EXPECT_MSG_EQ (dest.GetNextHeader (), 59, "The received Packet does not have Type2Routing Header");

  Ipv6ExtensionType2RoutingHeader type2; // Fetching Type2 extension header which contains home address
  p->RemoveHeader (type2);

  NS_TEST_EXPECT_MSG_EQ (type2.GetHomeAddress (), "2001:db80::1", "Type2Routing hoa does not match");

  return true;
}


void
Ipv6HeaderOptionTest::DoRun (void)
{

  NodeContainer n;
  n.Create (2);

  InternetStackHelper internet;
  internet.Install (n);

  // link the two nodes
  Ptr<SimpleNetDevice> txDev = CreateObject<SimpleNetDevice> ();
  Ptr<SimpleNetDevice> rxDev = CreateObject<SimpleNetDevice> ();
  txDev->SetAddress (Mac48Address ("00:00:00:00:00:01"));
  rxDev->SetAddress (Mac48Address ("00:00:00:00:00:02"));
  n.Get (0)->AddDevice (txDev);
  n.Get (1)->AddDevice (rxDev);
  Ptr<SimpleChannel> channel1 = CreateObject<SimpleChannel> ();
  rxDev->SetChannel (channel1);
  txDev->SetChannel (channel1);
  NetDeviceContainer d;
  d.Add (txDev);
  d.Add (rxDev);

  rxDev->SetReceiveCallback (MakeCallback (&Ipv6HeaderOptionTest::RxPacket,
                                          this));

  Simulator::Schedule (Seconds (1.0), &Ipv6HeaderOptionTest::SendOnePacket, this, txDev);

  Simulator::Run ();

  NS_TEST_EXPECT_MSG_EQ (m_recvdPacket->GetSize (), 48, " Unexpected packet size");

  Simulator::Destroy ();
}

/**
 * \brief IPv6 Type2RoutingHeader and HomeAddressOption TestSuite
 */
class InternetHeaderOptionTestSuite : public TestSuite
{
public:
  /**
   * \brief Constructor
   */
  InternetHeaderOptionTestSuite ();
};

InternetHeaderOptionTestSuite::InternetHeaderOptionTestSuite ()
  : TestSuite ("internet-header-option", UNIT)
{
  AddTestCase (new Ipv6HeaderOptionTest, TestCase::QUICK);
}

static InternetHeaderOptionTestSuite g_internetMipv6HeaderOptionTestSuite; //!< The testsuite