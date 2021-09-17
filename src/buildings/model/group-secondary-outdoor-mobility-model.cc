/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 University of Padova
 * Copyright (c) 2020 Institute for the Wireless Internet of Things, Northeastern University, Boston, MA
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
 * Author: Michele Polese <michele.polese@gmail.com>
 */

#include "group-secondary-outdoor-mobility-model.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/building.h"
#include "ns3/building-list.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (GroupSecondaryOutdoorMobilityModel);
NS_LOG_COMPONENT_DEFINE ("GroupSecondaryOutdoorMobilityModel");

TypeId
GroupSecondaryOutdoorMobilityModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GroupSecondaryOutdoorMobilityModel")
    .SetParent<GroupSecondaryMobilityModel> ()
    .SetGroupName ("Buildings")
    .AddConstructor<GroupSecondaryOutdoorMobilityModel> ();
  return tid;
}

bool
GroupSecondaryOutdoorMobilityModel::CheckForSpecialConditions (Vector position)
{
  return IsOutdoor (position);
}

bool
GroupSecondaryOutdoorMobilityModel::IsOutdoor (Vector position)
{
  for (BuildingList::Iterator bit = BuildingList::Begin (); bit != BuildingList::End (); ++bit)
    {
      if ((*bit)->IsInside (position)) // the position is inside a building
        {
          NS_LOG_LOGIC ("Indoor");
          return false;
        }
    }
  return true;
}

}