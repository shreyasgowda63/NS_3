/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Institute of Operating Systems and Computer Networks, TU Braunschweig
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
#ifndef LR_WPAN_GLOBAL_ROUTING_HELPER_H
#define LR_WPAN_GLOBAL_ROUTING_HELPER_H

#include <queue>

#include <ns3/lr-wpan-global-routing-device.h>
#include <ns3/net-device-container.h>
#include <ns3/simulator.h>

namespace ns3 {

class LrWpanGlobalRoutingHelper : public Object
{
public:
  static TypeId GetTypeId (void);

  LrWpanGlobalRoutingHelper ();
  ~LrWpanGlobalRoutingHelper ();

  virtual void DoDispose ();

  /**
   * \brief Callback for notifying that the global routing calculation is complete.
   * 
   * \param id The id used with LrWpanGlobalRoutingHelper::Install
   */
  typedef Callback<void, uint16_t> RoutingCalcCompleteCallback;

  /**
   * \brief Set the RoutingCalcCompleteCallback.
   * 
   * \param cb The callback
   */
  void SetRoutingCalcCompleteCallback (RoutingCalcCompleteCallback cb);

  /**
   * \brief Install LrWpanGlobalRoutingDevice for multiple LrWpanNetDevices.
   * 
   * \param lrWpanDevices Container with LrWpanNetDevices
   * \param id Unique id to prevent using discovery transmissions from different LrWpanGlobalRoutingHelpers
   * \return NetDeviceContainer Container with LrWpanGlobalRoutingDevice devices
   */
  NetDeviceContainer Install (NetDeviceContainer lrWpanDevices, uint16_t id);

  /**
   * \brief Installs LrWpanGlobalRoutingDevice for a single LrWpanNetDevice.
   * 
   * \param netDevice The LrWpanNetDevice to install for
   * \param id Unique id to prevent using discovery transmissions from different LrWpanGlobalRoutingHelpers
   * \return LrWpanGlobalRoutingDevice New LrWpanGlobalRoutingDevice instance
   */
  Ptr<LrWpanGlobalRoutingDevice> Install (Ptr<LrWpanNetDevice> netDevice, uint16_t id);

  void TransmissionReceived (Ptr<LrWpanGlobalRoutingDevice> device, const Address &sender,
                             uint16_t id);

  void PrintNeighbors (uint16_t id);

  void SetSendDiscoveryTime (Time t);
  Time GetSendDiscoveryTime () const;
  void SetPacketOffsetTime (Time t);
  Time GetPacketOffsetTime () const;
  void SetWaitTime (Time t);
  Time GetWaitTime () const;
  void SetCreateDirectRoutes (bool create);
  bool GetCreateDirectRoutes () const;

private:
  RoutingCalcCompleteCallback m_callback;

  struct Node
  {
    Node (uint32_t id, Address addr, Ptr<LrWpanStaticRoutingDevice> dev)
        : id (id), addr (addr), dev (dev), neighbors (), next (nullptr)
    {
    }
    uint32_t id;
    Address addr;
    Ptr<LrWpanStaticRoutingDevice> dev;
    std::vector<Node *> neighbors;
    Node *next; // The next node for BFS
  };

  void CalculateRoutes (uint16_t id);

  void BFS (std::map<Address, Node *> &graph, Node *dest);

  /**
   * \brief Info about neighbor addresses per device.
   */
  typedef std::map<Address, std::vector<Address>> NeighborsMap;

  std::map<uint16_t, NeighborsMap> m_neighborMaps;

  std::map<uint16_t, NetDeviceContainer> m_netDevices;

  Time m_sendDiscoveryTime;

  Time m_packetOffsetTime;

  Time m_waitTime;

  bool m_createDirectRoutes;

  EventId m_calcRoutesEvent;
};

} // namespace ns3

#endif /* LR_WPAN_GLOBAL_ROUTING_HELPER_H */