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

#include "lr-wpan-routing-device.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LrWpanRoutingDevice");

TypeId
LrWpanRoutingDevice::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::LrWpanRoutingDevice").SetParent<NetDevice> ().SetGroupName ("LrWpan");
  return tid;
}

void
LrWpanRoutingDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (this << index);
  m_ifIndex = index;
}

uint32_t
LrWpanRoutingDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION (this);
  return m_ifIndex;
}

Ptr<Channel>
LrWpanRoutingDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_netDevice != 0,
                 "LrWpanRouting: can't find any lower-layer protocol " << m_netDevice);

  return m_netDevice->GetChannel ();
}

void
LrWpanRoutingDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);
  NS_ASSERT_MSG (m_netDevice != 0,
                 "LrWpanRouting: can't find any lower-layer protocol " << m_netDevice);

  m_netDevice->SetAddress (address);
}

Address
LrWpanRoutingDevice::GetAddress (void) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_netDevice != 0,
                 "LrWpanRouting: can't find any lower-layer protocol " << m_netDevice);

  return m_netDevice->GetAddress ();
}

bool
LrWpanRoutingDevice::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION (this << mtu);
  NS_ASSERT_MSG (m_netDevice != 0,
                 "LrWpanRouting: can't find any lower-layer protocol " << m_netDevice);

  return m_netDevice->SetMtu (mtu);
}

uint16_t
LrWpanRoutingDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_netDevice != 0,
                 "LrWpanRouting: can't find any lower-layer protocol " << m_netDevice);

  return m_netDevice->GetMtu ();
}

bool
LrWpanRoutingDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_netDevice != 0,
                 "LrWpanRouting: can't find any lower-layer protocol " << m_netDevice);

  return m_netDevice->IsLinkUp ();
}

void
LrWpanRoutingDevice::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_netDevice != 0,
                 "LrWpanRouting: can't find any lower-layer protocol " << m_netDevice);

  m_netDevice->AddLinkChangeCallback (callback);
}

bool
LrWpanRoutingDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_netDevice != 0,
                 "LrWpanRouting: can't find any lower-layer protocol " << m_netDevice);

  return m_netDevice->IsBroadcast ();
}

Address
LrWpanRoutingDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_netDevice != 0,
                 "LrWpanRouting: can't find any lower-layer protocol " << m_netDevice);

  return m_netDevice->GetBroadcast ();
}

bool
LrWpanRoutingDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_netDevice != 0,
                 "LrWpanRouting: can't find any lower-layer protocol " << m_netDevice);

  return m_netDevice->IsMulticast ();
}

Address
LrWpanRoutingDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  NS_ASSERT_MSG (m_netDevice != 0,
                 "LrWpanRouting: can't find any lower-layer protocol " << m_netDevice);

  return m_netDevice->GetMulticast (multicastGroup);
}

bool
LrWpanRoutingDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_netDevice != 0,
                 "LrWpanRouting: can't find any lower-layer protocol " << m_netDevice);

  return m_netDevice->IsPointToPoint ();
}

bool
LrWpanRoutingDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_netDevice != 0,
                 "LrWpanRouting: can't find any lower-layer protocol " << m_netDevice);

  return m_netDevice->IsBridge ();
}

bool
LrWpanRoutingDevice::Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << *packet << dest << protocolNumber);
  NS_ASSERT_MSG (m_netDevice != 0,
                 "LrWpanRouting: can't find any lower-layer protocol " << m_netDevice);

  return SendFrom (packet, GetAddress (), dest, protocolNumber);
}

