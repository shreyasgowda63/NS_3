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

#include "sixlowpan-nd-prefix.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("SixLowPanNdPrefix");

SixLowPanNdPrefix::SixLowPanNdPrefix ()
{
  NS_LOG_FUNCTION (this);
}

SixLowPanNdPrefix::SixLowPanNdPrefix (Ipv6Address prefix, uint8_t prefixLen, Time prefTime, Time validTime, uint8_t flags)
  : m_prefix (prefix),
    m_prefixLength (prefixLen),
    m_preferredLifeTime (prefTime),
    m_validLifeTime (validTime),
    m_flags (flags)
{
  NS_LOG_FUNCTION (this << prefix << prefixLen << prefTime << validTime << flags);
}

SixLowPanNdPrefix::~SixLowPanNdPrefix ()
{
  NS_LOG_FUNCTION (this);
}

Ipv6Address SixLowPanNdPrefix::GetPrefix () const
{
  NS_LOG_FUNCTION (this);
  return m_prefix;
}

void SixLowPanNdPrefix::SetPrefix (Ipv6Address prefix)
{
  NS_LOG_FUNCTION (this << prefix);
  m_prefix = prefix;
}

uint8_t SixLowPanNdPrefix::GetPrefixLength () const
{
  NS_LOG_FUNCTION (this);
  return m_prefixLength;
}

void SixLowPanNdPrefix::SetPrefixLength (uint8_t prefixLen)
{
  NS_LOG_FUNCTION (this << prefixLen);
  m_prefixLength = prefixLen;
}

Time SixLowPanNdPrefix::GetValidLifeTime () const
{
  NS_LOG_FUNCTION (this);

  return m_validLifeTime;
}

void SixLowPanNdPrefix::SetValidLifeTime (Time validTime)
{
  NS_LOG_FUNCTION (this << validTime);
  m_validLifeTime = validTime;
}

Time SixLowPanNdPrefix::GetPreferredLifeTime () const
{
  NS_LOG_FUNCTION (this);
  return m_preferredLifeTime;
}

void SixLowPanNdPrefix::SetPreferredLifeTime (Time prefTime)
{
  NS_LOG_FUNCTION (this << prefTime);
  m_preferredLifeTime = prefTime;
}

uint8_t SixLowPanNdPrefix::GetFlags () const
{
  NS_LOG_FUNCTION (this);
  return m_flags;
}

void SixLowPanNdPrefix::SetFlags (uint8_t flags)
{
  NS_LOG_FUNCTION (this << flags);
  m_flags = flags;
}

void SixLowPanNdPrefix::PrintPrefix (Ptr<OutputStreamWrapper> stream)
{
  NS_LOG_FUNCTION (this << stream);
  std::ostream* os = stream->GetStream ();

  *os << " Prefix Length: " << GetPrefixLength ();

  if (GetFlags () & (1 << 7))
    {
      *os << " On-link flag: true ";
    }
  else
    {
      *os << " On-link flag: false ";
    }
  if (GetFlags () & (1 << 6))
    {
      *os << " Autonomous flag: true ";
    }
      else
    {
      *os << " Autonomous flag: false ";
    }
  if (GetFlags () & (1 << 5))
    {
      *os << " Router address flag: true ";
    }
  else
    {
      *os << " Router address flag: false ";
    }

  *os << " Valid Lifetime: " << GetValidLifeTime ();
  *os << " Preferred Lifetime: " << GetPreferredLifeTime ();
  *os << " Prefix: " << GetPrefix ();
}

} /* namespace ns3 */
