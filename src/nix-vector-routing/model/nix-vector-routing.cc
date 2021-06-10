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
 * This file is adopted from the old ipv4-nix-vector-routing.cc.
 *
 * Authors: Josh Pelkey <jpelkey@gatech.edu>
 * 
 * Modified by: Ameya Deshpande <ameyanrd@outlook.com>
 */

#include <queue>
#include <iomanip>

#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/names.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/loopback-net-device.h"

#include "nix-vector-routing.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NixVectorRouting");

NS_OBJECT_TEMPLATE_CLASS_DEFINE (NixVectorRouting, Ipv4RoutingProtocol);

template <typename parent>
bool NixVectorRouting<parent>::g_isCacheDirty = false;

template <typename parent>
typename NixVectorRouting<parent>::IpAddressToNodeMap NixVectorRouting<parent>::g_ipAddressToNodeMap;

template <typename parent>
TypeId 
NixVectorRouting<parent>::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NixVectorRouting")
    .SetParent<parent> ()
    .SetGroupName ("NixVectorRouting")
    .template AddConstructor<NixVectorRouting<parent> > ()
  ;
  return tid;
}

template <typename parent>
NixVectorRouting<parent>::NixVectorRouting ()
  : m_totalNeighbors (0)
{
  NS_LOG_FUNCTION_NOARGS ();
}

template <typename parent>
NixVectorRouting<parent>::~NixVectorRouting ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

template <typename parent>
void
NixVectorRouting<parent>::SetIpv4 (Ptr<Ip> ipv4)
{
  NS_ASSERT (ipv4 != 0);
  NS_ASSERT (m_ip == 0);
  NS_LOG_DEBUG ("Created Ipv4NixVectorProtocol");

  m_ip = ipv4;
}

template <typename parent>
void 
NixVectorRouting<parent>::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();

  m_node = 0;
  m_ip = 0;

  parent::DoDispose ();
}

template <typename parent>
void
NixVectorRouting<parent>::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_node = node;
}

template <typename parent>
void
NixVectorRouting<parent>::FlushGlobalNixRoutingCache (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  NodeList::Iterator listEnd = NodeList::End ();
  for (NodeList::Iterator i = NodeList::Begin (); i != listEnd; i++)
    {
      Ptr<Node> node = *i;
      Ptr<NixVectorRouting<parent> > rp = node->GetObject<NixVectorRouting> ();
      if (!rp)
        {
          continue;
        }
      NS_LOG_LOGIC ("Flushing Nix caches.");
      rp->FlushNixCache ();
      rp->FlushIpRouteCache ();
    }

  // IP address to node mapping is potentially invalid so clear it.
  // Will be repopulated in lazy evaluation when mapping is needed.
  g_ipAddressToNodeMap.clear ();
}

template <typename parent>
void
NixVectorRouting<parent>::FlushNixCache (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  m_nixCache.clear ();
}

template <typename parent>
void
NixVectorRouting<parent>::FlushIpRouteCache (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  m_ipRouteCache.clear ();
}

template <typename parent>
Ptr<NixVector>
NixVectorRouting<parent>::GetNixVector (Ptr<Node> source, IpAddress dest, Ptr<NetDevice> oif) const
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<NixVector> nixVector = Create<NixVector> ();

  // not in cache, must build the nix vector
  // First, we have to figure out the nodes 
  // associated with these IPs
  Ptr<Node> destNode = GetNodeByIp (dest);
  if (destNode == 0)
    {
      NS_LOG_ERROR ("No routing path exists");
      return 0;
    }

  // if source == dest, then we have a special case
  /// \internal
  /// Do not process packets to self (see \bugid{1308})
  if (source == destNode)
    {
      NS_LOG_DEBUG ("Do not process packets to self");
      return 0;
    }
  else
    {
      // otherwise proceed as normal 
      // and build the nix vector
      std::vector< Ptr<Node> > parentVector;

      BFS (NodeList::GetNNodes (), source, destNode, parentVector, oif);

      if (BuildNixVector (parentVector, source->GetId (), destNode->GetId (), nixVector))
        {
          return nixVector;
        }
      else
        {
          NS_LOG_ERROR ("No routing path exists");
          return 0;
        }
    }
}

template <typename parent>
Ptr<NixVector>
NixVectorRouting<parent>::GetNixVectorInCache (IpAddress address) const
{
  NS_LOG_FUNCTION_NOARGS ();

  CheckCacheStateAndFlush ();

  typename NixMap_t::iterator iter = m_nixCache.find (address);
  if (iter != m_nixCache.end ())
    {
      NS_LOG_LOGIC ("Found Nix-vector in cache.");
      return iter->second;
    }

  // not in cache
  return 0;
}

