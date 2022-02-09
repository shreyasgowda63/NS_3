/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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
 * Author: Greg Steinbrecher <grs@fb.com>
 */

#include "ns3/log.h"
#include "ns3/test.h"
#include "ns3/csma-helper.h"
#include "ns3/csma-net-device.h"
#include "ns3/csma-channel.h"
#include "ns3/simulator.h"
#include "ns3/data-rate.h"
#include "ns3/boolean.h"
#include "ns3/pointer.h"
#include "ns3/queue.h"

using namespace ns3;

// This is an example TestCase.
class CsmaFullDuplexTestCase1 : public TestCase
{
public:
  CsmaFullDuplexTestCase1 ();
  virtual ~CsmaFullDuplexTestCase1 ();
  void SendPackets (Ptr<NetDevice> txd, Ptr<NetDevice> rxd, size_t nPackets, size_t pktSize);
  bool
  Receive1 (Ptr<NetDevice> dev, Ptr<const Packet> pkt, uint16_t protocol, const Address &source)
  {
    m_packets1.push_back (pkt->Copy ());
    return true;
  }
  bool
  Receive2 (Ptr<NetDevice> dev, Ptr<const Packet> pkt, uint16_t protocol, const Address &source)
  {
    m_packets2.push_back (pkt->Copy ());
    return true;
  }

private:
  virtual void DoRun (void);
  void CheckPacketsInDeviceQueue (Ptr<NetDevice> dev, size_t nPackets, const std::string msg);
  void CheckNumPacketsReceived (size_t idx, size_t nPackets, const std::string msg);
  std::vector<Ptr<Packet>> m_packets1;
  std::vector<Ptr<Packet>> m_packets2;
};

// Add some help text to this case to describe what it is intended to test
CsmaFullDuplexTestCase1::CsmaFullDuplexTestCase1 ()
    : TestCase ("Send packets in both directions and ensure they arrive at right times")
{
}

// This destructor does nothing but we include it as a reminder that
// the test case should clean up after itself
CsmaFullDuplexTestCase1::~CsmaFullDuplexTestCase1 ()
{
}

void
CsmaFullDuplexTestCase1::CheckPacketsInDeviceQueue (Ptr<NetDevice> dev, size_t nPackets,
                                                    const std::string msg)
{
  PointerValue ptr;
  dev->GetAttributeFailSafe ("TxQueue", ptr);
  Ptr<Queue<Packet>> queue = ptr.Get<Queue<Packet>> ();
  NS_TEST_EXPECT_MSG_EQ (queue->GetNPackets (), nPackets, msg);
}

void
CsmaFullDuplexTestCase1::CheckNumPacketsReceived (size_t idx, size_t nPackets,
                                                  const std::string msg)
{
  switch (idx)
    {
    case 1:
      NS_TEST_EXPECT_MSG_EQ (m_packets1.size (), nPackets, msg);
      break;
    case 2:
      NS_TEST_EXPECT_MSG_EQ (m_packets2.size (), nPackets, msg);
      break;
    default:
      NS_ABORT_MSG ("Got invalid index for CheckNumPacketsReceived: " << idx);
    }
}

void
CsmaFullDuplexTestCase1::SendPackets (Ptr<NetDevice> txd, Ptr<NetDevice> rxd, size_t nPackets,
                                      size_t pktSize)
{
  auto dest = rxd->GetAddress ();
  size_t i;
  for (i = 0; i < nPackets; i++)
    {
      txd->Send (Create<Packet> (pktSize), dest, pktSize);
    }
}

