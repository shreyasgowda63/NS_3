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

#include "group-secondary-mobility-model.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/config.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (GroupSecondaryMobilityModel);
NS_LOG_COMPONENT_DEFINE ("GroupSecondaryMobilityModel");

TypeId
GroupSecondaryMobilityModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GroupSecondaryMobilityModel")
    .SetParent<MobilityModel> ()
    .SetGroupName ("Mobility")
    .AddConstructor<GroupSecondaryMobilityModel> ()
    .AddAttribute ("RandomVariable",
                   "A random variable used to pick the deviations (in each direction) from the primary position.",
                   StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=3]"),
                   MakePointerAccessor (&GroupSecondaryMobilityModel::m_randomVar),
                   MakePointerChecker<RandomVariableStream> ())
  ;
  return tid;
}

GroupSecondaryMobilityModel::GroupSecondaryMobilityModel ()
{
}
GroupSecondaryMobilityModel::~GroupSecondaryMobilityModel ()
{
}


void
GroupSecondaryMobilityModel::PrimaryCourseChanged (Ptr<MobilityModel const> primary)
{
  NS_ABORT_MSG_IF (primary != m_primary, "cb primary and private m_primary are not the same");

  // get the primary position
  Vector primaryPosition = m_primary->GetPosition ();
  // get a new position and randomize until a valid position is found
  do
    {
      m_lastPosition = Vector (
          primaryPosition.x + m_randomVar->GetValue (),
          primaryPosition.y + m_randomVar->GetValue (),
          primaryPosition.z
          );
    }
  while (!CheckForSpecialConditions (m_lastPosition));

  NS_LOG_INFO ("Primary position " << primaryPosition
                                  << " randomizedPosition " << m_lastPosition);
  NotifyCourseChange ();
}

void
GroupSecondaryMobilityModel::SetPrimaryMobilityModel (Ptr<MobilityModel> model)
{
  m_primary = model;

  // register the callback to PrimaryCourseChanged
  auto nodeid = model->GetObject<Node>()->GetId ();
  std::ostringstream oss;
  oss << "/NodeList/" << nodeid << "/$ns3::MobilityModel/CourseChange";
  Config::ConnectWithoutContext (oss.str (),
                                 MakeCallback (&GroupSecondaryMobilityModel::PrimaryCourseChanged, this));

  // get the primary position
  Vector primaryPosition = m_primary->GetPosition ();
  // get the initial position
  do
    {
      m_lastPosition = Vector (
          primaryPosition.x + m_randomVar->GetValue (),
          primaryPosition.y + m_randomVar->GetValue (),
          primaryPosition.z
          );
    }
  while (!CheckForSpecialConditions (m_lastPosition));
}

bool
GroupSecondaryMobilityModel::CheckForSpecialConditions (Vector position)
{
  // no special conditions to check here, simply a stub method
  // that it easy to extend
  return true;
}


Vector
GroupSecondaryMobilityModel::DoGetPosition (void) const
{
  return m_lastPosition;
}

void
GroupSecondaryMobilityModel::DoSetPosition (const Vector &position)
{
  NotifyCourseChange ();
}

Vector
GroupSecondaryMobilityModel::DoGetVelocity (void) const
{
  return m_primary->GetVelocity ();
}

} // namespace ns3
