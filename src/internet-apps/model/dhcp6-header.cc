/*
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
 */

#include "dhcp6-header.h"

#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Dhcp6Header");

Dhcp6Header::Dhcp6Header()
    : m_len(0),
      m_msgType(0),
      m_transactId(0)
{
}

Dhcp6Header::Dhcp6Header(uint8_t msgType, uint32_t transactId)
{
    m_msgType = msgType;
    m_transactId = transactId & 0x00FFFFFF;
    m_len = 4;
    elapsedTime = IntegerOptions<uint16_t>();
}

uint8_t
Dhcp6Header::GetMessageType()
{
    NS_LOG_FUNCTION(this);
    return m_msgType;
}

void
Dhcp6Header::SetMessageType(uint8_t msgType)
{
    NS_LOG_FUNCTION(this << msgType);
    m_msgType = msgType;
}

uint32_t
Dhcp6Header::GetTransactId()
{
    NS_LOG_FUNCTION(this);
    return m_transactId;
}

void
Dhcp6Header::SetTransactId(uint32_t transactId)
{
    NS_LOG_FUNCTION(this << transactId);
    m_transactId = transactId;
}

void
Dhcp6Header::ResetOptions()
{
    int i;
    for (i = 0; i < 65536; i++)
    {
        m_options[i] = false;
    }
}

TypeId
Dhcp6Header::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Dhcp6Header")
                            .SetParent<Header>()
                            .SetGroupName("Internet-Apps")
                            .AddConstructor<Dhcp6Header>();
    return tid;
}

TypeId
Dhcp6Header::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
Dhcp6Header::AddElapsedTime(uint16_t timestamp)
{
    uint16_t now = (uint16_t)Simulator::Now().GetMilliSeconds() / 10; // expressed in 0.01 seconds
    elapsedTime.SetOptionCode(OPTION_ELAPSED_TIME);
    elapsedTime.SetOptionLength(1);
    elapsedTime.SetOptionValue(now - timestamp);
}

void
Dhcp6Header::AddClientIdentifier(uint16_t hardwareType, Address linkLayerAddress)
{
    AddIdentifierOption(clientIdentifier, OPTION_CLIENTID, hardwareType, linkLayerAddress);
}

void
Dhcp6Header::AddServerIdentifier(uint16_t hardwareType, Address linkLayerAddress)
{
    AddIdentifierOption(serverIdentifier, OPTION_SERVERID, hardwareType, linkLayerAddress);
}

void
Dhcp6Header::AddIdentifierOption(IdentifierOption& identifier,
                                 uint16_t optionType,
                                 uint16_t hardwareType,
                                 Address linkLayerAddress)
{
    uint16_t duidLength = 2 + 2 + linkLayerAddress.GetLength();
    identifier.SetOptionCode(optionType);
    identifier.SetOptionLength(duidLength);
    identifier.SetHardwareType(hardwareType);
    identifier.SetLinkLayerAddress(linkLayerAddress);
}

void
Dhcp6Header::AddIanaOption(uint32_t iaid, uint32_t t1, uint32_t t2)
{
    // TODO: add check for IA_NA IAID number space.
    AddIaOption(OPTION_IA_NA, iaid, t1, t2);
}

void
Dhcp6Header::AddIataOption(uint32_t iaid)
{
    // TODO: add check for IA_TA IAID number space.
    AddIaOption(OPTION_IA_TA, iaid);
}

