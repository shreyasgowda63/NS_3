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

/** An example/test class that utilizes position-aware */
class PositionChange
{
protected:
  using ThisType = PositionChange; ///< type of this

public:
  /** callback for tracking position changes
   * @param _position_aware The PositionAware that triggered the callback */
  void PositionChangeCallback (Ptr<const PositionAware> _position_aware);
  /** callback for tracking timeouts
   * @param _position_aware The PositionAware that triggered the callback */
  void TimeoutCallback (Ptr<const PositionAware> _position_aware);

  /** Sets up the simulation */
  void Create ();
  /** Runs the simulation */
  void Run ();

  Vector3D m_lastPosition;  ///< tracks the last position of node in question
  Time m_lastTime;          ///< tracks the last time a node moved
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
  std::cout << "Creating Nodes" << std::endl;
  m_nodes.Create (2);

  std::cout << "Installing Mobility" << std::endl;
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator");
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (m_nodes.Get (0));
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  mobility.Install (m_nodes.Get (1));
  m_nodes.Get (1)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (
      ns3::Vector3D (100.0, 0.0, 0.0));

  std::cout << "Install Position Aware" << std::endl;
  PositionAwareHelper position_aware (Seconds (4), 50.0);
  position_aware.Install (m_nodes);

  std::cout << "Connecting Callbacks" << std::endl;
  m_nodes.Get (0)->GetObject<PositionAware> ()->TraceConnectWithoutContext (
      "Timeout", MakeCallback (&ThisType::TimeoutCallback, this));
  m_nodes.Get (1)->GetObject<PositionAware> ()->TraceConnectWithoutContext (
      "PositionChange", MakeCallback (&ThisType::PositionChangeCallback, this));

  m_lastPosition = m_nodes.Get (1)->GetObject<MobilityModel> ()->GetPosition ();
  m_lastTime = ns3::Time ("0s");
}

void
PositionChange::Run ()
{
  Simulator::Stop (Seconds (12));
  Simulator::Run ();
  Simulator::Destroy ();
}

void
PositionChange::PositionChangeCallback (Ptr<const PositionAware> _position_aware)
{
  Ptr<Node> node = _position_aware->GetObject<Node> ();
  Ptr<MobilityModel> mobility = _position_aware->GetObject<MobilityModel> ();
  std::cout << "[Node " << node->GetId () << "]"
            << " Position Change: " << mobility->GetPosition () << std::endl;
  if (50.0 != CalculateDistance (m_lastPosition, mobility->GetPosition ()))
    {
      NS_FATAL_ERROR("Position change error");
    }
  m_lastPosition = mobility->GetPosition ();
}

void
PositionChange::TimeoutCallback (Ptr<const PositionAware> _position_aware)
{
  Ptr<Node> node = _position_aware->GetObject<Node> ();
  Ptr<MobilityModel> mobility = _position_aware->GetObject<MobilityModel> ();
  std::cout << "[Node " << node->GetId () << "]"
            << " Timeout" << std::endl;
  if (Seconds (4) != Simulator::Now () - m_lastTime)
    {
      std::cerr << "Timeout at wrong time" << std::endl;
      exit (-1);
    }
  m_lastTime = Simulator::Now ();
}
