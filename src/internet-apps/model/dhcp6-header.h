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

#ifndef DHCP6_HEADER_H
#define DHCP6_HEADER_H

#include "dhcp6-duid.h"
#include "dhcp6-options.h"

#include "ns3/address.h"
#include "ns3/buffer.h"
#include "ns3/header.h"
#include "ns3/ipv6-address.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"

namespace ns3
{
namespace internetapps
{

/**
 * @ingroup dhcp6
 *
 * @class Dhcp6Header
 * @brief Implements the DHCPv6 header.
 */
class Dhcp6Header : public Header
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Default constructor.
     */
    Dhcp6Header();

    /**
     * @brief Get the type of message.
     * @return integer corresponding to the message type.
     */
    uint8_t GetMessageType() const;

    /**
     * @brief Set the message type.
     * @param msgType integer corresponding to the message type.
     */
    void SetMessageType(uint8_t msgType);

    /**
     * @brief Get the transaction ID.
     * @return the 32-bit transaction ID
     */
    uint32_t GetTransactId() const;

    /**
     * @brief Set the transaction ID.
     * @param transactId A 32-bit transaction ID.
     */
    void SetTransactId(uint32_t transactId);

    /**
     * @brief Reset all options.
     */
    void ResetOptions();

    /**
     * Enum to identify the message type.
     * RELAY_FORW, RELAY_REPL message types are not currently implemented.
     */
    enum MessageType
    {
        SOLICIT = 1,
        ADVERTISE = 2,
        REQUEST = 3,
        CONFIRM = 4,
        RENEW = 5,
        REBIND = 6,
        REPLY = 7,
        RELEASE = 8,
        DECLINE = 9,
        RECONFIGURE = 10,
        INFORMATION_REQUEST = 11,
        RELAY_FORW = 12,
        RELAY_REPL = 13
    };

    /// Enum to identify the option type.
    enum OptionType
    {
        OPTION_CLIENTID = 1,
        OPTION_SERVERID = 2,
        OPTION_IA_NA = 3,
        OPTION_IA_TA = 4,
        OPTION_IAADDR = 5,
        OPTION_ORO = 6,
        OPTION_PREFERENCE = 7,
        OPTION_ELAPSED_TIME = 8,
        OPTION_RELAY_MSG = 9,
        OPTION_AUTH = 11,
        OPTION_UNICAST = 12,
        OPTION_STATUS_CODE = 13,
        OPTION_RAPID_COMMIT = 14,
        OPTION_USER_CLASS = 15,
        OPTION_VENDOR_CLASS = 16,
        OPTION_VENDOR_OPTS = 17,
        OPTION_INTERFACE_ID = 18,
        OPTION_RECONF_MSG = 19,
        OPTION_RECONF_ACCEPT = 20,
        OPTION_IA_PD = 25,
        OPTION_IAPREFIX = 26,
        OPTION_INFORMATION_REFRESH_TIME = 32,
        OPTION_SOL_MAX_RT = 82,
        OPTION_INF_MAX_RT = 83,
    };

    /// Enum to identify the status code of the operation.
    enum StatusCodeValues
    {
        Success = 0,
        UnspecFail = 1,
        NoAddrsAvail = 2,
        NoBinding = 3,
        NotOnLink = 4,
        UseMulticast = 5,
        NoPrefixAvail = 6,
    };

    /**
     * @brief Get the client identifier.
     * @return the client identifier option.
     */
    IdentifierOption GetClientIdentifier();

    /**
     * @brief Get the server identifier.
     * @return the server identifier option.
     */
    IdentifierOption GetServerIdentifier();

    /**
     * @brief Get the list of IA_NA options.
     * @return the list of IA_NA options.
     */
    std::vector<IaOptions> GetIanaOptions();

    /**
     * @brief Get the status code of the operation.
     * @return the status code option.
     */
    StatusCodeOption GetStatusCodeOption();

    /**
     * @brief Set the elapsed time option.
     * @param timestamp the time at which the client began the exchange.
     */
    void AddElapsedTime(uint16_t timestamp);