void
Dhcp6Header::AddIaOption(uint16_t optionType, uint32_t iaid, uint32_t t1, uint32_t t2)
{
    // Add a new identity association.
    IaOptions newIa;
    newIa.SetOptionCode(optionType);
    uint16_t optionLength = 12;

    // Obtain the list of IANA or IATA options
    std::list<IaAddressOption> iaAddresses = newIa.m_iaAddressOption;
    auto itr = iaAddresses.begin();

    // Add length of each option to the IANA or IATA option length.
    while (itr != iaAddresses.end())
    {
        optionLength += (*itr).GetOptionLength();
        itr++;
    }

    newIa.SetOptionLength(optionLength);
    newIa.SetIaid(iaid);
    newIa.SetT1(t1);
    newIa.SetT2(t2);

    switch (optionType)
    {
    case OPTION_IA_NA:
        m_ianaList.push_back(newIa);
        break;

    case OPTION_IA_TA:
        m_iataList.push_back(newIa);
        break;
    }
}

void
Dhcp6Header::AddAddress(uint32_t iaid,
                        Ipv6Address address,
                        uint32_t prefLifetime,
                        uint32_t validLifetime)
{
    auto itr = m_ianaList.begin();
    bool isIana = false;
    bool isIata = false;
    // Check if IAID corresponds to an IANA option.
    while (itr != m_ianaList.end())
    {
        if (iaid == (*itr).GetIaid())
        {
            isIana = true;
            break;
        }
        itr++;
    }

    // Check if IAID corresponds to an IATA option.
    itr = m_iataList.begin();
    while (itr != m_iataList.end())
    {
        if (iaid == (*itr).GetIaid())
        {
            isIata = true;
            break;
        }
        itr++;
    }

    if (!isIana && !isIata)
    {
        NS_LOG_ERROR("Given IAID does not exist, cannot add address.");
    }

    std::list<IaAddressOption> addressOptions = (*itr).m_iaAddressOption;

    IaAddressOption adrOpt;
    adrOpt.SetOptionCode(5);

    // Set length of IA Address option without including additional option list.
    adrOpt.SetOptionLength(24);
    adrOpt.SetIaAddress(address);
    adrOpt.SetPreferredLifetime(prefLifetime);
    adrOpt.SetValidLifetime(validLifetime);

    addressOptions.push_back(adrOpt);

    // Add the address option length to the overall IANA or IATA length.
    (*itr).SetOptionLength((*itr).GetOptionLength() + 24);
}

// TODO: Add status code option and update the length accordingly.

uint32_t
Dhcp6Header::GetSerializedSize() const
{
    return m_len;
}

void
Dhcp6Header::Print(std::ostream& os) const
{
    os << "(type=" << m_msgType << ")";
}

void
Dhcp6Header::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    uint32_t mTTid = m_msgType << 24 | m_transactId;
    i.WriteU32(mTTid);

    if (m_options[OPTION_ELAPSED_TIME])
    {
        i.WriteU16(elapsedTime.GetOptionCode());
        i.WriteU16(elapsedTime.GetOptionLength());
        i.WriteU16(elapsedTime.GetOptionValue());
    }
}

uint32_t
Dhcp6Header::Deserialize(Buffer::Iterator start)
{
    uint32_t len;
    Buffer::Iterator i = start;
    uint32_t cLen = i.GetSize();

    uint32_t mTTid = i.ReadU32();
    m_msgType = mTTid >> 24;
    m_transactId = mTTid & 0x00FFFFFF;

    len = 4;
    uint16_t option;
    bool loop = true;
    do
    {
        if (len + 2 <= cLen)
        {
            option = i.ReadU16();
            len += 2;
        }
        else
        {
            m_len = len;
            return m_len;
        }
        switch (option)
        {
        case OPTION_ELAPSED_TIME:
            if (len + 4 <= cLen)
            {
                elapsedTime.SetOptionCode(option);
                elapsedTime.SetOptionLength(i.ReadU16());
                elapsedTime.SetOptionValue(i.ReadU16());
                m_options[OPTION_ELAPSED_TIME] = true;
                len += 4;
            }
            else
            {
                NS_LOG_WARN("Malformed Packet");
                return 0;
            }
            break;
        default:
            NS_LOG_WARN("Malformed Packet");
            return 0;
        }
    } while (loop);

    m_len = len;
    return m_len;
}

} // namespace ns3
