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
#ifndef LR_WPAN_GLOBAL_ROUTING_DEVICE_H
#define LR_WPAN_GLOBAL_ROUTING_DEVICE_H

#include <ns3/lr-wpan-static-routing-device.h>
#include <ns3/net-device-container.h>

namespace ns3 {

class LrWpanGlobalRoutingDevice : public LrWpanStaticRoutingDevice
{
public:
  static TypeId GetTypeId (void);

  LrWpanGlobalRoutingDevice (uint16_t id);
  ~LrWpanGlobalRoutingDevice ();

  virtual void Receive (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                        Address const &source, Address const &destination, PacketType packetType);

  /**
   * \brief Callback for receiving a discovery transmission on a device.
   * 
   * \param lrWpanGlobalRoutingDevice Receiving device
   * \param address Sender's address
   * \param id The LrWpanGlobalRoutingHelper's id
   */
  typedef Callback<void, Ptr<LrWpanGlobalRoutingDevice>, const Address &, uint16_t>
      TransmissionReceivedCallback;

  void SetTransmissionReceivedCallback (TransmissionReceivedCallback cb);

  void SendDiscoveryTransmission ();

private:
  uint16_t m_globalRoutingId;

  TransmissionReceivedCallback m_transmissionReceivedCallback;
};

} // namespace ns3

#endif /*  LR_WPAN_GLOBAL_ROUTING_DEVICE_H */