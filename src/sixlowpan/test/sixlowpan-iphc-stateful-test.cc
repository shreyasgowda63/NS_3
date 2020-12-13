/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Universita' di Firenze, Italy
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
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "ns3/test.h"
#include "ns3/socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/boolean.h"

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv6-address-helper.h"
#include "ns3/icmpv6-l4-protocol.h"

#include "ns3/sixlowpan-net-device.h"
#include "ns3/sixlowpan-helper.h"
#include "ns3/mock-net-device.h"

#include <string>
#include <limits>
#include <vector>

using namespace ns3;

/**
 * \ingroup sixlowpan
 * \defgroup sixlowpan-test 6LoWPAN module tests
 */


/**
 * \ingroup sixlowpan-test
 * \ingroup tests
 *
 * \brief 6LoWPAN IPHC stateful compression Test
 */
class SixlowpanIphcStatefulImplTest : public TestCase
{
  std::vector<Ptr<Packet>> m_txPackets; //!< Transmitted packets
  std::vector<Ptr<Packet>> m_rxPackets; //!< Received packets

  bool ReceiveFromMockDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                              Address const &source, Address const &destination, NetDevice::PacketType packetType);

  bool PromiscReceiveFromSixLowPanDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                                          Address const &source, Address const &destination, NetDevice::PacketType packetType);

  void SendOnePacket (NetDeviceContainer devices, Ipv6Address from, Ipv6Address to);

public:
  virtual void DoRun (void);
  SixlowpanIphcStatefulImplTest ();
};

SixlowpanIphcStatefulImplTest::SixlowpanIphcStatefulImplTest ()
  : TestCase ("Sixlowpan IPHC stateful implementation")
{
}

bool
SixlowpanIphcStatefulImplTest::ReceiveFromMockDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                                                      Address const &source, Address const &destination, NetDevice::PacketType packetType)
{
  std::cout << "MockDevice Received at " << device << " from " << source << " to " << destination << " - " << *packet << std::endl;

  m_txPackets.push_back(packet);

  Ptr<Packet> pkt = packet->Copy ();
  Ptr<MockNetDevice> mockDev = DynamicCast<MockNetDevice> (device);
  if (mockDev)
    {
      uint32_t id = mockDev->GetNode ()->GetId ();
      Simulator::ScheduleWithContext (id, Time(1), &MockNetDevice::Receive, mockDev, pkt, protocol, destination, source, packetType);
    }
  return true;
}

bool
SixlowpanIphcStatefulImplTest::PromiscReceiveFromSixLowPanDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                                                                  Address const &source, Address const &destination, NetDevice::PacketType packetType)
{
  std::cout << "SixLowPanDevice Promisc Received at " << device << " from " << source << " to " << destination << " - " << *packet << std::endl;

  m_rxPackets.push_back(packet);

  return true;
}

void
SixlowpanIphcStatefulImplTest::SendOnePacket (NetDeviceContainer devices, Ipv6Address from, Ipv6Address to)
{
  Ptr<Packet> pkt = Create<Packet> (10);
  Ipv6Header ipHdr;
  ipHdr.SetSourceAddress (from);
  ipHdr.SetDestinationAddress (to);
  ipHdr.SetHopLimit (64);
  ipHdr.SetPayloadLength (10);
  ipHdr.SetNextHeader (0xff);
  pkt->AddHeader (ipHdr);

  devices.Get (0)->Send (pkt, Mac48Address ("00:00:00:00:00:02"), 0);
//   devices.Get (0)->Send (pkt, Mac48Address::ConvertFrom (devices.Get (1)->GetAddress ()), 0);
}

