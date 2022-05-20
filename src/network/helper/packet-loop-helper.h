/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2022 IITP
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
 * Author: Alexander Krotov <krotov@iitp.ru>
 */
#ifndef PACKET_LOOP_HELPER_H
#define PACKET_LOOP_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-server.h"
namespace ns3 {
/**
 * \brief Creates pairs of PacketSocketClient and PacketSocketServer that keep constant number of packets in flight.
 *
 * This helper adds sequence headers to packets sent by PacketSocketClient and traces reception
 * of them on the PacketSocketServer. It dynamically adjusts the number of packets sent by the PacketSocketClient
 * in such a way that constant number of packets is kept in the loop.
 *
 * Primary use of this helper is to keep the connection saturated without overflowing the queue
 * and the packets being dropped.
 */
class PacketLoopHelper
{
public:
  /**
   * Creates PacketLoopHelper with defined sink address.
   *
   * \param address The server address.
   */
  PacketLoopHelper (PacketSocketAddress address);

  /**
   * Record an attribute to be set in each PacketSocketClient after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetSourceAttribute (std::string name, const AttributeValue &value);

  /**
   * Record an attribute to be set in each PacketSocketServer after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetSinkAttribute (std::string name, const AttributeValue &value);

  /**
   * Create one packet socket client and one packet socket server application.
   *
   * \param sourceNode The node on which to create the source Application.
   * \param sinkNode The node on which to create the sink Application.
   * \param packetsInFlight Number of packets to keep in the loop.
   * \returns The applications created.
   */
  ApplicationContainer Install (Ptr<Node> sourceNode, Ptr<Node> sinkNode, uint32_t packetsInFlight);

private:
  ObjectFactory m_sourceFactory; //!< Object factory.
  ObjectFactory m_sinkFactory; //!< Object factory.

  PacketSocketAddress m_address; //!< Server address.
};

} // namespace ns3

#endif /* PACKET_LOOP_HELPER_H */
