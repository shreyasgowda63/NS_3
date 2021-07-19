/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
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
 * Authors: Pavel Boyko <boyko@iitp.ru>, written after OlsrHelper by Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "aodv-helper.h"
#include "ns3/aodv-ipv4-routing-protocol.h"
#include "ns3/node-list.h"
#include "ns3/names.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-list-routing.h"

namespace ns3
{

AodvHelper::AodvHelper() : 
  Ipv4RoutingHelper ()
{
  m_ipv4Factory.SetTypeId ("ns3::aodv::Ipv4RoutingProtocol");
  m_commonFactory.SetTypeId("ns3::aodv::RoutingProtocol");
}

AodvHelper* 
AodvHelper::Copy (void) const 
{
  return new AodvHelper (*this); 
}

Ptr<Ipv4RoutingProtocol> 
AodvHelper::Create (Ptr<Node> node) const
{
  Ptr<aodv::RoutingProtocol> common_agent = m_commonFactory.Create<aodv::RoutingProtocol> ();
  Ptr<aodv::Ipv4RoutingProtocol> ipv4_agent = m_ipv4Factory.Create<aodv::Ipv4RoutingProtocol> (); 
  node->AggregateObject(common_agent);
  ipv4_agent->SetIPv4AODVProtocol(common_agent);
  common_agent->SetCommonAODVProtocol(ipv4_agent);
  node->AggregateObject(ipv4_agent);
  return ipv4_agent;
}

void 
AodvHelper::Set (std::string name, const AttributeValue &value)
{
  m_commonFactory.Set (name, value);
}

int64_t
AodvHelper::AssignStreams (NodeContainer c, int64_t stream)
{
  int64_t currentStream = stream;
  Ptr<Node> node;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      node = (*i);
      Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
      NS_ASSERT_MSG (ipv4, "Ipv4 not installed on node");
      Ptr<Ipv4RoutingProtocol> proto = ipv4->GetRoutingProtocol ();
      NS_ASSERT_MSG (proto, "Ipv4 routing not installed on node");
      Ptr<aodv::Ipv4RoutingProtocol> aodv = DynamicCast<aodv::Ipv4RoutingProtocol> (proto);
      if (aodv)
        {
          currentStream += aodv->AssignStreams (currentStream);
          continue;
        }
      // Aodv may also be in a list
      Ptr<Ipv4ListRouting> list = DynamicCast<Ipv4ListRouting> (proto);
      if (list)
        {
          int16_t priority;
          Ptr<Ipv4RoutingProtocol> listProto;
          Ptr<aodv::Ipv4RoutingProtocol> listAodv;
          for (uint32_t i = 0; i < list->GetNRoutingProtocols (); i++)
            {
              listProto = list->GetRoutingProtocol (i, priority);
              listAodv = DynamicCast<aodv::Ipv4RoutingProtocol> (listProto);
              if (listAodv)
                {
                  currentStream += listAodv->AssignStreams (currentStream);
                  break;
                }
            }
        }
    }
  return (currentStream - stream);
}

}
