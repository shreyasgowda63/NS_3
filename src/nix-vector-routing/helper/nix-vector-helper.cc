/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 The Georgia Institute of Technology
 * Copyright (c) 2021 NITK Surathkal
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
 * This file is adopted from the old ipv4-nix-vector-helper.cc.
 *
 * Authors: Josh Pelkey <jpelkey@gatech.edu>
 * 
 * Modified by: Ameya Deshpande <ameyanrd@outlook.com>
 */

#include "ipv4-nix-vector-helper.h"
#include "ns3/nix-vector-routing.h"
#include "ns3/assert.h"

namespace ns3 {

template <typename parent>
NixVectorHelper<parent>::NixVectorHelper ()
{
  m_agentFactory.SetTypeId ("ns3::NixVectorRouting");
  // Check if the parent is Ipv4RoutingHelper
  NS_ASSERT_MSG (IsIpv4::value, "Template parameter is not Ipv4RoutingHelper");
}

template <typename parent>
NixVectorHelper<parent>::NixVectorHelper (const NixVectorHelper<parent> &o)
  : m_agentFactory (o.m_agentFactory)
{
  // Check if the parent is Ipv4RoutingHelper
  NS_ASSERT_MSG (IsIpv4::value, "Template parameter is not Ipv4RoutingHelper");
}

template <typename parent>
NixVectorHelper<parent>*
NixVectorHelper<parent>::Copy (void) const
{
  return new NixVectorHelper<parent> (*this);
}

template <typename parent>
Ptr<typename NixVectorHelper<parent>::IpRoutingProtocol>
NixVectorHelper<parent>::Create (Ptr<Node> node) const
{
  Ptr<NixVectorRouting<IpRoutingProtocol>> agent = m_agentFactory.Create<NixVectorRouting<IpRoutingProtocol>> ();
  agent->SetNode (node);
  node->AggregateObject (agent);
  return agent;
}

template <typename parent>
void
NixVectorHelper<parent>::PrintRoutingPathAt (Time printTime, Ptr<Node> source, IpAddress dest, Ptr<OutputStreamWrapper> stream, Time::Unit unit)
{
  Simulator::Schedule (printTime, &NixVectorHelper<parent>::PrintRoute, source, dest, stream, unit);
}

template <typename parent>
void
NixVectorHelper<parent>::PrintRoute (Ptr<Node> source, IpAddress dest, Ptr<OutputStreamWrapper> stream, Time::Unit unit)
{
  Ptr<NixVectorRouting<IpRoutingProtocol>> rp = parent::template GetRouting <NixVectorRouting<IpRoutingProtocol>> (source->GetObject<Ip> ()->GetRoutingProtocol ());
  NS_ASSERT (rp);
  rp->PrintRoutingPath (source, dest, stream, unit);
}

template class NixVectorHelper<Ipv4RoutingHelper>;

} // namespace ns3
