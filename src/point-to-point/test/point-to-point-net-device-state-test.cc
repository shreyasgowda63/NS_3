/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 NITK Surathkal
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
 * Author: Ananthakrishnan S <ananthakrishnan190@gmail.com>
 */

#include "ns3/test.h"
#include "ns3/simulator.h"
#include "ns3/address.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

/**
 * \brief Test class for device state functionality of
 *        point-to-point net device.
 *
 * Several test cases are tried here to ensure that device states
 * change as expected regardless of the order in which admin and 
 * operational states are changed.
 * 
 */
class PointToPointNetDeviceStateTest : public TestCase
{
public:
  PointToPointNetDeviceStateTest ();

  virtual void DoRun (void);

private:

  uint32_t m_count;

  void SendOnePacket (Ptr<PointToPointNetDevice> from, Ptr<PointToPointNetDevice> to, bool checkCondition);
  bool IsOperational (Ptr<PointToPointNetDeviceState> state);
  bool Receive (Ptr<NetDevice> nd, Ptr<const Packet> p, uint16_t protocol, const Address& addr);
  void CheckReceived (uint32_t checkCount);
  void CheckSent ();

};

PointToPointNetDeviceStateTest::PointToPointNetDeviceStateTest ()
  : TestCase ("Tests for operational and administrative states in PointToPointNetDevice"),
    m_count (0)
{}

bool
PointToPointNetDeviceStateTest::IsOperational (Ptr<PointToPointNetDeviceState> state)
{
  return state->GetOperationalState () == NetDeviceState::IF_OPER_UP;
}

void
PointToPointNetDeviceStateTest::SendOnePacket (Ptr<PointToPointNetDevice> from, Ptr<PointToPointNetDevice> to, bool checkCondition)
{
  bool isOk = false;
  Ptr<Packet> p = Create<Packet> (1450);
  isOk = from->Send (p, to->GetAddress (), 0x800);
  NS_TEST_EXPECT_MSG_EQ (isOk, checkCondition, (checkCondition ? "Packet should have been send." : "Packet should not have send."));
}

bool
PointToPointNetDeviceStateTest::Receive (Ptr<NetDevice> nd, Ptr<const Packet> p, uint16_t protocol, const Address& addr)
{
  std::clog << "Callback is called\n";
  m_count++;
  return true;
}

void
PointToPointNetDeviceStateTest::CheckReceived (uint32_t checkCount)
{
  NS_TEST_EXPECT_MSG_EQ (m_count, checkCount, checkCount << " packets should have been received at " << Simulator::Now ().GetSeconds () << "s.");
}

