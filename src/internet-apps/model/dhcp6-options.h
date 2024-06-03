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

#ifndef DHCP6_OPTIONS_H
#define DHCP6_OPTIONS_H

#include "ns3/address.h"
#include "ns3/buffer.h"
#include "ns3/header.h"
#include "ns3/ipv6-address.h"
#include "ns3/random-variable-stream.h"

namespace ns3
{

/**
 * \ingroup dhcp6
 *
 * \class Options
 * \brief Implements the functionality of DHCPv6 options
 */
class Options
{
  public:
    /**
     * \brief Default constructor.
     */
    Options();

    /**
     * \brief Constructor.
     * \param code The option code.
     * \param length The option length.
     */
    Options(uint16_t code, uint16_t length);

    /**
     * \brief Get the option code.
     * \return option code
     */
    uint16_t GetOptionCode() const;

    /**
     * \brief Set the option code.
     * \param code The option code to be added.
     */
    void SetOptionCode(uint16_t code);

    /**
     * \brief Get the option length.
     * \return option length
     */
    uint16_t GetOptionLength() const;

    /**
     * \brief Set the option length.
     * \param length The option length to be parsed.
     */
    void SetOptionLength(uint16_t length);

  private:
    /**
     * \brief Code associated with the included option.
     */
    uint16_t m_optionCode;

    /**
     * \brief Length of the included option.
     */
    uint16_t m_optionLength;
};

/**
 * \ingroup internet-apps
 * \defgroup dhcp6 DHCPv6 Header options
 */

/**
 * \ingroup dhcp6
 *
 * \class IdentifierOption
 * \brief Implements the client and server identifier options.
 */
class IdentifierOption : public Options
{
  public:
    /**
     * \brief Default constructor.
     */
    IdentifierOption();

    /**
     * \brief Constructor.
     * \param hardwareType The hardware type.
     * \param linkLayerAddress The link-layer address.
     */
    IdentifierOption(uint16_t hardwareType, Address linkLayerAddress);

    /**
     * \brief Get the hardware type.
     * \return the hardware type
     */
    uint16_t GetHardwareType();

    /**
     * \brief Set the hardware type.
     * \param hardwareType the hardware type.
     */
    void SetHardwareType(uint16_t hardwareType);

    /**
     * \brief Get the link-layer address.
     * \return the link layer address of the node.
     */
    Address GetLinkLayerAddress();

    /**
     * \brief Set the link-layer address.
     * \param linkLayerAddress the link layer address of the node.
     */
    void SetLinkLayerAddress(Address linkLayerAddress);

  private:
    /**
     * \brief Type of the DUID.
     * Here, we implement only DUID type 2, based on the link-layer address.
     */
    uint16_t m_duidType;

    /**
     * \brief Valid hardware type assigned by IANA.
     */
    uint16_t m_hardwareType;

    /**
     * \brief Link-layer address of the node.
     */
    Address m_linkLayerAddress;
};

/**
 * \ingroup dhcp6
 *
 * \class StatusCodeOption
 * \brief Implements the Status Code option.
 */
class StatusCodeOption : public Options
{
  public:
    /**
     * \brief Default constructor.
     */
    StatusCodeOption();

    /**
     * \brief Constructor.
     * \param statusCode The status code of the operation.
     * \param statusMessage The status message of the operation.
     */
    StatusCodeOption(uint16_t statusCode, std::string statusMessage);

    /**
     * \brief Get the status code of the operation.
     * \return the status code.
     */
    uint16_t GetStatusCode();

    /**
     * \brief Set the status code of the operation.
     * \param statusCode the status code of the performed operation.
     */
    void SetStatusCode(uint16_t statusCode);

    /**
     * \brief Get the status message of the operation.
     * \return the status message
     */
    std::string GetStatusMessage();

    /**
     * \brief Set the status message of the operation.
     * \param statusMessage the status message of the operation.
     */
    void SetStatusMessage(std::string statusMessage);

  private:
    /**
     * \brief The status code of an operation involving the IANA, IATA or
     * IA address.
     */
    uint16_t m_statusCode;

    /**
     * \brief The status message of the operation. This is to be UTF-8 encoded
     * as per RFC 3629.
     */
    std::string m_statusMessage;
};

/**
 * \ingroup dhcp6
 *
 * \class IaAddressOption
 * \brief Implements the IA Address options.
 */
class IaAddressOption : public Options
{
  public:
    /**
     * \brief Default constructor.
     */
    IaAddressOption();

    /**
     * \brief Constructor.
     * \param iaAddress The IA Address.
     * \param preferredLifetime The preferred lifetime of the address.
     * \param validLifetime The valid lifetime of the address.
     */
    IaAddressOption(Ipv6Address iaAddress, uint32_t preferredLifetime, uint32_t validLifetime);

    /**
     * \brief Get the IA Address.
     * \return the IPv6 address of the Identity Association
     */
    Ipv6Address GetIaAddress();

    /**
     * \brief Set the IA Address.
     * \param iaAddress the IPv6 address of this Identity Association.
     */
    void SetIaAddress(Ipv6Address iaAddress);