template <typename parent>
Ptr<typename NixVectorRouting<parent>::IpRoute>
NixVectorRouting<parent>::GetIpRouteInCache (IpAddress address)
{
  NS_LOG_FUNCTION_NOARGS ();

  CheckCacheStateAndFlush ();

  typename IpRouteMap_t::iterator iter = m_ipRouteCache.find (address);
  if (iter != m_ipRouteCache.end ())
    {
      NS_LOG_LOGIC ("Found IpRoute in cache.");
      return iter->second;
    }

  // not in cache
  return 0;
}

template <typename parent>
bool
NixVectorRouting<parent>::BuildNixVectorLocal (Ptr<NixVector> nixVector)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint32_t numberOfDevices = m_node->GetNDevices ();

  // here we are building a nix vector to 
  // ourself, so we need to find the loopback 
  // interface and add that to the nix vector
  IpAddress loopback ("127.0.0.1");
  for (uint32_t i = 0; i < numberOfDevices; i++)
    {
      uint32_t interfaceIndex = (m_ip)->GetInterfaceForDevice (m_node->GetDevice (i));
      IpInterfaceAddress ifAddr = m_ip->GetAddress (interfaceIndex, 0);
      if (ifAddr.GetLocal () == loopback)
        {
          NS_LOG_LOGIC ("Adding loopback to nix.");
          NS_LOG_LOGIC ("Adding Nix: " << i << " with " << nixVector->BitCount (numberOfDevices) 
                                       << " bits, for node " << m_node->GetId ());
          nixVector->AddNeighborIndex (i, nixVector->BitCount (numberOfDevices));
          return true;
        }
    }
  return false;
}

template <typename parent>
bool
NixVectorRouting<parent>::BuildNixVector (const std::vector< Ptr<Node> > & parentVector, uint32_t source, uint32_t dest, Ptr<NixVector> nixVector) const
{
  NS_LOG_FUNCTION_NOARGS ();

  if (source == dest)
    {
      return true;
    }

  if (parentVector.at (dest) == 0)
    {
      return false;
    }

  Ptr<Node> parentNode = parentVector.at (dest);

  uint32_t numberOfDevices = parentNode->GetNDevices ();
  uint32_t destId = 0;
  uint32_t totalNeighbors = 0;

  // scan through the net devices on the parent node
  // and then look at the nodes adjacent to them
  for (uint32_t i = 0; i < numberOfDevices; i++)
    {
      // Get a net device from the node
      // as well as the channel, and figure
      // out the adjacent net devices
      Ptr<NetDevice> localNetDevice = parentNode->GetDevice (i);
      if (localNetDevice->IsBridge ())
        {
          continue;
        }
      Ptr<Channel> channel = localNetDevice->GetChannel ();
      if (channel == 0)
        {
          continue;
        }

      // this function takes in the local net dev, and channel, and
      // writes to the netDeviceContainer the adjacent net devs
      NetDeviceContainer netDeviceContainer;
      GetAdjacentNetDevices (localNetDevice, channel, netDeviceContainer);

      // Finally we can get the adjacent nodes
      // and scan through them.  If we find the 
      // node that matches "dest" then we can add 
      // the index  to the nix vector.
      // the index corresponds to the neighbor index
      uint32_t offset = 0;
      for (NetDeviceContainer::Iterator iter = netDeviceContainer.Begin (); iter != netDeviceContainer.End (); iter++)
        {
          Ptr<Node> remoteNode = (*iter)->GetNode ();

          if (remoteNode->GetId () == dest)
            {
              destId = totalNeighbors + offset;
            }
          offset += 1;
        }

      totalNeighbors += netDeviceContainer.GetN ();
    }
  NS_LOG_LOGIC ("Adding Nix: " << destId << " with " 
                               << nixVector->BitCount (totalNeighbors) << " bits, for node " << parentNode->GetId ());
  nixVector->AddNeighborIndex (destId, nixVector->BitCount (totalNeighbors));

  // recurse through parent vector, grabbing the path 
  // and building the nix vector
  BuildNixVector (parentVector, source, (parentVector.at (dest))->GetId (), nixVector);
  return true;
}

