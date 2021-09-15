/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Ananthakrishnan S
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
 * Authors: Ananthakrishnan S <ananthakrishnan190@gmail.com>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * \ingroup wifi-states
 * \ingroup tests
 *
 * \brief This class is a base class that contains common functions to be used in test
 *  cases in this test suite.
 *  
 */
class WifiNetDeviceStateTest : public TestCase
{
public:

  WifiNetDeviceStateTest (std::string description);

  /**
   * \brief Checks whether administrative state of the given device is UP.
   * 
   * \params device NetDevice that needs to be checked.
   */
  void VerifyAdminStateUp (Ptr<NetDevice> device);

  /**
   * \brief Checks whether administrative state of the given device is DOWN.
   * 
   * \params device NetDevice that needs to be checked.
   */
  void VerifyAdminStateDown (Ptr<NetDevice> device);

  /**
   * \brief Checks whether operational state of the given device is IF_OPER_UP.
   * 
   * \params device NetDevice that needs to be checked.
   */
  void VerifyOperationalUp (Ptr<NetDevice> device);

  /**
   * \brief Checks whether operational state of the given device is IF_OPER_DOWN.
   * 
   * \params device NetDevice that needs to be checked.
   */
  void VerifyOperationalDown (Ptr<NetDevice> device);

  /**
   * \brief Checks whether the given NetDevice received the given number of packets
   * 
   * \params device NetDevice that needs to be checked.
   * \params count Number of packets that should have received by the NetDevice.
   */
  void VerifyReceivedPacketCount (Ptr<NetDevice> device, uint16_t count);

  /**
   * \brief Creates and sends a single packet.
   * 
   * 
   * \params from Sender NetDevice
   * \params to Receiver NetDevice.
   */
  void SendOnePacket (Ptr<NetDevice> from, Ptr<NetDevice> to);

  /**
   * \brief Function that is added to receive callback of a Netdevice.
   * 
   * \param device a pointer to the net device which is calling this callback
   * \param packet the packet received
   * \param protocol the 16 bit protocol number associated with this packet.
   *        This protocol number is expected to be the same protocol number
   *        given to the Send method by the user on the sender side.
   * \param sender the address of the sender
   * \returns true if the callback could handle the packet successfully, false
   *          otherwise.
   */ 
  bool Receive (Ptr<NetDevice> nd, Ptr<const Packet> p, uint16_t protocol, const Address& addr);

private:
  /**
   * \brief Checks the operational state present in the given WifiNetDeviceState class.
   * 
   * \return true if operational.
   */
  bool IsOperational (Ptr<WifiNetDeviceState> state);

private:

  std::map<Address, uint32_t> m_count; ///< Number of packets received at given address.

};

WifiNetDeviceStateTest::WifiNetDeviceStateTest (std::string description)
  : TestCase (description)
{}

void
WifiNetDeviceStateTest::VerifyAdminStateUp (Ptr<NetDevice> device)
{
  Ptr<WifiNetDevice> wifiDevice = DynamicCast<WifiNetDevice> (device);
  Ptr<WifiNetDeviceState> netDevState = wifiDevice->GetObject<WifiNetDeviceState> ();

  NS_ASSERT (netDevState);
  NS_TEST_ASSERT_MSG_EQ (netDevState->IsUp (), true, "Device administrative state of WifiDevice with IfIndex " << wifiDevice->GetIfIndex () << "and mac " << wifiDevice->GetMac ()->GetInstanceTypeId () << " on node " << wifiDevice->GetNode ()->GetId () << " must be UP");
}

void
WifiNetDeviceStateTest::VerifyAdminStateDown (Ptr<NetDevice> device)
{
  Ptr<WifiNetDevice> wifiDevice = DynamicCast<WifiNetDevice> (device);
  Ptr<WifiNetDeviceState> netDevState = wifiDevice->GetObject<WifiNetDeviceState> ();

  NS_ASSERT (netDevState);
  NS_TEST_ASSERT_MSG_EQ (netDevState->IsUp (), false, "Device administrative state of WifiDevice with IfIndex " <<
                         wifiDevice->GetIfIndex () << "and mac " << wifiDevice->GetMac ()->GetInstanceTypeId () << " on node " << wifiDevice->GetNode ()->GetId () << " must be DOWN");
}

