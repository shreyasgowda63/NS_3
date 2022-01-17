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
#include "ns3/node.h"
#include "blist.h"
#include "mipv6-mn.h"
#include "mipv6-l4-protocol.h"
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BList");
NS_OBJECT_ENSURE_REGISTERED (BList);

TypeId BList::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::BList")
    .SetParent<Object> ()
  ;
  return tid;
}


BList::~BList ()
{
  NS_LOG_FUNCTION_NOARGS ();
  Flush ();
}

void BList::DoDispose ()
{
  Flush ();
  Object::DoDispose ();
}


void BList::Flush ()
{
  delete this;
}


Ptr<Node> BList::GetNode () const
{
  NS_LOG_FUNCTION (this);

  return m_node;
}

void BList::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION ( this << node );

  m_node = node;
}

BList::BList (std::list<Ipv6Address> haalist)
  : m_hstate (UNREACHABLE),
  m_tunnelIfIndex (-1),
  m_hpktbu (0),
  m_HaaList (haalist),
  m_hretransTimer (Timer::CANCEL_ON_DESTROY),
  m_hreachableTimer (Timer::CANCEL_ON_DESTROY),
  m_hrefreshTimer (Timer::CANCEL_ON_DESTROY),
  m_HomeAddressRegisteredFlag (false)

{
  NS_LOG_FUNCTION_NOARGS ();
}

void BList::SetHomeAddressRegistered (bool flag)
{
  m_HomeAddressRegisteredFlag = flag;
}

bool BList::IsHomeAddressRegistered ()
{
  return m_HomeAddressRegisteredFlag;
}

void BList::FunctionHomeRefreshTimeout ()
{
  NS_LOG_FUNCTION (this);
  Ptr<Mipv6Mn> mn = GetNode ()->GetObject<Mipv6Mn> ();

  if (mn == 0)
    {
      NS_LOG_WARN ("No MN for Binding Update List");

      return;
    }

  SetHomeLastBindingUpdateTime (MicroSeconds (Simulator::Now ().GetMicroSeconds ()));
  SetHomeLastBindingUpdateSequence (mn->GetHomeBUSequence ());

  Ptr<Packet> p = mn->BuildHomeBU (true, true, true, true, Mipv6L4Protocol::MAX_BINDING_LIFETIME, true);

  SetHomeBUPacket (p);

  ResetHomeRetryCount ();

  mn->SendMessage (p->Copy (), GetHA (), 64);

  MarkHomeRefreshing ();

  StartHomeRetransTimer ();
}

void BList::FunctionHomeReachableTimeout ()
{
  NS_LOG_FUNCTION (this);

  Ptr<Mipv6Mn> mn = GetNode ()->GetObject<Mipv6Mn>();

  NS_LOG_LOGIC ("Reachable Timeout");

  if ( mn == 0)
    {
      NS_LOG_WARN ("No MN for Binding Update List");

      return;
    }

  if (IsHomeReachable ())
    {
      MarkHomeUnreachable ();
    }
  else if (IsHomeRefreshing ())
    {
      MarkHomeUpdating ();
    }


  //delete routing && tunnel
  if (m_tunnelIfIndex >= 0)
    {
      mn->ClearTunnelAndRouting ();
    }
}

void BList::FunctionHomeRetransTimeout ()
{
  NS_LOG_FUNCTION (this);
  Ptr<Mipv6Mn> mn = GetNode ()->GetObject<Mipv6Mn>();

  if ( mn == 0)
    {
      NS_LOG_WARN ("No MN for Binding Update List");

      return;
    }

  IncreaseHomeRetryCount ();

  if ( GetHomeRetryCount () > Mipv6L4Protocol::MAX_BINDING_UPDATE_RETRY_COUNT )
    {
      NS_LOG_LOGIC ("Maximum retry count reached. Giving up..");

      return;
    }

  mn->SendMessage (GetHomeBUPacket ()->Copy (), GetHA (), 64);

  StartHomeRetransTimer ();
}

bool BList::IsHomeUnreachable () const
{
  NS_LOG_FUNCTION (this);

  return m_hstate == UNREACHABLE;
}

bool BList::IsHomeUpdating () const
{
  NS_LOG_FUNCTION (this);

  return m_hstate == UPDATING;
}

bool BList::IsHomeRefreshing () const
{
  NS_LOG_FUNCTION (this);

  return m_hstate == REFRESHING;
}

bool BList::IsHomeReachable () const
{
  NS_LOG_FUNCTION (this);

  return m_hstate == REACHABLE;
}

void BList::MarkHomeUnreachable ()
{
  NS_LOG_FUNCTION (this);

  m_hstate = UNREACHABLE;
}

void BList::MarkHomeUpdating ()
{
  NS_LOG_FUNCTION (this);

  m_hstate = UPDATING;
}

void BList::MarkHomeRefreshing ()
{
  NS_LOG_FUNCTION (this);

  m_hstate = REFRESHING;
}

void BList::MarkHomeReachable ()
{
  NS_LOG_FUNCTION (this);

  m_hstate = REACHABLE;
}

void BList::StartHomeReachableTimer ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT ( !m_hreachableTime.IsZero () );

  m_hreachableTimer.SetFunction (&BList::FunctionHomeReachableTimeout, this);
  m_hreachableTimer.SetDelay ( Seconds (m_hreachableTime.GetSeconds ()));
  m_hreachableTimer.Schedule ();
}

