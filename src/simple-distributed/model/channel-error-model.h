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
#ifndef CHANNEL_ERROR_MODEL_H
#define CHANNEL_ERROR_MODEL_H

#include "ns3/object.h"
#include "ns3/vector.h"

namespace ns3 {

class Packet;
class NetDevice;

/**
 * \ingroup simple-distributed
 * \brief Model used to compute packet loss in a channel.
 *
 * Interface for classes used to specify model used to compute packet
 * loss for the SimpleDistributedChannel.  Similar to ErrorModel used
 * in NetDevice.  The difference is in the IsCorrupt method, which has
 * source information and the destination device as arguments.
 */
class ChannelErrorModel : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  ChannelErrorModel ();
  virtual ~ChannelErrorModel ();

  /**
   * Determine if packet is lost.
   *
   * Similar in use to ErrorModel but exists in the channel to enable
   * easier access to destination and channel information.  Returning
   * true will cause the packet to be dropped; it will not be
   * delivered to the destination net device.
   *
   * Depending on the error model, this function may or may not alter
   * the contents of the packet upon returning true.
   *
   * For distributed simulations the method is invoked on the sending
   * rank.
   *
   * \returns true if the Packet is to be considered as errored/corrupted
   * \param pkt Packet to apply error model to
   * \param srcId NodeId of the source
   * \param srcPosition Source position
   * \param dst Destination NetDevice
   */
  bool IsCorrupt (Ptr<Packet> pkt, uint32_t srcId, Vector srcPosition, Ptr<NetDevice> dstNetDevice) const;
  /**
   * Reset any state associated with the error model
   */
  void Reset (void);
  /**
   * Enable the error model
   */
  void Enable (void);
  /**
   * Disable the error model
   */
  void Disable (void);
  /**
   * \return true if error model is enabled; false otherwise
   */
  bool IsEnabled (void) const;

private:
  /**
   * Corrupt a packet according to the specified model.
   *
   * \returns true if the Packet is to be considered as errored/corrupted
   * \param pkt Packet to apply error model to
   * \param srcId NodeId of the source
   * \param srcPosition Source position
   * \param dst Destination NetDevice
   */
  virtual bool DoIsCorrupt (Ptr<Packet> pkt, uint32_t srcId, Vector srcPosition, Ptr<NetDevice> dstNetDevice) const = 0;
  
  /**
   * Re-initialize any state
   */
  virtual void DoReset (void) = 0;

  bool m_enable; //!< True if the error model is enabled
};


} // namespace ns3
#endif