void
WifiNetDeviceStateTest::VerifyOperationalUp (Ptr<NetDevice> device)
{
  Ptr<WifiNetDevice> wifiDevice = DynamicCast<WifiNetDevice> (device);
  Ptr<WifiNetDeviceState> netDevState = device->GetObject<WifiNetDeviceState> ();

  NS_ASSERT (netDevState);
  NS_TEST_ASSERT_MSG_EQ (IsOperational (netDevState), true, wifiDevice->GetMac ()->GetInstanceTypeId ()
                         << " on device with IfIndex " << device->GetIfIndex () << " on node " << device->GetNode ()->GetId () << " should be RUNNING  at " << Simulator::Now ().GetSeconds () << "s.");
}

void
WifiNetDeviceStateTest::VerifyOperationalDown (Ptr<NetDevice> device)
{
  Ptr<WifiNetDevice> wifiDevice = DynamicCast<WifiNetDevice> (device);
  Ptr<WifiNetDeviceState> netDevState = device->GetObject<WifiNetDeviceState> ();

  NS_ASSERT (netDevState);
  NS_TEST_ASSERT_MSG_EQ (IsOperational (netDevState), false, wifiDevice->GetMac ()->GetInstanceTypeId ()
                         << " on device with IfIndex " << device->GetIfIndex () << " on node " << device->GetNode ()->GetId () << " should not be RUNNING at " << Simulator::Now ().GetSeconds () << "s.");
}

void
WifiNetDeviceStateTest::SendOnePacket (Ptr<NetDevice> from, Ptr<NetDevice> to)
{
  Ptr<Packet> p = Create<Packet> ();
  from->Send (p, to->GetAddress (), 1);
}

bool
WifiNetDeviceStateTest::Receive (Ptr<NetDevice> nd, Ptr<const Packet> p, uint16_t protocol, const Address& addr)
{
  m_count[nd->GetAddress ()]++;
  return true;
}

bool
WifiNetDeviceStateTest::IsOperational (Ptr<WifiNetDeviceState> state)
{
  return state->GetOperationalState () == NetDeviceState::IF_OPER_UP;
}

void
WifiNetDeviceStateTest::VerifyReceivedPacketCount (Ptr<NetDevice> device, uint16_t count)
{
  Ptr<WifiNetDevice> wifiDevice = DynamicCast<WifiNetDevice> (device);
  NS_TEST_ASSERT_MSG_EQ (m_count[wifiDevice->GetAddress ()], count, "Wrong number of received packets. Only " << count << " packet should have been received at " << Simulator::Now ().GetSeconds () << "s." );

}

/**
 * \ingroup wifi-states
 * \ingroup tests
 *
 * \brief Tests to verify the working of NetDeviceState class when 
 *  used with AdHoc MAC.
 * 
 */
class AdHocMacNetDeviceStateTest : public WifiNetDeviceStateTest
{
public:
  AdHocMacNetDeviceStateTest ();

  virtual void DoRun (void);

};

AdHocMacNetDeviceStateTest::AdHocMacNetDeviceStateTest ()
  : WifiNetDeviceStateTest ("Testcases for AdHoc Netdevice states")
{}

