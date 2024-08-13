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
#include "dhcp6-duid.h"

#include "ns3/address-utils.h"
#include "ns3/address.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/loopback-net-device.h"
#include "ns3/ptr.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Dhcp6Duid");

Duid::Duid()
{
    m_duidType = 3;
    m_hardwareType = 0;
    m_time = Time();
    memset(m_linkLayerAddress, 0x00, 16);
    m_idLen = 0;
}

uint16_t
Duid::GetDuidType() const
{
    NS_LOG_FUNCTION(this);
    return m_duidType;
}

void
Duid::SetDuidType(uint16_t duidType)
{
    NS_LOG_FUNCTION(this << duidType);
    m_duidType = duidType;
}

uint16_t
Duid::GetHardwareType() const
{
    NS_LOG_FUNCTION(this);
    return m_hardwareType;
}

void
Duid::SetHardwareType(uint16_t hardwareType)
{
    NS_LOG_FUNCTION(this << hardwareType);
    m_hardwareType = hardwareType;
}

Address
Duid::GetDuid() const
{
    NS_LOG_FUNCTION(this);
    Address addr;
    addr.CopyFrom(m_linkLayerAddress, m_idLen);
    return addr;
}

void
Duid::SetDuid(uint8_t linkLayerAddress[16], uint8_t idLen)
{
    NS_LOG_FUNCTION(this << linkLayerAddress << idLen);

    m_duidType = 3; // DUID-LL
    m_idLen = idLen;

    switch (m_idLen)
    {
    case 6:
        // Ethernet - 48 bit length
        SetHardwareType(1);
        break;
    case 8:
        // EUI-64 - 64 bit length
        SetHardwareType(27);
        break;
    }

    memcpy(m_linkLayerAddress, linkLayerAddress, m_idLen);
}

void
Duid::Initialize(Ptr<Node> node)
{
    uint32_t nInterfaces = node->GetNDevices();

    uint32_t maxAddressLength = 0;
    Address duidAddress;

    for (uint32_t i = 0; i < nInterfaces; i++)
    {
        Ptr<NetDevice> device = node->GetDevice(i);

        // Discard the loopback device.
        if (DynamicCast<LoopbackNetDevice>(node->GetDevice(i)))
        {
            continue;
        }

        // Check if the NetDevice is up.
        if (device->IsLinkUp())
        {
            Address address = device->GetAddress();
            if (address.GetLength() > maxAddressLength)
            {
                maxAddressLength = address.GetLength();
                duidAddress = address;
            }
        }
    }

    NS_ASSERT_MSG(!duidAddress.IsInvalid(), "Duid: No suitable NetDevice found for DUID.");

    // Consider the link-layer address of the first NetDevice in the list.
    uint8_t buffer[16];
    duidAddress.CopyTo(buffer);
    SetDuid(buffer, duidAddress.GetLength());
}

Time
Duid::GetTime() const
{
    NS_LOG_FUNCTION(this);
    return m_time;
}

void
Duid::SetTime(Time time)
{
    NS_LOG_FUNCTION(this << time);
    m_time = time;
}
} // namespace ns3
