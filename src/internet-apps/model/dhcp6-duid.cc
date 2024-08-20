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
#include "ns3/ipv6-l3-protocol.h"
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
    m_linkLayerAddress = nullptr;
    m_idLen = 0;
}

TypeId
Duid::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Duid")
                            .SetParent<Header>()
                            .SetGroupName("Internet-Apps")
                            .AddConstructor<Duid>();
    return tid;
}

TypeId
Duid::GetInstanceTypeId() const
{
    return GetTypeId();
}

bool
Duid::operator==(const Duid& o) const
{
    return (m_duidType == o.m_duidType && m_hardwareType == o.m_hardwareType &&
            m_idLen == o.m_idLen &&
            memcmp(m_linkLayerAddress, o.m_linkLayerAddress, m_idLen) == 0 && m_time == o.m_time);
}

bool
operator<(const Duid& a, const Duid& b)
{
    if (a.m_duidType < b.m_duidType)
    {
        return true;
    }
    else if (a.m_duidType > b.m_duidType)
    {
        return false;
    }
    if (a.m_hardwareType < b.m_hardwareType)
    {
        return true;
    }
    else if (a.m_hardwareType > b.m_hardwareType)
    {
        return false;
    }
    NS_ASSERT(a.GetLength() == b.GetLength());
    for (uint8_t i = 0; i < a.GetLength(); i++)
    {
        if (a.m_linkLayerAddress[i] < b.m_linkLayerAddress[i])
        {
            return true;
        }
        else if (a.m_linkLayerAddress[i] > b.m_linkLayerAddress[i])
        {
            return false;
        }
    }
    return false;
}

bool
Duid::IsInvalid() const
{
    return m_linkLayerAddress == nullptr;
}

uint8_t
Duid::GetLength() const
{
    return m_idLen;
}

uint32_t
Duid::CopyTo(uint8_t* buffer) const
{
    NS_LOG_FUNCTION(this << &buffer);
    std::memcpy(buffer, m_linkLayerAddress, m_idLen);
    return m_idLen;
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

void
Duid::SetDuid(uint8_t* linkLayerAddress, uint8_t idLen)
{
    NS_LOG_FUNCTION(this << linkLayerAddress << idLen);

    m_duidType = 3; // DUID-LL
    m_idLen = idLen;

    NS_ASSERT_MSG(m_idLen == 6 || m_idLen != 8, "Duid: Invalid link layer address length.");

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

    if (m_linkLayerAddress != nullptr)
    {
        delete[] m_linkLayerAddress;
    }
    m_linkLayerAddress = new uint8_t[m_idLen]();

    memcpy(m_linkLayerAddress, linkLayerAddress, m_idLen);
}

void
Duid::Initialize(Ptr<Node> node)
{
    Ptr<Ipv6L3Protocol> ipv6 = node->GetObject<Ipv6L3Protocol>();
    uint32_t nInterfaces = ipv6->GetNInterfaces();

    uint32_t maxAddressLength = 0;
    Address duidAddress;

    for (uint32_t i = 0; i < nInterfaces; i++)
    {
        Ptr<NetDevice> device = ipv6->GetNetDevice(i);

        // Discard the loopback device.
        if (DynamicCast<LoopbackNetDevice>(device))
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
    auto buffer = new uint8_t[duidAddress.GetLength()]();
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

uint32_t
Duid::GetSerializedSize() const
{
    return 10;
}

void
Duid::Print(std::ostream& os) const
{
    os << "( type = " << (uint32_t)m_duidType << " )";
}

void
Duid::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteHtonU16(m_duidType);
    i.WriteHtonU16(m_hardwareType);
    i.Write(m_linkLayerAddress, m_idLen);
}

uint32_t
Duid::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    m_duidType = i.ReadNtohU16();
    m_hardwareType = i.ReadNtohU16();
    return 4;
}

uint32_t
Duid::DeserializeIdentifier(Buffer::Iterator start, uint32_t len)
{
    Buffer::Iterator i = start;
    m_idLen = len;
    m_linkLayerAddress = new uint8_t[m_idLen]();
    i.Read(m_linkLayerAddress, m_idLen);
    return m_idLen;
}
} // namespace ns3
