/*
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
 */

#include "dhcp6-helper.h"

#include "ns3/dhcp6-client.h"
#include "ns3/dhcp6-server.h"
#include "ns3/ipv6.h"
#include "ns3/log.h"
#include "ns3/loopback-net-device.h"
#include "ns3/names.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Dhcp6Helper");

Dhcp6Helper::Dhcp6Helper()
{
    m_clientFactory.SetTypeId(Dhcp6Client::GetTypeId());
    m_serverFactory.SetTypeId(Dhcp6Server::GetTypeId());
}

void
Dhcp6Helper::SetClientAttribute(std::string name, const AttributeValue& value)
{
    m_clientFactory.Set(name, value);
}

void
Dhcp6Helper::SetServerAttribute(std::string name, const AttributeValue& value)
{
    m_serverFactory.Set(name, value);
}

ApplicationContainer
Dhcp6Helper::InstallDhcp6Client(Ptr<NetDevice> netDevice) const
{
    return ApplicationContainer(InstallDhcp6ClientPriv(netDevice));
}

Ptr<Application>
Dhcp6Helper::InstallDhcp6ClientPriv(Ptr<NetDevice> netDevice) const
{
    Ptr<Node> node = netDevice->GetNode();
    NS_ASSERT_MSG(node, "Dhcp6ClientHelper: NetDevice is not not associated with any node -> fail");

    Ptr<Ipv6> ipv6 = node->GetObject<Ipv6>();
    NS_ASSERT_MSG(ipv6,
                  "DhcpHelper: NetDevice is associated"
                  " with a node without IPv6 stack installed -> fail "
                  "(maybe need to use InternetStackHelper?)");

    int32_t interface = ipv6->GetInterfaceForDevice(netDevice);
    if (interface == -1)
    {
        interface = ipv6->AddInterface(netDevice);
    }
    NS_ASSERT_MSG(interface >= 0, "Dhcp6Helper: Interface index not found");

    ipv6->SetMetric(interface, 1);
    ipv6->SetUp(interface);

    Ptr<Dhcp6Client> app = m_clientFactory.Create<Dhcp6Client>();
    app->SetDhcp6ClientNetDevice(netDevice);
    node->AddApplication(app);

    return app;
}

ApplicationContainer
Dhcp6Helper::InstallDhcp6Client(NetDeviceContainer netDevices) const
{
    ApplicationContainer apps;
    for (auto i = netDevices.Begin(); i != netDevices.End(); ++i)
    {
        apps.Add(InstallDhcp6ClientPriv(*i));
    }
    return apps;
}

ApplicationContainer
Dhcp6Helper::InstallDhcp6Server(Ptr<NetDevice> netDevice)
{
    m_serverFactory.Set("LeaseTime", TimeValue(Seconds(30)));

    Ptr<Node> node = netDevice->GetNode();
    NS_ASSERT_MSG(node, "Dhcp6Helper: NetDevice is not not associated with any node -> fail");

    Ptr<Ipv6> ipv6 = node->GetObject<Ipv6>();
    NS_ASSERT_MSG(ipv6,
                  "Dhcp6Helper: NetDevice is associated"
                  " with a node without IPv6 stack installed -> fail "
                  "(maybe need to use InternetStackHelper?)");

    int32_t interface = ipv6->GetInterfaceForDevice(netDevice);
    if (interface == -1)
    {
        interface = ipv6->AddInterface(netDevice);
    }
    NS_ASSERT_MSG(interface >= 0, "DhcpHelper: Interface index not found");

    ipv6->SetMetric(interface, 1);
    ipv6->SetUp(interface);

    Ptr<Dhcp6Server> app = m_serverFactory.Create<Dhcp6Server>();
    app->SetDhcp6ServerNetDevice(netDevice);
    node->AddApplication(app);
    return ApplicationContainer(app);
}

} // namespace ns3