template <typename parent>
void
NixVectorRouting<parent>::GetAdjacentNetDevices (Ptr<NetDevice> netDevice, Ptr<Channel> channel, NetDeviceContainer & netDeviceContainer) const
{
  NS_LOG_FUNCTION_NOARGS ();

  for (std::size_t i = 0; i < channel->GetNDevices (); i++)
    {
      Ptr<NetDevice> remoteDevice = channel->GetDevice (i);
      if (remoteDevice != netDevice)
        {
          Ptr<BridgeNetDevice> bd = NetDeviceIsBridged (remoteDevice);
          // we have a bridged device, we need to add all 
          // bridged devices
          if (bd)
            {
              NS_LOG_LOGIC ("Looking through bridge ports of bridge net device " << bd);
              for (uint32_t j = 0; j < bd->GetNBridgePorts (); ++j)
                {
                  Ptr<NetDevice> ndBridged = bd->GetBridgePort (j);
                  if (ndBridged == remoteDevice)
                    {
                      NS_LOG_LOGIC ("That bridge port is me, don't walk backward");
                      continue;
                    }
                  Ptr<Channel> chBridged = ndBridged->GetChannel ();
                  if (chBridged == 0)
                    {
                      continue;
                    }
                  GetAdjacentNetDevices (ndBridged, chBridged, netDeviceContainer);
                }
            }
          else
            {
              netDeviceContainer.Add (channel->GetDevice (i));
            }
        }
    }
}

template <typename parent>
void
NixVectorRouting<parent>::BuildIpAddressToNodeMap (void) const
{
  NS_LOG_FUNCTION_NOARGS ();

  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      Ptr<Ip> ip = node->GetObject<Ip> ();

      if(ip)
        {
          uint32_t numberOfDevices = node->GetNDevices ();

          for (uint32_t deviceId = 0; deviceId < numberOfDevices; deviceId++)
            {
              Ptr<NetDevice> device = node->GetDevice (deviceId);

              // If this is not a loopback device add the IP address to the map
              if ( !DynamicCast<LoopbackNetDevice>(device) )
                {
                  int32_t interfaceIndex = (ip)->GetInterfaceForDevice (node->GetDevice (deviceId));
                  if (interfaceIndex != -1)
                    {
                      uint32_t numberOfAddresses = ip->GetNAddresses (interfaceIndex);
                      for (uint32_t addressIndex = 0; addressIndex < numberOfAddresses; addressIndex++)
                        {
                          IpInterfaceAddress ifAddr = ip->GetAddress (interfaceIndex, addressIndex);
                          IpAddress addr = ifAddr.GetLocal ();

                          NS_ABORT_MSG_IF (g_ipAddressToNodeMap.count (addr),
                                          "Duplicate IP address (" << addr << ") found during NIX Vector map construction for node " << node->GetId ());

                          NS_LOG_LOGIC ("Adding IP address " << addr << " for node " << node->GetId () << " to NIX Vector IP address to node map");
                          g_ipAddressToNodeMap[addr] = node;
                        }
                    }
                }
            }
        }
    }
}

template <typename parent>
Ptr<Node>
NixVectorRouting<parent>::GetNodeByIp (IpAddress dest) const
{
  NS_LOG_FUNCTION_NOARGS ();

  // Populate lookup table if is empty.
  if ( g_ipAddressToNodeMap.empty () )
    {
      BuildIpAddressToNodeMap ();
    }

  Ptr<Node> destNode;

  typename IpAddressToNodeMap::iterator iter = g_ipAddressToNodeMap.find(dest);

  if(iter == g_ipAddressToNodeMap.end ())
    {
      NS_LOG_ERROR ("Couldn't find dest node given the IP" << dest);
      destNode = 0;
    }
  else
    {
      destNode = iter -> second;
    }

  return destNode;
}

template <typename parent>
uint32_t
NixVectorRouting<parent>::FindTotalNeighbors (Ptr<Node> node) const
{
  uint32_t numberOfDevices = node->GetNDevices ();
  uint32_t totalNeighbors = 0;

  // scan through the net devices on the parent node
  // and then look at the nodes adjacent to them
  for (uint32_t i = 0; i < numberOfDevices; i++)
    {
      // Get a net device from the node
      // as well as the channel, and figure
      // out the adjacent net devices
      Ptr<NetDevice> localNetDevice = node->GetDevice (i);
      Ptr<Channel> channel = localNetDevice->GetChannel ();
      if (channel == 0)
        {
          continue;
        }

      // this function takes in the local net dev, and channel, and
      // writes to the netDeviceContainer the adjacent net devs
      NetDeviceContainer netDeviceContainer;
      GetAdjacentNetDevices (localNetDevice, channel, netDeviceContainer);

      totalNeighbors += netDeviceContainer.GetN ();
    }

  return totalNeighbors;
}

