/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c)
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
 * Authors: Liangcheng Yu <liangcheng.yu46@gmail.com>
 * GSoC 2019 project Mentors:
 *          Dizhi Zhou <dizhizhou@hotmail.com>
 *          Mohit P. Tahiliani <tahiliani.nitk@gmail.com>
 *          Tom Henderson <tomh@tomh.org>
*/

// Define an object to create a leaf spine topology.

#ifndef LEAF_SPINE_HELPER_H
#define LEAF_SPINE_HELPER_H

#define NUM_SPINE_MIN 1
#define NUM_LEAF_MIN 2
#define NUM_SERVER_PER_LEAF_MIN 1

#include <vector>

#include "csma-helper.h"
#include "internet-stack-helper.h"
#include "ipv4-address-helper.h"
#include "ipv4-interface-container.h"
#include "ipv6-address-helper.h"
#include "ipv6-interface-container.h"
#include "net-device-container.h"
#include "point-to-point-helper.h"
#include "traffic-control-helper.h"

namespace ns3 {

/**
 * \ingroup data-center
 *
 * \brief A helper to make it easier to create a leaf spine topology
 */
class LeafSpineHelper
{
public:
  /**
   * Create a LeafSpineHelper in order to easily create the IP layer leaf spine topology
   *
   * \param numSpine total number of spine switches in leaf spine
   * \param numLeaf total number of leaf switches in leaf spine
   * \param numServerPerLeaf number of servers under each leaf switch in leaf spine
   * \param p2pServerLeaf the PointToPointHelper used to connect servers to leaf switches
   * \param p2pLeafSpine the PointToPointHelper used to connect spine switches to leaf switches
   */
  LeafSpineHelper (uint32_t numSpine,
                   uint32_t numLeaf,
                   uint32_t numServerPerLeaf);

  ~LeafSpineHelper ();

  /**
   * \param col the column address of the target leaf switch
   *
   * \returns a pointer to the leaf switch specified by the column address
   */
  Ptr<Node> GetLeafNode (uint32_t col) const;

  /**
   * \param col the column address of the desired spine switch
   *
   * \returns a pointer to the spine switch specified by the column address
   */
  Ptr<Node> GetSpineNode (uint32_t col) const;

  /**
   * \param col the column address of the desired server
   *
   * \returns a pointer to the server specified by the column address
   */
  Ptr<Node> GetServerNode (uint32_t col) const;

  /**
   * This returns an IPv4 address at the spine switch specified by
   * col and the interfaceIndex. Technically, a spine switch will 
   * have multiple interfaces connected to each leaf switches in the spine leaf; 
   * therefore, it also has multiple IPv4 addresses. 
   * The interfaceIndex is marked from 0 to m_numLeaf-1 left to right according to 
   * the leaf spine diagram.
   *
   * \param col the column address of the desired spine switch
   * \param interfaceIndex the index of the desired network interface
   *
   * \returns Ipv4Address of the target interfaces of the spine switch
   */
  Ipv4Address GetSpineIpv4Address (uint32_t col, uint32_t interfaceIndex) const;

  /**
   * This returns an IPv4 address container at the spine switch specified by
   * col. Technically, a leaf switch will have multiple interfaces connected to each 
   * leaf switches in the spine leaf; therefore, it also has multiple IPv4 addresses.
   *
   * \param col the column address of the desired spine switch
   *
   * \returns Ipv4InterfaceContainer storing all interfaces for the target spine switch
   */
  Ipv4InterfaceContainer GetSpineIpv4Interfaces (uint32_t col) const;

  /**
   * This returns an IPv4 address at the leaf switch specified by
   * col and the interfaceIndex. Technically, a leaf switch will 
   * have multiple interfaces connected to each spine switches in the spine leaf
   * and each server belonging to the leaf switch; therefore, it also has multiple 
   * IPv4 addresses. The interfaceIndex is marked from 0 to m_numServerPerLeaf-1
   * for each interface connected to the servers left to right and from m_numServerPerLeaf
   * to m_numServerPerLeaf to m_numServerPerLeaf+m_numSpine-1 for each interface connected
   * to the spine switches left to right according to the leaf spine diagram.
   *
   * \param col the column address of the desired leaf switch
   * \param interfaceIndex the index of the desired network interface
   *
   * \returns Ipv4Address of the target interfaces of the leaf switch
   */  
  Ipv4Address GetLeafIpv4Address (uint32_t col, uint32_t interfaceIndex) const;

