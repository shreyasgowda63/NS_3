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
 */
#ifndef SIMPLE_DISTRIBUTED_CHANNEL_H
#define SIMPLE_DISTRIBUTED_CHANNEL_H

#include "ns3/traced-callback.h"
#include "ns3/channel.h"
#include "ns3/nstime.h"
#include "ns3/mac48-address.h"
#include "ns3/data-rate.h"
#include "ns3/vector.h"
#include <vector>
#include <unordered_map>
#include <functional>

namespace std {
template <>
class hash<ns3::Mac48Address>
{
public:
  size_t operator() (const ns3::Mac48Address &mac48Address ) const
  {
    union
    {
      uint64_t hash = 0;
      uint8_t macValues[8];
    } hashConvert;

    // Note putting 6 values from 48 bit address into low order bytes.
    mac48Address.CopyTo (&hashConvert.macValues[2]);
    return std::hash<uint64_t> ()(hashConvert.hash);
  }
};
}

namespace ns3 {

class SimpleDistributedNetDevice;
class MobilityModel;
class ChannelErrorModel;
class ChannelDelayModel;
class Packet;

/**
 * \ingroup simple-distributed
 * \brief A simple distributed channel.
 *
 * This channel is similar to the SimpleChannel extended to be usable
 * in a parallel simulation.  This channel does not model a real
 * network to any level of fidelity; the model is a fully connected
 * network with no interference or collision.
 *
 * The channel is considered to be 'distributed' across ranks with
 * each rank owning a different set of Nodes that are attached to the
 * same channel.  SimpleDistributed will send packets to the
 * appropriate remote rank when required.  The channel currently
 * assumes all net-devices and nodes are instantiated for the entire
 * topology (the same assumption as several other ns-3 classes when
 * running in parallel).  Channel attributes should be consistently
 * set on all ranks.
 *
 * By default the channel does not add any delay to the packets.
 * Delays may be added by setting the Delay and DataRate attributes
 * and/or setting the "DelayModel" on the channel. The total
 * transmission delay from the channel is computed as: Delay +
 * PacketSize * DataRate + DelayModel::ComputeDelay().  The channel
 * delay model has access to limited metadata from the sending
 * netdevice.  Supporting parallelism means one should not assume the
 * complete sending node state is available.  One may also add
 * transmission delay on the SimpleDistributedNetDevice; the delay
 * contributions from both the channel and sending net device are
 * added to get the total transmission time.
 *
 * An error model can be attached to the channel.  The error model is
 * applied to each destination, in the case of a broadcast packet this
 * means the error model will be invoked on a packet for every net
 * device within range.  As with the delay model the error model is 
 * evaluated on the receiving side if mobility models receiving Node
 * mobility model is not available on the sender processor.  Sender
 * metadata is sent and provided to the error model to enable 
 * error models based on distance.
 *
 * There is simple support for mobility models and limiting the
 * transmission range.  If the Distance attribute is non-negative and
 * mobility models exist on the source and destination nodes, the
 * distance attribute will be compared to distance between the
 * mobility model positions to determine if the destination is in
 * range.  An example use case is representing a maximum transmission
 * distance in a very simplistic wireless network.
 *
 * When running a parallel simulation the distance attribute will be
 * enforced on the sending side if the rank has a mobility model for
 * both sender and receiver.  If mobility models are only instantiated
 * on the ranks owning the node the distance attribute will be
 * enforced on the receiving side since the sender can't compute
 * distance locally.  This is done as a performance enhancement since
 * it avoids sending packets when sufficient information is available
 * locally on the sending side to enforce the distance restriction.
 *
 * The channel assumes the associated NetDevices are using 48-bit MAC
 * addresses.
 * 
 * This channel is meant to be used by ns3::SimpleDistributedNetDevices
 */
class SimpleDistributedChannel : public Channel
{
public:
  /**
   * \brief Get the type ID.
   *
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  SimpleDistributedChannel ();

  /**
   * A packet is sent by a net device.  
   *
   * When broadcasting or in promiscuous mode a receive event will be
   * scheduled for all net device connected to the channel other than
   * the net device who sent the packet subject to the Distance
   * constraint.  When not broadcasting or in promiscuous mode only
   * the destination will scheduled a receive event, again subject to
   * the Distance constraint.
   * 
   * Broadcasting and promiscuous modes do not currently scale well.
   *
   * \param p Packet to be sent
   * \param protocol Protocol number
   * \param to Address to send packet to
   * \param from Address the packet is coming from
   * \param sender Source netdevice who sent the packet
   * \param txTime Transmit time
   */
  virtual void Send (Ptr<Packet> p, uint16_t protocol, Mac48Address to, Mac48Address from,
                     Ptr<SimpleDistributedNetDevice> sender,
                     Time txTime);

  /**
   * Attached a net device to the channel.
   *
   * \param device the device to attach to the channel
   */
  virtual void Add (Ptr<SimpleDistributedNetDevice> device);

  // inherited from ns3::Channel
  virtual std::size_t GetNDevices (void) const;
  virtual Ptr<NetDevice> GetDevice (std::size_t i) const;

