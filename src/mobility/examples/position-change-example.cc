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

/*
 * This example demonstrates the basic functionality of the PositionAware class
 * We create two nodes and install mobility models on them. One of the nodes is moving
 * and the other is stationary. We expect the mobile node
 * to trigger the position change callback and the stationary node to trigger the timeout.
 * We also expect that each node will NOT trigger the unintended callback. */

#include "ns3/config.h"
#include "ns3/constant-velocity-mobility-model.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/nstime.h"
#include "ns3/position-aware-helper.h"
#include "ns3/position-aware.h"
#include "ns3/string.h"
#include "ns3/vector.h"
#include <iostream>
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("PositionChangeExample");

static constexpr double TOLERANCE {1e-8};  //!< for floating-point comparisons

/**
 * An example/test class that utilizes position-aware
 */
class PositionChange
{
public:
  /** callback for tracking position changes
   * @param context The context
   * @param positionAware The PositionAware that triggered the callback */
  void PositionChangeCallback (std::string context, Ptr<const PositionAware> positionAware);
    /** callback for tracking position changes and also checks that it happened at the right time to test
   * @param context The context
   * @param positionAware The PositionAware that triggered the callback */
  void PositionChangeCallback2 (std::string context, Ptr<PositionAware> positionAware);
  /** callback for tracking timeouts
   * @param context The context
   * @param positionAware The PositionAware that triggered the callback */
  void TimeoutCallback (std::string context, Ptr<const PositionAware> positionAware);
  /** callback for detecting errors
   * @param context The context
   * @param positionAware The PositionAware that triggered the callback */
  void CallbackReachedInError (std::string context, Ptr<const PositionAware> positionAware);

  /** Sets up the simulation */
  void Create ();
  /** Runs the simulation */
  void Run ();

  Vector3D m_lastPosition;  ///< tracks the last position of node in question
  Time m_lastTimeout;          ///< tracks the last time a node moved
  Time m_lastTimeCrossed;   ///< last time position change occurred
  NodeContainer m_nodes;    ///< nodes to be used
};

int
main (int argc, char **argv)
{
  PositionChange test;
  test.Create ();
  test.Run ();
  return 0;
}

void
PositionChange::Create ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Creating two nodes");
  m_nodes.Create (3);

  NS_LOG_DEBUG ("Installing mobility models");
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator");
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (m_nodes.Get (0));
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  mobility.Install (m_nodes.Get (1));
  mobility.Install (m_nodes.Get (2));
  // 100 m/s in X direction
  m_nodes.Get (1)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (
    ns3::Vector3D (100.0, 0.0, 0.0));
  auto m = m_nodes.Get (2)->GetObject<ConstantVelocityMobilityModel> ();
  ns3::Vector3D v (25.0, 0.0, 0.0);
  m->SetVelocity (v);
  v.x = -25.0;
  Simulator::Schedule (Seconds (1.0), &ConstantVelocityMobilityModel::SetVelocity, m, v);

  NS_LOG_DEBUG ("Install PositionAware objects");
  // 
  // We want to create a position aware object that notifies us when the
  // mobility model has moved by 50 m. If it fails to do so within 4 seconds,
  // it notifies us that it timed out.
  //
  PositionAwareHelper positionAware (Seconds (4), 50.0);
  // Install a PositionAware object on each of two nodes, one that is moving
  // and one that is stationary
  positionAware.Install (m_nodes);

  NS_LOG_DEBUG ("Connecting Callbacks");
  m_nodes.Get (0)->GetObject<PositionAware> ()->TraceConnect ("0",
    "Timeout", MakeCallback (&PositionChange::TimeoutCallback, this));
  m_nodes.Get (1)->GetObject<PositionAware> ()->TraceConnect ("1", 
    "PositionChange", MakeCallback (&PositionChange::PositionChangeCallback, this));

  // Then we connect the callbacks considered to be an error if they get called.
  // The stationary node should not see any PositionChange callbacks 
  m_nodes.Get (0)->GetObject<PositionAware> ()->TraceConnect ("0",
    "PositionChange", MakeCallback (&PositionChange::CallbackReachedInError, this));
  // The mobile node should not see any Timeout callbacks
  m_nodes.Get (1)->GetObject<PositionAware> ()->TraceConnect ("1",
    "Timeout", MakeCallback (&PositionChange::CallbackReachedInError, this));
  m_nodes.Get (2)->GetObject<PositionAware> ()->TraceConnect ("2",
    "Timeout", MakeCallback (&PositionChange::CallbackReachedInError, this));

  // Initialize the state variables
  m_lastPosition = m_nodes.Get (1)->GetObject<MobilityModel> ()->GetPosition ();
  m_lastTimeout = Seconds (0);

  
}

void
PositionChange::Run ()
{
  NS_LOG_FUNCTION (this);
  Simulator::Stop (Seconds (12));
  Simulator::Run ();
  Simulator::Destroy ();
}

void
PositionChange::PositionChangeCallback (std::string context, Ptr<const PositionAware> positionAware)
{
  NS_LOG_FUNCTION (this << context << positionAware);
  Ptr<MobilityModel> mobility = positionAware->GetObject<MobilityModel> ();
  NS_LOG_DEBUG ("[Node " << context << "]" << " Position Change: " << mobility->GetPosition () );
  NS_ABORT_MSG_IF ((50.0 + TOLERANCE) < CalculateDistance (m_lastPosition, mobility->GetPosition ()),
    "Position change error");
  NS_ABORT_MSG_IF ((50.0 - TOLERANCE) > CalculateDistance (m_lastPosition, mobility->GetPosition ()),
    "Position change error");
  m_lastPosition = mobility->GetPosition ();
}

void
PositionChange::PositionChangeCallback2 (std::string context, Ptr<PositionAware> positionAware)
{
  NS_LOG_FUNCTION (this << context << positionAware);
  PositionChangeCallback(context,positionAware);
  NS_ABORT_MSG_IF ((3.0 + TOLERANCE) < (Simulator::Now() - m_lastTimeCrossed).GetSeconds(),
    "Position crosstime error");
  NS_ABORT_MSG_IF ((3.0 - TOLERANCE) > (Simulator::Now() - m_lastTimeCrossed).GetSeconds(),
    "Position crosstime error");
  //Disable the position aware.
  positionAware->SetDeltaPosition(0.0);
  positionAware->SetTimeout(Seconds(0.0));
}

void
PositionChange::TimeoutCallback (std::string context, Ptr<const PositionAware> positionAware)
{
  NS_LOG_FUNCTION (this << context << positionAware);
  Ptr<MobilityModel> mobility = positionAware->GetObject<MobilityModel> ();
  NS_LOG_DEBUG ("[Node " << context << "]" << " Position Change: " << mobility->GetPosition () );
  NS_ABORT_MSG_IF (Seconds (4) != Simulator::Now () - m_lastTimeout, "Timeout at wrong time");
  m_lastTimeout = Simulator::Now ();
}

void
PositionChange::CallbackReachedInError (std::string context, Ptr<const PositionAware> positionAware)
{
  NS_LOG_FUNCTION (this << context << positionAware);
  NS_FATAL_ERROR ("Callback should not have been called");
}
