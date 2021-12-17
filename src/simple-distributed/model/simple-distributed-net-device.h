/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *  Copyright 2020. Lawrence Livermore National Security, LLC.
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
 * Author: Steven Smith <smith84@llnl.gov>
 *
 * Based on SimpleNetDevice by Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef SIMPLE_DISTRIBUTED_NET_DEVICE_H
#define SIMPLE_DISTRIBUTED_NET_DEVICE_H

#include "ns3/traced-callback.h"
#include "ns3/net-device.h"
#include "ns3/data-rate.h"
#include "ns3/event-id.h"
#include "ns3/vector.h"
#include "ns3/mac48-address.h"

#include <map>
#include <stdint.h>
#include <string>

namespace ns3 {

template <typename Item>
class Queue;
class SimpleDistributedChannel;
class Node;
class ErrorModel;
class NetDeviceQueueInterface;

/**
 * \defgroup simple-distributed Simple-Distributed Network Device
 * This section documents the API of the ns-3 simple-distributed module. For a
 * functional description, please refer to the ns-3 manual here:
 * http://www.nsnam.org/docs/models/html/simple-distributed.html
 *
 * Be sure to read the manual BEFORE going down to the API.
 */


/**
 * \ingroup simple-distributed
 * \brief Simple distributed net device for simple parallel runs and testing
 *
 * This net device does not model any a network to any level of
 * fidelity; it models a fully connected network with no collision.
 *
 * By default net device does not add any delay to the packets; delays
 * may be added by setting the "Delay" and "DataRate" attributes
 * and/or setting the "DelayModel" on the net device.  There is also
 * the possibility to add an ErrorModel if you want to force losses on
 * the device.
 *
 * The total transmission delay from the net device is computed as: Delay
 * + PacketSize * DataRate.
 *
 * One may also add transmission delay on the
 * SimpleDistributedChannel; the delay contributions from both the
 * channel and net device are added.
 *
 * This device assumes 48-bit mac addressing.
 *
 * The device can be installed on a node through the SimpleDistributedNetDeviceHelper.
 * In case of manual creation, the user is responsible for assigning an unique
 * address to the device.
 *
 */
class SimpleDistributedNetDevice : public NetDevice
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  SimpleDistributedNetDevice ();

  /**
   * Receive a packet from a connected SimpleDistributedChannel.  The
   * SimpleDistributedNetDevice receives packets from its connected channel
   * and then forwards them by calling its rx callback method
   *
   * \param packet Packet received on the channel
   * \param protocol Protocol number
   * \param to Address packet should be sent to
   * \param from Address packet was sent from
   */
  void Receive (Ptr<Packet> packet, uint16_t protocol, Mac48Address to, Mac48Address from);

  /**
   * Attach a channel to this net device.  This will be the
   * channel the net device sends on
   *
   * \param channel channel to assign to this net device
   *
   */
  void SetChannel (Ptr<SimpleDistributedChannel> channel);

  /**
   * Attach a queue to the SimpleDistributedNetDevice.
   *
   * \param queue The new queue
   */
  void SetQueue (Ptr<Queue<Packet> > queue);

  /**
   * Get a copy of the attached Queue.
   *
   * \returns Ptr to the queue.
   */
  Ptr<Queue<Packet> > GetQueue (void) const;

  /**
   * Attach a receive ErrorModel to the SimpleDistributedNetDevice.
   *
   * The SimpleDistributedNetDevice may optionally include an ErrorModel in
   * the packet receive chain.
   *
   * \see ErrorModel
   * \param errorModel The ErrorModel
   */
  void SetReceiveErrorModel (Ptr<ErrorModel> errorModel);

  /**
   * Receive a packet from a connected a remote channel.
   *
   * Packet must have a SimpleDistributedTag attached.
   *
   * \param p Ptr to the received packet.
   */
  void ReceiveRemote (Ptr<Packet> p);

  /**
   * Get the delay used for transmission of packets.
   */
  Time GetDelay (void);

  /**
   * Set the delay used for transmission of packets.
   *
   * \param delay the delay time
   */
  void SetDelay (Time delay);

  /**
   * Get the data rate used for transmission of packets.
   */
  DataRate GetDataRate (void);

  /**
   * Set the data rate used for transmission of packets.
   *
   * \param rate the data rate at which this object operates
   */
  void SetDataRate (DataRate rate);

  /**
   * Set the interframe gap used to separate packets.  The interframe gap
   * defines the minimum space required between packets sent by this device.
   *
   * Default is 0s
   *
   * \param t the interframe gap time
   */
  void SetInterframeGap (Time t);

  // inherited from NetDevice base class.
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
  virtual bool Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber);
  virtual bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);
  virtual Ptr<Node> GetNode (void) const;
  virtual void SetNode (Ptr<Node> node);
  virtual bool NeedsArp (void) const;
  virtual void SetReceiveCallback (NetDevice::ReceiveCallback cb);
  virtual Address GetMulticast (Ipv6Address addr) const;
  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb);
  virtual bool SupportsSendFrom (void) const;

protected:
  virtual void DoDispose (void);

