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

#include "ns3/log.h"
#include "ns3/node.h"
#include "blist.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BList");
NS_OBJECT_ENSURE_REGISTERED (BList);

TypeId BList::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::BList")
    .SetParent<Object> ()
  ;
  return tid;
}


BList::~BList ()
{
  NS_LOG_FUNCTION (this);
}

BList::BList (std::list<Ipv6Address> haalist)
{
  NS_LOG_FUNCTION (this);
}

Ipv6Address BList::GetHoa (void) const
{
  // TODO: returns home address of the MN
  return Ipv6Address::GetAny ();
}

} /* namespace ns3 */
