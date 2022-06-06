/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 INRIA, 2012 University of Washington
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
 */

#ifndef L3_EMU_FD_NET_DEVICE_H
#define L3_EMU_FD_NET_DEVICE_H

#include "ns3/address.h"
#include "ns3/callback.h"
#include "ns3/fd-net-device.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"

namespace ns3 {

class Node;

/**
 * \ingroup fd-net-device
 *
 * \brief a NetDevice to read/write layer 3 traffic from/to a file descriptor.
 *
 * A L3EmuFdNetDevice object will read and write packets from/to a file
 * descriptor. This file descriptor might be associated to a Linux GRE device,
 * allowing the simulation to exchange layer 3 traffic with the "outside-world"
 */
class L3EmuFdNetDevice : public FdNetDevice
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Constructor for the L3EmuFdNetDevice.
   */
  L3EmuFdNetDevice ();

  /**
   * Destructor for the L3EmuFdNetDevice.
   */
  virtual ~L3EmuFdNetDevice ();

  // inherited from NetDevice base class.
  virtual bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);
  virtual bool NeedsArp (void) const;
  virtual void SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb);

private:
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse as suggested in
   * http://www.nsnam.org/wiki/NS-3_Python_Bindings#.22invalid_use_of_incomplete_type.22
   */
  L3EmuFdNetDevice (L3EmuFdNetDevice const &);

  /**
   * Forward the frame to the appropriate callback for processing
   */
  void ForwardUp (void);

  /**
   * The MTU associated to the file descriptor technology
   */
  uint16_t m_mtu;

  /**
   * The callback used to notify higher layers that a packet has been received in promiscuous mode.
   */
  NetDevice::PromiscReceiveCallback m_promiscRxCallback;

  /**
   * A trace source that emulates a promiscuous mode protocol sniffer connected
   * to the device.  This trace source fire on packets destined for any host
   * just like your average everyday packet sniffer.
   *
   * On the transmit size, this trace hook will fire after a packet is dequeued
   * from the device queue for transmission.  In Linux, for example, this would
   * correspond to the point just before a device hard_start_xmit where
   * dev_queue_xmit_nit is called to dispatch the packet to the PF_PACKET
   * ETH_P_ALL handlers.
   *
   * On the receive side, this trace hook will fire when a packet is received,
   * just before the receive callback is executed.  In Linux, for example,
   * this would correspond to the point at which the packet is dispatched to
   * packet sniffers in netif_receive_skb.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_promiscSnifferTrace;
};

} // namespace ns3

#endif /* L3_EMU_FD_NET_DEVICE_H */

