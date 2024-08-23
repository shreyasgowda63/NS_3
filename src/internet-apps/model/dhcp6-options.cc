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

#include "dhcp6-options.h"

#include "dhcp6-duid.h"

#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/loopback-net-device.h"
#include "ns3/ptr.h"
#include "ns3/simulator.h"

namespace ns3
{
namespace internetApplications
{

NS_LOG_COMPONENT_DEFINE("Dhcp6Options");

Options::Options()
{
    m_optionCode = 0;
    m_optionLength = 0;
}

Options::Options(uint16_t code, uint16_t length)
{
    NS_LOG_FUNCTION(this << code << length);
    m_optionCode = code;
    m_optionLength = length;
}

uint16_t
Options::GetOptionCode() const
{
    return m_optionCode;
}

void
Options::SetOptionCode(uint16_t code)
{
    NS_LOG_FUNCTION(this << code);
    m_optionCode = code;
}

uint16_t
Options::GetOptionLength() const
{
    return m_optionLength;
}

void
Options::SetOptionLength(uint16_t length)
{
    NS_LOG_FUNCTION(this << length);
    m_optionLength = length;
}

IdentifierOption::IdentifierOption()
{
}

IdentifierOption::IdentifierOption(uint16_t hardwareType, Address linkLayerAddress, Time time)
{
    NS_LOG_FUNCTION(this << hardwareType << linkLayerAddress);
    if (time.IsZero())
    {
        m_duid.SetDuidType(3);
    }
    else
    {
        m_duid.SetDuidType(1);
    }

    m_duid.SetHardwareType(hardwareType);

    uint8_t buffer[16];
    linkLayerAddress.CopyTo(buffer);

    std::vector<uint8_t> identifier;
    std::copy(buffer, buffer + linkLayerAddress.GetLength(), identifier.begin());
    m_duid.SetDuid(identifier);
}

void
IdentifierOption::SetDuid(Duid duid)
{
    NS_LOG_FUNCTION(this);
    m_duid = duid;
}

Duid
IdentifierOption::GetDuid() const
{
    return m_duid;
}

StatusCodeOption::StatusCodeOption()
{
    m_statusCode = 0;
    m_statusMessage = "";
}

uint16_t
StatusCodeOption::GetStatusCode() const
{
    return m_statusCode;
}

void
StatusCodeOption::SetStatusCode(uint16_t statusCode)
{
    NS_LOG_FUNCTION(this << statusCode);
    m_statusCode = statusCode;
}

std::string
StatusCodeOption::GetStatusMessage() const
{
    return m_statusMessage;
}

void
StatusCodeOption::SetStatusMessage(std::string statusMessage)
{
    NS_LOG_FUNCTION(this);
    m_statusMessage = statusMessage;
}

IaAddressOption::IaAddressOption()
{
    m_iaAddress = Ipv6Address("::");
    m_preferredLifetime = 0;
    m_validLifetime = 0;
}

IaAddressOption::IaAddressOption(Ipv6Address iaAddress,
                                 uint32_t preferredLifetime,
                                 uint32_t validLifetime)
{
    m_iaAddress = iaAddress;
    m_preferredLifetime = preferredLifetime;
    m_validLifetime = validLifetime;
}

Ipv6Address
IaAddressOption::GetIaAddress() const
{
    return m_iaAddress;
}

void
IaAddressOption::SetIaAddress(Ipv6Address iaAddress)
{
    NS_LOG_FUNCTION(this << iaAddress);
    m_iaAddress = iaAddress;
}

uint32_t
IaAddressOption::GetPreferredLifetime() const
{
    return m_preferredLifetime;
}

void
IaAddressOption::SetPreferredLifetime(uint32_t preferredLifetime)
{
    NS_LOG_FUNCTION(this << preferredLifetime);
    m_preferredLifetime = preferredLifetime;
}

uint32_t
IaAddressOption::GetValidLifetime() const
{
    return m_validLifetime;
}

void
IaAddressOption::SetValidLifetime(uint32_t validLifetime)
{
    NS_LOG_FUNCTION(this << validLifetime);
    m_validLifetime = validLifetime;
}

IaOptions::IaOptions()
{
    m_iaid = 0;
    m_t1 = 0;
    m_t2 = 0;
}

uint32_t
IaOptions::GetIaid() const
{
    return m_iaid;
}

void
IaOptions::SetIaid(uint32_t iaid)
{
    NS_LOG_FUNCTION(this << iaid);
    m_iaid = iaid;
}

uint32_t
IaOptions::GetT1() const
{
    return m_t1;
}

void
IaOptions::SetT1(uint32_t t1)
{
    NS_LOG_FUNCTION(this << t1);
    m_t1 = t1;
}

uint32_t
IaOptions::GetT2() const
{
    return m_t2;
}

void
IaOptions::SetT2(uint32_t t2)
{
    NS_LOG_FUNCTION(this << t2);
    m_t2 = t2;
}

RequestOptions::RequestOptions()
{
    m_requestedOptions = std::vector<uint16_t>();
}

std::vector<uint16_t>
RequestOptions::GetRequestedOptions() const
{
    return m_requestedOptions;
}

void
RequestOptions::AddRequestedOption(uint16_t requestedOption)
{
    m_requestedOptions.push_back(requestedOption);
}

template <typename T>
IntegerOptions<T>::IntegerOptions()
{
    m_optionValue = 0;
}

template <typename T>
T
IntegerOptions<T>::GetOptionValue() const
{
    return m_optionValue;
}

template <typename T>
void
IntegerOptions<T>::SetOptionValue(T optionValue)
{
    NS_LOG_FUNCTION(this << optionValue);
    m_optionValue = optionValue;
}

ServerUnicastOption::ServerUnicastOption()
{
    m_serverAddress = Ipv6Address("::");
}

Ipv6Address
ServerUnicastOption::GetServerAddress()
{
    return m_serverAddress;
}

void
ServerUnicastOption::SetServerAddress(Ipv6Address serverAddress)
{
    NS_LOG_FUNCTION(this << serverAddress);
    m_serverAddress = serverAddress;
}

// Public template function declarations.

template class IntegerOptions<uint16_t>;
template class IntegerOptions<uint8_t>;

} // namespace internetApplications
} // namespace ns3