template <typename parent>
Ptr<BridgeNetDevice>
NixVectorRouting<parent>::NetDeviceIsBridged (Ptr<NetDevice> nd) const
{
  NS_LOG_FUNCTION (nd);

  Ptr<Node> node = nd->GetNode ();
  uint32_t nDevices = node->GetNDevices ();

  //
  // There is no bit on a net device that says it is being bridged, so we have
  // to look for bridges on the node to which the device is attached.  If we
  // find a bridge, we need to look through its bridge ports (the devices it
  // bridges) to see if we find the device in question.
  //
  for (uint32_t i = 0; i < nDevices; ++i)
    {
      Ptr<NetDevice> ndTest = node->GetDevice (i);
      NS_LOG_LOGIC ("Examine device " << i << " " << ndTest);

      if (ndTest->IsBridge ())
        {
          NS_LOG_LOGIC ("device " << i << " is a bridge net device");
          Ptr<BridgeNetDevice> bnd = ndTest->GetObject<BridgeNetDevice> ();
          NS_ABORT_MSG_UNLESS (bnd, "NixVectorRouting::NetDeviceIsBridged (): GetObject for <BridgeNetDevice> failed");

          for (uint32_t j = 0; j < bnd->GetNBridgePorts (); ++j)
            {
              NS_LOG_LOGIC ("Examine bridge port " << j << " " << bnd->GetBridgePort (j));
              if (bnd->GetBridgePort (j) == nd)
                {
                  NS_LOG_LOGIC ("Net device " << nd << " is bridged by " << bnd);
                  return bnd;
                }
            }
        }
    }
  NS_LOG_LOGIC ("Net device " << nd << " is not bridged");
  return 0;
}

template <typename parent>
uint32_t
NixVectorRouting<parent>::FindNetDeviceForNixIndex (Ptr<Node> node, uint32_t nodeIndex, IpAddress & gatewayIp) const
{
  uint32_t numberOfDevices = node->GetNDevices ();
  uint32_t index = 0;
  uint32_t totalNeighbors = 0;

  // scan through the net devices on the parent node
  // and then look at the nodes adjacent to them
  for (uint32_t i = 0; i < numberOfDevices; i++)
    {
      // Get a net device from the node
      // as well as the channel, and figure
      // out the adjacent net devices
      Ptr<NetDevice> localNetDevice = node->GetDevice (i);
      Ptr<Channel> channel = localNetDevice->GetChannel ();
      if (channel == 0)
        {
          continue;
        }

      // this function takes in the local net dev, and channel, and
      // writes to the netDeviceContainer the adjacent net devs
      NetDeviceContainer netDeviceContainer;
      GetAdjacentNetDevices (localNetDevice, channel, netDeviceContainer);

      // check how many neighbors we have
      if (nodeIndex < (totalNeighbors + netDeviceContainer.GetN ()))
        {
          // found the proper net device
          index = i;
          Ptr<NetDevice> gatewayDevice = netDeviceContainer.Get (nodeIndex-totalNeighbors);
          Ptr<Node> gatewayNode = gatewayDevice->GetNode ();
          Ptr<Ip> ip = gatewayNode->GetObject<Ip> ();

          uint32_t interfaceIndex = (ip)->GetInterfaceForDevice (gatewayDevice);
          IpInterfaceAddress ifAddr = ip->GetAddress (interfaceIndex, 0);
          gatewayIp = ifAddr.GetLocal ();
          break;
        }
      totalNeighbors += netDeviceContainer.GetN ();
    }

  return index;
}

