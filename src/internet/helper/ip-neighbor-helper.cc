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

#include "ip-neighbor-helper.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/abort.h"
#include "ns3/arp-cache.h"
#include "ns3/ndisc-cache.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv6-interface.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("IpNeighborHelper");


IpNeighborHelper::IpNeighborHelper ()
{
}

IpNeighborHelper::~IpNeighborHelper ()
{
}

IpNeighborHelper::IpNeighborHelper (const IpNeighborHelper &o)
{
}

IpNeighborHelper &
IpNeighborHelper::operator = (const IpNeighborHelper &o)
{
  if (this == &o)
    {
      return *this;
    }
  return *this;
}


void
IpNeighborHelper::Add (Ptr<NetDevice> netDevice, Ipv4Address ipv4Address, Address macAddress, NudState_e nud)
{
  if (nud != PERMANENT)
    {
      NS_ABORT_MSG ("Call to add ARP cache entry failed, only PERMANENT entries can be added (so far)");
    }

  Ptr<Node> node = netDevice->GetNode ();
  NS_ABORT_MSG_IF (!node, "Call to add ARP cache entry, but NetDevice is not associated with a node");

  Ptr<Ipv4L3Protocol> ipv4 = node->GetObject<Ipv4L3Protocol> ();
  NS_ABORT_MSG_IF (!ipv4, "Call to add ARP cache entry, but IPv4 not found in the node");

  int32_t interface = ipv4->GetInterfaceForDevice (netDevice);
  NS_ABORT_MSG_IF (interface == -1, "Call to add ARP cache entry, but no Ipv4Interface can be found for the target NetDevice");

  Ptr<Ipv4Interface> ipv4Interface = ipv4->GetInterface (interface);
  Ptr<ArpCache> arpCache = ipv4Interface->GetArpCache ();
  NS_ABORT_MSG_IF (!arpCache, "Call to add ARP cache entry, but no ArpCache can be found for the target NetDevice");

  ArpCache::Entry* entry;
  entry = arpCache->Lookup (ipv4Address);
  if (!entry)
    {
      entry = arpCache->Add (ipv4Address);
    }
  entry->SetMacAddress (macAddress);
  entry->MarkPermanent ();
  return;
}

bool
IpNeighborHelper::Remove (Ptr<NetDevice> netDevice, Ipv4Address ipv4Address)
{
  Ptr<Node> node = netDevice->GetNode ();
  NS_ABORT_MSG_IF (!node, "Call to remove ARP cache entry, but NetDevice is not associated with a node");

  Ptr<Ipv4L3Protocol> ipv4 = node->GetObject<Ipv4L3Protocol> ();
  NS_ABORT_MSG_IF (!ipv4, "Call to remove ARP cache entry, but IPv4 not found in the node");

  int32_t interface = ipv4->GetInterfaceForDevice (netDevice);
  NS_ABORT_MSG_IF (interface == -1, "Call to remove ARP cache entry, but no Ipv4Interface can be found for the target NetDevice");

  Ptr<Ipv4Interface> ipv4Interface = ipv4->GetInterface (interface);
  Ptr<ArpCache> arpCache = ipv4Interface->GetArpCache ();
  NS_ABORT_MSG_IF (!arpCache, "Call to remove ARP cache entry, but no ArpCache can be found for the target NetDevice");

  ArpCache::Entry* entry;
  entry = arpCache->Lookup (ipv4Address);
  if (entry)
    {
      arpCache->Remove (entry);
      return true;
    }

  NS_LOG_INFO ("Call to remove ARP cache entry, but no entry has been found");
  return false;
}

void
IpNeighborHelper::Add (Ptr<NetDevice> netDevice, Ipv6Address ipv6Address, Address macAddress, NudState_e nud)
{
  if (nud != PERMANENT)
    {
      NS_ABORT_MSG ("Call to add NDISC cache entry failed, only PERMANENT entries can be added (so far)");
    }

  Ptr<Node> node = netDevice->GetNode ();
  NS_ABORT_MSG_IF (!node, "Call to add NDISC cache entry, but NetDevice is not associated with a node");

  Ptr<Ipv6L3Protocol> ipv6 = node->GetObject<Ipv6L3Protocol> ();
  NS_ABORT_MSG_IF (!ipv6, "Call to add NDISC cache entry, but IPv6 not found in the node");

  int32_t interface = ipv6->GetInterfaceForDevice (netDevice);
  NS_ABORT_MSG_IF (interface == -1, "Call to add NDISC cache entry, but no Ipv6Interface can be found for the target NetDevice");

  Ptr<Ipv6Interface> ipv6Interface = ipv6->GetInterface (interface);
  Ptr<NdiscCache> ndiscCache = ipv6Interface->GetNdiscCache ();
  NS_ABORT_MSG_IF (!ndiscCache, "Call to add NDISC cache entry, but no NdiscCache can be found for the target NetDevice");

  NdiscCache::Entry* entry;
  entry = ndiscCache->Lookup (ipv6Address);
  if (!entry)
    {
      entry = ndiscCache->Add (ipv6Address);
    }
  entry->SetMacAddress (macAddress);
  entry->MarkPermanent ();
  return;
}

bool
IpNeighborHelper::Remove (Ptr<NetDevice> netDevice, Ipv6Address ipv6Address)
{
  Ptr<Node> node = netDevice->GetNode ();
  NS_ABORT_MSG_IF (!node, "Call to remove NDISC cache entry, but NetDevice is not associated with a node");

  Ptr<Ipv6L3Protocol> ipv6 = node->GetObject<Ipv6L3Protocol> ();
  NS_ABORT_MSG_IF (!ipv6, "Call to remove NDISC cache entry, but IPv6 not found in the node");

  int32_t interface = ipv6->GetInterfaceForDevice (netDevice);
  NS_ABORT_MSG_IF (interface == -1, "Call to remove NDISC cache entry, but no Ipv6Interface can be found for the target NetDevice");

  Ptr<Ipv6Interface> ipv6Interface = ipv6->GetInterface (interface);
  Ptr<NdiscCache> ndiscCache = ipv6Interface->GetNdiscCache ();
  NS_ABORT_MSG_IF (!ndiscCache, "Call to remove NDISC cache entry, but no NdiscCache can be found for the target NetDevice");

  NdiscCache::Entry* entry;
  entry = ndiscCache->Lookup (ipv6Address);
  if (entry)
    {
      ndiscCache->Remove (entry);
      return true;
    }

  NS_LOG_INFO ("Call to remove NDISC cache entry, but no entry has been found");
  return false;
}


} // namespace ns3
