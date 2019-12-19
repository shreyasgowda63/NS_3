/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Università di Firenze, Italy
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
 * Author: Alessio Bonadio <alessio.bonadio@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/ipv6-address.h"
#include "ns3/mac16-address.h"
#include "ns3/mac64-address.h"

#include "sixlowpan-ndisc-cache.h"
#include "sixlowpan-nd-protocol.h"
#include "sixlowpan-nd-context.h"
#include "sixlowpan-nd-prefix.h"


namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("SixLowPanNdiscCache");

NS_OBJECT_ENSURE_REGISTERED (SixLowPanNdiscCache);

TypeId SixLowPanNdiscCache::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::SixLowPanNdiscCache")
    .SetParent<NdiscCache> ()
    .SetGroupName ("SixLowPan")
  ;
  return tid;
}

SixLowPanNdiscCache::SixLowPanNdiscCache ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

SixLowPanNdiscCache::~SixLowPanNdiscCache ()
{
  NS_LOG_FUNCTION_NOARGS ();
  Flush ();
}

void SixLowPanNdiscCache::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();

  Flush ();
  NdiscCache::DoDispose();
}

NdiscCache::Entry* SixLowPanNdiscCache::Lookup (Ipv6Address dst)
{
  NS_LOG_FUNCTION (this << dst);
  SixLowPanNdiscCache::SixLowPanEntry* entry;

  if (m_ndCache.find (dst) != m_ndCache.end ())
    {
      entry = dynamic_cast<SixLowPanNdiscCache::SixLowPanEntry*> (m_ndCache[dst]);
    }
  else if (dst.IsLinkLocal())
    {
      entry = dynamic_cast<SixLowPanNdiscCache::SixLowPanEntry*> (Add (dst));
      uint8_t buf[16];
      dst.GetBytes(buf);
      Mac16Address address;
      address.CopyFrom(buf+14);
      entry->SetRouter (true);
      entry->SetMacAddress (address);
      entry->StartReachableTimer ();
      entry->MarkReachable ();
    }
  else
    {
      entry = 0;
    }
  return entry;
}

NdiscCache::Entry* SixLowPanNdiscCache::Add (Ipv6Address to)
{
  NS_LOG_FUNCTION (this << to);
  NS_ASSERT (m_ndCache.find (to) == m_ndCache.end ());

  SixLowPanNdiscCache::SixLowPanEntry* entry = new SixLowPanNdiscCache::SixLowPanEntry (this);
  entry->SetIpv6Address (to);
  m_ndCache[to] = entry;
  return entry;
}

void SixLowPanNdiscCache::PrintNdiscCache (Ptr<OutputStreamWrapper> stream)
{
  NS_LOG_FUNCTION (this << stream);
  std::ostream* os = stream->GetStream ();

  for (auto i = m_ndCache.begin (); i != m_ndCache.end (); i++)
    {
      *os << i->first << " dev ";
      std::string found = Names::FindName (GetDevice ());
      if (Names::FindName (GetDevice ()) != "")
        {
          *os << found;
        }
      else
        {
          *os << static_cast<int> (GetDevice ()->GetIfIndex ());
        }

      SixLowPanNdiscCache::SixLowPanEntry* entry = dynamic_cast<SixLowPanNdiscCache::SixLowPanEntry*> (i->second);
      *os << " lladdr " << entry->GetMacAddress ();

      if (entry->IsReachable ())
        {
          *os << " REACHABLE ";
        }
      else if (entry->IsDelay ())
        {
          *os << " DELAY ";
        }
      else if (entry->IsIncomplete ())
        {
          *os << " INCOMPLETE ";
        }
      else if (entry->IsProbe ())
        {
          *os << " PROBE ";
        }
      else
        {
          *os << " STALE ";
        }

      if (entry->IsRegistered ())
        {
          *os << " REGISTERED\n";
        }
      else if (entry->IsTentative ())
        {
          *os << " TENTATIVE\n";
        }
      else
        {
          *os << " GARBAGE\n";
        }
    }
}

SixLowPanNdiscCache::SixLowPanEntry::SixLowPanEntry (NdiscCache* nd)
  : NdiscCache::Entry::Entry (nd),
    m_registeredTimer (Timer::CANCEL_ON_DESTROY),
    m_tentativeTimer (Timer::CANCEL_ON_DESTROY)
{
  NS_LOG_FUNCTION_NOARGS ();
}

void SixLowPanNdiscCache::SixLowPanEntry::MarkRegistered (uint16_t time)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_type = REGISTERED;

  if (m_registeredTimer.IsRunning ())
    {
      m_registeredTimer.Cancel ();
    }
  m_registeredTimer.SetFunction (&SixLowPanNdiscCache::SixLowPanEntry::FunctionTimeout, this);
  m_registeredTimer.SetDelay (Minutes (time));
  m_registeredTimer.Schedule ();
}

void SixLowPanNdiscCache::SixLowPanEntry::MarkTentative ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_type = TENTATIVE;

  if (m_tentativeTimer.IsRunning ())
    {
      m_tentativeTimer.Cancel ();
    }
  m_tentativeTimer.SetFunction (&SixLowPanNdiscCache::SixLowPanEntry::FunctionTimeout, this);
  m_tentativeTimer.SetDelay (Seconds (SixLowPanNdProtocol::TENTATIVE_NCE_LIFETIME));
  m_tentativeTimer.Schedule ();
}

void SixLowPanNdiscCache::SixLowPanEntry::MarkGarbage ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_type = GARBAGE;
}

bool SixLowPanNdiscCache::SixLowPanEntry::IsRegistered () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return (m_type == REGISTERED);
}

bool SixLowPanNdiscCache::SixLowPanEntry::IsTentative () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return (m_type == TENTATIVE);
}

bool SixLowPanNdiscCache::SixLowPanEntry::IsGarbage () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return (m_type == GARBAGE);
}

void SixLowPanNdiscCache::SixLowPanEntry::FunctionTimeout ()
{
  NS_LOG_FUNCTION_NOARGS ();

  m_ndCache->Remove (this);
  return;
}

Mac64Address SixLowPanNdiscCache::SixLowPanEntry::GetEui64 () const
{
  NS_LOG_FUNCTION (this);
  return m_eui64;
}

void SixLowPanNdiscCache::SixLowPanEntry::SetEui64 (Mac64Address eui)
{
  NS_LOG_FUNCTION (this << eui);
  m_eui64 = eui;
}

} /* namespace ns3 */
