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

#include "ns3/core-module.h"
#include "ns3/test.h"
#include "ns3/simulator.h"
#include "ns3/address.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/csma-net-device.h"
#include "ns3/csma-channel.h"
#include "ns3/csma-net-device-state.h"

using namespace ns3;

/**
 * \brief CsmaNetDeviceTest A Description of various tests conducted
 * in this suite.
 *
 * All the tests are detailed in their order below:
 *
 * Test 1: After creating 2 CsmaNetDevices and aggregating NetDeviceState
 * check if admin state of device is UP. The devices are not connected so
 * devices should not be RUNNING.
 *
 * Test 2: Attach channel to both devices and check admin and running states
 * of the device. At this point devices should be UP and RUNNING.
 *
 * Test 3: Detach channel from a device and check the states. Device should be
 * UP but not RUNNING. Tests are done with the 2 available Detach () methods in
 * CsmaChannel. One takes pointer to NetDevice as argument while the other
 * takes deviceId assigned by CsmaChannel to each attached devices.
 *
 * Test 4: When a device is UP and RUNNING, check what happens to the states
 * when the device admin state is set to DOWN. Device should not be UP and
 * RUNNING.
 *
 * Test 5: Test what happens to the states when a Device which is not UP and
 * RUNNING is set to UP. i.e the device is already DOWN but connected to a
 * channel. After setting UP, device should be UP and RUNNING.
 *
 * Test 6: Test what happens when a disconnected device is set DOWN. Device
 * should not be UP and RUNNING.
 *
 * Test 7: Test what happens when a disconnected device is set UP. Device
 * should be UP but not RUNNING.
 *
 * Test 8: Check whether packets can be sent when device is UP and RUNNING.
 *
 * Test 9: Check whether device can transmit packets when it is not UP and
 * RUNNING.
 *
 * Test 10: Check whether device can transmit packets when it is UP but not
 * RUNNING.
 *
 */

class CsmaNetDeviceStateTest : public TestCase
{
public:
  CsmaNetDeviceStateTest ();

  virtual void DoRun (void);

private:
  bool SendPackets (Ptr<CsmaNetDevice> devA, Ptr<CsmaNetDevice> devB);

  void DeviceStateChangeCatcher (bool administrativeState, NetDeviceState::OperationalState);

  bool IsOperational (Ptr<CsmaNetDeviceState> csmaState);

  uint32_t m_stateChangeCallbackCount;

};

CsmaNetDeviceStateTest::CsmaNetDeviceStateTest ()
  :  TestCase ("CsmaNetDeviceStateTest"),
     m_stateChangeCallbackCount (0)
{}

bool
CsmaNetDeviceStateTest::SendPackets (Ptr<CsmaNetDevice> devA, Ptr<CsmaNetDevice> devB)
{
  Ptr<Packet> p = Create<Packet> ();
  return devA->Send (p, devB->GetAddress (), 0x800);

}

bool
CsmaNetDeviceStateTest::IsOperational (Ptr<CsmaNetDeviceState> state)
{
  return state->GetOperationalState () == NetDeviceState::IF_OPER_UP;
}

void
CsmaNetDeviceStateTest::DeviceStateChangeCatcher (bool adminState, NetDeviceState::OperationalState opState)
{
  m_stateChangeCallbackCount++;
}

