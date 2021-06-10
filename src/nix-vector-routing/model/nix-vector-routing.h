/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 The Georgia Institute of Technology
 * Copyright (c) 2021 NITK Surathkal
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
 * This file is adopted from the old ipv4-nix-vector-routing.h.
 *
 * Authors: Josh Pelkey <jpelkey@gatech.edu>
 * 
 * Modified by: Ameya Deshpande <ameyanrd@outlook.com>
 */

#ifndef NIX_VECTOR_ROUTING_H
#define NIX_VECTOR_ROUTING_H

#include "ns3/channel.h"
#include "ns3/node-container.h"
#include "ns3/node-list.h"
#include "ns3/net-device-container.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv6-routing-protocol.h"
#include "ns3/nix-vector.h"
#include "ns3/bridge-net-device.h"
#include "ns3/nstime.h"

#include <map>
#include <unordered_map>

namespace ns3 {

/**
 * \defgroup nix-vector-routing Nix-Vector Routing
 *
 * Nix-vector routing is a simulation specific routing protocol and is
 * intended for large network topologies.
 */

/**
 * \ingroup nix-vector-routing
 * Nix-vector routing protocol
 */
template <typename parent>
class NixVectorRouting : public parent
{
  using IsIpv4 = std::is_same <Ipv4RoutingProtocol, parent>;
  using Ip = typename std::conditional <IsIpv4::value, Ipv4, Ipv6>::type;
  using IpAddress = typename std::conditional<IsIpv4::value, Ipv4Address, Ipv6Address>::type;
  using IpRoute = typename std::conditional<IsIpv4::value, Ipv4Route, Ipv6Route>::type;
  using IpAddressHash = typename std::conditional<IsIpv4::value, Ipv4AddressHash, Ipv6AddressHash>::type;
  using IpHeader = typename std::conditional<IsIpv4::value, Ipv4Header, Ipv6Header>::type;
  using IpInterfaceAddress = typename std::conditional<IsIpv4::value, Ipv4InterfaceAddress, Ipv6InterfaceAddress>::type;
  using IpMulticastRoute = typename std::conditional<IsIpv4::value, Ipv4MulticastRoute, Ipv6MulticastRoute>::type;

public:
  NixVectorRouting ();
  ~NixVectorRouting ();
  /**
   * @brief The Interface ID of the Global Router interface.
   * @return The Interface ID
   * @see Object::GetObject ()
   */
  static TypeId GetTypeId (void);
  /**
   * @brief Set the Node pointer of the node for which this
   * routing protocol is to be placed
   *
   * @param node Node pointer
   */
  void SetNode (Ptr<Node> node);

  /**
   * @brief Called when run-time link topology change occurs
   * which iterates through the node list and flushes any
   * nix vector caches
   *
   * \internal
   * \c const is used here due to need to potentially flush the cache
   * in const methods such as PrintRoutingTable.  Caches are stored in
   * mutable variables and flushed in const methods.
   */
  void FlushGlobalNixRoutingCache (void) const;

  /**
   * @brief Print the Routing Path according to Nix Routing
   * \param source Source node
   * \param dest Destination node address
   * \param stream The ostream the Routing path is printed to
   * \param unit the time unit to be used in the report
   */
  void PrintRoutingPath (Ptr<Node> source, IpAddress dest, Ptr<OutputStreamWrapper> stream, Time::Unit unit) const;


private:

  /**
   * Flushes the cache which stores nix-vector based on
   * destination IP
   */
  void FlushNixCache (void) const;

  /**
   * Flushes the cache which stores the Ip route
   * based on the destination IP
   */
  void FlushIpRouteCache (void) const;

  /**
   * Upon a run-time topology change caches are
   * flushed and the total number of neighbors is
   * reset to zero
   */
  void ResetTotalNeighbors (void);

  /**
   * Takes in the source node and dest IP and calls GetNodeByIp,
   * BFS, accounting for any output interface specified, and finally
   * BuildNixVector to return the built nix-vector
   *
   * \param source Source node
   * \param dest Destination node address
   * \param oif Preferred output interface
   * \returns The NixVector to be used in routing.
   */
  Ptr<NixVector> GetNixVector (Ptr<Node> source, IpAddress dest, Ptr<NetDevice> oif) const;

  /**
   * Checks the cache based on dest IP for the nix-vector
   * \param address Address to check
   * \returns The NixVector to be used in routing.
   */
  Ptr<NixVector> GetNixVectorInCache (IpAddress address) const;

  /**
   * Checks the cache based on dest IP for the IpRoute
   * \param address Address to check
   * \returns The cached route.
   */
  Ptr<IpRoute> GetIpRouteInCache (IpAddress address);

  /**
   * Given a net-device returns all the adjacent net-devices,
   * essentially getting the neighbors on that channel
   * \param [in] netDevice the NetDevice attached to the channel.
   * \param [in] channel the channel to check
   * \param [out] netDeviceContainer the NetDeviceContainer of the NetDevices in the channel.
   */
  void GetAdjacentNetDevices (Ptr<NetDevice> netDevice, Ptr<Channel> channel, NetDeviceContainer & netDeviceContainer) const;

  /**
   * Iterates through the node list and finds the one
   * corresponding to the given IpAddress
   * \param dest destination node IP
   * \return The node with the specified IP.
   */
  Ptr<Node> GetNodeByIp (IpAddress dest) const;

