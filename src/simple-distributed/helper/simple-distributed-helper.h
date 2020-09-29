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
 */
#ifndef SIMPLE_DISTRIBUTED_NETDEVICE_HELPER_H
#define SIMPLE_DISTRIBUTED_NETDEVICE_HELPER_H

#include <string>

#include "ns3/attribute.h"
#include "ns3/object-factory.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/simple-distributed-channel.h"

#include "ns3/trace-helper.h"

namespace ns3 {

/**
 * \brief build a set of SimpleNetDevice objects
 *
 * Normally we eschew multiple inheritance, however, the classes 
 * PcapUserHelperForDevice and AsciiTraceUserHelperForDevice are
 * "mixins".
 */
class SimpleDistributedHelper : public PcapHelperForDevice,
                                public AsciiTraceHelperForDevice
{
public:
  /**
   * Construct a SimpleDistributedHelper.
   */
  SimpleDistributedHelper ();
  virtual ~SimpleDistributedHelper () {}

  /**
   * Each net device must have a queue to pass packets through.
   * This method allows one to set the type of the queue that is automatically
   * created when the device is created and attached to a node.
   *
   * \param type the type of queue
   * \param n1 the name of the attribute to set on the queue
   * \param v1 the value of the attribute to set on the queue
   * \param n2 the name of the attribute to set on the queue
   * \param v2 the value of the attribute to set on the queue
   * \param n3 the name of the attribute to set on the queue
   * \param v3 the value of the attribute to set on the queue
   * \param n4 the name of the attribute to set on the queue
   * \param v4 the value of the attribute to set on the queue
   *
   * Set the type of queue to create and associated to each
   * SimpleNetDevice created through SimpleDistributedHelper::Install.
   */
  void SetQueue (std::string type,
                 std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                 std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                 std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                 std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue ());

  /**
   * Each net device must have a channel to pass packets through.
   * This method allows one to set the type of the channel that is automatically
   * created when the device is created and attached to a node.
   *
   * \param type the type of queue
   * \param n1 the name of the attribute to set on the queue
   * \param v1 the value of the attribute to set on the queue
   * \param n2 the name of the attribute to set on the queue
   * \param v2 the value of the attribute to set on the queue
   * \param n3 the name of the attribute to set on the queue
   * \param v3 the value of the attribute to set on the queue
   * \param n4 the name of the attribute to set on the queue
   * \param v4 the value of the attribute to set on the queue
   *
   * Set the type of channel to create and associated to each
   * SimpleNetDevice created through SimpleDistributedHelper::Install.
   */
  void SetChannel (std::string type,
                   std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                   std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                   std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                   std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue ());


  /**
   * \param n1 the name of the attribute to set
   * \param v1 the value of the attribute to set
   *
   * Set these attributes on each ns3::SimpleNetDevice created
   * by SimpleDistributedHelper::Install
   */
  void SetDeviceAttribute (std::string n1, const AttributeValue &v1);

  /**
   * \param n1 the name of the attribute to set
   * \param v1 the value of the attribute to set
   *
   * Set these attributes on each ns3::CsmaChannel created
   * by SimpleDistributedHelper::Install
   */
  void SetChannelAttribute (std::string n1, const AttributeValue &v1);

  /**
   * SimpleNetDevice is Broadcast capable and ARP needing. This function
   * limits the number of SimpleNetDevices on one channel to two, disables
   * Broadcast and ARP and enables PointToPoint mode.
   *
   * \warning It must be used before installing a NetDevice on a node.
   *
   * \param pointToPointMode True for PointToPoint SimpleNetDevice
   */
  void SetNetDevicePointToPointMode (bool pointToPointMode);

  /**
   * This method creates an ns3::SimpleDistributedChannel with the attributes configured by
   * SimpleDistributedHelper::SetChannelAttribute, an ns3::SimpleNetDevice with the attributes
   * configured by SimpleDistributedHelper::SetDeviceAttribute and then adds the device
   * to the node and attaches the channel to the device.
   *
   * \param node The node to install the device in
   * \returns A container holding the added net device.
   */
  NetDeviceContainer Install (Ptr<Node> node) const;

  /**
   * This method creates an ns3::SimpleNetDevice with the attributes configured by
   * SimpleDistributedHelper::SetDeviceAttribute and then adds the device to the node and
   * attaches the provided channel to the device.
   *
   * \param node The node to install the device in
   * \param channel The channel to attach to the device.
   * \returns A container holding the added net device.
   */
  NetDeviceContainer Install (Ptr<Node> node, Ptr<SimpleDistributedChannel> channel) const;

  /**
   * This method creates an ns3::SimpleDistributedChannel with the attributes configured by
   * SimpleDistributedHelper::SetChannelAttribute.  For each Ptr<node> in the provided
   * container: it creates an ns3::SimpleNetDevice (with the attributes
   * configured by SimpleDistributedHelper::SetDeviceAttribute); adds the device to the
   * node; and attaches the channel to the device.
   *
   * \param c The NodeContainer holding the nodes to be changed.
   * \returns A container holding the added net devices.
   */
  NetDeviceContainer Install (const NodeContainer &c) const;

  /**
   * For each Ptr<node> in the provided container, this method creates an
   * ns3::SimpleNetDevice (with the attributes configured by
   * SimpleDistributedHelper::SetDeviceAttribute); adds the device to the node; and attaches
   * the provided channel to the device.
   *
   * \param c The NodeContainer holding the nodes to be changed.
   * \param channel The channel to attach to the devices.
   * \returns A container holding the added net devices.
   */
  NetDeviceContainer Install (const NodeContainer &c, Ptr<SimpleDistributedChannel> channel) const;

private:
  /**
   * \brief Enable pcap output the indicated net device.
   *
   * NetDevice-specific implementation mechanism for hooking the trace and
   * writing to the trace file.
   *
   * \param prefix Filename prefix to use for pcap files.
   * \param nd Net device for which you want to enable tracing.
   * \param promiscuous If true capture all possible packets available at the device.
   * \param explicitFilename Treat the prefix as an explicit filename if true
   */
  virtual void EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, bool promiscuous, bool explicitFilename);

  /**
   * \brief Enable ascii trace output on the indicated net device.
   *
   * NetDevice-specific implementation mechanism for hooking the trace and
   * writing to the trace file.
   *
   * \param stream The output stream object to use when logging ascii traces.
   * \param prefix Filename prefix to use for ascii trace files.
   * \param nd Net device for which you want to enable tracing.
   * \param explicitFilename Treat the prefix as an explicit filename if true
   */
  virtual void EnableAsciiInternal (
    Ptr<OutputStreamWrapper> stream,
    std::string prefix,
    Ptr<NetDevice> nd,
    bool explicitFilename);

  /**
   * This method creates an ns3::SimpleNetDevice with the attributes configured by
   * SimpleDistributedHelper::SetDeviceAttribute and then adds the device to the node and
   * attaches the provided channel to the device.
   *
   * \param node The node to install the device in
   * \param channel The channel to attach to the device.
   * \returns The new net device.
   */
  Ptr<NetDevice> InstallPriv (Ptr<Node> node, Ptr<SimpleDistributedChannel> channel) const;

  ObjectFactory m_queueFactory; //!< Queue factory
  ObjectFactory m_deviceFactory; //!< NetDevice factory
  ObjectFactory m_channelFactory; //!< Channel factory
  bool m_pointToPointMode; //!< Install PointToPoint SimpleNetDevice or Broadcast ones

};

} // namespace ns3

#endif /* SIMPLE_DISTRIBUTED_NETDEVICE_HELPER_H */