void
PointToPointNetDeviceStateTest::DoRun (void)
{
  NodeContainer container;

  container.Create (2);

  PointToPointHelper p2pHelper;

  NetDeviceContainer devices = p2pHelper.Install (container);

  Ptr<PointToPointNetDeviceState> stateA = devices.Get (0)->GetObject<PointToPointNetDeviceState> ();
  Ptr<PointToPointNetDeviceState> stateB = devices.Get (1)->GetObject<PointToPointNetDeviceState> ();

  Ptr<PointToPointNetDevice> devA = DynamicCast<PointToPointNetDevice> (devices.Get (0));
  Ptr<PointToPointNetDevice> devB = DynamicCast<PointToPointNetDevice> (devices.Get (1));
  Ptr<PointToPointChannel> channel = DynamicCast<PointToPointChannel> (devA->GetChannel ());

  devB->SetReceiveCallback (MakeCallback (&PointToPointNetDeviceStateTest::Receive, this));

  NS_ASSERT (stateA);
  NS_ASSERT (stateB);

  /** Basic functionality test: Check whether deivce is UP and RUNNING.
  */
  NS_TEST_ASSERT_MSG_EQ (stateA->IsUp (), true, "Administrative state of device should be UP.");
  NS_TEST_ASSERT_MSG_EQ (IsOperational (stateA), true, "Device should be running since it is connected.");

  NS_TEST_ASSERT_MSG_EQ (stateB->IsUp (), true, "Administrative state of device should be UP.");
  NS_TEST_ASSERT_MSG_EQ (IsOperational (stateB), true, "Device should be running since it is connected.");

  /**
   * Tests on administrative state:Toggle a device on/off when it is connected to channel.
   */
  stateA->SetDown ();
  NS_TEST_EXPECT_MSG_EQ (stateA->IsUp (), false, "Device is disabled.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateA), false, "Device should not be running since it is disabled.");

  stateA->SetUp ();
  NS_TEST_EXPECT_MSG_EQ (stateA->IsUp (), true, "Device is enabled.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateA), true, "Device should be running.");

  /**
   * Test on administrative state: Toggle a device on/off while it is disconnected.
   */
  devA->Detach (channel);

  stateA->SetDown ();
  NS_TEST_EXPECT_MSG_EQ (stateA->IsUp (), false, "Device is disabled.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateA), false, "Device should not be running.");

  stateA->SetUp ();
  NS_TEST_EXPECT_MSG_EQ (stateA->IsUp (), true, "Device is enabled.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateA), false, "Device should not be running.");

  devA->Attach (channel);
  NS_TEST_EXPECT_MSG_EQ (stateA->IsUp (), true, "Device is enabled.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateA), true, "Device should be running.");

  // Detach both devices and test device states (Detach device A and then device B)

  devA->Detach (channel);
  NS_TEST_EXPECT_MSG_EQ (stateA->IsUp (), true, "Device is not disabled by user.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateA), false, "Device is detached.");

  devB->Detach (channel);
  NS_TEST_EXPECT_MSG_EQ (stateB->IsUp (), true, "Device is not disabled by user.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateB), false, "Device is detached.");

  devA->Attach (channel);
  devB->Attach (channel);

  // Detach both devices and test device states (Detach device B and then device A)

  devB->Detach (channel);
  NS_TEST_EXPECT_MSG_EQ (stateB->IsUp (), true, "Device is not disabled by user.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateB), false, "Device is detached.");

  devA->Detach (channel);
  NS_TEST_EXPECT_MSG_EQ (stateA->IsUp (), true, "Device is not disabled by user.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateA), false, "Device is detached.");

  devB->Attach (channel);
  devA->Attach (channel);

  //Test whether packet is sent when device is running and device is not running.
  Simulator::Schedule (Seconds (1.0), &PointToPointNetDeviceStateTest::SendOnePacket, this, devA, devB, true);
  Simulator::Schedule (Seconds (2.0), &PointToPointNetDeviceStateTest::CheckReceived, this, 1);
  Simulator::Schedule (Seconds (2.5), &PointToPointNetDeviceState::SetDown, stateA);
  Simulator::Schedule (Seconds (3.0), &PointToPointNetDeviceStateTest::SendOnePacket, this, devA, devB, false);

  Simulator::Schedule (Seconds (3.1), &PointToPointNetDeviceState::SetUp, stateA);
  Simulator::Schedule (Seconds (3.2), &PointToPointNetDevice::Detach, devB, channel);
  Simulator::Schedule (Seconds (3.3), &PointToPointNetDeviceStateTest::SendOnePacket, this, devA, devB, false);
  Simulator::Schedule (Seconds (3.4), &PointToPointNetDeviceStateTest::CheckReceived, this, 1);
  Simulator::Schedule (Seconds (3.5), &PointToPointNetDevice::Attach, devB, channel);

  Simulator::Schedule (Seconds (3.6), &PointToPointNetDeviceStateTest::SendOnePacket, this, devA, devB, true);
  Simulator::Schedule (Seconds (4.6), &PointToPointNetDeviceStateTest::CheckReceived, this, 2);

  Simulator::Stop (Seconds(10.0));
  Simulator::Run ();
  
  Simulator::Destroy ();

}

class PointToPointNetDeviceStateTestSuite : public TestSuite
{
public:
  PointToPointNetDeviceStateTestSuite ();
};

PointToPointNetDeviceStateTestSuite::PointToPointNetDeviceStateTestSuite ()
  : TestSuite ("states-p2p",UNIT)
{
  AddTestCase (new PointToPointNetDeviceStateTest, TestCase::QUICK);
}

static PointToPointNetDeviceStateTestSuite g_pointToPointNetDeviceStateTestSuite; //!< The testsuite