void
AdHocMacNetDeviceStateTest::DoRun ()
{
  NodeContainer adHocNodes;
  adHocNodes.Create (2);

  YansWifiPhyHelper phy;
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211a);
  wifi.SetRemoteStationManager ("ns3::ArfWifiManager");

  WifiMacHelper mac;
  NetDeviceContainer adHocDevices;
  mac.SetType ("ns3::AdhocWifiMac");
  adHocDevices = wifi.Install (phy, mac, adHocNodes);

  adHocDevices.Get (1)->SetReceiveCallback (MakeCallback (&AdHocMacNetDeviceStateTest::Receive, this));

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (adHocNodes);

  /**
   * Both Admin and operational state of AdHoc device are always UP. 
   * Verify that they are UP from the start of the simulation.
   * Verify state of AdHoc Device 0.
   */
  Simulator::Schedule (Seconds (0.0), &AdHocMacNetDeviceStateTest::VerifyAdminStateUp, this,
                       adHocDevices.Get (0));
  Simulator::Schedule (Seconds (0.0), &AdHocMacNetDeviceStateTest::VerifyOperationalUp, this,
                       adHocDevices.Get (0));

  /**
   * Verify state of AdHoc device 1.
   */
  Simulator::Schedule (Seconds (0.0), &AdHocMacNetDeviceStateTest::VerifyAdminStateUp, this,
                       adHocDevices.Get (1));
  Simulator::Schedule (Seconds (0.0), &AdHocMacNetDeviceStateTest::VerifyOperationalUp, this,
                       adHocDevices.Get (1));

  Ptr<WifiNetDeviceState> adHocDeviceState = adHocDevices.Get (0)->GetObject<WifiNetDeviceState> ();
  NS_ASSERT (adHocDeviceState);

  /**
   * 1 second into the simulation, Set Down Device 0 and Verify whether admin and operational state has
   * gone down.
   */
  Simulator::Schedule (Seconds (1.0), &WifiNetDeviceState::SetDown, adHocDeviceState);
  Simulator::Schedule (Seconds (2.0), &AdHocMacNetDeviceStateTest::VerifyAdminStateDown, this,
                       adHocDevices.Get (0));
  Simulator::Schedule (Seconds (2.0), &AdHocMacNetDeviceStateTest::VerifyOperationalDown, this,
                       adHocDevices.Get (0));

  /**
   * Bring up the previously brought down device and verify states.
   */
  Simulator::Schedule (Seconds (3.0), &WifiNetDeviceState::SetUp, adHocDeviceState);
  Simulator::Schedule (Seconds (4.0), &AdHocMacNetDeviceStateTest::VerifyAdminStateUp, this,
                       adHocDevices.Get (0));
  Simulator::Schedule (Seconds (4.0), &AdHocMacNetDeviceStateTest::VerifyOperationalUp, this,
                       adHocDevices.Get (0));

  /**
   * Try sending one packet from AdHoc device 0 to AdHoc device 1. Packet must be received because both devices
   * are operationally UP.
   */
  Simulator::Schedule (Seconds (5.0), &AdHocMacNetDeviceStateTest::SendOnePacket, this,
                       adHocDevices.Get (0), adHocDevices.Get (1));

  /**
   * Verify the number of packets received by AdHoc Device 1. Should be 1.
   */
  Simulator::Schedule (Seconds (5.2),&AdHocMacNetDeviceStateTest::VerifyReceivedPacketCount, this,
                       adHocDevices.Get (1), 1);

  /**
   * Bring down AdHoc Device 1 and try sending a packet. Device won't even receive anything since PHY will be
   * turned OFF as part of changing admin state to DOWN. Therefore, the number of received packets must stay the
   * same as above tested number.
   */
  Ptr<WifiNetDeviceState> adHocDeviceState1 = adHocDevices.Get (1)->GetObject<WifiNetDeviceState> ();
  NS_ASSERT (adHocDeviceState1);
  Simulator::Schedule (Seconds (6.0), &WifiNetDeviceState::SetDown, adHocDeviceState1);
  Simulator::Schedule (Seconds (6.3), &AdHocMacNetDeviceStateTest::SendOnePacket, this,
                       adHocDevices.Get (0), adHocDevices.Get (1));
  Simulator::Schedule (Seconds (6.5),&AdHocMacNetDeviceStateTest::VerifyReceivedPacketCount, this,
                       adHocDevices.Get (1), 1);

  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();

}

/**
 * \ingroup wifi-states
 * \ingroup tests
 *
 * \brief Tests to verify the working of NetDeviceState class when 
 *  used with a combination of STA and AP MAC.
 */