    /**
     * \brief Get the preferred lifetime.
     * \return the preferred lifetime
     */
    uint32_t GetPreferredLifetime();

    /**
     * \brief Set the preferred lifetime.
     * \param preferredLifetime the preferred lifetime for this address.
     */
    void SetPreferredLifetime(uint32_t preferredLifetime);

    /**
     * \brief Get the valid lifetime.
     * \return the lifetime for which the address is valid.
     */
    uint32_t GetValidLifetime();

    /**
     * \brief Set the valid lifetime.
     * \param validLifetime the lifetime for which the address is valid.
     */
    void SetValidLifetime(uint32_t validLifetime);

  private:
    /**
     * \brief The IPv6 address offered to the client.
     */
    Ipv6Address m_iaAddress;

    /**
     * \brief The preferred lifetime of the address, in seconds.
     */
    uint32_t m_preferredLifetime;

    /**
     * \brief The valid lifetime of the address, in seconds.
     */
    uint32_t m_validLifetime;

    /**
     * \brief (optional) The status code of any operation involving this address
     */
    StatusCodeOption m_statusCodeOption;
};

/**
 * \ingroup dhcp6
 *
 * \class IaOptions
 * \brief Implements the IANA and IATA options.
 */
class IaOptions : public Options
{
  public:
    /**
     * \brief Default constructor.
     */
    IaOptions();

    /**
     * \brief Get the unique identifier for the given IANA or IATA.
     * \return the ID of the IANA or IATA
     */
    uint32_t GetIaid();

    /**
     * \brief Set the unique identifier for the given IANA or IATA.
     * \param iaid the unique ID for the IANA or IATA.
     */
    void SetIaid(uint32_t iaid);

    /**
     * \brief Get the time interval in seconds after which the client contacts
     * the server which provided the address to extend the lifetime.
     * \return the time interval T1
     */
    uint32_t GetT1();

    /**
     * \brief Set the time interval in seconds after which the client contacts
     * the server which provided the address to extend the lifetime.
     * \param t1 the time interval in seconds.
     */
    void SetT1(uint32_t t1);

    /**
     * \brief Get the time interval in seconds after which the client contacts
     * any available server to extend the address lifetime.
     * \return the time interval T2
     */
    uint32_t GetT2();

    /**
     * \brief Set the time interval in seconds after which the client contacts
     * any available server to extend the address lifetime.
     * \param t2 time interval in seconds.
     */
    void SetT2(uint32_t t2);

    /**
     * \brief The list of IA Address options associated with the IANA.
     */
    std::list<IaAddressOption> m_iaAddressOption;

  private:
    /**
     * \brief The unique identifier for the given IANA or IATA.
     */
    uint32_t m_iaid;

    /**
     * \brief The time interval in seconds after which the client contacts the
     * server which provided the address to extend the lifetime.
     */
    uint32_t m_t1;

    /**
     * \brief The time interval in seconds after which the client contacts any
     * available server to extend the address lifetime.
     */
    uint32_t m_t2;

    /**
     * \brief (optional) The status code of any operation involving the IANA.
     */
    StatusCodeOption m_statusCodeOption;
};

/**
 * \ingroup dhcp6
 *
 * \class RequestOptions
 * \brief Implements the Option Request option.
 */
class RequestOptions : public Options
{
  public:
    /**
     * \brief Constructor.
     */
    RequestOptions();

    /**
     * \brief Get the option values
     * \return requested option list.
     */
    std::list<uint16_t> GetRequestedOptions();

    /**
     * \brief Set the option values.
     * \param requestedOptions option list.
     */
    void SetRequestedOptions(std::list<uint16_t> requestedOptions);

  private:
    /**
     * \brief List of requested options.
     */
    std::list<uint16_t> m_requestedOptions;
};

/**
 * \ingroup dhcp6
 *
 * \class IntegerOptions
 * \brief Implements the Preference and Elapsed Time options.
 */
template <typename T>
class IntegerOptions : public Options
{
  public:
    /**
     * \brief Constructor.
     */
    IntegerOptions();

    /**
     * \brief Get the option value
     * \return elapsed time, preference or option list.
     */
    T GetOptionValue() const;

    /**
     * \brief Set the option value.
     * \param optionValue elapsed time, preference or option list.
     */
    void SetOptionValue(T optionValue);

  private:
    /**
     * \brief Value that indicates the elapsed time, preference value or option
     * list.
     */
    T m_optionValue;
};

/**
 * \ingroup dhcp6
 *
 * \class ServerUnicastOption
 * \brief Implements the Server Unicast option.
 */
class ServerUnicastOption : public Options
{
  public:
    ServerUnicastOption();

    /**
     * \brief Get the server address.
     * \return The 128 bit server address.
     */
    Ipv6Address GetServerAddress();

    /**
     * \brief Set the server address.
     * \param serverAddress the 128-bit server address.
     */
    void SetServerAddress(Ipv6Address serverAddress);

  private:
    /**
     * \brief The 128-bit server address to which the client should send
     * unicast messages.
     */
    Ipv6Address m_serverAddress;
};

} // namespace ns3

#endif
