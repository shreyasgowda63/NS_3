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

#include "dhcp6-header.h"

#include "dhcp6-duid.h"

#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

#include <bitset>

namespace ns3
{
namespace internetApplications
{

NS_LOG_COMPONENT_DEFINE("Dhcp6Header");

Dhcp6Header::Dhcp6Header()
    : m_len(4),
      m_msgType(0),
      m_transactId(0)
{
    m_solMaxRt = 7200;
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
Dhcp6Header::AddMessageLength(uint32_t len)
{
    m_len += len;
}

void
Dhcp6Header::ResetOptions()
{
    m_len = 4;
    m_options.clear();
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

IdentifierOption
Dhcp6Header::GetClientIdentifier()
{
    return m_clientIdentifier;
}

IdentifierOption
Dhcp6Header::GetServerIdentifier()
{
    return m_serverIdentifier;
}

StatusCodeOption
Dhcp6Header::GetStatusCodeOption()
{
    return m_statusCode;
}

std::vector<IaOptions>
Dhcp6Header::GetIanaOptions()
{
    return m_ianaList;
}

void
Dhcp6Header::AddElapsedTime(uint16_t timestamp)
{
    // Set the code, length, value.
    m_elapsedTime.SetOptionCode(OPTION_ELAPSED_TIME);
    m_elapsedTime.SetOptionLength(2);
    m_elapsedTime.SetOptionValue(timestamp);

    // Increase the total length by 6 bytes.
    AddMessageLength(6);

    // Set the option flag to true.
    m_options[OPTION_ELAPSED_TIME] = true;
}

void
Dhcp6Header::AddClientIdentifier(Duid duid)
{
    AddIdentifierOption(m_clientIdentifier, OPTION_CLIENTID, duid);
}

void
Dhcp6Header::AddServerIdentifier(Duid duid)
{
    AddIdentifierOption(m_serverIdentifier, OPTION_SERVERID, duid);
}

void
Dhcp6Header::AddIdentifierOption(IdentifierOption& identifier, OptionType optionType, Duid duid)
{
    // DUID type (2 bytes) + hw type (2 bytes) + Link-layer Address (variable)
    uint16_t duidLength = 2 + 2 + duid.GetLength();

    // Set the option code, length, hardware type, link layer address.
    identifier.SetOptionCode(optionType);
    identifier.SetOptionLength(duidLength);
    identifier.SetDuid(duid);

    // Increase the total length by (4 + duidLength) bytes.
    AddMessageLength(4 + duidLength);

    // Set the option flag to true.
    m_options[optionType] = true;
}

RequestOptions
Dhcp6Header::GetOptionRequest()
{
    return m_optionRequest;
}

void
Dhcp6Header::AddOptionRequest(uint16_t optionType)
{
    // Check if this is the first option request.
    if (m_optionRequest.GetOptionLength() == 0)
    {
        AddMessageLength(4);
    }

    // Set the option code, length, and add the requested option.
    m_optionRequest.SetOptionCode(OPTION_ORO);
    m_optionRequest.SetOptionLength(m_optionRequest.GetOptionLength() + 2);
    m_optionRequest.AddRequestedOption(optionType);

    // Increase the total length by 2 bytes.
    AddMessageLength(2);

    // Set the option flag to true.
    m_options[OPTION_ORO] = true;
}

void
Dhcp6Header::HandleOptionRequest(std::vector<uint16_t> requestedOptions)
{
    // Currently, only OPTION_SOL_MAX_RT is supported.
    for (auto itr : requestedOptions)
    {
        switch (itr)
        {
        case OPTION_SOL_MAX_RT:
            AddSolMaxRt();
            break;
        default:
            NS_LOG_WARN("Requested Option not supported.");
        }
    }
}

void
Dhcp6Header::AddSolMaxRt()
{
    // Increase the total message length.
    // 4 bytes - for option code + option length.
    // 4 bytes - for the option value.
    AddMessageLength(4 + 4);

    m_options[OPTION_SOL_MAX_RT] = true;
}

void
Dhcp6Header::AddIanaOption(uint32_t iaid, uint32_t t1, uint32_t t2)
{
    AddIaOption(OPTION_IA_NA, iaid, t1, t2);
}

void
Dhcp6Header::AddIataOption(uint32_t iaid)
{
    AddIaOption(OPTION_IA_TA, iaid);
}

void
Dhcp6Header::AddIaOption(OptionType optionType, uint32_t iaid, uint32_t t1, uint32_t t2)
{
    // Create a new identity association.
    IaOptions newIa;
    newIa.SetOptionCode(optionType);

    // Minimum option length of an IA is 12 bytes.
    uint16_t optionLength = 12;

    newIa.SetOptionLength(optionLength);
    newIa.SetIaid(iaid);
    newIa.SetT1(t1);
    newIa.SetT2(t2);

    // Check if the IA is to be added to the list of IANA or IATA options.
    // If the IAID is already present, it is not added.
    switch (optionType)
    {
    case OPTION_IA_NA: {
        bool iaidPresent = false;
        for (const auto& itr : m_ianaList)
        {
            if (itr.GetIaid() == newIa.GetIaid())
            {
                iaidPresent = true;
                break;
            }
        }

        if (!iaidPresent)
        {
            m_ianaList.push_back(newIa);
            AddMessageLength(4 + optionLength);
        }
        break;
    }

    case OPTION_IA_TA: {
        bool iaidPresent = false;
        for (const auto& itr : m_iataList)
        {
            if (itr.GetIaid() == newIa.GetIaid())
            {
                iaidPresent = true;
                break;
            }
        }

        if (!iaidPresent)
        {
            m_iataList.push_back(newIa);
            AddMessageLength(4 + optionLength);
        }
        break;
    }

    default:
        break;
    }

    // Set the option flag to true.
    m_options[optionType] = true;
}

void
Dhcp6Header::AddAddress(uint32_t iaid,
                        Ipv6Address address,
                        uint32_t prefLifetime,
                        uint32_t validLifetime)
{
    bool isIana = false;
    bool isIata = false;

    // Check if IAID corresponds to an IANA option.
    auto itr = m_ianaList.begin();
    while (itr != m_ianaList.end())
    {
        if (iaid == (*itr).GetIaid())
        {
            isIana = true;
            break;
        }
        itr++;
    }

    // Else, check if IAID corresponds to an IATA option.
    if (!isIana)
    {
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
    }

    if (!isIana && !isIata)
    {
        NS_LOG_ERROR("Given IAID does not exist, cannot add address.");
    }

    IaAddressOption adrOpt;
    adrOpt.SetOptionCode(5);

    // Set length of IA Address option without including additional option list.
    adrOpt.SetOptionLength(24);
    adrOpt.SetIaAddress(address);
    adrOpt.SetPreferredLifetime(prefLifetime);
    adrOpt.SetValidLifetime(validLifetime);

    (*itr).m_iaAddressOption.push_back(adrOpt);

    // Add the address option length to the overall IANA or IATA length.
    (*itr).SetOptionLength((*itr).GetOptionLength() + 28);

    // Increase the total message length.
    AddMessageLength(4 + 24);
}

std::map<Dhcp6Header::OptionType, bool>
Dhcp6Header::GetOptionList()
{
    return m_options;
}

void
Dhcp6Header::AddStatusCode(uint16_t status, std::string statusMsg)
{
    m_statusCode.SetOptionCode(OPTION_STATUS_CODE);
    m_statusCode.SetStatusCode(status);
    m_statusCode.SetStatusMessage(statusMsg);

    m_statusCode.SetOptionLength(2 + m_statusCode.GetStatusMessage().length());

    // Increase the total message length.
    AddMessageLength(4 + m_statusCode.GetOptionLength());

    // Set the option flag to true.
    m_options[OPTION_STATUS_CODE] = true;
}

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
    i.WriteHtonU32(mTTid);

    if (m_options.find(OPTION_CLIENTID) != m_options.end())
    {
        i.WriteHtonU16(m_clientIdentifier.GetOptionCode());
        i.WriteHtonU16(m_clientIdentifier.GetOptionLength());
        Duid duid = m_clientIdentifier.GetDuid();
        uint32_t size = duid.GetSerializedSize();
        duid.Serialize(i);
        i.Next(size);
    }
    if (m_options.find(OPTION_SERVERID) != m_options.end())
    {
        i.WriteHtonU16(m_serverIdentifier.GetOptionCode());
        i.WriteHtonU16(m_serverIdentifier.GetOptionLength());
        Duid duid = m_serverIdentifier.GetDuid();
        uint32_t size = duid.GetSerializedSize();
        duid.Serialize(i);
        i.Next(size);
    }
    if (m_options.find(OPTION_IA_NA) != m_options.end())
    {
        for (const auto& itr : m_ianaList)
        {
            i.WriteHtonU16(itr.GetOptionCode());
            i.WriteHtonU16(itr.GetOptionLength());
            i.WriteHtonU32(itr.GetIaid());
            i.WriteHtonU32(itr.GetT1());
            i.WriteHtonU32(itr.GetT2());

            std::vector<IaAddressOption> iaAddresses = itr.m_iaAddressOption;
            for (const auto& iaItr : iaAddresses)
            {
                i.WriteHtonU16(iaItr.GetOptionCode());
                i.WriteHtonU16(iaItr.GetOptionLength());

                Address addr = iaItr.GetIaAddress();
                uint8_t addrBuf[16];
                addr.CopyTo(addrBuf);
                i.Write(addrBuf, 16);
                i.WriteHtonU32(iaItr.GetPreferredLifetime());
                i.WriteHtonU32(iaItr.GetValidLifetime());
            }
        }
    }
    if (m_options.find(OPTION_ELAPSED_TIME) != m_options.end())
    {
        i.WriteHtonU16(m_elapsedTime.GetOptionCode());
        i.WriteHtonU16(m_elapsedTime.GetOptionLength());
        i.WriteHtonU16(m_elapsedTime.GetOptionValue());
    }
    if (m_options.find(OPTION_ORO) != m_options.end())
    {
        i.WriteHtonU16(m_optionRequest.GetOptionCode());
        i.WriteHtonU16(m_optionRequest.GetOptionLength());

        std::vector<uint16_t> requestedOptions = m_optionRequest.GetRequestedOptions();
        for (const auto& itr : requestedOptions)
        {
            i.WriteHtonU16(itr);
        }
    }
    if (m_options.find(OPTION_SOL_MAX_RT) != m_options.end())
    {
        i.WriteHtonU16(OPTION_SOL_MAX_RT);
        i.WriteHtonU16(4);
        i.WriteHtonU32(m_solMaxRt);
    }
    if (m_options.find(OPTION_STATUS_CODE) != m_options.end())
    {
        i.WriteHtonU16(OPTION_STATUS_CODE);
        i.WriteHtonU16(m_statusCode.GetOptionLength());
        i.WriteHtonU16(m_statusCode.GetStatusCode());

        // Considering a maximum message length of 128 bytes (arbitrary).
        uint8_t strBuf[128];
        m_statusCode.GetStatusMessage().copy((char*)strBuf,
                                             m_statusCode.GetStatusMessage().length());
        strBuf[m_statusCode.GetOptionLength() - 2] = '\0';

        i.Write(strBuf, m_statusCode.GetStatusMessage().length());
    }
}

uint32_t
Dhcp6Header::Deserialize(Buffer::Iterator start)
{
    uint32_t len;
    Buffer::Iterator i = start;
    uint32_t cLen = i.GetSize();

    uint32_t mTTid = i.ReadNtohU32();
    m_msgType = mTTid >> 24;
    m_transactId = mTTid & 0x00FFFFFF;

    len = 4;
    uint16_t option;
    bool loop = true;
    do
    {
        if (len + 2 <= cLen)
        {
            option = i.ReadNtohU16();
            len += 2;
        }
        else
        {
            m_len = len;
            return m_len;
        }
        switch (option)
        {
        case OPTION_CLIENTID:
            NS_LOG_INFO("Client Identifier Option");
            if (len + 2 <= cLen)
            {
                m_clientIdentifier.SetOptionCode(option);
                m_clientIdentifier.SetOptionLength(i.ReadNtohU16());
                len += 2;
            }
            if (len + m_clientIdentifier.GetOptionLength() <= cLen)
            {
                // Total length - DUID Type length(2) - Hardware Type length(2)
                uint32_t addrLen = m_clientIdentifier.GetOptionLength() - 4;

                // Read DUID.
                Duid duid;
                uint32_t read = duid.Deserialize(i);
                i.Next(read);
                uint32_t idLen = duid.DeserializeIdentifier(i, addrLen);
                i.Next(idLen);
                m_clientIdentifier.SetDuid(duid);
                len += m_clientIdentifier.GetOptionLength();
            }
            break;

        case OPTION_SERVERID:
            NS_LOG_INFO("Server ID Option");
            if (len + 2 <= cLen)
            {
                m_serverIdentifier.SetOptionCode(option);
                m_serverIdentifier.SetOptionLength(i.ReadNtohU16());
                len += 2;
            }
            if (len + m_clientIdentifier.GetOptionLength() <= cLen)
            {
                // Total length - DUID Type length(2) - Hardware Type length(2)
                uint32_t addrLen = m_serverIdentifier.GetOptionLength() - 4;

                // Read DUID.
                Duid duid;
                uint32_t read = duid.Deserialize(i);
                i.Next(read);
                uint32_t idLen = duid.DeserializeIdentifier(i, addrLen);
                i.Next(idLen);
                m_serverIdentifier.SetDuid(duid);
                len += m_serverIdentifier.GetOptionLength();
            }
            break;

        case OPTION_IA_NA: {
            NS_LOG_INFO("IANA Option");
            IaOptions iana;
            uint32_t iaAddrOptLen = 0;
            if (len + 2 <= cLen)
            {
                iana.SetOptionCode(option);
                iana.SetOptionLength(i.ReadNtohU16());
                iaAddrOptLen = iana.GetOptionLength();
                len += 2;
            }

            if (len + 12 <= cLen)
            {
                iana.SetIaid(i.ReadNtohU32());
                iana.SetT1(i.ReadNtohU32());
                iana.SetT2(i.ReadNtohU32());
                len += 12;
                iaAddrOptLen -= 12;
            }

            uint32_t readLen = 0;
            while (readLen < iaAddrOptLen)
            {
                IaAddressOption iaAddrOpt;
                iaAddrOpt.SetOptionCode(i.ReadNtohU16());
                iaAddrOpt.SetOptionLength(i.ReadNtohU16());

                uint8_t addrBuf[16];
                i.Read(addrBuf, 16);
                iaAddrOpt.SetIaAddress(Ipv6Address(addrBuf));

                iaAddrOpt.SetPreferredLifetime(i.ReadNtohU32());
                iaAddrOpt.SetValidLifetime(i.ReadNtohU32());

                iana.m_iaAddressOption.push_back(iaAddrOpt);
                len += 4 + iaAddrOpt.GetOptionLength();

                readLen += 4 + iaAddrOpt.GetOptionLength();
            }
            m_ianaList.push_back(iana);
            m_options[OPTION_IA_NA] = true;
            break;
        }

        case OPTION_ELAPSED_TIME:
            NS_LOG_INFO("Elapsed Time Option");
            if (len + 4 <= cLen)
            {
                m_elapsedTime.SetOptionCode(option);
                m_elapsedTime.SetOptionLength(i.ReadNtohU16());
                m_elapsedTime.SetOptionValue(i.ReadNtohU16());
                m_options[OPTION_ELAPSED_TIME] = true;
                len += 4;
            }
            else
            {
                NS_LOG_WARN("Malformed Packet");
                return 0;
            }
            break;

        case OPTION_ORO:
            NS_LOG_INFO("Option Request Option");
            if (len + 2 <= cLen)
            {
                m_optionRequest.SetOptionCode(option);
                m_optionRequest.SetOptionLength(i.ReadNtohU16());
                len += 2;
            }
            while (len + 2 <= cLen)
            {
                m_optionRequest.AddRequestedOption(i.ReadNtohU16());
                len += 2;
            }
            m_options[OPTION_ORO] = true;
            break;

        case OPTION_SOL_MAX_RT:
            NS_LOG_INFO("Solicit Max RT Option");
            if (len + 6 <= cLen)
            {
                i.ReadNtohU16();
                m_solMaxRt = i.ReadNtohU32();
                len += 6;
            }
            m_options[OPTION_SOL_MAX_RT] = true;
            break;

        case OPTION_STATUS_CODE:
            NS_LOG_INFO("Status Code Option");
            if (len + 2 <= cLen)
            {
                m_statusCode.SetOptionCode(option);
                m_statusCode.SetOptionLength(i.ReadNtohU16());
                len += 2;
            }
            if (len + 2 <= cLen)
            {
                m_statusCode.SetStatusCode(i.ReadNtohU16());
                len += 2;
            }

            if (len + (m_statusCode.GetOptionLength() - 2) <= cLen)
            {
                uint8_t msgLength = m_statusCode.GetOptionLength() - 2;
                uint8_t strBuf[128];
                i.Read(strBuf, msgLength);
                strBuf[msgLength] = '\0';

                std::string statusMsg((char*)strBuf);
                m_statusCode.SetStatusMessage(statusMsg);
                len += msgLength;
            }
            m_options[OPTION_STATUS_CODE] = true;
            break;

        default:
            NS_LOG_WARN("Unidentified Option " << option);
            NS_LOG_WARN("Malformed Packet");
            return 0;
        }
    } while (loop);

    m_len = len;
    return m_len;
}
} // namespace internetApplications
} // namespace ns3
