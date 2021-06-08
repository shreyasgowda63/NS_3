/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Universita' di Firenze, Italy
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
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#ifndef IP_NEIGHBOR_HELPER_H
#define IP_NEIGHBOR_HELPER_H

#include "ns3/ptr.h"
#include "ns3/ipv6-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/address.h"
#include "internet-trace-helper.h"

namespace ns3 {

class NetDevice;

/**
 * \ingroup ipv4Helpers
 * \ingroup ipv6Helpers
 *
 * \brief Manipulates the ARP or NDISC entries.
 *
 * This helper enables the manipulation of ARP and NDISC caches, with an
 * interface loosely based on the "ip neighbor" Linux command.
 *
 * At the moment the interface allows to:
 *  - remove an entry from the ARP or NDISC cache
 *  - remove a permanent entry to the ARP or NDISC cache
 */

class IpNeighborHelper
{
public:
  /**
   * Create a new IpNeighborHelper
   */
  IpNeighborHelper(void);

  /**
   * Destroy the IpNeighborHelper
   */
  virtual ~IpNeighborHelper(void);

  /**
   * \brief Copy constructor
   * \param o Object to copy from.
   */
  IpNeighborHelper (const IpNeighborHelper &o);

  /**
   * \brief Copy constructor
   * \param o Object to copy from.
   * \returns A copy of the IpNeighborHelper.
   */
  IpNeighborHelper &operator = (const IpNeighborHelper &o);

  /**
   * \brief Neighbor entries states
   */
  enum NudState_e {
    PERMANENT,  //!< the neighbour entry is valid forever and can only be removed administratively.
    NOARP,      //!< the neighbour entry is valid. No attempts to validate this entry will be made but it can be removed when its lifetime expires.
    REACHABLE,  //!< the neighbour entry is valid until the reachability timeout expires.
    STALE,      //!< the neighbour entry is valid but suspicious. Does not change the neighbour state if it was valid and the address is not changed.
    INCOMPLETE, //!< the neighbour entry has not (yet) been validated/resolved.
    DELAY,      //!< neighbor entry validation is currently delayed.
    PROBE,      //!< neighbor is being probed.
  };

  /**
   * Add a permanent ARP entry in the ARP cache relative to a NetDevice.
   *
   * Equivalent to "ip neighbor add 192.168.100.1 lladdr 00:c0:7b:7d:00:c8 dev eth3 nud permanent"
   *
   * \param netDevice the NetDevice whose ARP cache is to modify.
   * \param ipv4Address the IPv4 address to add to the cache.
   * \param macAddress the MAC address to add to the cache.
   * \param nud the neighbor NUD state.
   */
  void Add (Ptr<NetDevice> netDevice, Ipv4Address ipv4Address, Address macAddress, NudState_e nud = PERMANENT);

  /**
   * Remove an ARP entry from the ARP cache relative to a NetDevice.
   *
   * Equivalent to "ip neighbor remove 192.168.100.1 dev eth3"
   *
   * \param netDevice the NetDevice whose ARP cache is to modify.
   * \param ipv4Address the IPv4 address to add to the cache.
   * \return true on success.
   */
  bool Remove (Ptr<NetDevice> netDevice, Ipv4Address ipv4Address);

  /**
   * Add a permanent NDISC entry in the NDISC cache relative to a NetDevice.
   *
   * Equivalent to "ip neighbor add 2001:db8:food::1 lladdr 00:c0:7b:7d:00:c8 dev eth3 nud permanent"
   *
   * \param netDevice the NetDevice whose NDISC cache is to modify.
   * \param ipv6Address the IPv6 address to add to the cache.
   * \param macAddress the MAC address to add to the cache.
   * \param nud the neighbor NUD state.
   */
  void Add (Ptr<NetDevice> netDevice, Ipv6Address ipv6Address, Address macAddress, NudState_e nud = PERMANENT);

  /**
   * Remove a NDISC entry from the NDISC cache relative to a NetDevice.
   *
   * Equivalent to "ip neighbor remove 2001:db8:food::1 dev eth3"
   *
   * \param netDevice the NetDevice whose NDISC cache is to modify.
   * \param ipv6Address the IPv6 address to add to the cache.
   * \return true on success.
   */
  bool Remove (Ptr<NetDevice> netDevice, Ipv6Address ipv6Address);

private:

};

} // namespace ns3

#endif /* IP_NEIGHBOR_HELPER_H */
