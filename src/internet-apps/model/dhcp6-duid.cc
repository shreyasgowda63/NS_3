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

namespace dhcp6
{
Duid::Duid()
{
    m_duidType = 3;
    m_hardwareType = 0;
    m_time = Time();
    m_identifier = std::vector<uint8_t>();
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
            m_identifier == o.m_identifier);
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
        if (a.m_identifier[i] < b.m_identifier[i])
        {
            return true;
        }
        else if (a.m_identifier[i] > b.m_identifier[i])
        {
            return false;
        }
    }
    return false;
}

bool
Duid::IsInvalid() const
{
    return m_identifier.empty();
}

uint8_t
Duid::GetLength() const
{
    return m_identifier.size();
}

std::vector<uint8_t>
Duid::GetIdentifier() const
{
    NS_LOG_FUNCTION(this);
    return m_identifier;
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
Duid::SetDuid(std::vector<uint8_t> identifier)
{
    NS_LOG_FUNCTION(this << identifier);

    m_duidType = 3; // DUID-LL
    uint8_t idLen = identifier.size();

    NS_ASSERT_MSG(idLen == 6 || idLen == 8, "Duid: Invalid identifier length.");

    switch (idLen)
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

    m_identifier.resize(idLen);
    m_identifier = identifier;
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
    uint8_t buffer[16];
    duidAddress.CopyTo(buffer);

    std::vector<uint8_t> identifier(duidAddress.GetLength());
    std::copy(buffer, buffer + duidAddress.GetLength(), identifier.begin());
    SetDuid(identifier);
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
    return 4 + m_identifier.size();
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

    for (uint32_t j = 0; j < m_identifier.size(); j++)
    {
        i.WriteU8(m_identifier[j]);
    }
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
    m_identifier.resize(len);

    for (uint32_t j = 0; j < len; j++)
    {
        m_identifier[j] = i.ReadU8();
    }

    return m_identifier.size();
}

size_t
Duid::DuidHash::operator()(const Duid& x) const noexcept
{
    uint8_t duidLen = x.GetLength();
    std::vector<uint8_t> buffer = x.GetIdentifier();

    std::string s(buffer.begin(), buffer.begin() + duidLen);
    return std::hash<std::string>{}(s);
}
} // namespace dhcp6
} // namespace ns3