  /**
   * Recurses the parent vector, created by BFS and actually builds the nixvector
   * \param [in] parentVector Parent vector for retracing routes
   * \param [in] source Source Node index
   * \param [in] dest Destination Node index
   * \param [out] nixVector the NixVector to be used for routing
   * \returns true on success, false otherwise.
   */
  bool BuildNixVector (const std::vector< Ptr<Node> > & parentVector, uint32_t source, uint32_t dest, Ptr<NixVector> nixVector) const;

  /**
   * Special variation of BuildNixVector for when a node is sending to itself
   * \param [out] nixVector the NixVector to be used for routing
   * \returns true on success, false otherwise.
   */
  bool BuildNixVectorLocal (Ptr<NixVector> nixVector);

  /**
   * Simply iterates through the nodes net-devices and determines
   * how many neighbors the node has.
   * \param [in] node node pointer
   * \returns the number of neighbors of m_node.
   */
  uint32_t FindTotalNeighbors (Ptr<Node> node) const;


  /**
   * Determine if the NetDevice is bridged
   * \param nd the NetDevice to check
   * \returns the bridging NetDevice (or null if the NetDevice is not bridged)
   */
  Ptr<BridgeNetDevice> NetDeviceIsBridged (Ptr<NetDevice> nd) const;


  /**
   * Nix index is with respect to the neighbors.  The net-device index must be
   * derived from this
   * \param [in] node the current node under consideration
   * \param [in] nodeIndex Nix Node index
   * \param [out] gatewayIp IP address of the gateway
   * \returns the index of the NetDevice in the node.
   */
  uint32_t FindNetDeviceForNixIndex (Ptr<Node> node, uint32_t nodeIndex, IpAddress & gatewayIp) const;

  /**
   * \brief Breadth first search algorithm.
   * \param [in] numberOfNodes total number of nodes
   * \param [in] source Source Node
   * \param [in] dest Destination Node
   * \param [out] parentVector Parent vector for retracing routes
   * \param [in] oif specific output interface to use from source node, if not null
   * \returns false if dest not found, true o.w.
   */
  bool BFS (uint32_t numberOfNodes,
            Ptr<Node> source,
            Ptr<Node> dest,
            std::vector< Ptr<Node> > & parentVector,
            Ptr<NetDevice> oif) const;

  void DoDispose (void);

  /// Map of IpAddress to NixVector
  typedef std::map<IpAddress, Ptr<NixVector> > NixMap_t;
  /// Map of IpAddress to IpRoute
  typedef std::map<IpAddress, Ptr<IpRoute> > IpRouteMap_t;

  /// Callback for unicast packets to be forwarded
  typedef Callback<void, Ptr<IpRoute>, Ptr<const Packet>, const IpHeader &> UnicastForwardCallback;

  /// Callback for multicast packets to be forwarded
  typedef Callback<void, Ptr<IpMulticastRoute>, Ptr<const Packet>, const IpHeader &> MulticastForwardCallback;

  /// Callback for packets to be locally delivered
  typedef Callback<void, Ptr<const Packet>, const IpHeader &, uint32_t > LocalDeliverCallback;

  /// Callback for routing errors (e.g., no route found)
  typedef Callback<void, Ptr<const Packet>, const IpHeader &, Socket::SocketErrno > ErrorCallback;


  /* From Ipv4RoutingProtocol */
  virtual Ptr<IpRoute> RouteOutput (Ptr<Packet> p, const IpHeader &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
  virtual bool RouteInput (Ptr<const Packet> p, const IpHeader &header, Ptr<const NetDevice> idev,
                           UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                           LocalDeliverCallback lcb, ErrorCallback ecb);
  virtual void NotifyInterfaceUp (uint32_t interface);
  virtual void NotifyInterfaceDown (uint32_t interface);
  virtual void NotifyAddAddress (uint32_t interface, IpInterfaceAddress address);
  virtual void NotifyRemoveAddress (uint32_t interface, IpInterfaceAddress address);
  virtual void SetIpv4 (Ptr<Ip> ipv4);
  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;
 
  /**
   * Flushes routing caches if required.
   */
  void CheckCacheStateAndFlush (void) const;

  /**
   * Build map from IP Address to Node for faster lookup.
   */
  void BuildIpAddressToNodeMap (void) const;

  /**
   * Flag to mark when caches are dirty and need to be flushed.  
   * Used for lazy cleanup of caches when there are many topology changes.
   */
  static bool g_isCacheDirty;

  /** Cache stores nix-vectors based on destination ip */
  mutable NixMap_t m_nixCache;

  /** Cache stores IpRoutes based on destination ip */
  mutable IpRouteMap_t m_ipRouteCache;

  Ptr<Ip> m_ip; //!< IP object
  Ptr<Node> m_node; //!< Node object

  /** Total neighbors used for nix-vector to determine number of bits */
  uint32_t m_totalNeighbors;


  /**
   * Mapping of IP address to ns-3 node.
   *
   * Used to avoid linear searching of nodes/devices to find a node in
   * GetNodeByIp() method.  NIX vector routing assumes IP addresses
   * are unique so mapping can be done without duplication.
   **/
  typedef std::unordered_map<IpAddress, ns3::Ptr<ns3::Node>, IpAddressHash > IpAddressToNodeMap;
  static IpAddressToNodeMap g_ipAddressToNodeMap; //!< Address to node map.
};


/**
 * \ingroup nix-vector-routing
 * Create the typedef Ipv4NixVectorRouting with parent as Ipv4RoutingProtocol
 *
 * Note: This is kept to have backwards compatibility with original Ipv4NixVectorRouting.
 */
typedef NixVectorRouting<Ipv4RoutingProtocol> Ipv4NixVectorRouting;
} // namespace ns3

#endif /* NIX_VECTOR_ROUTING_H */