  /**
   * This returns an IPv4 address container at the leaf switch specified by
   * col. Technically, a leaf switch will have multiple interfaces connected to each 
   * spine switches in the spine leaf and each server belonging to the leaf switch; 
   * therefore, it also has multiple IPv4 addresses.
   *
   * \param col the column address of the desired leaf switch
   *
   * \returns Ipv4InterfaceContainer storing all interfaces for the target leaf switch
   */
  Ipv4InterfaceContainer GetLeafIpv4Interfaces (uint32_t col) const;

  /**
   * This returns an IPv4 address at the server specified by
   * the col. There is only one interface for each server 
   * connected to the leaf switch (a.k.a. ToR switch).
   *
   * \param col the column address of the desired server.
   *
   * \returns Ipv4Address of the target server.
   */
  Ipv4Address GetServerIpv4Address (uint32_t serverIndex) const;

  /**
   * This returns an IPv6 address at the spine switch specified by
   * col and the interfaceIndex. Technically, a spine switch will 
   * have multiple interfaces connected to each leaf switches in the spine leaf; 
   * therefore, it also has multiple IPv6 addresses. 
   * The interfaceIndex is marked from 0 to m_numLeaf-1 left to right according 
   * to the leaf spine diagram.
   *
   * \param col the column address of the desired spine switch
   * \param interfaceIndex the index of the desired network interface
   *
   * \returns Ipv6Address of the target interfaces of the spine switch
   */
  Ipv6Address GetSpineIpv6Address (uint32_t col, uint32_t interfaceIndex) const;

  /**
   * This returns an IPv6 address container at the spine switch specified by
   * col. Technically, a leaf switch will have multiple interfaces connected to each 
   * leaf switches in the spine leaf; therefore, it also has multiple IPv6 addresses.
   *
   * \param col the column address of the desired spine switch
   *
   * \returns Ipv6InterfaceContainer storing all interfaces for the target spine switch
   */
  Ipv6InterfaceContainer GetSpineIpv6Interfaces (uint32_t col) const;

  /**
   * This returns an IPv6 address at the leaf switch specified by
   * col and the interfaceIndex. Technically, a leaf switch will 
   * have multiple interfaces connected to each spine switches in the spine leaf
   * and each server belonging to the leaf switch; therefore, it also has multiple 
   * IPv6 addresses. The interfaceIndex is marked from 0 to m_numServerPerLeaf-1
   * for each interface connected to the servers left to right and from m_numServerPerLeaf
   * to m_numServerPerLeaf to m_numServerPerLeaf+m_numSpine-1 for each interface connected
   * to the spine switches left to right according to the leaf spine diagram.
   *
   * \param col the column address of the desired leaf switch
   * \param interfaceIndex the index of the desired network interface
   *
   * \returns Ipv6Address of the target interfaces of the leaf switch
   */
  Ipv6Address GetLeafIpv6Address (uint32_t col, uint32_t interfaceIndex) const;

  /**
   * This returns an IPv6 address container at the leaf switch specified by
   * col. Technically, a leaf switch will have multiple interfaces connected to each 
   * spine switches in the spine leaf and each server belonging to the leaf switch; 
   * therefore, it also has multiple IPv6 addresses.
   *
   * \param col the column address of the desired leaf switch
   *
   * \returns Ipv6InterfaceContainer storing all interfaces for the target leaf switch
   */
  Ipv6InterfaceContainer GetLeafIpv6Interfaces (uint32_t col) const;

  /**
   * This returns an IPv6 address at the server specified by
   * the col. There is only one interface for each server 
   * connected to the leaf switch (a.k.a. ToR switch).
   *
   * \param serverIndex the column address of the desired server.
   *
   * \returns Ipv6Address of the target server.
   */
  Ipv6Address GetServerIpv6Address (uint32_t col) const;

  /**
   * \returns total number of spine switches
   */
  uint32_t  SpineCount () const;

  /**
   * \returns total number of leaf switches
   */
  uint32_t  LeafCount () const;

  /**
   * \returns total number of servers
   */
  uint32_t  ServerCount () const;

  /**
   * \returns total number of nodes
   */
  uint32_t  TotalCount () const;

  /**
   * \param stackSpine an InternetStackHelper which is used to install
   *                   on every spine switches in the leaf spine
   * \param stackLeaf  an InternetStackHelper which is used to install 
   *                   on every leaf switches in the leaf spine
   * \param stackServer an InternetStackHelper which is used to install
   *                    on every servers in the leaf spine
   */
  void InstallStack (InternetStackHelper stackSpine,
                     InternetStackHelper stackLeaf,
                     InternetStackHelper stackServer);

