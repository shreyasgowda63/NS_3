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

#include "lr-wpan-static-routing-device.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LrWpanStaticRoutingDevice");

TypeId
LrWpanStaticRoutingDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LrWpanStaticRoutingDevice")
                          .SetParent<LrWpanRoutingDevice> ()
                          .SetGroupName ("LrWpan");
  return tid;
}

Ptr<LrWpanRoute>
LrWpanStaticRoutingDevice::GetRouteTo (Address &dest)
{
  NS_LOG_FUNCTION (this << dest);

  // Find route to destination
  LrWpanRoutes::iterator route;
  for (route = m_staticRoutes.begin (); route != m_staticRoutes.end (); route++)
    {
      Address routeDest = (*route)->GetDestination ();

      // Check for matching destination address
      if (routeDest == dest)
        {
          return *route;
        }
    }

  // If no route found, return a direct (unchecked) routed to the destination
  Ptr<LrWpanRoute> directRoute = CreateObject<LrWpanRoute> ();
  directRoute->SetDestination (dest);
  directRoute->SetGateway (dest);
  directRoute->SetSource (GetAddress ());
  return directRoute;
}

void
LrWpanStaticRoutingDevice::AddStaticRoute (Ptr<LrWpanRoute> route)
{
  NS_LOG_FUNCTION (this << route);
  m_staticRoutes.push_back (route);
}

} // namespace ns3