template <typename parent>
Ptr<typename NixVectorRouting<parent>::IpRoute> 
NixVectorRouting<parent>::RouteOutput (Ptr<Packet> p, const IpHeader &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{
  NS_LOG_FUNCTION_NOARGS ();
  Ptr<IpRoute> rtentry;
  Ptr<NixVector> nixVectorInCache;
  Ptr<NixVector> nixVectorForPacket;

  CheckCacheStateAndFlush ();

  NS_LOG_DEBUG ("Dest IP from header: " << header.GetDestination ());
  // check if cache
  nixVectorInCache = GetNixVectorInCache (header.GetDestination ());

  // not in cache
  if (!nixVectorInCache)
    {
      NS_LOG_LOGIC ("Nix-vector not in cache, build: ");
      // Build the nix-vector, given this node and the
      // dest IP address
      nixVectorInCache = GetNixVector (m_node, header.GetDestination (), oif);

      // cache it
      m_nixCache.insert (typename NixMap_t::value_type (header.GetDestination (), nixVectorInCache));
    }

  // path exists
  if (nixVectorInCache)
    {
      NS_LOG_LOGIC ("Nix-vector contents: " << *nixVectorInCache);

      // create a new nix vector to be used, 
      // we want to keep the cached version clean
      nixVectorForPacket = nixVectorInCache->Copy (); 

      // Get the interface number that we go out of, by extracting
      // from the nix-vector
      if (m_totalNeighbors == 0)
        {
          m_totalNeighbors = FindTotalNeighbors (m_node);
        }

      // Get the interface number that we go out of, by extracting
      // from the nix-vector
      uint32_t numberOfBits = nixVectorForPacket->BitCount (m_totalNeighbors);
      uint32_t nodeIndex = nixVectorForPacket->ExtractNeighborIndex (numberOfBits);

      // Search here in a cache for this node index 
      // and look for a IpRoute
      rtentry = GetIpRouteInCache (header.GetDestination ());

      if (!rtentry || !(rtentry->GetOutputDevice () == oif))
        {
          // not in cache or a different specified output
          // device is to be used

          // first, make sure we erase existing (incorrect)
          // rtentry from the map
          if (rtentry)
            {
              m_ipRouteCache.erase (header.GetDestination ());
            }

          NS_LOG_LOGIC ("IpRoute not in cache, build: ");
          IpAddress gatewayIp;
          uint32_t index = FindNetDeviceForNixIndex (m_node, nodeIndex, gatewayIp);
          int32_t interfaceIndex = 0;

          if (!oif)
            {
              interfaceIndex = (m_ip)->GetInterfaceForDevice (m_node->GetDevice (index));
            }
          else
            {
              interfaceIndex = (m_ip)->GetInterfaceForDevice (oif);
            }

          NS_ASSERT_MSG (interfaceIndex != -1, "Interface index not found for device");

          IpAddress sourceIPAddr = m_ip->SourceAddressSelection (interfaceIndex, header.GetDestination ());

          // start filling in the IpRoute info
          rtentry = Create<IpRoute> ();
          rtentry->SetSource (sourceIPAddr);

          rtentry->SetGateway (gatewayIp);
          rtentry->SetDestination (header.GetDestination ());

          if (!oif)
            {
              rtentry->SetOutputDevice (m_ip->GetNetDevice (interfaceIndex));
            }
          else
            {
              rtentry->SetOutputDevice (oif);
            }

          sockerr = Socket::ERROR_NOTERROR;

          // add rtentry to cache
          m_ipRouteCache.insert (typename IpRouteMap_t::value_type (header.GetDestination (), rtentry));
        }

      NS_LOG_LOGIC ("Nix-vector contents: " << *nixVectorInCache << " : Remaining bits: " << nixVectorForPacket->GetRemainingBits ());

      // Add  nix-vector in the packet class 
      // make sure the packet exists first
      if (p)
        {
          NS_LOG_LOGIC ("Adding Nix-vector to packet: " << *nixVectorForPacket);
          p->SetNixVector (nixVectorForPacket);
        }
    }
  else // path doesn't exist
    {
      NS_LOG_ERROR ("No path to the dest: " << header.GetDestination ());
      sockerr = Socket::ERROR_NOROUTETOHOST;
    }

  return rtentry;
}

template <typename parent>
bool 
NixVectorRouting<parent>::RouteInput (Ptr<const Packet> p, const IpHeader &header, Ptr<const NetDevice> idev,
                                      UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                                      LocalDeliverCallback lcb, ErrorCallback ecb)
{
  NS_LOG_FUNCTION_NOARGS ();

  CheckCacheStateAndFlush ();

  NS_ASSERT (m_ip != 0);
  // Check if input device supports IP
  NS_ASSERT (m_ip->GetInterfaceForDevice (idev) >= 0);
  uint32_t iif = m_ip->GetInterfaceForDevice (idev);

  // Local delivery
  if (m_ip->IsDestinationAddress (header.GetDestination (), iif))
    {
      if (!lcb.IsNull ())
        {
          NS_LOG_LOGIC ("Local delivery to " << header.GetDestination ());
          lcb (p, header, iif);
          return true;
        }
      else
        {
          // The local delivery callback is null.  This may be a multicast
          // or broadcast packet, so return false so that another
          // multicast routing protocol can handle it.  It should be possible
          // to extend this to explicitly check whether it is a unicast
          // packet, and invoke the error callback if so
          return false;
        }
    }

  Ptr<IpRoute> rtentry;

  // Get the nix-vector from the packet
  Ptr<NixVector> nixVector = p->GetNixVector ();

  // If nixVector isn't in packet, something went wrong
  NS_ASSERT (nixVector);

  // Get the interface number that we go out of, by extracting
  // from the nix-vector
  if (m_totalNeighbors == 0)
    {
      m_totalNeighbors = FindTotalNeighbors (m_node);
    }
  uint32_t numberOfBits = nixVector->BitCount (m_totalNeighbors);
  uint32_t nodeIndex = nixVector->ExtractNeighborIndex (numberOfBits);

  rtentry = GetIpRouteInCache (header.GetDestination ());
  // not in cache
  if (!rtentry)
    {
      NS_LOG_LOGIC ("IpRoute not in cache, build: ");
      IpAddress gatewayIp;
      uint32_t index = FindNetDeviceForNixIndex (m_node, nodeIndex, gatewayIp);
      uint32_t interfaceIndex = (m_ip)->GetInterfaceForDevice (m_node->GetDevice (index));
      IpInterfaceAddress ifAddr = m_ip->GetAddress (interfaceIndex, 0);

      // start filling in the IpRoute info
      rtentry = Create<IpRoute> ();
      rtentry->SetSource (ifAddr.GetLocal ());

      rtentry->SetGateway (gatewayIp);
      rtentry->SetDestination (header.GetDestination ());
      rtentry->SetOutputDevice (m_ip->GetNetDevice (interfaceIndex));

      // add rtentry to cache
      m_ipRouteCache.insert (typename IpRouteMap_t::value_type (header.GetDestination (), rtentry));
    }

  NS_LOG_LOGIC ("At Node " << m_node->GetId () << ", Extracting " << numberOfBits <<
                " bits from Nix-vector: " << nixVector << " : " << *nixVector);

  // call the unicast callback
  // local deliver is handled by Ipv4StaticRoutingImpl
  // so this code is never even called if the packet is
  // destined for this node.
  ucb (rtentry, p, header);

  return true;
}

template <typename parent>
void
NixVectorRouting<parent>::PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{

  CheckCacheStateAndFlush ();

  std::ostream* os = stream->GetStream ();
  // Copy the current ostream state
  std::ios oldState (nullptr);
  oldState.copyfmt (*os);

  *os << std::resetiosflags (std::ios::adjustfield) << std::setiosflags (std::ios::left);

  *os << "Node: " << m_ip->template GetObject<Node> ()->GetId ()
      << ", Time: " << Now().As (unit)
      << ", Local time: " << m_ip->template GetObject<Node> ()->GetLocalTime ().As (unit)
      << ", Nix Routing" << std::endl;

  *os << "NixCache:" << std::endl;
  if (m_nixCache.size () > 0)
    {
      *os << "Destination     NixVector" << std::endl;
      for (typename NixMap_t::const_iterator it = m_nixCache.begin (); it != m_nixCache.end (); it++)
        {
          std::ostringstream dest;
          dest << it->first;
          *os << std::setw (16) << dest.str ();
          *os << *(it->second) << std::endl;
        }
    }
  *os << "Ipv4RouteCache:" << std::endl;
  if (m_ipRouteCache.size () > 0)
    {
      *os << "Destination     Gateway         Source            OutputDevice" << std::endl;
      for (typename IpRouteMap_t::const_iterator it = m_ipRouteCache.begin (); it != m_ipRouteCache.end (); it++)
        {
          std::ostringstream dest, gw, src;
          dest << it->second->GetDestination ();
          *os << std::setw (16) << dest.str ();
          gw << it->second->GetGateway ();
          *os << std::setw (16) << gw.str ();
          src << it->second->GetSource ();
          *os << std::setw (16) << src.str ();
          *os << "  ";
          if (Names::FindName (it->second->GetOutputDevice ()) != "")
            {
              *os << Names::FindName (it->second->GetOutputDevice ());
            }
          else
            {
              *os << it->second->GetOutputDevice ()->GetIfIndex ();
            }
          *os << std::endl;
        }
    }
  *os << std::endl;
  // Restore the previous ostream state
  (*os).copyfmt (oldState);
}

// virtual functions from Ipv4RoutingProtocol 
template <typename parent>
void
NixVectorRouting<parent>::NotifyInterfaceUp (uint32_t i)
{
  g_isCacheDirty = true;
}
template <typename parent>
void
NixVectorRouting<parent>::NotifyInterfaceDown (uint32_t i)
{
  g_isCacheDirty = true;
}
template <typename parent>
void
NixVectorRouting<parent>::NotifyAddAddress (uint32_t interface, IpInterfaceAddress address)
{
  g_isCacheDirty = true;
}
template <typename parent>
void
NixVectorRouting<parent>::NotifyRemoveAddress (uint32_t interface, IpInterfaceAddress address)
{
  g_isCacheDirty = true;
}

template <typename parent>
bool
NixVectorRouting<parent>::BFS (uint32_t numberOfNodes, Ptr<Node> source, 
                           Ptr<Node> dest, std::vector< Ptr<Node> > & parentVector,
                           Ptr<NetDevice> oif) const
{
  NS_LOG_FUNCTION_NOARGS ();

  NS_LOG_LOGIC ("Going from Node " << source->GetId () << " to Node " << dest->GetId ());
  std::queue< Ptr<Node> > greyNodeList;  // discovered nodes with unexplored children

  // reset the parent vector
  parentVector.assign (numberOfNodes, 0); // initialize to 0

  // Add the source node to the queue, set its parent to itself 
  greyNodeList.push (source);
  parentVector.at (source->GetId ()) = source;

  // BFS loop
  while (greyNodeList.size () != 0)
    {
      Ptr<Node> currNode = greyNodeList.front ();
      Ptr<Ip> ip = currNode->GetObject<Ip> ();
 
      if (currNode == dest) 
        {
          NS_LOG_LOGIC ("Made it to Node " << currNode->GetId ());
          return true;
        }

      // if this is the first iteration of the loop and a 
      // specific output interface was given, make sure 
      // we go this way
      if (currNode == source && oif)
        {
          // make sure that we can go this way
          if (ip)
            {
              uint32_t interfaceIndex = (ip)->GetInterfaceForDevice (oif);
              if (!(ip->IsUp (interfaceIndex)))
                {
                  NS_LOG_LOGIC ("IpInterface is down");
                  return false;
                }
            }
          if (!(oif->IsLinkUp ()))
            {
              NS_LOG_LOGIC ("Link is down.");
              return false;
            }
          Ptr<Channel> channel = oif->GetChannel ();
          if (channel == 0)
            { 
              return false;
            }

          // this function takes in the local net dev, and channel, and
          // writes to the netDeviceContainer the adjacent net devs
          NetDeviceContainer netDeviceContainer;
          GetAdjacentNetDevices (oif, channel, netDeviceContainer);

          // Finally we can get the adjacent nodes
          // and scan through them.  We push them
          // to the greyNode queue, if they aren't 
          // already there.
          for (NetDeviceContainer::Iterator iter = netDeviceContainer.Begin (); iter != netDeviceContainer.End (); iter++)
            {
              Ptr<Node> remoteNode = (*iter)->GetNode ();

              // check to see if this node has been pushed before
              // by checking to see if it has a parent
              // if it doesn't (null or 0), then set its parent and 
              // push to the queue
              if (parentVector.at (remoteNode->GetId ()) == 0)
                {
                  parentVector.at (remoteNode->GetId ()) = currNode;
                  greyNodeList.push (remoteNode);
                }
            }
        }
      else
        {
          // Iterate over the current node's adjacent vertices
          // and push them into the queue
          for (uint32_t i = 0; i < (currNode->GetNDevices ()); i++)
            {
              // Get a net device from the node
              // as well as the channel, and figure
              // out the adjacent net device
              Ptr<NetDevice> localNetDevice = currNode->GetDevice (i);

              // make sure that we can go this way
              if (ip)
                {
                  uint32_t interfaceIndex = (ip)->GetInterfaceForDevice (currNode->GetDevice (i));
                  if (!(ip->IsUp (interfaceIndex)))
                    {
                      NS_LOG_LOGIC ("IpInterface is down");
                      continue;
                    }
                }
              if (!(localNetDevice->IsLinkUp ()))
                {
                  NS_LOG_LOGIC ("Link is down.");
                  continue;
                }
              Ptr<Channel> channel = localNetDevice->GetChannel ();
              if (channel == 0)
                { 
                  continue;
                }

              // this function takes in the local net dev, and channel, and
              // writes to the netDeviceContainer the adjacent net devs
              NetDeviceContainer netDeviceContainer;
              GetAdjacentNetDevices (localNetDevice, channel, netDeviceContainer);

              // Finally we can get the adjacent nodes
              // and scan through them.  We push them
              // to the greyNode queue, if they aren't 
              // already there.
              for (NetDeviceContainer::Iterator iter = netDeviceContainer.Begin (); iter != netDeviceContainer.End (); iter++)
                {
                  Ptr<Node> remoteNode = (*iter)->GetNode ();

                  // check to see if this node has been pushed before
                  // by checking to see if it has a parent
                  // if it doesn't (null or 0), then set its parent and 
                  // push to the queue
                  if (parentVector.at (remoteNode->GetId ()) == 0)
                    {
                      parentVector.at (remoteNode->GetId ()) = currNode;
                      greyNodeList.push (remoteNode);
                    }
                }
            }
        }

      // Pop off the head grey node.  We have all its children.
      // It is now black.
      greyNodeList.pop ();
    }

  // Didn't find the dest...
  return false;
}

template <typename parent>
void
NixVectorRouting<parent>::PrintRoutingPath (Ptr<Node> source, IpAddress dest,
                                        Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
  NS_LOG_FUNCTION (this << source << dest);
  Ptr<NixVector> nixVectorInCache;
  Ptr<NixVector> nixVector;
  Ptr<IpRoute> rtentry;

  CheckCacheStateAndFlush ();

  Ptr<Node> destNode = GetNodeByIp (dest);
  if (destNode == 0)
    {
      NS_LOG_ERROR ("No routing path exists");
      return;
    }

  std::ostream* os = stream->GetStream ();
  // Copy the current ostream state
  std::ios oldState (nullptr);
  oldState.copyfmt (*os);

  *os << std::resetiosflags (std::ios::adjustfield) << std::setiosflags (std::ios::left);
  *os << "Time: " << Now().As (unit)
      << ", Nix Routing" << std::endl;
  *os << "Route Path: ";
  *os << "(Node " << source->GetId () << " to Node " << destNode->GetId () << ", ";
  *os << "Nix Vector: ";

  nixVectorInCache = GetNixVectorInCache (dest);

  // not in cache
  if (!nixVectorInCache)
    {
      NS_LOG_LOGIC ("Nix-vector not in cache, build: ");
      // Build the nix-vector, given the source node and the
      // dest IP address
      nixVectorInCache = GetNixVector (source, dest, nullptr);
    }

  if (nixVectorInCache || (!nixVectorInCache && source == destNode))
    {
      Ptr<Node> curr = source;
      uint32_t totalNeighbors = 0;

      if (nixVectorInCache)
        {
          // cache it
          m_nixCache.insert (typename NixMap_t::value_type (dest, nixVectorInCache));
          // Make a NixVector copy to work with. This is because
          // we don't want to extract the bits from nixVectorInCache
          // which is stored in the m_nixCache.
          nixVector = nixVectorInCache->Copy ();

          *os << *nixVector;
        }
      *os << ")" << std::endl;

      if (source == destNode)
        {
          std::ostringstream src, dst;
          src << dest << " (Node " << destNode->GetId () << ")";
          *os << std::setw (20) << src.str ();
          dst << "---->   " << dest << " (Node " << destNode->GetId () << ")";
          *os << dst.str () << std::endl;
        }

      while (curr != destNode)
        {
          totalNeighbors = FindTotalNeighbors (curr);
          // Get the number of bits required
          // to represent all the neighbors
          uint32_t numberOfBits = nixVector->BitCount (totalNeighbors);
          // Get the nixIndex
          uint32_t nixIndex = nixVector->ExtractNeighborIndex (numberOfBits);
          // gatewayIP is the IP of next
          // node on channel found from nixIndex
          IpAddress gatewayIp;
          // Get the Net Device index from the nixIndex
          uint32_t NetDeviceIndex = FindNetDeviceForNixIndex (curr, nixIndex, gatewayIp);
          // Get the interfaceIndex with the help of NetDeviceIndex.
          // It will be used to get the IP address on interfaceIndex
          // interface of 'curr' node.
          Ptr<Ip> ip = curr->GetObject<Ip> ();
          Ptr<NetDevice> outDevice = curr->GetDevice (NetDeviceIndex);
          uint32_t interfaceIndex = ip->GetInterfaceForDevice (outDevice);
          IpAddress sourceIPAddr;
          if (curr == source)
            {
              sourceIPAddr = ip->SourceAddressSelection (interfaceIndex, dest);
            }
          else
            {
              // We use the first address because it's indifferent which one
              // we use to identify intermediate routers
              sourceIPAddr = ip->GetAddress (interfaceIndex, 0).GetLocal ();
            }

          std::ostringstream currNode, nextNode;
          currNode << sourceIPAddr << " (Node " << curr->GetId () << ")";
          *os << std::setw (20) << currNode.str ();
          // Replace curr with the next node
          curr = GetNodeByIp (gatewayIp);
          nextNode << "---->   " << ((curr == destNode) ? dest : gatewayIp) << " (Node " << curr->GetId () << ")";
          *os << nextNode.str () << std::endl;
        }
        *os << std::endl;
    }
  else
    {
      *os << ")" << std::endl;
      // No Route exists
      *os << "There does not exist a path from Node " << source->GetId ()
          << " to Node " << destNode->GetId () << "." << std::endl;
    }
  // Restore the previous ostream state
  (*os).copyfmt (oldState);
}

template <typename parent>
void 
NixVectorRouting<parent>::CheckCacheStateAndFlush (void) const
{
  if (g_isCacheDirty)
    {
      FlushGlobalNixRoutingCache ();
      g_isCacheDirty = false;
    }
}

} // namespace ns3