class StaApMacNetDeviceStateTest : public WifiNetDeviceStateTest
{
public:
  StaApMacNetDeviceStateTest ();

  virtual void DoRun ();

private:

  /**
   * \brief Change the velocity of the given node to given vector
   * 
   * \param node Node which requires change in velocity.
   * \param v New value of velocity.
   */
  void ChangeVelocity (Ptr<Node> node, Vector v);

  /**
   * \brief Verify that the given NetDevice is associated to an AP.
   * 
   * \param device The NetDevice that needs to be checked.
   */
  void VerifyAssociation (Ptr<NetDevice> device);

  /**
   * \brief Verify that the given NetDevice is dissociated to an AP.
   * 
   * \param device The NetDevice that needs to be checked.
   */
  void VerifyDissociation (Ptr<NetDevice> device);

};

StaApMacNetDeviceStateTest::StaApMacNetDeviceStateTest ()
  : WifiNetDeviceStateTest ("Testcases for STA and AP Netdevice states")
{}

void
StaApMacNetDeviceStateTest::ChangeVelocity (Ptr<Node> node,Vector v)
{
  Ptr<ConstantVelocityMobilityModel> velocityMobility =
    node->GetObject<ConstantVelocityMobilityModel>();
  velocityMobility->SetVelocity (v);
}

void
StaApMacNetDeviceStateTest::VerifyAssociation (Ptr<NetDevice> device)
{
  Ptr<WifiNetDevice> wifiDevice = DynamicCast<WifiNetDevice> (device);
  Ptr<WifiNetDeviceState> netDevState = wifiDevice->GetObject<WifiNetDeviceState> ();
  Ptr<StaWifiMac> mac = DynamicCast<StaWifiMac> (wifiDevice->GetMac ());

  NS_ASSERT (netDevState);
  NS_ASSERT_MSG (mac->IsAssociated (), wifiDevice->GetMac ()->GetInstanceTypeId () << " should be associated at " << Simulator::Now ().GetSeconds () << "s." );
}

void
StaApMacNetDeviceStateTest::VerifyDissociation (Ptr<NetDevice> device)
{
  Ptr<WifiNetDevice> wifiDevice = DynamicCast<WifiNetDevice> (device);
  Ptr<WifiNetDeviceState> netDevState = wifiDevice->GetObject<WifiNetDeviceState> ();
  Ptr<StaWifiMac> mac = DynamicCast<StaWifiMac> (wifiDevice->GetMac ());

  NS_ASSERT (netDevState);
  NS_ASSERT_MSG (!mac->IsAssociated (), wifiDevice->GetMac ()->GetInstanceTypeId () << " should be dissociated at " << Simulator::Now ().GetSeconds () << "s." );
}

