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

#include "sixlowpan-nd-context.h"


namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("SixLowPanRadvdContext");

SixLowPanNdContext::SixLowPanNdContext (bool flagC, uint8_t cid, uint16_t time, Ipv6Prefix context)
  : m_c (flagC),
    m_cid (cid),
    m_validTime (time),
    m_context (context)
{
  NS_LOG_FUNCTION (this << flagC << static_cast<uint32_t> (cid) << time << context);

  m_length = context.GetPrefixLength ();
}

SixLowPanNdContext::~SixLowPanNdContext ()
{
  NS_LOG_FUNCTION (this);
}

uint8_t SixLowPanNdContext::GetContextLen () const
{
  NS_LOG_FUNCTION (this);
  return m_length;
}

void SixLowPanNdContext::SetContextLen (uint8_t length)
{
  NS_LOG_FUNCTION (this << static_cast<uint32_t> (length));
  m_length = length;
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

uint16_t SixLowPanNdContext::GetValidTime () const
{
  NS_LOG_FUNCTION (this);
  return m_validTime;
}

void SixLowPanNdContext::SetValidTime (uint16_t time)
{
  NS_LOG_FUNCTION (this << time);
  m_validTime = time;
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

} /* namespace ns3 */
