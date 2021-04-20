/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Philip Hönnecke
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
#ifndef LR_WPAN_ROUTING_DEVICE_H
#define LR_WPAN_ROUTING_DEVICE_H

#include <ns3/node.h>
#include <ns3/channel.h>
#include <ns3/lr-wpan-net-device.h>
#include <ns3/lr-wpan-route.h>
#include <ns3/lr-wpan-mac-header.h>

namespace ns3 {

class LrWpanRoutingDevice : public NetDevice
{
public:
  static TypeId GetTypeId (void);

  // inherited from NetDevice base class
  virtual void SetIfIndex (const uint32_t index);
  virtual uint32_t GetIfIndex (void) const;
  virtual Ptr<Channel> GetChannel (void) const;
  virtual void SetAddress (Address address);
  virtual Address GetAddress (void) const;
  virtual bool SetMtu (const uint16_t mtu);
  virtual uint16_t GetMtu (void) const;
  virtual bool IsLinkUp (void) const;
  virtual void AddLinkChangeCallback (Callback<void> callback);
  virtual bool IsBroadcast (void) const;
  virtual Address GetBroadcast (void) const;
  virtual bool IsMulticast (void) const;
  virtual Address GetMulticast (Ipv4Address multicastGroup) const;
  virtual bool IsPointToPoint (void) const;
  virtual bool IsBridge (void) const;
  virtual bool Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
  virtual bool SendFrom (Ptr<Packet> packet, const Address &source, const Address &dest,
                         uint16_t protocolNumber);
  virtual Ptr<Node> GetNode (void) const;
  virtual void SetNode (Ptr<Node> node);
  virtual bool NeedsArp (void) const;
  virtual void SetReceiveCallback (NetDevice::ReceiveCallback cb);
  virtual void SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb);
  virtual bool SupportsSendFrom () const;
  virtual Address GetMulticast (Ipv6Address addr) const;

  void SetDevice (Ptr<LrWpanNetDevice> device);
  Ptr<LrWpanNetDevice> GetDevice (void) const;

  /**
   * \brief Receive a packet from a device.
   * \param device The device
   * \param packet The packet
   * \param protocol The protocol
   * \param from The sender
   * \returns True if the packet will be forwarded by the routing protocol.
   */
  virtual void Receive (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                        Address const &source, Address const &destination,
                        NetDevice::PacketType packetType);

  virtual Ptr<LrWpanRoute> GetRouteTo (Address &dest) = 0;

protected:
  Ptr<Node> m_node;
  Ptr<LrWpanNetDevice> m_netDevice;
  NetDevice::ReceiveCallback m_rxCallback;
  NetDevice::PromiscReceiveCallback m_promiscCallback;
  uint32_t m_ifIndex;
};

} // namespace ns3

#endif /*  LR_WPAN_ROUTING_DEVICE_H */