void
StaApMacNetDeviceStateTest::DoRun ()
{
  NodeContainer apNode, staNode;
  apNode.Create (1);
  staNode.Create (2);

  Ssid ssid = Ssid ("wifi-default");

  YansWifiPhyHelper phy;
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211a);
  wifi.SetRemoteStationManager ("ns3::ArfWifiManager");

  WifiMacHelper mac;
  NetDeviceContainer apDevice;
  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));
  apDevice = wifi.Install (phy, mac, apNode);

  NetDeviceContainer staDevices;
  mac.SetType ("ns3::StaWifiMac",
               "ActiveProbing", BooleanValue (true),
               "Ssid", SsidValue (ssid));
  staDevices = wifi.Install (phy, mac, staNode);

  staDevices.Get (1)->SetReceiveCallback (MakeCallback (&StaApMacNetDeviceStateTest::Receive, this));

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  mobility.Install (apNode);
  mobility.Install (staNode);

  /**
   * Make the STA node move away from AP.
   */
  Simulator::Schedule (Seconds (1.0), &StaApMacNetDeviceStateTest::ChangeVelocity, this,
                       staNode.Get (0), Vector (10,0,0));

  /**
   * After travelling enough distance so that STA node is outside the radio range of AP node,
   * make STA node travel back inside AP node's radio range.
   */
  Simulator::Schedule (Seconds (8.0), &StaApMacNetDeviceStateTest::ChangeVelocity, this,
                       staNode.Get (0), Vector (-10,0,0));

  /**
   * Stop the movement of STA node once it reaches its original position.
   */
  Simulator::Schedule (Seconds (15.0), &StaApMacNetDeviceStateTest::ChangeVelocity, this,
                       staNode.Get (0), Vector (0,0,0));

  /**
   * Once the simulation starts, AP Node will be UP and RUNNING. From the point of view of AP, link is always UP.
   * Therefore, we need to verify that AP's admin state is UP and it is operationally UP as well.
   */
  Simulator::Schedule (Seconds (0.0), &StaApMacNetDeviceStateTest::VerifyAdminStateUp, this,  apDevice.Get (0));
  Simulator::Schedule (Seconds (0.0), &StaApMacNetDeviceStateTest::VerifyOperationalUp, this, apDevice.Get (0));

  /**
   * At the start of the simulation, STA node will be administratively UP. STA node won't be operational since it
   * has not yet established a connection with AP. Therefore, verify that Admin state of STA is UP and Operational
   * state is DOWN.
   */
  Simulator::Schedule (Seconds (0.0), &StaApMacNetDeviceStateTest::VerifyAdminStateUp, this, staDevices.Get (0));
  Simulator::Schedule (Seconds (0.0), &StaApMacNetDeviceStateTest::VerifyOperationalDown,this, staDevices.Get (0));

  /**
   * Verify that STA has established a connection with AP. At this point, STA will be associated and operational
   * state will be UP.
   */
  Simulator::Schedule (Seconds (0.1), &StaApMacNetDeviceStateTest::VerifyAssociation, this, staDevices.Get (0));
  Simulator::Schedule (Seconds (0.1), &StaApMacNetDeviceStateTest::VerifyOperationalUp, this, staDevices.Get (0));

  /**
   * 8s into the simulation, STA would have travelled outside the radio range of AP. At this point, verify that
   * STA is dissociated and operational state is DOWN. Admin state will of STA will still be UP at this point.
   */
  Simulator::Schedule (Seconds (8.0), &StaApMacNetDeviceStateTest::VerifyDissociation, this, staDevices.Get (0));
  Simulator::Schedule (Seconds (0.0), &StaApMacNetDeviceStateTest::VerifyAdminStateUp, this, staDevices.Get (0));
  Simulator::Schedule (Seconds (8.0), &StaApMacNetDeviceStateTest::VerifyOperationalDown,this, staDevices.Get (0));

  /**
   * 15s into the simulation, STA node would have travelled back to where it started. It is now inside AP's radio
   * range and would have associated with AP. Verify association as well as operational state.
   */
  Simulator::Schedule (Seconds (15.0), &StaApMacNetDeviceStateTest::VerifyAssociation, this, staDevices.Get (0));
  Simulator::Schedule (Seconds (15.0), &StaApMacNetDeviceStateTest::VerifyOperationalUp, this, staDevices.Get (0));

  /**
   * Bring down AP. This will render AP administratively down and turn off radio.
   * Verify admin state as well as operational state. Both must be down.
   */
  Ptr<WifiNetDeviceState> apDeviceState = apDevice.Get (0)->GetObject<WifiNetDeviceState> ();
  Simulator::Schedule (Seconds (16.0), &WifiNetDeviceState::SetDown, apDeviceState);
  Simulator::Schedule (Seconds (16.5), &StaApMacNetDeviceStateTest::VerifyOperationalDown,this, apDevice.Get (0));
  Simulator::Schedule (Seconds (16.5), &StaApMacNetDeviceStateTest::VerifyAdminStateDown, this, apDevice.Get (0));

  /**
   * Since AP is now turned off, STA must have dissociated. Verify the admin state as well as operational state
   * of STA. STA is admin up, it is searching for devices in range. Since STA is now dissociated, its operational
   * state is DOWN.
   */
  Simulator::Schedule (Seconds (17.0), &StaApMacNetDeviceStateTest::VerifyAdminStateUp, this, staDevices.Get (0));
  Simulator::Schedule (Seconds (17.0), &StaApMacNetDeviceStateTest::VerifyDissociation, this, staDevices.Get (0));
  Simulator::Schedule (Seconds (17.0), &StaApMacNetDeviceStateTest::VerifyOperationalDown,this, staDevices.Get (0));

  /**
   * Turn AP back on. AP must be administratively and operatively UP. STA must be able to connect to AP
   * turning STA's operational state to UP.
   */
  Simulator::Schedule (Seconds (18.0), &WifiNetDeviceState::SetUp, apDeviceState);
  Simulator::Schedule (Seconds (18.5), &StaApMacNetDeviceStateTest::VerifyAdminStateUp, this, apDevice.Get (0));
  Simulator::Schedule (Seconds (18.5), &StaApMacNetDeviceStateTest::VerifyOperationalUp,this, apDevice.Get (0));
  Simulator::Schedule (Seconds (20.0), &StaApMacNetDeviceStateTest::VerifyAssociation, this, staDevices.Get (0));
  Simulator::Schedule (Seconds (20.0), &StaApMacNetDeviceStateTest::VerifyOperationalUp, this, staDevices.Get (0));

  /**
   * Verify that STA Device 1 is associated to AP.
   */
  Simulator::Schedule (Seconds (20.5), &StaApMacNetDeviceStateTest::VerifyAssociation, this, staDevices.Get (1));

  /**
   * Send a packet from STA 0 to STA 1 and verify that packet has indeed received.
   */
  Simulator::Schedule (Seconds (21.0), &StaApMacNetDeviceStateTest::SendOnePacket, this, staDevices.Get (0),
                       staDevices.Get (1));
  Simulator::Schedule (Seconds (21.3), &StaApMacNetDeviceStateTest::VerifyReceivedPacketCount, this,
                       staDevices.Get (1), 1);

  /**
   * Bring Down STA Device 1 and try sending a packet from STA Device 0. Packet won't be received and
   * count remains same.
   */
  Ptr<WifiNetDeviceState> staDeviceState1 = staDevices.Get (1)->GetObject<WifiNetDeviceState> ();
  Simulator::Schedule (Seconds (21.5), &WifiNetDeviceState::SetDown, staDeviceState1);
  Simulator::Schedule (Seconds (22.0), &StaApMacNetDeviceStateTest::SendOnePacket, this, staDevices.Get (0),
                       staDevices.Get (1));
  Simulator::Schedule (Seconds (22.3), &StaApMacNetDeviceStateTest::VerifyReceivedPacketCount, this,
                       staDevices.Get (1), 1);

  /**
   * Just to make sure that bringing UP a STA Device creates no issues, the previously brought down
   * STA Device 1 is brought UP and a packet is send from STA Device 0. Packet count must increase.
   * It should be 2 at this point.
   */
  Simulator::Schedule (Seconds (22.4), &WifiNetDeviceState::SetUp, staDeviceState1);
  Simulator::Schedule (Seconds (23.0), &StaApMacNetDeviceStateTest::SendOnePacket, this, staDevices.Get (0),
                       staDevices.Get (1));
  Simulator::Schedule (Seconds (23.3), &StaApMacNetDeviceStateTest::VerifyReceivedPacketCount, this,
                       staDevices.Get (1), 2);

  Simulator::Stop (Seconds (23.5));
  Simulator::Run ();

  Simulator::Destroy ();

}

class WifiDeviceStateTestSuite : public TestSuite
{
public:
  WifiDeviceStateTestSuite ();
};

WifiDeviceStateTestSuite::WifiDeviceStateTestSuite ()
  : TestSuite ("wifi-states", UNIT)
{
  AddTestCase (new StaApMacNetDeviceStateTest, TestCase::QUICK);
  AddTestCase (new AdHocMacNetDeviceStateTest, TestCase::QUICK);
}

static WifiDeviceStateTestSuite g_wifiDeviceStateTestSuite; ///< the test suite