void BList::StopHomeReachableTimer ()
{
  NS_LOG_FUNCTION (this);
  m_hreachableTimer.Cancel ();
}

void BList::StartHomeRetransTimer ()
{
  NS_LOG_FUNCTION (this);
  m_hretransTimer.SetFunction (&BList::FunctionHomeRetransTimeout, this);

  if (GetHomeRetryCount () == 0)
    {
      if (IsHomeAddressRegistered ())
        {
          m_hretransTimer.SetDelay (Seconds (Mipv6L4Protocol::INITIAL_BINDING_ACK_TIMEOUT_FIRSTREG));
        }
      else
        {
          m_hretransTimer.SetDelay (Seconds (Mipv6L4Protocol::INITIAL_BINDING_ACK_TIMEOUT_FIRSTREG + 1.0));
        }
    }
  else
    {
      m_hretransTimer.SetDelay (Seconds (Mipv6L4Protocol::INITIAL_BINDING_ACK_TIMEOUT_REREG));
    }

  m_hretransTimer.Schedule ();
}

void BList::StopHomeRetransTimer ()
{
  NS_LOG_FUNCTION (this);

  m_hretransTimer.Cancel ();
}

void BList::StartHomeRefreshTimer ()
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT ( !m_hreachableTime.IsZero () );

  m_hrefreshTimer.SetFunction (&BList::FunctionHomeRefreshTimeout, this);
  m_hrefreshTimer.SetDelay ( Seconds ( m_hreachableTime.GetSeconds () * 0.9 ) );
  m_hrefreshTimer.Schedule ();
}

void BList::StopHomeRefreshTimer ()
{
  NS_LOG_FUNCTION (this);
  m_hrefreshTimer.Cancel ();
}

Time BList::GetHomeReachableTime () const
{
  NS_LOG_FUNCTION (this);

  return m_hreachableTime;
}

void BList::SetHomeReachableTime (Time tm)
{
  NS_LOG_FUNCTION (this << tm );

  m_hreachableTime = tm;
}

uint8_t BList::GetHomeRetryCount () const
{
  NS_LOG_FUNCTION (this);
  return m_hretryCount;
}

void BList::ResetHomeRetryCount ()
{
  m_hretryCount = 0;
}

void BList::IncreaseHomeRetryCount ()
{
  NS_LOG_FUNCTION (this);
  m_hretryCount++;
}

Time BList::GetHomeInitialLifeTime () const
{
  NS_LOG_FUNCTION (this);
  return m_hinitiallifetime;
}

void BList::SetHomeInitialLifeTime (Time tm)
{
  NS_LOG_FUNCTION ( this << tm );

  m_hinitiallifetime = tm;
}

Time BList::GetHomeRemainingLifeTime () const
{
  NS_LOG_FUNCTION (this);
  return m_hreachableTimer.GetDelayLeft ();
}


Time BList::GetHomeLastBindingUpdateTime () const
{
  NS_LOG_FUNCTION (this);
  return m_hbulastsent;
}

void BList::SetHomeLastBindingUpdateTime (Time tm)
{
  NS_LOG_FUNCTION ( this << tm );
  m_hbulastsent = tm;
}


uint16_t BList::GetHomeLastBindingUpdateSequence () const
{
  NS_LOG_FUNCTION (this);

  return m_hlastBindingUpdateSequence;
}

void BList::SetHomeLastBindingUpdateSequence (uint16_t seq)
{
  NS_LOG_FUNCTION ( this << seq);

  m_hlastBindingUpdateSequence = seq;
}

Ptr<Packet> BList::GetHomeBUPacket () const
{
  NS_LOG_FUNCTION (this);

  return m_hpktbu;
}

void BList::SetHomeBUPacket (Ptr<Packet> pkt)
{
  NS_LOG_FUNCTION ( this << pkt );

  m_hpktbu = pkt;
}

int16_t BList::GetTunnelIfIndex () const
{
  NS_LOG_FUNCTION (this);

  return m_tunnelIfIndex;
}

void BList::SetTunnelIfIndex (int16_t tunnelif)
{
  NS_LOG_FUNCTION ( this << tunnelif );

  m_tunnelIfIndex = tunnelif;
}
void BList::SetHoa (Ipv6Address hoa)
{
  m_hoa = hoa;
}
Ipv6Address BList::GetHoa (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_hoa;
}
void BList::SetCoa (Ipv6Address addr)
{
  m_coa = addr;
}
Ipv6Address BList::GetCoa (void) const
{
  NS_LOG_FUNCTION (this);
  return m_coa;
}

Ipv6Address BList::GetHA () const
{
  return m_ha;
}

void BList::SetHA (Ipv6Address ha)
{
  m_ha = ha;
}

std::list<Ipv6Address> BList::GetHomeAgentList () const
{
  return m_HaaList;
}

void BList::SetHomeAgentList (std::list<Ipv6Address> haalist)
{
  m_HaaList = haalist;
}

bool BList::GetHomeBUFlag () const
{
  return m_hflag;
}

void BList::SetHomeBUFlag (bool f)
{
  m_hflag = f;
}

} /* namespace ns3 */