bool
LrWpanRoutingDevice::SendFrom (Ptr<Packet> packet, const Address &source, const Address &dest,
                               uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << *packet << source << dest << protocolNumber);
  NS_ASSERT_MSG (m_netDevice != 0,
                 "LrWpanRouting: can't find any lower-layer protocol " << m_netDevice);

  Ptr<LrWpanRoute> route;
  LrWpanMacHeader header;
  if (Mac16Address::IsMatchingType (dest))
    { // Short Address Mode
      header.SetDstAddrMode (SHORT_ADDR);
      header.SetDstAddrFields (0, Mac16Address::ConvertFrom (dest));
    }
  else if (Mac64Address::IsMatchingType (dest))
    { // Extended Address Mode
      header.SetDstAddrMode (EXT_ADDR);
      header.SetDstAddrFields (0, Mac64Address::ConvertFrom (dest));
    }
  else
    { // Pseudo 48 bit Mac
      Mac16Address addr16 = Mac16Address::ConvertFrom (LrWpanRoute::ConvertAddress (dest));
      header.SetDstAddrMode (SHORT_ADDR);
      header.SetDstAddrFields (0, addr16);
    }
  Address copyDest = LrWpanRoute::ConvertAddress (dest);
  route = GetRouteTo (copyDest);
  if (Mac16Address::IsMatchingType (source))
    { // Short Address Mode
      header.SetSrcAddrMode (SHORT_ADDR);
      header.SetSrcAddrFields (0, Mac16Address::ConvertFrom (source));
    }
  else if (Mac64Address::IsMatchingType (source))
    { // Extended Address Mode
      header.SetSrcAddrMode (EXT_ADDR);
      header.SetSrcAddrFields (0, Mac64Address::ConvertFrom (source));
    }
  else
    { // Pseudo 48 bit Mac
      Mac16Address addr16 = Mac16Address::ConvertFrom (LrWpanRoute::ConvertAddress (source));
      header.SetSrcAddrMode (SHORT_ADDR);
      header.SetSrcAddrFields (0, addr16);
    }
  packet->AddHeader (header);

  NS_LOG_DEBUG ("LrWpanRoutingDevice: Sending packet from "
                << GetAddress () << " to " << route->GetGateway () << ". Packet source: " << source
                << "; destination: " << dest);

  return m_netDevice->Send (packet, route->GetGateway (), 0);
}

Ptr<Node>
LrWpanRoutingDevice::GetNode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_node;
}

void
LrWpanRoutingDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);

  m_node = node;
}

bool
LrWpanRoutingDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_netDevice != 0,
                 "LrWpanRouting: can't find any lower-layer protocol " << m_netDevice);

  return m_netDevice->NeedsArp ();
}

void
LrWpanRoutingDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_rxCallback = cb;
}

void
LrWpanRoutingDevice::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_promiscCallback = cb;
}

bool
LrWpanRoutingDevice::SupportsSendFrom () const
{
  NS_LOG_FUNCTION (this);
  return true;
}

Address
LrWpanRoutingDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  NS_ASSERT_MSG (m_netDevice != 0,
                 "LrWpanRouting: can't find any lower-layer protocol " << m_netDevice);

  return m_netDevice->GetMulticast (addr);
}

void
LrWpanRoutingDevice::SetDevice (Ptr<LrWpanNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);

  m_node->RegisterProtocolHandler (MakeCallback (&LrWpanRoutingDevice::Receive, this), 0, device,
                                   false);

  m_netDevice = device;
}

Ptr<LrWpanNetDevice>
LrWpanRoutingDevice::GetDevice (void) const
{
  NS_LOG_FUNCTION (this);
  return m_netDevice;
}

void
LrWpanRoutingDevice::Receive (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                              Address const &source, Address const &destination,
                              NetDevice::PacketType packetType)
{
  NS_LOG_FUNCTION (this << device << packet << protocol << source << destination << packetType);

  // Find out whether the packet needs to be forwarded
  Ptr<Packet> copyPacket = packet->Copy ();
  LrWpanMacHeader header;
  copyPacket->RemoveHeader (header);
  Address dest;
  Address src;
  if (header.GetDstAddrMode () == SHORT_ADDR)
    {
      dest = header.GetShortDstAddr ();
    }
  else if (header.GetDstAddrMode () == EXT_ADDR)
    {
      dest = header.GetExtDstAddr ();
    }
  else
    {
      // Ignore
      NS_LOG_INFO ("LrWpanRoutingDevice: Ignoring received packet.");
      return;
    }
  if (header.GetSrcAddrMode () == SHORT_ADDR)
    {
      src = header.GetShortSrcAddr ();
    }
  else if (header.GetSrcAddrMode () == EXT_ADDR)
    {
      src = header.GetExtSrcAddr ();
    }
  else
    {
      // Ignore
      NS_LOG_INFO ("LrWpanRoutingDevice: Ignoring received packet.");
      return;
    }

  NS_LOG_DEBUG ("LrWpanRoutingDevice: Node " << GetAddress () << " received from " << source
                                             << ". Packet source: " << src
                                             << "; destination: " << dest);

  Address us = LrWpanRoute::ConvertAddress (GetAddress ());
  if (us == dest)
    { // Packet reached its destination
      NS_LOG_INFO ("LrWpanRoutingDevice: Packet reached its destination");
      if (!m_rxCallback.IsNull ())
        {
          m_rxCallback (this, copyPacket, 0, src);
        }
    }
  else
    { // Packet needs to be forwarded
      NS_LOG_INFO ("LrWpanRoutingDevice: Packet will be forwarded");
      if (!m_promiscCallback.IsNull ())
        {
          m_promiscCallback (this, copyPacket, 0, src, dest, PacketType::PACKET_OTHERHOST);
        }
      SendFrom (copyPacket, src, dest, 0);
    }
}

} // namespace ns3