void
CsmaNetDeviceStateTest::DoRun (void)
{
  Ptr<Node> NodeA = CreateObject<Node> ();
  Ptr<Node> NodeB = CreateObject<Node> ();

  Ptr<CsmaChannel> channel = CreateObject<CsmaChannel> ();

  Ptr<CsmaNetDevice> devA = CreateObject<CsmaNetDevice> ();
  Ptr<CsmaNetDevice> devB = CreateObject<CsmaNetDevice> ();

  Ptr<CsmaNetDeviceState> stateA = CreateObject<CsmaNetDeviceState> ();
  Ptr<CsmaNetDeviceState> stateB = CreateObject<CsmaNetDeviceState> ();

  stateA->SetDevice (devA);
  stateB->SetDevice (devB);

  devA->AggregateObject (stateA);
  devB->AggregateObject (stateB);

  devA->SetAddress (Mac48Address::Allocate ());
  devB->SetAddress (Mac48Address::Allocate ());

  devA->SetQueue (CreateObject<DropTailQueue<Packet> > ());
  devB->SetQueue (CreateObject<DropTailQueue<Packet> > ());

  NS_TEST_EXPECT_MSG_EQ (stateA->IsUp (), true, "Administrative state of device should be UP.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateA), false, "Device is yet to be connected and should not be running.");

  NS_TEST_EXPECT_MSG_EQ (stateB->IsUp (), true, "Administrative state of device should be UP (auto configure).");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateB), false, "Device is yet to be connected and should not be running.");


  /** Check whether deivce is UP and RUNNING.
  */
  devA->Attach (channel);
  NS_TEST_EXPECT_MSG_EQ (stateA->IsUp (), true, "Administrative state of device should be UP.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateA), true, "Device should be running since it is connected.");

  devB->Attach (channel);
  NS_TEST_EXPECT_MSG_EQ (stateB->IsUp (), true, "Administrative state of device should be UP.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateB), true, "Device should be running since it is connected.");

  // Detaching channel from a device should change OperationalState to IF_OPER_DOWN.
  channel->Detach (devA);
  NS_TEST_EXPECT_MSG_EQ (stateA->IsUp (), true, "Device should still be enabled.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateA), false, "Device should be not be running.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateB), true, "The other device in the channel should still be running since it is connected.");

  channel->Reattach (devA);
  NS_TEST_EXPECT_MSG_EQ (stateA->IsUp (), true, "Device should still be enabled.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateA), true, "Device should be running since it is reconnected to channel.");

  /** Detach using deviceId.
   */
  channel->Detach (devA->GetDeviceId ());
  NS_TEST_EXPECT_MSG_EQ (stateA->IsUp (), true, "Device should still be enabled.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateA), false, "Device should not be running.");

  channel->Reattach (devA);
  NS_TEST_EXPECT_MSG_EQ (stateA->IsUp (), true, "Device should still be enabled.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateA), true, "Device should running since it is reconnected to channel.");

  /**
   * Tests on administrative state. Turn off and on a device when it is connected to channel.
   */
  stateA->SetDown ();
  NS_TEST_EXPECT_MSG_EQ (stateA->IsUp (), false, "Device is disabled.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateA), false, "Device should not be running since it is disabled.");

  stateA->SetUp ();
  NS_TEST_EXPECT_MSG_EQ (stateA->IsUp (), true, "Device is enabled.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateA), true, "Device should be running.");

  /**
   * Test on administrative state. Turn on and off a device when it is disconnected.
   */
  channel->Detach (devA);
  
  stateA->SetDown ();
  NS_TEST_EXPECT_MSG_EQ (stateA->IsUp (), false, "Device is disabled.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateA), false, "Device should not be running.");

  stateA->SetUp ();
  NS_TEST_EXPECT_MSG_EQ (stateA->IsUp (), true, "Device is enabled.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateA), false, "Device should not be running.");

  channel->Reattach (devA);

  /* Disconnect CsmaNetDevice A from the channel when the device is administratively DOWN.
   */
  stateA->SetDown ();

  channel->Detach (devA);

  NS_TEST_EXPECT_MSG_EQ (stateA->IsUp (), false, "Device is disabled.");
  NS_TEST_EXPECT_MSG_EQ (IsOperational (stateA), false, "Device should not be running.");

  stateA->SetUp ();
  channel->Reattach (devA);

  //Test whether packet is sent when device is running.
  bool isOk;
  isOk = SendPackets (devA, devB);
  NS_TEST_EXPECT_MSG_EQ (isOk, true, "Device is running therefore packet should be send.");

  //Test whether packet is sent when device is administratively DOWN.
  stateA->SetDown ();
  isOk = SendPackets (devA, devB);
  NS_TEST_EXPECT_MSG_EQ (isOk, false, "Device is disabled therefore packet should not be send.");
  stateA->SetUp ();

  //Test whether packet is sent when Device A is detached from Channel.
  channel->Detach (devA);

  isOk = SendPackets (devA, devB);
  NS_TEST_EXPECT_MSG_EQ (isOk, false, "Channel is detached therefore packet should not be send.");

}

class CsmaNetDeviceStateTestSuite : public TestSuite
{
public:
  CsmaNetDeviceStateTestSuite ();
};

CsmaNetDeviceStateTestSuite::CsmaNetDeviceStateTestSuite ()
  : TestSuite ("states-csma",UNIT)
{
  AddTestCase (new CsmaNetDeviceStateTest, TestCase::QUICK);
}

static CsmaNetDeviceStateTestSuite g_csmaNetDeviceStateTestSuite; //!< The testsuite

