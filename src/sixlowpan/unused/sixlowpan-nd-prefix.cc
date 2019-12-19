/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 Università di Firenze, Italy
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
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "sixlowpan-nd-prefix.h"
#include <ns3/log.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("SixLowPanNdPrefix");

SixLowPanNdPrefix::SixLowPanNdPrefix (Ipv6Address network, uint8_t prefixLength, uint32_t preferredLifeTime, uint32_t validLifeTime, bool autonomousFlag, bool routerAddrFlag)
  : m_network (network),
    m_prefixLength (prefixLength),
    m_preferredLifeTime (preferredLifeTime),
    m_validLifeTime (validLifeTime),
    m_autonomousFlag (autonomousFlag),
    m_routerAddrFlag (routerAddrFlag)
{
  NS_LOG_FUNCTION (this << network << prefixLength << preferredLifeTime << validLifeTime << autonomousFlag << routerAddrFlag);
}

SixLowPanNdPrefix::~SixLowPanNdPrefix ()
{
  NS_LOG_FUNCTION (this);
}

Ipv6Address SixLowPanNdPrefix::GetNetwork () const
{
  NS_LOG_FUNCTION (this);
  return m_network;
}

void SixLowPanNdPrefix::SetNetwork (Ipv6Address network)
{
  NS_LOG_FUNCTION (this << network);
  m_network = network;
}

uint8_t SixLowPanNdPrefix::GetPrefixLength () const
{
  NS_LOG_FUNCTION (this);
  return m_prefixLength;
}

void SixLowPanNdPrefix::SetPrefixLength (uint8_t prefixLength)
{
  NS_LOG_FUNCTION (this << prefixLength);
  m_prefixLength = prefixLength;
}

uint32_t SixLowPanNdPrefix::GetValidLifeTime () const
{
  NS_LOG_FUNCTION (this);
  return m_validLifeTime;
}

void SixLowPanNdPrefix::SetValidLifeTime (uint32_t validLifeTime)
{
  NS_LOG_FUNCTION (this << validLifeTime);
  m_validLifeTime = validLifeTime;
}

uint32_t SixLowPanNdPrefix::GetPreferredLifeTime () const
{
  NS_LOG_FUNCTION (this);
  return m_preferredLifeTime;
}

void SixLowPanNdPrefix::SetPreferredLifeTime (uint32_t preferredLifeTime)
{
  NS_LOG_FUNCTION (this << preferredLifeTime);
  m_preferredLifeTime = preferredLifeTime;
}

bool SixLowPanNdPrefix::IsAutonomousFlag () const
{
  NS_LOG_FUNCTION (this);
  return m_autonomousFlag; 
}

void SixLowPanNdPrefix::SetAutonomousFlag (bool autonomousFlag)
{
  NS_LOG_FUNCTION (this << autonomousFlag);
  m_autonomousFlag = autonomousFlag;
}

bool SixLowPanNdPrefix::IsRouterAddrFlag () const
{
  NS_LOG_FUNCTION (this);
  return m_routerAddrFlag;
}

void SixLowPanNdPrefix::SetRouterAddrFlag (bool routerAddrFlag)
{
  NS_LOG_FUNCTION (this << routerAddrFlag);
  m_routerAddrFlag = routerAddrFlag;
}

} /* namespace ns3 */