private:

  /**
   * Add a standard header to the packet.
   * 
   * Since simple distributed is a made-up technology is has no
   * header.  For easier processing of sniffers (e.g. writing to a
   * PCAP file) a header is added to packets passed to trace
   * callbacks.
   */
  void AddHeader (Ptr<Packet> p,   Mac48Address source,  Mac48Address dest,  uint16_t protocolNumber);

  /**
   * Process incoming packets.
   *
   * In order to enforce a deterministic ordering on incoming remote
   * packets the packets at each timestep are first queued then
   * ProcessRemote will sort and schedule the receive events.  The
   * incoming MPI messages from remote ranks can arrive in a
   * non-deterministic ordering.
   *
   * \note The ordering is currently biased.
   */
  void ProcessRemote (void);

  static const uint16_t DEFAULT_MTU = 1500; //!< Default MTU

  Ptr<SimpleDistributedChannel> m_channel; //!< the channel the device is connected to
  NetDevice::ReceiveCallback m_rxCallback; //!< Receive callback
  NetDevice::PromiscReceiveCallback m_promiscCallback; //!< Promiscuous receive callback
  Ptr<Node> m_node; //!< Node this netDevice is associated to
  Ptr<NetDeviceQueueInterface> m_queueInterface;   //!< NetDevice queue interface

  uint16_t m_mtu;   //!< MTU
  uint32_t m_ifIndex; //!< Interface index
  Mac48Address m_address; //!< MAC address
  Ptr<ErrorModel> m_receiveErrorModel; //!< Receive error model.

  /**
   * The trace source fired when the phy layer drops a packet it has received
   * due to the error model being active.  Although SimpleDistributedNetDevice doesn't
   * really have a Phy model, we choose this trace source name for alignment
   * with other trace sources.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_phyRxDropTrace;

  /**
   * The TransmitComplete method is used internally to finish the process
   * of sending a packet out on the channel.
   */
  void TransmitComplete (void);

  bool m_linkUp; //!< Flag indicating whether or not the link is up

  /**
   * Flag indicating whether or not the NetDevice is a Point to Point model.
   * Enabling this will disable Broadcast and Arp.
   */
  bool m_pointToPointMode;

  Ptr<Queue<Packet> > m_queue; //!< The Queue for outgoing packets.

  /**
   * The delay that the Net Device uses to simulate packet transmission
   * timing.
   */
  Time m_delay; //!< The device nominal delay time.

  /**
   * The data rate that the Net Device uses to simulate packet transmission
   * timing. A zero value means infinite bps.
   */
  DataRate m_dataRate; //!< The device nominal data rate.

  /**
   * The interframe gap that the Net Device uses to throttle packet
   * transmission
   */
  Time           m_tInterframeGap;

  EventId TransmitCompleteEvent; //!< the Tx Complete event

  /**
   * List of callbacks to fire if the link changes state (up or down).
   */
  TracedCallback<> m_linkChangeCallbacks;


  /**
   * The trace source fired when packets come into the "top" of the device
   * at the L3/L2 transition, before being queued for transmission.
   */
  TracedCallback<Ptr<const Packet> > m_macTxTrace;

  /**
   * The trace source fired when packets coming into the "top" of the device
   * at the L3/L2 transition are dropped before being queued for transmission.
   */
  TracedCallback<Ptr<const Packet> > m_macTxDropTrace;

  /**
   * The trace source fired for packets successfully received by the device
   * immediately before being forwarded up to higher layers (at the L2/L3 
   * transition).  This is a promiscuous trace (which doesn't mean a lot here
   * in the point-to-point device).
   */
  TracedCallback<Ptr<const Packet> > m_macPromiscRxTrace;

  /**
   * The trace source fired for packets successfully received by the device
   * immediately before being forwarded up to higher layers (at the L2/L3 
   * transition).  This is a non-promiscuous trace (which doesn't mean a lot 
   * here in the point-to-point device).
   */
  TracedCallback<Ptr<const Packet> > m_macRxTrace;
  
  /**
   * A trace source that emulates a non-promiscuous protocol sniffer connected
   * to the device.  Unlike your average everyday sniffer, this trace source
   * will not fire on PACKET_OTHERHOST events.
   *
   * On the transmit size, this trace hook will fire after a packet is dequeued
   * from the device queue for transmission.  In Linux, for example, this would
   * correspond to the point just before a device \c hard_start_xmit where
   * \c dev_queue_xmit_nit is called to dispatch the packet to the PF_PACKET
   * ETH_P_ALL handlers.
   *
   * On the receive side, this trace hook will fire when a packet is received,
   * just before the receive callback is executed.  In Linux, for example,
   * this would correspond to the point at which the packet is dispatched to
   * packet sniffers in \c netif_receive_skb.
   */
  TracedCallback<Ptr<const Packet> > m_snifferTrace;

  /**
   * A trace source that emulates a promiscuous mode protocol sniffer connected
   * to the device.
   *
   * Does not work correctly yet; maybe never and just disable?
   */
  TracedCallback<Ptr<const Packet> > m_promiscSnifferTrace;

  /**
   * Incoming packets for the current timestep.
   *
   * Used to enforce deterministic schedule ordering for ties in time on incoming remote packets.
   */
  std::multimap<Mac48Address, Ptr<Packet> > m_remoteIncoming; 
};

} // namespace ns3

#endif /* SIMPLE_DISTRIBUTED_NET_DEVICE_H */
