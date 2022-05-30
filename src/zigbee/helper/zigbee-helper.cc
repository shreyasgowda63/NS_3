/*
 * Copyright (c) 2023 Tokushima University, Japan
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
 * Author:
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "zigbee-helper.h"

#include "ns3/log.h"
#include "ns3/lr-wpan-net-device.h"
#include "ns3/names.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/zigbee-stack.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ZigbeeHelper");

ZigbeeHelper::ZigbeeHelper()
{
    NS_LOG_FUNCTION(this);
    m_stackFactory.SetTypeId("ns3::ZigbeeStack");
}

void
ZigbeeHelper::SetDeviceAttribute(std::string n1, const AttributeValue& v1)
{
    NS_LOG_FUNCTION(this);
    m_stackFactory.Set(n1, v1);
}

ZigbeeStackContainer
ZigbeeHelper::Install(const NetDeviceContainer c)
{
    NS_LOG_FUNCTION(this);

    ZigbeeStackContainer zigbeeStackContainer;

    for (uint32_t i = 0; i < c.GetN(); ++i)
    {
        Ptr<NetDevice> device = c.Get(i);

        NS_ASSERT_MSG(device != nullptr, "No LrWpanNetDevice found in the node " << i);
        Ptr<LrWpanNetDevice> lrwpanNetdevice = DynamicCast<LrWpanNetDevice>(device);

        Ptr<Node> node = lrwpanNetdevice->GetNode();
        NS_LOG_LOGIC("**** Install Zigbee on node " << node->GetId());

        Ptr<ZigbeeStack> zigbeeStack = m_stackFactory.Create<ZigbeeStack>();
        zigbeeStackContainer.Add(zigbeeStack);
        node->AggregateObject(zigbeeStack);
        zigbeeStack->SetLrWpanNetDevice(lrwpanNetdevice);
    }
    return zigbeeStackContainer;
}

} // namespace ns3