void
CsmaFullDuplexTestCase1::DoRun (void)
{
  NodeContainer n;
  n.Create (2);

  CsmaHelper helper;
  helper.SetChannelAttribute ("FullDuplexMode", BooleanValue (true));
  helper.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("1GB/s")));

  NetDeviceContainer rxC = helper.Install (n.Get (1));
  Ptr<NetDevice> dev1, dev2;
  dev1 = rxC.Get (0);

  auto channel = DynamicCast<CsmaChannel> (rxC.Get (0)->GetChannel ());
  NS_ASSERT_MSG (channel != 0, "Couldn't cast channel to CsmaChannel");
  dev2 = helper.Install (n.Get (0), channel).Get (0);

  dev1->SetReceiveCallback (MakeCallback (&CsmaFullDuplexTestCase1::Receive1, this));
  dev2->SetReceiveCallback (MakeCallback (&CsmaFullDuplexTestCase1::Receive2, this));

  // Leave room for ethernet header & footer so frame size is exactly 1000 bytes
  // Makes calculations easier
  auto frameBodySize = 1000 - 18;

  auto ifgTime = Time (NanoSeconds (12)); // Inter-Frame Gap
  auto frameTime = Time (MicroSeconds (1)); // 1000 bytes / 1 GB/s = 1 microsecond
  auto stepTime = ifgTime + frameTime;
  size_t totalPackets = 10;

  Simulator::Schedule (Time (Seconds (0)), &CsmaFullDuplexTestCase1::SendPackets, this, dev1, dev2,
                       totalPackets, frameBodySize);
  Simulator::Schedule (Time (Seconds (0)), &CsmaFullDuplexTestCase1::SendPackets, this, dev2, dev1,
                       totalPackets, frameBodySize);
  size_t numRx;
  auto checkTime = stepTime / 2.0;
  size_t nPktsInQ;
  // Check halfway through each transmission
  for (numRx = 0; numRx <= totalPackets; numRx++)
    {
      nPktsInQ = totalPackets - numRx;
      if (nPktsInQ)
        {
          nPktsInQ--;
        }
      Simulator::Schedule (checkTime, &CsmaFullDuplexTestCase1::CheckPacketsInDeviceQueue, this,
                           dev1, nPktsInQ,
                           "There must be " + std::to_string (nPktsInQ) +
                               " packets in device 1's queue at this point");
      Simulator::Schedule (checkTime, &CsmaFullDuplexTestCase1::CheckPacketsInDeviceQueue, this,
                           dev2, nPktsInQ,
                           "There must be " + std::to_string (nPktsInQ) +
                               " packets in device 2's queue at this point");
      Simulator::Schedule (checkTime, &CsmaFullDuplexTestCase1::CheckNumPacketsReceived, this, 1,
                           numRx,
                           "There must be " + std::to_string (numRx) +
                               " packets received by device 1 at this point");
      Simulator::Schedule (checkTime, &CsmaFullDuplexTestCase1::CheckNumPacketsReceived, this, 2,
                           numRx,
                           "There must be " + std::to_string (numRx) +
                               " packets received by device 2 at this point");
      checkTime += stepTime;
    }
  // Check right after packet should have been received
  checkTime = frameTime + Time (NanoSeconds (1));
  for (numRx = 1; numRx <= totalPackets; numRx++)
    {
      size_t nPktsInQ = totalPackets - numRx;
      Simulator::Schedule (checkTime, &CsmaFullDuplexTestCase1::CheckPacketsInDeviceQueue, this,
                           dev1, nPktsInQ,
                           "There must be " + std::to_string (nPktsInQ) +
                               " packets in device 1's queue at this point");
      Simulator::Schedule (checkTime, &CsmaFullDuplexTestCase1::CheckPacketsInDeviceQueue, this,
                           dev2, nPktsInQ,
                           "There must be " + std::to_string (nPktsInQ) +
                               " packets in device 2's queue at this point");
      Simulator::Schedule (checkTime, &CsmaFullDuplexTestCase1::CheckNumPacketsReceived, this, 1,
                           numRx,
                           "There must be " + std::to_string (numRx) +
                               " packets received by device 1 at this point");
      Simulator::Schedule (checkTime, &CsmaFullDuplexTestCase1::CheckNumPacketsReceived, this, 2,
                           numRx,
                           "There must be " + std::to_string (numRx) +
                               " packets received by device 2 at this point");
      checkTime += stepTime;
    }
  Simulator::Run ();
  Simulator::Destroy ();
}

class CsmaFullDuplexTestSuite : public TestSuite
{
public:
  CsmaFullDuplexTestSuite ();
};

CsmaFullDuplexTestSuite::CsmaFullDuplexTestSuite () : TestSuite ("csma-full-duplex", UNIT)
{
  // TestDuration for TestCase can be QUICK, EXTENSIVE or TAKES_FOREVER
  AddTestCase (new CsmaFullDuplexTestCase1, TestCase::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
static CsmaFullDuplexTestSuite scsmaFullDuplexTestSuite;