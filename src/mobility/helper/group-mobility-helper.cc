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

#include "ns3/group-mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/position-allocator.h"
#include "ns3/group-secondary-mobility-model.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/config.h"
#include "ns3/simulator.h"
#include "ns3/names.h"
#include "ns3/string.h"
#include <iostream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GroupMobilityHelper");

GroupMobilityHelper::GroupMobilityHelper ()
{
  m_mobilityHelper = 0;
}

GroupMobilityHelper::~GroupMobilityHelper ()
{
  m_mobilityHelper = 0;
}

MobilityHelper*
GroupMobilityHelper::GetMobilityHelper ()
{
  return m_mobilityHelper;
}

TypeId
GroupMobilityHelper::GetTypeId (void)
{
  static TypeId
    tid =
    TypeId ("ns3::GroupMobilityHelper")
    .SetParent<Object> ()
    .AddConstructor<GroupMobilityHelper> ()
    .AddAttribute ("GroupSecondaryMobilityModel",
                   "A string the specifies which secondary mobility model should be used.",
                   StringValue ("ns3::GroupSecondaryMobilityModel"),
                   MakeStringAccessor (&GroupMobilityHelper::m_secondaryMobilityModel),
                   MakeStringChecker ())
    .AddAttribute ("PathDeviationRandomVariable",
                   "A random variable used to pick the deviations (in each direction) from the primary position.",
                   StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=3]"),
                   MakeStringAccessor (&GroupMobilityHelper::m_randomVarString),
                   MakeStringChecker ())
  ;
  return tid;
}

void
GroupMobilityHelper::SetMobilityHelper (MobilityHelper* helper)
{
  m_mobilityHelper = helper;
  m_privateMobilityHelper.SetMobilityModel (m_secondaryMobilityModel,
                                            "RandomVariable", StringValue (m_randomVarString));
}

NodeContainer
GroupMobilityHelper::InstallGroupMobility (NodeContainer nodes)
{
  // check that the MobilityHelper is valid
  NS_ABORT_MSG_IF (m_mobilityHelper == 0, "MobilityHelper not set");

  // create the primary node
  Ptr<Node> referenceNode = CreateObject<Node>();
  NodeContainer toBeReturned;
  toBeReturned.Add (referenceNode);

  // install the mobilitymodel in the referenceNode
  (*m_mobilityHelper).Install (referenceNode);
  Ptr<MobilityModel> referenceMobilityModel = referenceNode->GetObject<MobilityModel>();
  Vector referencePosition = referenceMobilityModel->GetPosition ();

  NS_LOG_INFO ("Reference starting position " << referencePosition);

  // install the secondary MobilityModel in the other nodes
  for (auto nodeIt = nodes.Begin (); nodeIt != nodes.End (); ++nodeIt)
    {
      NS_LOG_INFO ("Install secondary mobility in normal nodes");
      m_privateMobilityHelper.Install ((*nodeIt));
      (*nodeIt)->GetObject<GroupSecondaryMobilityModel>()->SetPrimaryMobilityModel (referenceMobilityModel);
      (*nodeIt)->GetObject<MobilityModel>()->SetPosition (referencePosition);
    }

  toBeReturned.Add (nodes);

  return toBeReturned;
}

}