/*
 * Copyright (c) 2024 NITK Surathkal
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
 * Author: Kavya Bhat <kavyabhat@gmail.com>
 *
 */

#ifndef DHCP6_HELPER_H
#define DHCP6_HELPER_H

#include "ns3/application-container.h"
#include "ns3/dhcp6-server.h"
#include "ns3/ipv6-address.h"
#include "ns3/ipv6-interface-container.h"
#include "ns3/net-device-container.h"
#include "ns3/object-factory.h"

#include <stdint.h>

namespace ns3
{

/**
 * @ingroup dhcp6
 *
 * @class Dhcp6Helper
 * @brief The helper class used to configure and install DHCPv6 applications on nodes
 */
class Dhcp6Helper
{
  public:
    /**
     * @brief Default constructor.
     */
    Dhcp6Helper();

    /**
     * @brief Set DHCPv6 client attributes
     * @param name Name of the attribute
     * @param value Value to be set
     */
    void SetClientAttribute(std::string name, const AttributeValue& value);

    /**
     * @brief Set DHCPv6 server attributes
     * @param name Name of the attribute
     * @param value Value to be set
     */
    void SetServerAttribute(std::string name, const AttributeValue& value);

    /**
     * @brief Install DHCP client on a set of nodes / NetDevices
     * @param netDevices The NetDevices that the DHCP client will use
     * @return The application container with DHCP client installed
     */
    ApplicationContainer InstallDhcp6Client(NetDeviceContainer netDevices) const;

    /**
     * @brief Install DHCPv6 server on a node / NetDevice. Also updates the
     * interface -> server map.
     * @param netDevices The NetDevices on which DHCPv6 server application has to be installed
     * @return The application container with DHCPv6 server installed
     */
    ApplicationContainer InstallDhcp6Server(NetDeviceContainer netDevices);

  private:
    ObjectFactory m_clientFactory; //!< DHCPv6 client factory.
    ObjectFactory m_serverFactory; //!< DHCPv6 server factory.
};

} // namespace ns3

#endif /* DHCP6_HELPER_H */