    /**
     * @brief Add the client identifier option.
     * @param duid The DUID which identifies the client.
     */
    void AddClientIdentifier(Duid duid);

    /**
     * @brief Add the server identifier option.
     * @param duid The DUID which identifies the server.
     */
    void AddServerIdentifier(Duid duid);

    /**
     * @brief Request additional options.
     * @param optionType the option to be requested.
     */
    void AddOptionRequest(uint16_t optionType);

    /**
     * @brief Add the status code option.
     * @param statusCode the status code of the operation.
     * @param statusMsg the status message.
     */
    void AddStatusCode(uint16_t statusCode, std::string statusMsg);

    /**
     * @brief Add IANA option.
     * @param iaid
     * @param t1
     * @param t2
     */
    void AddIanaOption(uint32_t iaid, uint32_t t1, uint32_t t2);

    /**
     * @brief Add IATA option.
     * @param iaid
     */
    void AddIataOption(uint32_t iaid);

    /**
     * @brief Add IA address option to the IANA or IATA.
     * @param iaid the unique identifier of the identity association.
     * @param address The IPv6 address to be offered.
     * @param prefLifetime the preferred lifetime in seconds.
     * @param validLifetime the valid lifetime in seconds.
     */
    void AddAddress(uint32_t iaid,
                    Ipv6Address address,
                    uint32_t prefLifetime,
                    uint32_t validLifetime);

    /**
     * @brief Get the option request option.
     * @return the option request option.
     */
    RequestOptions GetOptionRequest();

    /**
     * @brief Handle all options requested by client.
     * @param requestedOptions the options requested by the client.
     */
    void HandleOptionRequest(std::vector<uint16_t> requestedOptions);

    /**
     * @brief Add the SOL_MAX_RT option.
     */
    void AddSolMaxRt();

    /**
     * @brief Get list of all options set in the header.
     * @return the list of options.
     */
    std::map<OptionType, bool> GetOptionList();

    /**
     * @brief The port number of the DHCPv6 client.
     */
    static const uint16_t CLIENT_PORT = 546;

    /**
     * @brief The port number of the DHCPv6 server.
     */
    static const uint16_t SERVER_PORT = 547;

  private:
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * @brief Update the message length.
     * @param len The length to be added to the total.
     */
    void AddMessageLength(uint32_t len);

    /**
     * @brief Add an identifier option to the header.
     * @param identifier the client or server identifier option object.
     * @param optionType identify whether to add a client or server identifier.
     * @param duid The unique identifier for the client or server.
     */
    void AddIdentifierOption(IdentifierOption& identifier, OptionType optionType, Duid duid);

    /**
     * @brief Add IANA or IATA option to the header.
     * @param optionType identify whether to add an IANA or IATA.
     * @param iaid
     * @param t1
     * @param t2
     */
    void AddIaOption(OptionType optionType, uint32_t iaid, uint32_t t1 = 0, uint32_t t2 = 0);

    uint32_t m_len;                      //!< The length of the message.
    uint32_t m_msgType : 8;              //!< The message type.
    IdentifierOption m_clientIdentifier; //!< The client identifier option.
    IdentifierOption m_serverIdentifier; //!< The server identifier option.
    std::vector<IaOptions> m_ianaList;   //!< Vector of IA_NA options.
    std::vector<IaOptions> m_iataList;   //!< Vector of IA_TA options.
    uint32_t m_solMaxRt;                 //!< Default value for SOL_MAX_RT option.

    /**
     * The transaction ID calculated by the client or the server.
     * This is a 24-bit integer.
     */
    uint32_t m_transactId : 24;

    /**
     * Options present in the header, indexed by option code.
     * TODO: Use std::set instead.
     */
    std::map<OptionType, bool> m_options;

    /// (optional) The status code of the operation just performed.
    StatusCodeOption m_statusCode;

    /// List of additional options requested.
    RequestOptions m_optionRequest;

    /// The preference value for the server.
    PreferenceOption m_preference;

    /// The amount of time since the client began the transaction.
    ElapsedTimeOption m_elapsedTime;
};
} // namespace internetapps
} // namespace ns3

#endif
