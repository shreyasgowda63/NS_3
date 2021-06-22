/* -*- Mode:C++; c-file-style:"gnu"; indent-
 * Copyright (c) 2021 Institute of Operating Systems and Computer Networks, TU Braunschweig
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
 * Author: Philip Hönnecke <p.hoennecke@tu-braunschweig.de>
 */

#include <iostream>

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lr-wpan-module.h"

using namespace ns3;

/**
 * This example shows how to use the LrWpanGlobalRouting(Device|Helper).
 * The scenario consists of a grid with 20 nodes (2x10) which are placed with 100m distance between "adjacent" nodes.
 * We want to send a packet from node 0 to 19 (one corner of the grid to the opposite one).
 * The multi-hop routing functionality needed for this is provided by the LrWpanGlobalRoutingDevices installed on the LrWpanNetDevices.
 * 
 * The routes for this could either be set up manually (LrWpanStaticRoutingDevice) or automatically, as we do it here.
 * The LrWpanGlobalRoutingHelper instance will tell each LrWpanGlobalRoutingDevice when to send what kind of packet (neighbor discovery)
 * and it will collect the resulting data.
 * After a set amount of time, it will calculate routes for all devices using breadth-first search.
 * The resulting routes are then automatically installed on the devices and the network is operational.
 * 
 * After this, we will send the packet from node 0 to 19 and print a message whenever a LrWpanNetDevice (which is used by our routing devices)
 * receives a packet.
 * This is to show the path taken by the packet.
 */

bool routesCalculated = false;

void
ProtocolHandler (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                 const Address &sender, const Address &receiver, NetDevice::PacketType packetType)
{
  // Only print a message if the routing calculation has been completed yet
  // Otherwise all the transmissions during "neighbor discovery" would be printed as well
  if (routesCalculated)
    {
      std::cout << "A packet was received on node " << device->GetNode ()->GetId () << std::endl;
    }
}

/**
 * @brief This method is called when the LrWpanGlobalRoutingHelper has completed it's calculation of static routes.
 * 
 * @param id The id of the global routing network.
 */
static void
RoutingCalcCompleteCallback (uint16_t id)
{
  // Print a simple message
  std::cout << "The calculation of routes is completed for id at time: "
            << Simulator::Now ().As (Time::S) << std::endl;
  // Change routesCalculated to enable printing of messages upon receiving a packet (see ProtocolHandler method)
  routesCalculated = true;
}

int
main (int argc, char *argv[])
{
  // Create 20 nodes
  NodeContainer nodes;
  nodes.Create (20);

  // Set up the mobility models for the nodes
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator", "GridWidth", UintegerValue (10),
                                 "DeltaX", DoubleValue (100.0), "DeltaY", DoubleValue (100.0));
  mobility.Install (nodes);

  // Install LrWpanNetDevices on nodes
  LrWpanHelper lrWpanHelper;
  NetDeviceContainer devices;
  devices = lrWpanHelper.Install (nodes);
  lrWpanHelper.AssociateToPan (devices, 1);

  // Install the global routing
  LrWpanGlobalRoutingHelper routingHelper;
  // Set the time after which the first discovery packet should be sent
  routingHelper.SetSendDiscoveryTime (Seconds (0.1));
  // Set the time to wait between sending the next discovery packet to prevent interference problems
  routingHelper.SetPacketOffsetTime (Seconds (0.01));
  // Set the time after which the received data should be used to calculate the routes
  routingHelper.SetWaitTime (Seconds (0.9));
  // Set the method to be called when the calculation of routes is completed (i.e. the network is "operational")
  routingHelper.SetRoutingCalcCompleteCallback (MakeCallback (&RoutingCalcCompleteCallback));
  NetDeviceContainer routingDevices = routingHelper.Install (devices, 0);

  // Set up a protocol handler for both nodes.
  // The ProtocolHandler method is called when either of both nodes receives a packet
  for (uint32_t i = 0; i < nodes.GetN (); i++)
    {
      nodes.Get (i)->RegisterProtocolHandler (MakeCallback (&ProtocolHandler), 0, devices.Get (i));
    }

  // Create a packet with dummy data
  Ptr<Packet> packet = Create<Packet> (10);

  // Send packet from node 0 to node 19 after 1 second
  // This could instead also be connected to the RoutingCalcCompleteCallback to make it more dynamic
  Simulator::Schedule (Seconds (1.0), &NetDevice::Send, routingDevices.Get (0), packet,
                       routingDevices.Get (19)->GetAddress (), 0);

  // Stop the simulation after 5 seconds
  Simulator::Stop (Seconds (5.0));
  Simulator::Run ();
  Simulator::Destroy ();
}