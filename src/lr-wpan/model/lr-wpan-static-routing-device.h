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
#ifndef LR_WPAN_STATIC_ROUTING_DEVICE_H
#define LR_WPAN_STATIC_ROUTING_DEVICE_H

#include <list>

#include <ns3/lr-wpan-routing-device.h>

namespace ns3 {

/**
 * @brief LR-WPAN static routing device.
 * 
 * This class implements a static routing method.
 * Each route has to be added manually using AddStaticRoute
 * and the class will refer to those routes to find the gateway
 * for each packet.
 */
class LrWpanStaticRoutingDevice : public LrWpanRoutingDevice
{
public:
  static TypeId GetTypeId (void);

  Ptr<LrWpanRoute> GetRouteTo (Address &dest);

  void AddStaticRoute (Ptr<LrWpanRoute> route);

  typedef std::list<Ptr<LrWpanRoute>> LrWpanRoutes;

private:
  LrWpanRoutes m_staticRoutes;
};

} // namespace ns3

#endif /*  LR_WPAN_STATIC_ROUTING_DEVICE_H */