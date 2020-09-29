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
#ifndef CHANNEL_DELAY_MODEL_H
#define CHANNEL_DELAY_MODEL_H

#include "ns3/object.h"
#include "ns3/vector.h"

namespace ns3 {

class Packet;
class NetDevice;
class Time;

/**
 * \ingroup simple-distributed
 */
/**
 * \ingroup simple-distributed
 * \brief Model used to compute packet delays on a channel.
 *
 * Interface for classes used to specify model used to compute delays
 * for a channel.  The principle method is the DoComputeDelay method
 * which returns a delay to be added to the packet delay in the
 * channel.
 */
class ChannelDelayModel : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  ChannelDelayModel ();
  virtual ~ChannelDelayModel ();

  /**
   * Compute the packet delay.
   *
   * Each packet transmission may have a unique delay computed based
   * on the source and destination.  This method will be invoked for
   * each packet sent on the channel.
   *
   * For distributed simulations the method is invoked on the
   * receiving rank to avoid requiring sending rank to instantiate and
   * maintain mobility models for all receivers.  The srcId should be
   * used with caution since the full state of remote nodes is not
   * instantiated.
   *
   * \returns The packet delay
   * \param pkt Packet
   * \param srcId NodeId of the source
   * \param srcPosition Source position
   * \param dstNetDevice Destination NetDevice
   */
  Time ComputeDelay (Ptr<Packet> pkt, uint32_t srcId, Vector srcPosition, Ptr<NetDevice> dstNetDevice) const;
  
  /**
   * Compute the minimum packet delay.
   *
   * Used by distributed simulations to constrain the look-ahead
   * window in conservative DES algorithms.  For best parallel
   * performance this value should be as large as possible.  For
   * example, if the Delay, DataRate and smallest packet size is known
   * then Delay + DataRate * minimum packet size is a better estimate
   * than just Delay.
   *
   * \returns The minimum packet delay
   */
  Time GetMinimumDelay (void) const;
  
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
   * Implementation of ComputeDelay.
   *
   * \returns Packet delay
   * \param pkt Packet
   * \param srcId NodeId of the source
   * \param srcPosition Source position
   * \param dstNetDevice Destination NetDevice
   */
  virtual Time DoComputeDelay (Ptr<Packet> pkt, uint32_t srcId, Vector srcPosition, Ptr<NetDevice> dst) const = 0 ;
  /**
   * Implementation of GetMinimumDelay
   *
   * \returns The packet delay
   */
  virtual Time DoGetMinimumDelay (void) const = 0 ;
  /**
   * Re-initialize any state
   */
  virtual void DoReset (void) = 0;

  bool m_enable; //!< True if the error model is enabled
};

} // namespace ns3
#endif
