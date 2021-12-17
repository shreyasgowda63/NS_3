/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 INRIA
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
 */

#include "simulator-impl.h"
#include "log.h"

/**
 * \file
 * \ingroup simulator
 * ns3::SimulatorImpl implementation.
 */

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimulatorImpl");

NS_OBJECT_ENSURE_REGISTERED (SimulatorImpl);

TypeId
SimulatorImpl::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimulatorImpl")
    .SetParent<Object> ()
    .SetGroupName ("Core")
  ;
  return tid;
}

void
SimulatorImpl::BoundLookahead (const Time lookahead)
{
  NS_LOG_FUNCTION (this << lookahead);
  // This method is only useful conservative parallel DES implementations
  // A default that does nothing for non-parallel implementations.
}

Time
SimulatorImpl::GetLookahead (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  // For non-parallel DES implementations there is no lookahead needed so return infinity, time
  // advancement does not require lookahead constraints.
  return Time::Max();
}

} // namespace ns3
