/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Universita' di Firenze
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
#include "ns3/simulator.h"
#include "ns3/csma-net-device.h"
#include "ns3/csma-channel.h"
#include "ns3/csma-helper.h"

using namespace ns3;

/**
 * \brief Test class for Csma model
 * 
 * It does the following:
 * 
 * Create 2 CSMA networks not connected to each other. One network has 
 * 3 nodes (Network A) and the other has 2 nodes (Network B). Broadcast
 * a packet in Network from a device (device A).
 * 
 * Detach device A from Network A and try broadcasting from device A
 * again. This should fail since device is disconnected.
 * 
 * Attach device A to Network B and broadcast a packet from device A
 * in Network B.
 * 
 * Following are the expected results:
 * 
 * All devices except device A should have received one packet.
 * At the end. Network A should have only 2 nodes connected to it 
 * and Network B should have 3 nodes connected to it.
 *
 */
class CsmaTest : public TestCase
{
public:
  /**
   * \brief Create the test
   */
  CsmaTest ();

  /**
   * \brief Run the test
   */
  virtual void DoRun (void);

private:
  /**
   * \brief Send one packet to the device specified
   *
   * \param device NetDevice to send to
   */
  void SendOnePacket (Ptr<NetDevice> device);

  /**
   * Receive form a NetDevice
   * \param nd The NetDevice.
   * \param p The received packet.
   * \param protocol The protocol received.
   * \param addr The sender address.
   * \return True on success.
   */
  bool Receive (Ptr<NetDevice> nd, Ptr<const Packet> p, uint16_t protocol, const Address& addr);
  void Attach (Ptr<CsmaChannel> channel, Ptr<CsmaNetDevice> nd);
  void Detach (Ptr<CsmaChannel> channel, Ptr<CsmaNetDevice> nd);
  void Remove (Ptr<CsmaChannel> channel, Ptr<CsmaNetDevice> nd);

  std::map<Address, uint32_t> m_count; //!< Number of received packets
};

CsmaTest::CsmaTest ()
  : TestCase ("Csma")
{
}

void
CsmaTest::SendOnePacket (Ptr<NetDevice> device)
{
  Ptr<Packet> p = Create<Packet> (1450);
  device->Send (p, device->GetBroadcast (), 0x800);
}

bool
CsmaTest::Receive (Ptr<NetDevice> nd, Ptr<const Packet> p, uint16_t protocol, const Address& addr)
{
  m_count[nd->GetAddress ()]++;

  return true;
}

void
CsmaTest::Attach (Ptr<CsmaChannel> channel, Ptr<CsmaNetDevice> nd)
{
  nd->Attach (channel);
  return;
}

void
CsmaTest::Detach (Ptr<CsmaChannel> channel, Ptr<CsmaNetDevice> nd)
{
  channel->Detach (nd);
  return;
}

void
CsmaTest::Remove (Ptr<CsmaChannel> channel, Ptr<CsmaNetDevice> nd)
{
  channel->Remove (nd);
  return;
}

void
CsmaTest::DoRun (void)
{
  NodeContainer netA;
  NodeContainer netB;

  netA.Create (3);
  netB.Create (2);

  Ptr<Node> moving = netA.Get (2);

  CsmaHelper csmaA;
  CsmaHelper csmaB;

  NetDeviceContainer devsA = csmaA.Install (netA);
  NetDeviceContainer devsB = csmaB.Install (netB);

  devsA.Get (0)->SetReceiveCallback (MakeCallback (&CsmaTest::Receive, this));
  devsA.Get (1)->SetReceiveCallback (MakeCallback (&CsmaTest::Receive, this));
  devsA.Get (2)->SetReceiveCallback (MakeCallback (&CsmaTest::Receive, this));
  devsB.Get (0)->SetReceiveCallback (MakeCallback (&CsmaTest::Receive, this));
  devsB.Get (1)->SetReceiveCallback (MakeCallback (&CsmaTest::Receive, this));

  Ptr<CsmaChannel> channelA = DynamicCast<CsmaChannel> (devsA.Get (0)->GetChannel ());
  Ptr<CsmaChannel> channelB = DynamicCast<CsmaChannel> (devsB.Get (0)->GetChannel ());
  Ptr<CsmaNetDevice> deviceA = DynamicCast<CsmaNetDevice> (devsA.Get (0));

  // First transmission, send to 2 devices on Channel A
  Simulator::Schedule (Seconds (1.0), &CsmaTest::SendOnePacket, this, devsA.Get (0));

  // Second transmission, send to 2 devices on Channel A - fails because the devices is removed
  Simulator::Schedule (Seconds (2.0), &CsmaTest::SendOnePacket, this, devsA.Get (0));
  Simulator::Schedule (Seconds (2.0) + MicroSeconds (1), &CsmaTest::Remove, this, channelA, deviceA);

  // Third transmission, send to 2 devices on Channel B
  Simulator::Schedule (Seconds (3.5), &CsmaTest::Attach, this, channelB, deviceA);
  Simulator::Schedule (Seconds (4.0), &CsmaTest::SendOnePacket, this, devsA.Get (0));

  Simulator::Run ();

  Simulator::Destroy ();

  NS_TEST_ASSERT_MSG_EQ (m_count[devsA.Get (1)->GetAddress ()], 1, "Wrong number of received packets on device 1 - channel A");
  NS_TEST_ASSERT_MSG_EQ (m_count[devsA.Get (2)->GetAddress ()], 1, "Wrong number of received packets on device 2 - channel A");
  NS_TEST_ASSERT_MSG_EQ (m_count[devsB.Get (0)->GetAddress ()], 1, "Wrong number of received packets on device 0 - channel B");
  NS_TEST_ASSERT_MSG_EQ (m_count[devsB.Get (1)->GetAddress ()], 1, "Wrong number of received packets on device 1 - channel B");
  NS_TEST_ASSERT_MSG_EQ (channelA->GetNDevices (), 2, "Wrong number of devices on channel A");
  NS_TEST_ASSERT_MSG_EQ (channelB->GetNDevices (), 3, "Wrong number of devices on channel B");
}

/**
 * \brief TestSuite for Csma module
 */
class CsmaTestSuite : public TestSuite
{
public:
  /**
   * \brief Constructor
   */
  CsmaTestSuite ();
};

CsmaTestSuite::CsmaTestSuite ()
  : TestSuite ("devices-csma", UNIT)
{
  AddTestCase (new CsmaTest, TestCase::QUICK);
}

static CsmaTestSuite g_csmaTestSuite; //!< The testsuite