  /**
   * Attach an error model to the channel.
   *
   * The channel may optionally include an ChannelErrorModel in
   * the packet transmission chain.
   *
   * \see ChannelErrorModel
   * \param error_model The ChannelErrorModel error model.
   */
  void SetErrorModel (Ptr<ChannelErrorModel> error_model);

  /**
   * Return the error model.
   * 
   * \return The error model.
   */
  Ptr<ChannelErrorModel> GetErrorModel (void);

  /**
   * Attach an delay model to the channel.
   *
   * The channel may optionally include an ChannelDelayModel in
   * the packet transmission chain to compute transmit delays on a per
   * packet basis.
   *
   * \see ChannelDelayModel
   * \param error_model The ChannelDelayModel error model.
   */
  void SetDelayModel (Ptr<ChannelDelayModel> delay_model);

  /**
   * Return the delay model.
   * 
   * \return The delay model.
   */
  Ptr<ChannelDelayModel> GetDelayModel (void);

  /**
   * Compute the minimum packet delay between any two net devices
   * on the channel.
   *
   * For distributed simulations the method is invoked to constrain
   * the look-ahead window in conservative DES algorithms.  This is
   * computed the sum of the Delay attribute and
   * ChannelModel::GetMinimumDelay.
   *
   * \returns The minimum packet delay
   */
  Time GetMinimumDelay (void);

private:
  friend class SimpleDistributedNetDevice;

  /**
   *  Packet send to a destination device.
   *
   * \param p Packet to be sent
   * \param protocol Protocol number
   * \param to Address to send packet to
   * \param from Address the packet is coming from
   * \param srcDevice Source netdevice who sent the packet
   * \param dstDevice Destination netdevice to receive the packet
   * \param srcMobilityModel Source mobility model
   * \param txTime Transmit time
   *
   */
  void Send (Ptr<Packet> p, uint16_t protocol,
             Mac48Address to, Mac48Address from,
             Ptr<SimpleDistributedNetDevice> srcDevice,
             Ptr<SimpleDistributedNetDevice> dstDevice,
             Ptr<MobilityModel> srcMobilityModel,
             Time txTime);

  /**
   * Compute packet specific delay between a source and destination on sender rank.
   * 
   * The calculation is split into send and receive side since each side has different
   * data in a distributed simulation.
   *
   * The sum from the two methods is the amount of time the packet
   * will be delayed in the channel.  
   *
   * \return delay packet delay time
   * \param pkt Packet to be sent
   * \param to address to send packet to
   * \param from address the packet is coming from
   * \param sender netdevice who sent the packet
   */
  Time TransmitDelaySendSide (Ptr<Packet> pkt,
                              Mac48Address to,
                              Mac48Address from,
                              Ptr<SimpleDistributedNetDevice> sender);

  /**
   * Compute packet specific delay between a source and destination on receiver rank.
   * 
   * The calculation is split into send and receive side since each side has different
   * data in a distributed simulation.
   *
   * The sum from the two methods is the amount of time the packet
   * will be delayed in the channel.  
   *
   * \return delay packet delay time
   * \param pkt packet to be sent
   * \param srcNodeId NodeId of the sender node
   * \param srcPosition Position of the sender node
   * \param dstDevice  Receiving NetDevice
   *
   */
  Time TransmitDelayReceiveSide (Ptr<Packet> pkt,
                                 uint32_t srcNodeId,
                                 Vector srcPosition,
                                 Ptr<SimpleDistributedNetDevice> dstDevice
                                 );
  /**
   * The delay that the channel uses to simulate packet transmission
   * timing.
   */
  Time m_delay; //!< The device nominal delay time.

  /**
   * The data rate that the channel uses to simulate packet transmission
   * timing. A zero value means infinite bps.
   */
  DataRate m_dataRate; //!< The device nominal data rate.

  /**
   * Two data structures are used for keeping track of the attached net devices since there
   * are two common access method, by index and by MAC address
   */
  typedef std::unordered_map<Mac48Address, Ptr<SimpleDistributedNetDevice> > DeviceMap;
  DeviceMap m_devicesMap; //!< devices connected by the channel; lookup by MAC address

  std::vector<Ptr<SimpleDistributedNetDevice> > m_devicesVector; //!< devices connected by the channel; lookup by index

  bool m_promiscuous = false;  //!< Is the device in promiscuous mode.  If enabled performance will be negatively impacted; messages need to be sent to all ranks

  double m_distance = -1.0; //!< Limit communication to nodes within a distance of sender.  Negative implies no distance constraint enforced.

  Ptr<ChannelErrorModel> m_errorModel; //!< Optional error model
  Ptr<ChannelDelayModel> m_delayModel; //!< Optional delay model

  /**
   * The trace source fired when the phy layer drops a packet it has
   * received due to the error model being active.  Although
   * SimpleDistributedChannel doesn't really have a Phy model, we
   * choose this trace source name for alignment with other trace
   * sources.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet> > m_phyRxDropTrace;

};

} // namespace ns3

#endif /* SIMPLE_DISTRIBUTED_CHANNEL_H */
