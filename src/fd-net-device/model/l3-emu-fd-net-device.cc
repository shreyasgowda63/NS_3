/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 INRIA, 2012 University of Washington
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
 */

#include "l3-emu-fd-net-device.h"

#include "ns3/ipv4-l3-protocol.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/pointer.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("L3EmuFdNetDevice");

NS_OBJECT_ENSURE_REGISTERED (L3EmuFdNetDevice);

TypeId
L3EmuFdNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::L3EmuFdNetDevice")
    .SetParent<NetDevice> ()
    .SetGroupName ("L3EmuFdNetDevice")
    .AddConstructor<L3EmuFdNetDevice> ()
    .AddAttribute ("Address",
                   "The MAC address of this device.",
                   Mac48AddressValue (Mac48Address ("00:00:00:00:00:00")),
                   MakeMac48AddressAccessor (&L3EmuFdNetDevice::m_address),
                   MakeMac48AddressChecker ())
    .AddAttribute ("Start",
                   "The simulation time at which to spin up "
                   "the device thread.",
                   TimeValue (Seconds (0.)),
                   MakeTimeAccessor (&L3EmuFdNetDevice::m_tStart),
                   MakeTimeChecker ())
    .AddAttribute ("Stop",
                   "The simulation time at which to tear down "
                   "the device thread.",
                   TimeValue (Seconds (0.)),
                   MakeTimeAccessor (&L3EmuFdNetDevice::m_tStop),
                   MakeTimeChecker ())
    .AddAttribute ("RxQueueSize", "Maximum size of the read queue.  "
                   "This value limits number of packets that have been read "
                   "from the network into a memory buffer but have not yet "
                   "been processed by the simulator.",
                   UintegerValue (1000),
                   MakeUintegerAccessor (&L3EmuFdNetDevice::m_maxPendingReads),
                   MakeUintegerChecker<uint32_t> ())

    //
    // Trace sources designed to simulate a packet sniffer facility (tcpdump).
    //
    .AddTraceSource ("PromiscSniffer",
                     "Trace source simulating a promiscuous "
                     "packet sniffer attached to the device",
                     MakeTraceSourceAccessor (&L3EmuFdNetDevice::m_promiscSnifferTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

L3EmuFdNetDevice::L3EmuFdNetDevice () : m_mtu (1500)
{
  NS_LOG_FUNCTION (this);
}

L3EmuFdNetDevice::L3EmuFdNetDevice (L3EmuFdNetDevice const &)
{}

L3EmuFdNetDevice::~L3EmuFdNetDevice ()
{
  NS_LOG_FUNCTION (this);
}

void
L3EmuFdNetDevice::ForwardUp (void)
{
  uint8_t *buf = 0;
  ssize_t len = 0;

  {
    CriticalSection cs (m_pendingReadMutex);
    std::pair<uint8_t *, ssize_t> next = m_pendingQueue.front ();
    m_pendingQueue.pop ();

    buf = next.first;
    len = next.second;
  }

  NS_LOG_FUNCTION (this << buf << len);

  //
  // Create a packet out of the buffer we received and free that buffer.
  //
  Ptr<Packet> packet = Create<Packet> (reinterpret_cast<const uint8_t *> (buf), len);
  FreeBuffer (buf);
  buf = 0;

  // TODO: allow both IPv4 and IPv6
  // Assume only ipv4 traffic at this time
  uint16_t protocol = Ipv4L3Protocol::PROT_NUMBER;

  Mac48Address src = Mac48Address ("00:00:00:00:00:00");
  Mac48Address dst = Mac48Address ("00:00:00:00:00:00");
  PacketType packetType = NS3_PACKET_OTHERHOST;

  // For all kinds of packetType we receive, we hit the promiscuous sniffer
  // hook and pass a copy up to the promiscuous callback.
  if (!m_promiscRxCallback.IsNull ())
    {
      m_promiscSnifferTrace (packet);
      m_promiscRxCallback (this, packet, protocol, src, dst, packetType);
    }
}

bool
L3EmuFdNetDevice::SendFrom (Ptr<Packet> packet, const Address& src, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << src << dest << protocolNumber);
  NS_LOG_LOGIC ("packet: " << packet << " UID: " << packet->GetUid ());

  if (IsLinkUp () == false)
    {
      return false;
    }

  NS_LOG_LOGIC ("Transmit packet with UID " << packet->GetUid ());
  NS_ASSERT_MSG (packet->GetSize () <= m_mtu, "L3EmuFdNetDevice::SendFrom(): Packet too big " << packet->GetSize ());

  m_promiscSnifferTrace (packet);

  NS_LOG_LOGIC ("calling write");

  size_t len =  (size_t) packet->GetSize ();
  uint8_t *buffer = AllocateBuffer (len);
  packet->CopyData (buffer, len);

  ssize_t written = Write (buffer, len);
  FreeBuffer (buffer);

  if (written == -1 || (size_t) written != len)
    {
      return false;
    }

  return true;
}

bool
L3EmuFdNetDevice::NeedsArp (void) const
{
  return false;
}

void
L3EmuFdNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  m_promiscRxCallback = cb;
}

} // namespace ns3
