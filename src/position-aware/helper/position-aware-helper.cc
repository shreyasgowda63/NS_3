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

namespace ns3{

PositionAwareHelper::PositionAwareHelper()
{
  //position_aware_factory_.SetTypeId("ns3::PositionAware");
  SetTimeout(Seconds(10));
  SetDistance(100);
}

PositionAwareHelper::PositionAwareHelper(const Time& _t, const double& _d)
{
  //position_aware_factory_.SetTypeId("ns3::PositionAware");
  SetTimeout(_t);
  SetDistance(_d);
}

PositionAwareHelper::~PositionAwareHelper()
{
}

void PositionAwareHelper::SetTimeout(const Time& _t)
{
  //position_aware_factory_.Set("Timeout",TimeValue(_t));
  timeout_ = _t;
}

Time PositionAwareHelper::GetTimeout() const
{
  return timeout_;
}

void PositionAwareHelper::SetDistance(const double& _d)
{
  //position_aware_factory_.Set("Distance",DoubleValue(_d));
  distance_ = _d;
}

double PositionAwareHelper::GetDistance() const
{
  return distance_;
}

void PositionAwareHelper::Install(Ptr<Node> _node) const
{
  Ptr<Object> object = _node;
  NS_ASSERT_MSG(0 != object->GetObject<MobilityModel>(),
                "Must install MobilityModel before PositionAware");
  NS_ASSERT_MSG(0 == object->GetObject<PositionAware>() ,
                "PositionAware Already installed");
  Ptr<PositionAware> position_aware = CreateObject<PositionAware>();//position_aware_factory_.Create()->GetObject<PositionAware>();
  position_aware->SetDistance(distance_);
  position_aware->SetTimeout(timeout_);
  object->AggregateObject(position_aware);
}

void PositionAwareHelper::Install(std::string& _node_name) const
{
  Install(Names::Find<Node>(_node_name));
}

void PositionAwareHelper::Install(NodeContainer _container) const
{
  for(NodeContainer::Iterator i = _container.Begin(); i != _container.End(); ++i)
    Install(*i);
}

void PositionAwareHelper::InstallAll(void)
{
  Install(NodeContainer::GetGlobal());
}

}//namespace ns3
