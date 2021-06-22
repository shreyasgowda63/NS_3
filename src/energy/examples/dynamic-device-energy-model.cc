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
#include "ns3/energy-module.h"
#include "ns3/lr-wpan-helper.h"

using namespace ns3;

/**
 * This example shows how to create and use the DynamicDeviceEnergyModel.
 * The scenario consists of two nodes with LrWpanNetDevices that send packets between each other.
 * Whenever one node receives a packet (ProtocolHandler method), it's DynamicDeviceEnergyModel
 * will change between different states for 1000ms and then send the same packet back to the other node.
 * This will stop after 10 seconds.
 * 
 * The concrete state changes are (with time being relative to the point when the packet was received):
 * - 0ms: Work state
 * - 500ms: Peak state
 * - 550ms: Work state
 * - 1000ms: Idle state
 * 
 * Both energy models start in the Idle state.
 * The currents for each state are:
 * - Off: 0A (implicit state, is always present and has index 0)
 * - Idle: 3mA
 * - Work: 50mA
 * - Peak: 100mA
 */

int stateIdle;
int stateWork;
int statePeak;
DeviceEnergyModelContainer energyModels;

void
ProtocolHandler (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                 const Address &sender, const Address &receiver, NetDevice::PacketType packetType)
{
  // Find my id to be able to refer to the correct DynamicDeviceEnergyModel
  uint32_t myId = device->GetNode ()->GetId ();
  std::cout << "Received a packet on node " << myId << std::endl;

  // Change into the Work state now
  energyModels.Get (myId)->ChangeState (stateWork);
  // In 500ms, change into the Peak state
  energyModels.Get (myId)->GetObject<DynamicDeviceEnergyModel> ()->ScheduleChangeState (MilliSeconds (500), statePeak);
  // In 550ms, change into the Work state again
  energyModels.Get (myId)->GetObject<DynamicDeviceEnergyModel> ()->ScheduleChangeState (MilliSeconds (550), stateWork);
  // In 1000ms, finally revert into the Idle state again
  energyModels.Get (myId)->GetObject<DynamicDeviceEnergyModel> ()->ScheduleChangeState (MilliSeconds (1000), stateIdle);

  // Create a copy of the received packet
  Ptr<Packet> copiedPacket = packet->Copy ();
  // Also in 1000ms, send the packet back to the partner
  Simulator::Schedule (MilliSeconds (1000), &NetDevice::Send, device, copiedPacket, sender, 0);
}

int
main (int argc, char *argv[])
{
  // Create two nodes
  NodeContainer nodes;
  nodes.Create (2);
  
  // Install LrWpanNetDevices on both nodes
  LrWpanHelper lrWpanHelper;
  NetDeviceContainer devices;
  devices = lrWpanHelper.Install (nodes);
  lrWpanHelper.AssociateToPan (devices, 1);

  // Install BasicEnergySources on both nodes
  BasicEnergySourceHelper sourceHelper;
  sourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (10));
  EnergySourceContainer sources = sourceHelper.Install (nodes);

  // Create the DynamicEnergyModelStates object which will be used for both DynamicDeviceEnergyModels
  Ptr<DynamicEnergyModelStates> states = CreateObject<DynamicEnergyModelStates> ();
  // Add the three states Idle, Work, and Peak
  // Each returned index is stored in a variable for easy access later in the simulation
  stateIdle = states->AddState ("Idle", 0.003);
  stateWork = states->AddState ("Work", 0.05);
  statePeak = states->AddState ("Peak", 0.1);

  // Install DynamicDeviceEnergyModels on both nodes
  DynamicDeviceEnergyModelHelper modelHelper;
  modelHelper.Set ("DynamicEnergyModelStates", PointerValue (states));
  modelHelper.Set ("DefaultState", UintegerValue (stateIdle));
  energyModels = modelHelper.Install (nodes, sources);

  // Set up a protocol handleer for both nodes.
  // The ProtocolHandler method is called when either of both nodes receives a packet
  nodes.Get (0)->RegisterProtocolHandler (MakeCallback (&ProtocolHandler), 0, devices.Get (0));
  nodes.Get (1)->RegisterProtocolHandler (MakeCallback (&ProtocolHandler), 0, devices.Get (1));

  // Create a packet with dummy data to send between the nodes
  Ptr<Packet> packet = Create<Packet> (10);

  // Start sending the first packet from node 0 to node 1 immediately
  devices.Get (0)->Send (packet, devices.Get (1)->GetAddress (), 0);

   // Stop the simulation after 2 seconds
  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();

  // Print out some info about the consumed energy after the simulation has finished
  for (uint32_t i = 0; i < nodes.GetN (); i++)
  {
    std::cout << "Energy stats for node " << i << ":" << std::endl;
    std::cout << "\tInitial Energy: " << sources.Get (i)->GetInitialEnergy () << std::endl;
    std::cout << "\tRemaining Energy: " << sources.Get (i)->GetRemainingEnergy () << std::endl;
  }
  Simulator::Destroy ();
}