  /**
   * \param helperEdge the layer 2 helper which is used to install
   *                   on every server to leaf switch links
   * \param helperCore the layer 2 helper which is used to install
   *                   on every links between spine switches and leaf switches
   */
  template <typename T>
  void InstallNetDevices (T helperEdge, T helperCore);

  /**
   * \param tchSpine a TrafficControlHelper which is used to install
   *                   on every spine switches in the leaf spine
   * \param tchLeaf  a TrafficControlHelper which is used to install 
   *                   on every leaf switches in the leaf spine
   * \param tchServer a TrafficControlHelper which is used to install
   *                    on every servers in the leaf spine
   */
  void InstallTrafficControl (TrafficControlHelper tchSpine,
                              TrafficControlHelper tchLeaf,
                              TrafficControlHelper tchServer);

  /**
   * Assigns IPv4 addresses to all the interfaces of switches and servers
   *
   * \param network an IPv4 address representing the network portion
   *                of the IPv4 address
   *
   * \param mask the mask length
   */
  void AssignIpv4Addresses (Ipv4Address network, Ipv4Mask mask);

  /**
   * Assigns IPv6 addresses to all the interfaces of the switches and servers
   *
   * \param network an IPv6 address representing the network portion
   *                of the IPv6 address
   *
   * \param prefix the prefix length
   */
  void AssignIpv6Addresses (Ipv6Address network, Ipv6Prefix prefix);

  /**
   * Sets up the node canvas locations for every node in the leaf spine.
   * This is needed for use with the animation interface
   *
   * \param ulx upper left x value
   * \param uly upper left y value
   * \param lrx lower right x value
   * \param lry lower right y value
   */
  void BoundingBox (double ulx, double uly, double lrx, double lry);

private:
  bool     m_l2Installed;                            //!< If layer 2 components are installed
  uint32_t m_numSpine;                               //!< Number of spine switches
  uint32_t m_numLeaf;                                //!< Number of leaf switches
  uint32_t m_numServerPerLeaf;                       //!< Number of servers per leaf switch
  std::vector<NetDeviceContainer> m_spineDevices;    //!< Net Device container for each spine switch
  std::vector<NetDeviceContainer> m_leafDevices;     //!< Net Device container for each leaf switch
  std::vector<NetDeviceContainer> m_serverDevices;   //!< Net Device container for each server
  NodeContainer m_spineSwitches;                     //!< Node container for all spine switches
  NodeContainer m_leafSwitches;                      //!< Node container for all leaf switches
  NodeContainer m_servers;                           //!< Node container for all servers
  std::vector<Ipv4InterfaceContainer> m_spineInterfaces;         //!< IPv4 interfaces of each spine switch
  std::vector<Ipv4InterfaceContainer> m_leafInterfaces;          //!< IPv4 interfaces of each leaf switch
  std::vector<Ipv4InterfaceContainer> m_serverInterfaces;        //!< IPv4 interfaces of each server
  std::vector<Ipv6InterfaceContainer> m_spineInterfaces6;        //!< IPv6 interfaces of each spine switch
  std::vector<Ipv6InterfaceContainer> m_leafInterfaces6;         //!< IPv6 interfaces of each leaf switch
  std::vector<Ipv6InterfaceContainer> m_serverInterfaces6;       //!< IPv6 interfaces of each server

};

template <typename T>
void
LeafSpineHelper::InstallNetDevices (T helperEdge,
                                    T helperCore)
{
  /*
    There are four types of NetDevice:
    - Server towards the leaf switch (ToR switch)
    - Leaf switch towards servers
    - Leaf switch towards spine switches
    - Spine switch towards leaf switches
   */
  // Connect servers to leaf switches
  uint32_t serverId = 0;
  for (uint32_t i = 0; i < m_numLeaf; i++)
    {
      for (uint32_t j = 0; j < m_numServerPerLeaf; j++)
        {
          NetDeviceContainer ndc = helperEdge.Install (m_servers.Get (serverId), m_leafSwitches.Get (i));
          m_serverDevices[serverId].Add (ndc.Get (0));
          m_leafDevices[i].Add (ndc.Get (1));
          serverId += 1;
        }
    }

  // Connect leaf switches to spine switches in a complete bipartie graph
  for (uint32_t i = 0; i < m_numSpine; i++)
    {
      for (uint32_t j = 0; j < m_numLeaf; j++)
        {
            NetDeviceContainer ndc = helperCore.Install (m_leafSwitches.Get (j),
                                                           m_spineSwitches.Get (i));
            m_leafDevices[j].Add (ndc.Get (0));
            m_spineDevices[i].Add (ndc.Get (1));
        }
    }
  m_l2Installed = true;
}

} // namespace ns3

#endif /* LEAF_SPINE_HELPER_H */