void
SixlowpanIphcStatefulImplTest::DoRun (void)
{
  NodeContainer nodes;
  nodes.Create(1);
  Ptr<Node> node = nodes.Get (0);

  Ptr<MockNetDevice> netDevice = CreateObject<MockNetDevice> ();
  node->AddDevice (netDevice);
  netDevice->SetNode (node);
  netDevice->SetAddress (Mac48Address ("00:00:00:00:00:01"));
  netDevice->SetMtu (150);
  netDevice->SetSendCallback ( MakeCallback (&SixlowpanIphcStatefulImplTest::ReceiveFromMockDevice, this) );
  NetDeviceContainer mockDevices;
  mockDevices.Add (netDevice);

  InternetStackHelper internetv6;
  internetv6.Install (nodes);

  SixLowPanHelper sixlowpan;
  NetDeviceContainer devices = sixlowpan.Install (mockDevices);
  devices.Get (0)->SetPromiscReceiveCallback ( MakeCallback (&SixlowpanIphcStatefulImplTest::PromiscReceiveFromSixLowPanDevice, this) );

  Ipv6AddressHelper ipv6;
  ipv6.SetBase (Ipv6Address ("2001:2::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer deviceInterfaces;
  deviceInterfaces = ipv6.Assign (devices);

  std::cout << "Device 0: address 0 " << Mac48Address::ConvertFrom (devices.Get (0)->GetAddress ()) << " -> "  << deviceInterfaces.GetAddress (0, 0) << std::endl;
  std::cout << "Device 0: address 1 " << Mac48Address::ConvertFrom (devices.Get (0)->GetAddress ()) << " -> "  << deviceInterfaces.GetAddress (0, 1) << std::endl;


  // This is a hack to prevent Router Solicitations and Duplicate Address Detection being sent.
  for (auto i = nodes.Begin (); i != nodes.End (); i++)
    {
      Ptr<Node> node = *i;
      Ptr<Ipv6L3Protocol> ipv6L3 = (*i)->GetObject<Ipv6L3Protocol> ();
      if (ipv6L3)
        {
          ipv6L3->SetAttribute ("IpForward", BooleanValue (true));
          ipv6L3->SetAttribute ("SendIcmpv6Redirect", BooleanValue (false));
        }
      Ptr<Icmpv6L4Protocol> icmpv6 = (*i)->GetObject<Icmpv6L4Protocol> ();
      if (icmpv6)
        {
          icmpv6->SetAttribute ("DAD", BooleanValue (false));
        }
    }

  sixlowpan.AddContext (devices, 0, Ipv6Prefix ("2001:2::", 64), Time (Minutes (30)));
  sixlowpan.AddContext (devices, 1, Ipv6Prefix ("2001:1::", 64), Time (Minutes (30)));

//  Simulator::Schedule (Seconds (1), SendOnePacket, devices, deviceInterfaces,
//                       Ipv6Address::GetAny (),
//                       deviceInterfaces.GetAddress (1, 1));

  Simulator::Schedule (Seconds (2), &SixlowpanIphcStatefulImplTest::SendOnePacket, this, devices,
                       deviceInterfaces.GetAddress (0, 1),
                       Ipv6Address ("2001:1::0000:00ff:fe00:cafe"));

  Simulator::Schedule (Seconds (4), &SixlowpanIphcStatefulImplTest::SendOnePacket, this, devices,
                       deviceInterfaces.GetAddress (0, 1),
                       Ipv6Address ("2001:1::f00d:f00d:cafe:cafe"));

  // 64-bit inline source address test is not possible because LrWpanNetDevice can not send packets using the 64-bit address.

  Simulator::Stop (Seconds (10));

  Simulator::Run ();
  Simulator::Destroy ();


  // ------ Now the tests ------------

  // Unicast test
//  SendData (txSocket, "2001:0100::1");
//  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket->GetSize (), 180, "trivial");
//  uint8_t rxBuffer [180];
//  uint8_t txBuffer [180] = "\"Can you tell me where my country lies?\" \\ said the unifaun to his true love's eyes. \\ \"It lies with me!\" cried the Queen of Maybe \\ - for her merchandise, he traded in his prize.";
//  m_receivedPacket->CopyData (rxBuffer, 180);
//  NS_TEST_EXPECT_MSG_EQ (memcmp (rxBuffer, txBuffer, 180), 0, "trivial");
//
//  m_receivedPacket->RemoveAllByteTags ();
//
//  Simulator::Destroy ();

}


/**
 * \ingroup sixlowpan-test
 * \ingroup tests
 *
 * \brief 6LoWPAN IPHC TestSuite
 */
class SixlowpanIphcStatefulTestSuite : public TestSuite
{
public:
  SixlowpanIphcStatefulTestSuite ();
private:
};

SixlowpanIphcStatefulTestSuite::SixlowpanIphcStatefulTestSuite ()
  : TestSuite ("sixlowpan-iphc-stateful", UNIT)
{
  AddTestCase (new SixlowpanIphcStatefulImplTest (), TestCase::QUICK);
}

static SixlowpanIphcStatefulTestSuite g_sixlowpanIphcStatefulTestSuite; //!< Static variable for test initialization
