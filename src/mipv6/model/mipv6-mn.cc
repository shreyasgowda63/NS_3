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

#include <algorithm>
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/ipv6.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/callback.h"
#include "ns3/icmpv6-l4-protocol.h"
#include "mipv6-mn.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/pointer.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/ipv6-interface.h"

using namespace std;

NS_LOG_COMPONENT_DEFINE ("Mipv6Mn");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (Mipv6Mn);

TypeId
Mipv6Mn::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Mipv6Mn")
    .SetParent<Mipv6Agent> ()
    .AddAttribute ("BList", "The binding list associated with this MN.",
                   PointerValue (),
                   MakePointerAccessor (&Mipv6Mn::m_buinf),
                   MakePointerChecker<BList> ())
    .AddTraceSource ("RxBA",
                     "Received BA packet from HA",
                     MakeTraceSourceAccessor (&Mipv6Mn::m_rxbaTrace),
                     "ns3::Mipv6Mn::RxBaTracedCallback")
    .AddTraceSource ("TxBU",
                     "Sent BU packet from MN",
                     MakeTraceSourceAccessor (&Mipv6Mn::m_txbuTrace),
                     "ns3::Mipv6Mn::TxBuTracedCallback")


    ;
  return tid;
}

Mipv6Mn::Mipv6Mn (std::list<Ipv6Address> haalist)
{
  m_Haalist = haalist;
  m_hsequence = 0;
  m_roflag = false;
}

Mipv6Mn::~Mipv6Mn ()
{
  delete this;
}

void Mipv6Mn::NotifyNewAggregate ()
{
  NS_LOG_FUNCTION (this);

  if (GetNode () == 0)
    {
      
      Ptr<Icmpv6L4Protocol> icmpv6l4 = GetNode ()->GetObject<Icmpv6L4Protocol> ();
      icmpv6l4->SetNewIPCallback (MakeCallback (&Mipv6Mn::HandleNewAttachment, this));
      icmpv6l4->SetCheckAddressCallback (MakeCallback (&Mipv6Mn::CheckAddresses, this));

      Ptr<Ipv6L3Protocol> ipv6l3 = GetNode ()->GetObject<Ipv6L3Protocol> ();
      ipv6l3->SetPrefixCallback (MakeCallback (&Mipv6Mn::SetDefaultRouterAddress, this));

      Ptr<UdpL4Protocol> udpl4 = GetNode ()->GetObject<UdpL4Protocol> ();
      udpl4->SetMipv6Callback (MakeCallback (&BList::GetHoa, m_buinf));

      Ptr<TcpL4Protocol> tcpl4 = GetNode ()->GetObject<TcpL4Protocol> ();
      tcpl4->SetMipv6Callback (MakeCallback (&BList::GetHoa, m_buinf));
    }
  Mipv6Agent::NotifyNewAggregate ();
}

void Mipv6Mn::HandleNewAttachment (Ipv6Address ipr)
{
  // TODO: handles how mn will react when attached to another router
}

uint8_t Mipv6Mn::HandleBA (Ptr<Packet> packet, const Ipv6Address &src, const Ipv6Address &dst, Ptr<Ipv6Interface> interface)
{
  // TODO: handling ba
  return 0;
}

bool Mipv6Mn::IsHomeMatch (Ipv6Address addr)
{
  // TODO: check if address is in home address list
  return false;
}

Ipv6Address Mipv6Mn::GetCoA ()
{
  // TODO: return CoA from BList
  return Ipv6Address::GetAny ();
}

void Mipv6Mn::SetRouteOptimizationRequiredField (bool roflag)
{
  m_roflag = roflag;
}


bool Mipv6Mn::IsRouteOptimizationRequired ()
{
  return m_roflag;
}

void Mipv6Mn::SetDefaultRouterAddress (Ipv6Address addr,  uint32_t index)
{
  // TODO: set router address and index
}

bool Mipv6Mn::CheckAddresses (Ipv6Address ha, Ipv6Address hoa)
{
  // TODO: check if addresses are home agent address and home address respectively

  return false;
}

Ipv6Address Mipv6Mn::GetHomeAddress ()
{
  // TODO: return HoA from BList

  return Ipv6Address::GetAny ();
}

} /* namespace ns3 */

