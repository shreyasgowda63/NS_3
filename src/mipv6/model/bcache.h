/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 Jadavpur University, India
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
 * Author: Manoj Kumar Rana <manoj24.rana@gmail.com>
 */

#ifndef B_CACHE_H
#define B_CACHE_H


#include "ns3/node.h"

namespace ns3 {

/**
 * \class BCache
 * \brief BCache class is associated with Mipv6Ha and Mipv6Cn class. It contain data
 * members: CoA, HoA, lifetime, HA address, tunnel interface index, sequence
 * number, BU state, and nonce indices as defined in RFC 6275. To handle the
 * information of multiple MNs and HAs, each entry in the BCache is keyed by
 * the HoA.
*/
class BCache : public Object
{
public:

  /**
   * \brief typeid
   */
  static TypeId GetTypeId ();

  /**
   * \brief constructor
   */
  BCache ();

  /**
   * \brief destructor
   */
  ~BCache ();

  // TODO: define bcache class
};

}

#endif /* BINDING_CACHE_H */
