/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Philip Hönnecke
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

#include "lr-wpan-global-routing-device.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LrWpanGlobalRoutingDevice");

NS_OBJECT_ENSURE_REGISTERED (LrWpanGlobalRoutingDevice);

TypeId
LrWpanGlobalRoutingDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LrWpanGlobalRoutingDevice")
                          .SetParent<LrWpanStaticRoutingDevice> ()
                          .SetGroupName ("LrWpan");
  return tid;
}

LrWpanGlobalRoutingDevice::LrWpanGlobalRoutingDevice (uint16_t id)
{
  NS_LOG_FUNCTION (this);
  m_globalRoutingId = id;
}

LrWpanGlobalRoutingDevice::~LrWpanGlobalRoutingDevice ()
{
  NS_LOG_FUNCTION (this);
}

void
LrWpanGlobalRoutingDevice::Receive (Ptr<NetDevice> device, Ptr<const Packet> packet,
                                    uint16_t protocol, Address const &source,
                                    Address const &destination, PacketType packetType)
{
  NS_LOG_FUNCTION (this << device << packet << protocol << source << destination << packetType);

  LrWpanMacHeader header;
  packet->PeekHeader (header);
  // Check if destination address is short broadcast address and id is same as ours
  // This marks a discovery packet
  if (header.GetDstAddrMode () == SHORT_ADDR && header.GetDstPanId () == m_globalRoutingId &&
      header.GetShortDstAddr () ==
          Mac16Address::ConvertFrom (LrWpanRoute::ConvertAddress (m_netDevice->GetBroadcast ())))
    {
      if (!m_transmissionReceivedCallback.IsNull ())
        {
          m_transmissionReceivedCallback (this, source, m_globalRoutingId);
        }
      else
        {
          NS_LOG_WARN ("LrWpanGlobalRoutingDevice: No TransmissionReceivedCallbackSet!");
        }
    }
  else
    {
      LrWpanStaticRoutingDevice::Receive (device, packet, protocol, source, destination,
                                          packetType);
    }
}

void
LrWpanGlobalRoutingDevice::SetTransmissionReceivedCallback (TransmissionReceivedCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_transmissionReceivedCallback = cb;
}

void
LrWpanGlobalRoutingDevice::SendDiscoveryTransmission ()
{
  NS_LOG_FUNCTION (this);
  LrWpanMacHeader header;
  Address dest = LrWpanRoute::ConvertAddress (m_netDevice->GetBroadcast ());
  NS_ASSERT (
      Mac16Address::IsMatchingType (dest)); // LrWpanNetDevice::GetBroadcast returns Mac48Address
  header.SetDstAddrFields (m_globalRoutingId, Mac16Address::ConvertFrom (dest));
  header.SetDstAddrMode (SHORT_ADDR);
  Ptr<Packet> packet = Create<Packet> (1);
  packet->AddHeader (header);
  m_netDevice->Send (packet, m_netDevice->GetBroadcast (), 0);
}
} // namespace ns3