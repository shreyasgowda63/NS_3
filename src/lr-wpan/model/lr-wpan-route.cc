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

#include "lr-wpan-route.h"
#include "lr-wpan-net-device.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LrWpanRoute");

void
LrWpanRoute::SetDestination (Address dest)
{
  m_dest = ConvertAddress (dest);
}

Address
LrWpanRoute::GetDestination () const
{
  return m_dest;
}

void
LrWpanRoute::SetSource (Address src)
{
  m_source = ConvertAddress (src);
}

Address
LrWpanRoute::GetSource () const
{
  return m_source;
}

void
LrWpanRoute::SetGateway (Address gw)
{
  m_gateway = ConvertAddress (gw);
}

Address
LrWpanRoute::GetGateway () const
{
  return m_gateway;
}

Address
LrWpanRoute::ConvertAddress (Address addr)
{
  NS_LOG_FUNCTION_NOARGS ();

  if (Mac16Address::IsMatchingType (addr))
    { // Mac16Address
      return Mac16Address::ConvertFrom (addr);
    }
  else if (Mac48Address::IsMatchingType (addr))
    { // Mac48Address
      uint8_t buf[6];
      Mac48Address addr48 = Mac48Address::ConvertFrom (addr);
      addr48.CopyTo (buf);
      Mac16Address addr16;
      addr16.CopyFrom (buf + 4);
      return addr16;
    }
  else if (Mac64Address::IsMatchingType (addr))
    { // Mac64Address
      return Mac64Address::ConvertFrom (addr);
    }
  else
    { // No compatible address given
      NS_ABORT_MSG (
          "LrWpanRoute::ConvertAddress: The input address is not compatible to Mac16, Mac48, "
          "or Mac64. Input address: "
          << addr);
    }
}

} // namespace ns3
