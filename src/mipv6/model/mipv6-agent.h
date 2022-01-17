/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 Jadavpur University, India
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
 * Author: Manoj Kumar Rana <manoj24.rana@gmail.com>
 */

#ifndef MIPV6_AGENT_H
#define MIPV6_AGENT_H

#include "ns3/object.h"
#include "ns3/ipv6-address.h"
#include "ns3/traced-value.h"

namespace ns3 {

class Node;
class Packet;
class Ipv6Interface;

/**
 * \class Mipv6Agent
 * \brief Mipv6 Agent Class
 */
class Mipv6Agent : public Object
{
public:
  /**
   * \brief Interface ID
   * \return type ID
   */
  static TypeId GetTypeId ();

  /**
   * \brief Constructor.
   */
  Mipv6Agent ();

  /**
   * \brief Destructor.
   */
  virtual ~Mipv6Agent ();

  /**
   * \brief Set the node.
   * \param node the node to set
   */
  void SetNode (Ptr<Node> node);

  /**
   * \brief Get the node.
   * \return node
   */
  Ptr<Node> GetNode (void);

  /**
   * \brief receive mobility handling packets (BU/BA).
   * \param packet the packet
   * \param src source address
   * \param dst the destination address
   * \param interface the interface where the packet is received
   * \return status
   */
  virtual uint8_t Receive (Ptr<Packet> packet, const Ipv6Address &src, const Ipv6Address &dst, Ptr<Ipv6Interface> interface);

  /**
   * \brief send a mobility handling packets (BU).
   * \param packet the packet
   * \param dst the destination address
   * \param ttl time to live field 
   */
  void SendMessage (Ptr<Packet> packet, Ipv6Address dst, uint32_t ttl);

  /**
   * \brief send a mobility handling packets (BA).
   * \param packet the packet
   * \param src the source address
   * \param dst the destination address
   * \param ttl time to live field 
   */
  void SendReply (Ptr<Packet> packet, Ipv6Address src, Ipv6Address dst, uint32_t ttl);

protected:

  /**
   * \brief hanling packets if BU is received and calls the corresponding function inherited from this class.
   * \param packet the packet
   * \param src source address
   * \param dst the destination address
   * \param interface the interface where the packet is received
   * \return status
   */
  virtual uint8_t HandleBU (Ptr<Packet> packet, const Ipv6Address &src, const Ipv6Address &dst, Ptr<Ipv6Interface> interface);

  /**
   * \brief hanling packets if BA is received and calls the corresponding function inherited from this class.
   * \param packet the packet
   * \param src source address
   * \param dst the destination address
   * \param interface the interface where the packet is received
   * \return status
   */
  virtual uint8_t HandleBA (Ptr<Packet> packet, const Ipv6Address &src, const Ipv6Address &dst, Ptr<Ipv6Interface> interface);

  /**
   * \brief Dispose this object.
   */
  virtual void DoDispose ();

private:
  /**
   * \brief The node.
   */
  Ptr<Node> m_node;

  /**
   * \brief Trace source indicating a transmitted mobility handling packets by this agent 
   */
  TracedCallback<Ptr<const Packet> > m_agentTxTrace;

  /**
   * \brief Trace source indicating a received mobility handling packets by this agent. This is a non-promiscuous trace 
   */
  TracedCallback<Ptr<const Packet> > m_agentRxTrace;

  /**
   * \brief Trace source indicating a received mobility handling packets by this agent. This is a promiscuous trace 
   */
  TracedCallback<Ptr<const Packet> > m_agentPromiscRxTrace;

};

} /* namespace ns3 */

#endif /* MIPV6_AGENT_H */

