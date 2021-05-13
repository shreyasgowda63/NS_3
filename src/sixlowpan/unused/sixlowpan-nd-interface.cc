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
#include "sixlowpan-nd-dad-entry.h"
#include "sixlowpan-nd-interface.h"

namespace ns3 
{

NS_LOG_COMPONENT_DEFINE ("SixLowPanRadvdInterface");

SixLowPanNdInterface::SixLowPanNdInterface (uint32_t interface)
{
  NS_LOG_FUNCTION (this << interface);
  
  m_interface = interface;
  m_abroVersion = 0;
  m_abroValidLifeTime = 10000;
}

SixLowPanNdInterface::~SixLowPanNdInterface ()
{
  NS_LOG_FUNCTION (this);
  /* clear prefixes */
  for (SixLowPanNdContextListI it = m_contexts.begin (); it != m_contexts.end (); ++it)
    {
      (*it) = 0;
    }
  m_contexts.clear ();
}

uint32_t SixLowPanNdInterface::GetInterface () const
{
  NS_LOG_FUNCTION (this);
  return m_interface;
}

uint32_t SixLowPanNdInterface::GetReachableTime () const
{
  NS_LOG_FUNCTION (this);
  return m_reachableTime;
}

void SixLowPanNdInterface::SetReachableTime (uint32_t reachableTime)
{
  NS_LOG_FUNCTION (this << reachableTime);
  m_reachableTime = reachableTime;
}

uint32_t SixLowPanNdInterface::GetDefaultLifeTime () const
{
  NS_LOG_FUNCTION (this);
  return m_defaultLifeTime;
}

void SixLowPanNdInterface::SetDefaultLifeTime (uint32_t defaultLifeTime)
{
  NS_LOG_FUNCTION (this << defaultLifeTime);
  m_defaultLifeTime = defaultLifeTime;
}

void SixLowPanNdInterface::AddContext (Ptr<SixLowPanNdContext> context)
{
  NS_LOG_FUNCTION (this << context);
  m_contexts.push_back (context);
}

Ipv6Address SixLowPanNdInterface::GetPioNetwork () const
{
  return m_pioNetwork;
}

void SixLowPanNdInterface::SetPioNetwork (Ipv6Address network)
{
  NS_LOG_FUNCTION (this << network);
  m_pioNetwork = network;
}

uint32_t SixLowPanNdInterface::GetPioValidLifeTime () const
{
  NS_LOG_FUNCTION (this);
  return m_pioValidLifeTime;
}

void SixLowPanNdInterface::SetPioValidLifeTime (uint32_t validLifeTime)
{
  NS_LOG_FUNCTION (this << validLifeTime);
  m_pioValidLifeTime = validLifeTime;
}

uint32_t SixLowPanNdInterface::GetPioPreferredLifeTime () const
{
  NS_LOG_FUNCTION (this);
  return m_pioPreferredLifeTime;
}

void SixLowPanNdInterface::SetPioPreferredLifeTime (uint32_t preferredLifeTime)
{
  NS_LOG_FUNCTION (this << preferredLifeTime);
  m_pioPreferredLifeTime = preferredLifeTime;
}

std::list<Ptr<SixLowPanNdContext> > SixLowPanNdInterface::GetContexts () const
{
  NS_LOG_FUNCTION (this);
  return m_contexts;
}

void SixLowPanNdInterface::AddDadEntry (Ptr<SixLowPanNdDadEntry> entry)
{
  NS_LOG_FUNCTION (this << entry);
  m_dadTable.push_back (entry);
}

std::list<Ptr<SixLowPanNdDadEntry> > SixLowPanNdInterface::GetDadTable () const
{
  NS_LOG_FUNCTION (this);
  return m_dadTable;
}

uint32_t SixLowPanNdInterface::GetAbroVersion () const
{
  NS_LOG_FUNCTION (this);
  return m_abroVersion;
}

void SixLowPanNdInterface::SetAbroVersion (uint32_t version)
{
  NS_LOG_FUNCTION (this << version);
  m_abroVersion = version;
}

uint16_t SixLowPanNdInterface::GetAbroValidLifeTime () const
{
  NS_LOG_FUNCTION (this);
  return m_abroValidLifeTime;
}

void SixLowPanNdInterface::SetAbroValidLifeTime (uint16_t time)
{
  NS_LOG_FUNCTION (this << time);
  m_abroValidLifeTime = time;
}

} /* namespace ns3 */
