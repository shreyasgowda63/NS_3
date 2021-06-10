/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Universita' di Firenze, Italy
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
 *         Adnan Rashid <adnan.rashid@unifi.it>
 */

#include "ns3/log.h"
#include "ns3/sixlowpan-net-device.h"
#include "ns3/sixlowpan-nd-protocol.h"
#include "ns3/mac16-address.h"
#include "ns3/mac64-address.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/names.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/ipv6-routing-protocol.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/ipv6-address-helper.h"
#include "ns3/sixlowpan-nd-context.h"
#include "ns3/sixlowpan-nd-prefix.h"
#include "sixlowpan-helper.h"

namespace ns3 {

class Address;

NS_LOG_COMPONENT_DEFINE ("SixLowPanHelper");

SixLowPanHelper::SixLowPanHelper ()
{
  NS_LOG_FUNCTION (this);
  m_deviceFactory.SetTypeId ("ns3::SixLowPanNetDevice");
}

void SixLowPanHelper::SetDeviceAttribute (std::string n1,
                                          const AttributeValue &v1)
{
  NS_LOG_FUNCTION (this);
  m_deviceFactory.Set (n1, v1);
}

NetDeviceContainer SixLowPanHelper::Install (const NetDeviceContainer c)
{
  NS_LOG_FUNCTION (this);

  NetDeviceContainer devs;

  for (uint32_t i = 0; i < c.GetN (); ++i)
    {
      Ptr<NetDevice> device = c.Get (i);
      NS_ASSERT_MSG (device != 0, "No NetDevice found in the node " << int(i) );

      Ptr<Node> node = device->GetNode ();
      NS_LOG_LOGIC ("**** Install 6LoWPAN on node " << node->GetId ());

      Ptr<SixLowPanNetDevice> dev = m_deviceFactory.Create<SixLowPanNetDevice> ();
      devs.Add (dev);
      node->AddDevice (dev);
      dev->SetNetDevice (device);
    }
  return devs;
}

Ipv6InterfaceContainer SixLowPanHelper::InstallSixLowPanNdBorderRouter (NetDeviceContainer c, Ipv6Address baseAddr)
{
  InstallSixLowPanNd (c, true);

  Ipv6AddressHelper ipv6;
  ipv6.SetBase (baseAddr, Ipv6Prefix (64));
  Ipv6InterfaceContainer deviceInterfaces;
  deviceInterfaces = ipv6.AssignWithoutOnLink (c);

  for (uint32_t index = 0; index < c.GetN (); index++)
    {
      Ptr<Node> node = c.Get (index)->GetNode ();
      Ptr<Ipv6L3Protocol> ipv6l3 = node->GetObject<Ipv6L3Protocol> ();
      ipv6l3->SetAttribute ("SendIcmpv6Redirect", BooleanValue (false));

      Ptr<SixLowPanNetDevice> sixLowPanNetDevice = DynamicCast<SixLowPanNetDevice> (c.Get (index));
      Ptr<SixLowPanNdProtocol> sixLowPanNdProtocol = node->GetObject<SixLowPanNdProtocol> ();
      if (sixLowPanNdProtocol->IsBorderRouterOnInterface (sixLowPanNetDevice))
        {
          NS_ABORT_MSG ("Interface " << sixLowPanNetDevice << " has been already initialized, aborting.");
        }
      sixLowPanNdProtocol->SetInterfaceAs6lbr (sixLowPanNetDevice);
    }
  return deviceInterfaces;

}

Ipv6InterfaceContainer SixLowPanHelper::InstallSixLowPanNdNode (NetDeviceContainer c)
{
  InstallSixLowPanNd (c, false);

  Ipv6AddressHelper ipv6;
  Ipv6InterfaceContainer deviceInterfaces;
  deviceInterfaces = ipv6.AssignWithoutAddress (c);

  return deviceInterfaces;
}

void SixLowPanHelper::InstallSixLowPanNd (NetDeviceContainer c, bool borderRouter)
{
  for (uint32_t i = 0; i < c.GetN (); ++i)
    {
      Ptr<NetDevice> device = c.Get (i);
      Ptr<Node> node = device->GetNode ();
      Ptr<SixLowPanNetDevice> dev = DynamicCast<SixLowPanNetDevice> (device);
      Ptr<Ipv6L3Protocol> ipv6 = node->GetObject<Ipv6L3Protocol> ();
      int32_t interfaceId = ipv6->GetInterfaceForDevice (dev);

      if (interfaceId == -1)
        {
          interfaceId = ipv6->AddInterface (dev);
        }

      Ptr<SixLowPanNdProtocol> sixLowPanNdProtocol = node->GetObject<SixLowPanNdProtocol> ();
      if (!sixLowPanNdProtocol)
        {
          sixLowPanNdProtocol = CreateObject<SixLowPanNdProtocol> ();
          sixLowPanNdProtocol->SetAttribute ("DAD", BooleanValue (false));
          sixLowPanNdProtocol->SetAttribute ("MaxRtrSolicitations", UintegerValue (3));
          sixLowPanNdProtocol->SetAttribute ("RtrSolicitationInterval", TimeValue (Seconds (10)));
          // sixLowPanNdProtocol->SetAttribute ("RsJitter", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1000.0]"));
          node->AggregateObject (sixLowPanNdProtocol);
        }
      ipv6->Insert (sixLowPanNdProtocol, interfaceId);

      if (borderRouter)
        {
          ipv6->SetForwarding (interfaceId, true);
        }
    }
  return;
}

void SixLowPanHelper::SetAdvertisedPrefix (const Ptr<NetDevice> nd, Ipv6Prefix prefix)
{
  NS_LOG_FUNCTION (this << nd << prefix.ConvertToIpv6Address () << prefix);

  Ptr<Node> node = nd->GetNode ();

  Ptr<SixLowPanNetDevice> sixLowPanNetDevice = DynamicCast<SixLowPanNetDevice> (nd);
  if (sixLowPanNetDevice)
    {
      Ptr<SixLowPanNdProtocol> sixLowPanNdProtocol = node->GetObject<SixLowPanNdProtocol> ();
      if (!sixLowPanNdProtocol)
        {
          NS_ABORT_MSG ("Can not add a Prefix to a 6LBR on a node because I can not find 6LoWPAN-ND protocol");
        }
      sixLowPanNdProtocol->SetAdvertisedPrefix (sixLowPanNetDevice, prefix);
    }
  else
    {
      NS_LOG_WARN ("Not a SixLowPan NetDevice - doing nothing");
    }
}

void SixLowPanHelper::AddAdvertisedContext (const Ptr<NetDevice> nd, Ipv6Prefix context)
{
  NS_LOG_FUNCTION (this << nd << context.ConvertToIpv6Address () << context);

  Ptr<Node> node = nd->GetNode ();

  Ptr<SixLowPanNetDevice> sixLowPanNetDevice = DynamicCast<SixLowPanNetDevice> (nd);
  if (sixLowPanNetDevice)
    {

      Ptr<SixLowPanNdProtocol> sixLowPanNdProtocol = node->GetObject<SixLowPanNdProtocol> ();
      if (!sixLowPanNdProtocol)
        {
          NS_ABORT_MSG ("Can not add a Context to a 6LBR on a node because I can not find 6LoWPAN-ND protocol");
        }

      sixLowPanNdProtocol->AddAdvertisedContext (sixLowPanNetDevice, context);
    }
  else
    {
      NS_LOG_WARN ("Not a SixLowPan NetDevice - doing nothing");
    }
}

void SixLowPanHelper::RemoveAdvertisedContext (const Ptr<NetDevice> nd, Ipv6Prefix context)
{
  NS_LOG_FUNCTION (this << nd << context.ConvertToIpv6Address () << context);

  Ptr<Node> node = nd->GetNode ();

  Ptr<SixLowPanNetDevice> sixLowPanNetDevice = DynamicCast<SixLowPanNetDevice> (nd);
  if (sixLowPanNetDevice)
    {

      Ptr<SixLowPanNdProtocol> sixLowPanNdProtocol = node->GetObject<SixLowPanNdProtocol> ();
      if (!sixLowPanNdProtocol)
        {
          NS_ABORT_MSG ("Can not remove a Context from a 6LBR on a node because I can not find 6LoWPAN-ND protocol");
        }

      sixLowPanNdProtocol->RemoveAdvertisedContext (sixLowPanNetDevice, context);
    }
  else
    {
      NS_LOG_WARN ("Not a SixLowPan NetDevice - doing nothing");
    }
}

void SixLowPanHelper::AddContext (NetDeviceContainer c, uint8_t contextId, Ipv6Prefix context, Time validity)
{
  NS_LOG_FUNCTION (this << +contextId << context << validity);

  for (uint32_t i = 0; i < c.GetN (); ++i)
    {
      Ptr<NetDevice> device = c.Get (i);
      Ptr<SixLowPanNetDevice> sixDevice = DynamicCast<SixLowPanNetDevice> (device);
      if (sixDevice)
        {
          sixDevice->AddContext (contextId, context, true, validity);
        }
    }
}

void SixLowPanHelper::RenewContext (NetDeviceContainer c, uint8_t contextId, Time validity)
{
  NS_LOG_FUNCTION (this << +contextId << validity);

  for (uint32_t i = 0; i < c.GetN (); ++i)
    {
      Ptr<NetDevice> device = c.Get (i);
      Ptr<SixLowPanNetDevice> sixDevice = DynamicCast<SixLowPanNetDevice> (device);
      if (sixDevice)
        {
          sixDevice->RenewContext (contextId, validity);
        }
    }
}

void SixLowPanHelper::InvalidateContext (NetDeviceContainer c, uint8_t contextId)
{
  NS_LOG_FUNCTION (this << +contextId);

  for (uint32_t i = 0; i < c.GetN (); ++i)
    {
      Ptr<NetDevice> device = c.Get (i);
      Ptr<SixLowPanNetDevice> sixDevice = DynamicCast<SixLowPanNetDevice> (device);
      if (sixDevice)
        {
          sixDevice->InvalidateContext (contextId);
        }
    }
}

void SixLowPanHelper::RemoveContext (NetDeviceContainer c, uint8_t contextId)
{
  NS_LOG_FUNCTION (this << +contextId);

  for (uint32_t i = 0; i < c.GetN (); ++i)
    {
      Ptr<NetDevice> device = c.Get (i);
      Ptr<SixLowPanNetDevice> sixDevice = DynamicCast<SixLowPanNetDevice> (device);
      if (sixDevice)
        {
          sixDevice->RemoveContext (contextId);
        }
    }
}

int64_t SixLowPanHelper::AssignStreams (NetDeviceContainer c, int64_t stream)
{
  int64_t currentStream = stream;
  Ptr<NetDevice> netDevice;
  for (NetDeviceContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      netDevice = (*i);
      Ptr<SixLowPanNetDevice> dev = DynamicCast<SixLowPanNetDevice> (netDevice);
      if (dev)
        {
          currentStream += dev->AssignStreams (currentStream);
        }
      Ptr<SixLowPanNdProtocol> sixLowPanNdProtocol = dev->GetNode ()->GetObject<SixLowPanNdProtocol> ();
      if (sixLowPanNdProtocol)
        {
          currentStream += sixLowPanNdProtocol->AssignStreams (currentStream);
        }
    }
  return (currentStream - stream);
}

} // namespace ns3
