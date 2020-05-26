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
#include "ns3/boolean.h"
#include "ns3/ipv6-address-helper.h"
#include "ns3/sixlowpan-nd-context.h"
#include "ns3/sixlowpan-nd-prefix.h"
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

NetDeviceContainer SixLowPanHelper::InstallInternal (const NetDeviceContainer c, bool borderRouter)
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
          node->AggregateObject (sixLowPanNdProtocol);
        }
      ipv6->Insert (sixLowPanNdProtocol, interfaceId);
      ipv6->SetForwarding (interfaceId, true);

      Address devAddr = device->GetAddress ();
      Ipv6Address linkLocalAddr;
      if (Mac16Address::IsMatchingType(devAddr))
        {
          linkLocalAddr = Ipv6Address::MakeAutoconfiguredLinkLocalAddress(Mac16Address::ConvertFrom (device->GetAddress ()));
        }
      else if (Mac64Address::IsMatchingType(devAddr))
        {
          linkLocalAddr = Ipv6Address::MakeAutoconfiguredLinkLocalAddress(Mac64Address::ConvertFrom (device->GetAddress ()));
        }
      else if (Mac48Address::IsMatchingType(devAddr))
        {
          linkLocalAddr = Ipv6Address::MakeAutoconfiguredLinkLocalAddress(Mac48Address::ConvertFrom (device->GetAddress ()));
        }
      else
        {
          NS_ABORT_MSG ("SixLowPanNdProtocol -- failed to found a link local address from MAC address " << devAddr);
        }

      if (!borderRouter)
        {
          Simulator::Schedule (Time (MilliSeconds (1)),
                               &Icmpv6L4Protocol::SendRS, sixLowPanNdProtocol, linkLocalAddr, Ipv6Address::GetAllRoutersMulticast (), devAddr);
        }
    }
  return devs;
}

void SixLowPanHelper::Set6LowPanBorderRouter (const Ptr<NetDevice> nd)
{
  NS_LOG_FUNCTION (this << nd);

  NetDeviceContainer devs;

  Ptr<Node> node = nd->GetNode ();
  NS_LOG_LOGIC ("**** Install 6LoWPAN Border Router on node " << node->GetId ());

  Ptr<SixLowPanNetDevice> sixLowPanNetDevice = DynamicCast<SixLowPanNetDevice> (nd);

  Ptr<SixLowPanNdProtocol> sixLowPanNdProtocol = node->GetObject<SixLowPanNdProtocol> ();
  if (sixLowPanNetDevice)
    {
      if (sixLowPanNdProtocol->IsBorderRouterOnInterface (sixLowPanNetDevice))
        {
          NS_ABORT_MSG ("Interface " << sixLowPanNetDevice << " has been already initialized, skipping.");
          return;
        }
      sixLowPanNdProtocol->SetInterfaceAs6lbr (sixLowPanNetDevice);
    }
  else
    {
      NS_LOG_WARN ("Not a SixLowPan NetDevice - doing nothing");
    }

  return;
}


NetDeviceContainer SixLowPanHelper::Install (NetDeviceContainer c)
{
  NetDeviceContainer devices;

  devices = InstallInternal (c, false);
  Ipv6AddressHelper ipv6;
  Ipv6InterfaceContainer deviceInterfaces;
  deviceInterfaces = ipv6.AssignWithoutAddress (devices);

  return devices;
}

NetDeviceContainer SixLowPanHelper::InstallSixLowPanBorderRouter (NetDeviceContainer c, Ipv6Address baseAddr)
{
  NetDeviceContainer devices;

  devices = InstallInternal (c, true);

  Ipv6AddressHelper ipv6;
  ipv6.SetBase (baseAddr, Ipv6Prefix (64));
  Ipv6InterfaceContainer deviceInterfaces;
  deviceInterfaces = ipv6.Assign (devices.Get (0));

  return devices;

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
    }
  return (currentStream - stream);
}

} // namespace ns3
