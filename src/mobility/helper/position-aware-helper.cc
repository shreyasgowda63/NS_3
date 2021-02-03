/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2020 National Technology & Engineering Solutions of Sandia,
 * LLC (NTESS). Under the terms of Contract DE-NA0003525 with NTESS,
 * the U.S. Government retains certain rights in this software.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 *
 */

#include "ns3/position-aware-helper.h"

#include "ns3/double.h"
#include "ns3/names.h"
#include "ns3/position-aware.h"
#include "ns3/pointer.h"
#include "ns3/string.h"

NS_LOG_COMPONENT_DEFINE ("PositionAwareHelper");

namespace ns3 {

PositionAwareHelper::PositionAwareHelper (const Time& time, const double& deltaPosition)
{
  SetTimeout (time);
  SetDeltaPosition (deltaPosition);
}

PositionAwareHelper::~PositionAwareHelper ()
{}

void PositionAwareHelper::SetTimeout (const Time& time)
{
  m_timeout = time;
}

Time PositionAwareHelper::GetTimeout () const
{
  return m_timeout;
}

void PositionAwareHelper::SetDeltaPosition (const double& deltaPosition)
{
  m_deltaPosition = deltaPosition;
}

double PositionAwareHelper::GetDeltaPosition () const
{
  return m_deltaPosition;
}

void PositionAwareHelper::Install (Ptr<Node> node) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (0 != node->GetObject<MobilityModel> (),
                 "Must install MobilityModel before PositionAware");
  NS_ASSERT_MSG (0 == node->GetObject<PositionAware> (),
                 "PositionAware Already installed");
  Ptr<PositionAware> positionAware = CreateObject<PositionAware> ();
  positionAware->SetDeltaPosition (m_deltaPosition);
  positionAware->SetTimeout (m_timeout);
  node->AggregateObject (positionAware);
}

void PositionAwareHelper::Install (std::string& nodeName) const
{
  NS_LOG_FUNCTION (this);
  Install (Names::Find<Node> (nodeName));
}

void PositionAwareHelper::Install (NodeContainer container) const
{
  NS_LOG_FUNCTION (this);
  for (NodeContainer::Iterator i = container.Begin (); i != container.End (); ++i)
    {
      Install (*i);
    }
}

void PositionAwareHelper::InstallAll (void)
{
  NS_LOG_FUNCTION (this);
  Install (NodeContainer::GetGlobal ());
}

} //namespace ns3
