/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 Emmanuelle Laprise
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
 * Author: Emmanuelle Laprise <emmanuelle.laprise@bluekazoo.ca>
 */

#ifndef ETHERNET_ENCAP_H
#define ETHERNET_ENCAP_H

#include "ns3/ethernet-header.h"
#include "ns3/ethernet-trailer.h"
#include "ns3/mac48-address.h"
#include "ns3/net-device.h"

namespace ns3 {

namespace Ethernet {
enum EncapMode {
  ILLEGAL, /**< Encapsulation mode not set */
  DIX, /**< DIX II / Ethernet II packet */
  LLC, /**< 802.2 LLC/SNAP Packet*/
};
} // namespace Ethernet

void EthernetEncap (Ptr<Packet> packet, Mac48Address source, Mac48Address dest,
                    uint16_t protocolNumber, Ethernet::EncapMode mode = Ethernet::DIX);

bool EthernetDecap (Ptr<Packet> packet, uint16_t &protocol, EthernetHeader &header);
} // namespace ns3

#endif // ETHERNET_ENCAP_H