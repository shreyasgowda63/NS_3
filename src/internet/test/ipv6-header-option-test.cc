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

using namespace ns3;

/**
 * \brief Test class for Ipv6ExtensionType2RoutingHeader and Ipv6HomeAddressOptionHeader
 *
 * It tries to send one packet with Ipv6ExtensionType2RoutingHeader and Ipv6HomeAddressOptionHeader from one NetDevice to another, over a
 * PointToPointChannel.
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
   * \param device NetDevice to send to.
   * \param buffer Payload content of the packet.
   * \param size Size of the payload.
   */
  void SendOnePacket (Ptr<PointToPointNetDevice> device, uint8_t const *buffer, uint32_t size);
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
Ipv6HeaderOptionTest::SendOnePacket (Ptr<PointToPointNetDevice> device, uint8_t const *buffer, uint32_t size)
{
  Ptr<Packet> p = Create<Packet> (buffer, size);
  Ipv6ExtensionType2RoutingHeader type2extn;
  type2extn.SetHomeAddress ("2001:db80::1");
  p->AddHeader (type2extn);

  Ipv6ExtensionDestinationHeader destextnhdr;
  Ipv6HomeAddressOptionHeader homeopt;
  homeopt.SetHomeAddress ("2001:db80::1");
  destextnhdr.AddOption (homeopt);

  destextnhdr.SetNextHeader (59);
  p->AddHeader (destextnhdr);

  device->Send (p, device->GetBroadcast (), 0x800);
}

bool
Ipv6HeaderOptionTest::RxPacket (Ptr<NetDevice> dev, Ptr<const Packet> pkt, uint16_t mode, const Address &sender)
{
  m_recvdPacket = pkt;

  Ptr <Packet> packet = pkt->Copy ();
  Ipv6ExtensionDestinationHeader dest;
  packet->RemoveHeader (dest);

  Buffer buf;

  Buffer::Iterator start;
  buf = dest.GetOptionBuffer ();
  start = buf.Begin ();
  Ipv6HomeAddressOptionHeader homopt;
  homopt.Deserialize (start);

  NS_TEST_EXPECT_MSG_EQ (homopt.GetHomeAddress (), "2001:db80::1", "HomeAddressOption does not match");

  Ipv6ExtensionType2RoutingHeader type2; // Fetching Type2 extension header which contains home address
  packet->RemoveHeader (type2);

  NS_TEST_EXPECT_MSG_EQ (type2.GetHomeAddress (), "2001:db80::1", "Type2Routing hoa does not match");

  return true;
}


void
Ipv6HeaderOptionTest::DoRun (void)
{
  Ptr<Node> a = CreateObject<Node> ();
  Ptr<Node> b = CreateObject<Node> ();
  Ptr<PointToPointNetDevice> devA = CreateObject<PointToPointNetDevice> ();
  Ptr<PointToPointNetDevice> devB = CreateObject<PointToPointNetDevice> ();
  Ptr<PointToPointChannel> channel = CreateObject<PointToPointChannel> ();

  devA->Attach (channel);
  devA->SetAddress (Mac48Address::Allocate ());
  devA->SetQueue (CreateObject<DropTailQueue<Packet> > ());
  devB->Attach (channel);
  devB->SetAddress (Mac48Address::Allocate ());
  devB->SetQueue (CreateObject<DropTailQueue<Packet> > ());

  a->AddDevice (devA);
  b->AddDevice (devB);
  
  devB->SetReceiveCallback (MakeCallback (&Ipv6HeaderOptionTest::RxPacket,
                                          this));
  uint8_t txBuffer [] = "\"Can you tell me where my country lies?\" \\ said the unifaun to his true love's eyes. \\ \"It lies with me!\" cried the Queen of Maybe \\ - for her merchandise, he traded in his prize.";
  size_t txBufferSize = sizeof(txBuffer);
  
  Simulator::Schedule (Seconds (1.0), &Ipv6HeaderOptionTest::SendOnePacket, this, devA, txBuffer, txBufferSize);

  Simulator::Run ();
  
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
