/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 Jadavpur University, India
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
 * Author: Manoj Kumar Rana <manoj24.rana@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/icmpv6-l4-protocol.h"
#include "ns3/callback.h"
#include "mipv6-ha.h"
#include "ns3/pointer.h"
#include "ns3/ipv6-interface.h"

using namespace std;

NS_LOG_COMPONENT_DEFINE ("Mipv6Ha");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (Mipv6Ha);

TypeId
Mipv6Ha::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Mipv6Ha")
    .SetParent<Mipv6Agent> ()
    .AddConstructor<Mipv6Ha> ()
    .AddAttribute ("BCache", "The binding cache associated with this agent.",
                   PointerValue (),
                   MakePointerAccessor (&Mipv6Ha::m_bCache),
                   MakePointerChecker<BCache> ())
    .AddTraceSource ("RxBU",
                     "Receive BU packet from MN",
                     MakeTraceSourceAccessor (&Mipv6Ha::m_rxbuTrace),
                     "ns3::Mipv6Ha::RxBuTracedCallback")

    ;
  return tid;
}

Mipv6Ha::Mipv6Ha ()
  : m_bCache (0)
{
}

Mipv6Ha::~Mipv6Ha ()
{
  m_bCache = 0;
}

void Mipv6Ha::NotifyNewAggregate ()
{
  if (GetNode () == 0)
    {
      Ptr<Node> node = this->GetObject<Node> ();
      
      Ptr<Icmpv6L4Protocol> icmpv6l4 = node->GetObject<Icmpv6L4Protocol> ();
      Ptr<Ipv6L3Protocol> ipv6 = node->GetObject<Ipv6L3Protocol> ();
      icmpv6l4->SetDADCallback (MakeCallback (&Mipv6Ha::DADFailureIndication, this));
      icmpv6l4->SetNSCallback (MakeCallback (&Mipv6Ha::IsAddress, this));
      icmpv6l4->SetHandleNSCallback (MakeCallback (&Mipv6Ha::HandleNS, this));
      ipv6->SetNSCallback2 (MakeCallback (&Mipv6Ha::IsAddress2, this));
    }

  Mipv6Agent::NotifyNewAggregate ();
}

void Mipv6Ha::DADFailureIndication (Ipv6Address addr)
{
  // TODO: Set entry in BCache as Invalid
}

bool Mipv6Ha::IsAddress (Ipv6Address addr)
{
  // TODO: check if address is home address of MN
  return false;
}

bool Mipv6Ha::IsAddress2 (Ipv6Address addr)
{
  NS_LOG_FUNCTION(this << addr);
  // TODO: check if address is solicited home address of MN
  return false;
}


uint8_t Mipv6Ha::HandleBU (Ptr<Packet> packet, const Ipv6Address &src, const Ipv6Address &dst, Ptr<Ipv6Interface> interface)
{
  // TODO: handle BU messages
  return 0;
}

void Mipv6Ha::DoDADForOffLinkAddress (Ipv6Address target, Ptr<Ipv6Interface> interface)
{
  // TODO: Perform DAD for MN HoA
}

void Mipv6Ha::FunctionDadTimeoutForOffLinkAddress (Ptr<Ipv6Interface> interface, Ptr<Packet> ba, Ipv6Address homeaddr)
{
  // TODO: Binding process
}

void Mipv6Ha::HandleNS (Ptr<Packet> packet, Ptr<Ipv6Interface> interface, Ipv6Address src, Ipv6Address target)
{
  // TODO: handle ns
}

} /* namespace ns3 */

