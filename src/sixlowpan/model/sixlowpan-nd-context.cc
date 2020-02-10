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

#include <ns3/log.h>
#include "ns3/simulator.h"

#include "sixlowpan-nd-context.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("SixLowPanNdContext");


SixLowPanNdContext::SixLowPanNdContext ()
  : m_c (false),
    m_cid (0),
    m_validTime (Seconds(0)),
    m_context (Ipv6Prefix ())
{
  NS_LOG_FUNCTION (this);
}

SixLowPanNdContext::SixLowPanNdContext (bool flagC, uint8_t cid, Time time, Ipv6Prefix context)
  : m_c (flagC),
    m_cid (cid),
    m_context (context)
{
  NS_LOG_FUNCTION (this << flagC << static_cast<uint32_t> (cid) << time << context);
  SetValidTime (time);
}

SixLowPanNdContext::~SixLowPanNdContext ()
{
  NS_LOG_FUNCTION (this);
}

uint8_t SixLowPanNdContext::GetContextLen () const
{
  NS_LOG_FUNCTION (this);
  return m_context.GetPrefixLength ();
}

bool SixLowPanNdContext::IsFlagC () const
{
  NS_LOG_FUNCTION (this);
  return m_c;
}

void SixLowPanNdContext::SetFlagC (bool c)
{
  NS_LOG_FUNCTION (this << c);
  m_c = c;
}

uint8_t SixLowPanNdContext::GetCid () const
{
  NS_LOG_FUNCTION (this);
  return m_cid;
}

void SixLowPanNdContext::SetCid (uint8_t cid)
{
  NS_LOG_FUNCTION (this << static_cast<uint32_t> (cid));
  NS_ASSERT (cid <= 15);
  m_cid = cid;
}

Time SixLowPanNdContext::GetValidTime () const
{
  NS_LOG_FUNCTION (this);

  return m_validTime;
}

void SixLowPanNdContext::SetValidTime (Time time)
{
  NS_LOG_FUNCTION (this << time);

  uint64_t timeInMillisecs = time.GetMilliSeconds ();
  uint64_t remainder = timeInMillisecs % 60000;

  if (remainder)
    {
      NS_LOG_WARN ("ValidTime must be a multiple of 60 seconds, increasing to the next valid value ");
      m_validTime += MilliSeconds (60000-remainder);
      return;
    }

  m_validTime = time;
}

void SixLowPanNdContext::SetLastUpdateTime (Time time)
{
  NS_LOG_FUNCTION (this << time);
  m_lastUpdateTime = time;
}

Time SixLowPanNdContext::GetLastUpdateTime ()
{
  NS_LOG_FUNCTION (this);
  return m_lastUpdateTime;
}

Ipv6Prefix SixLowPanNdContext::GetContextPrefix () const
{
  NS_LOG_FUNCTION (this);
  return m_context;
}

void SixLowPanNdContext::SetContextPrefix (Ipv6Prefix context)
{
  NS_LOG_FUNCTION (this << context);
  m_context = context;
}

void SixLowPanNdContext::PrintContext (Ptr<OutputStreamWrapper> stream)
{
  NS_LOG_FUNCTION (this << stream);
  std::ostream* os = stream->GetStream ();

  *os << " Context Length: " << GetContextLen ();

  if (IsFlagC ())
    {
      *os << " Compression flag: true ";
    }
  else
    {
      *os << " Compression flag: false ";
    }

  *os << " Context Identifier: " << GetCid ();
  *os << " Valid Lifetime: " << GetValidTime ();
  *os << " Context Prefix: " << GetContextPrefix ();
}

} /* namespace ns3 */
