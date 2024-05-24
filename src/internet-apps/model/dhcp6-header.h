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

#ifndef DHCP6_HEADER_H
#define DHCP6_HEADER_H

#include "dhcp6-options.h"

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
 * \class Dhcp6Header
 * \brief Implements the DHCPv6 header.
 */
class Dhcp6Header : public Header
{
  public:
    /**
     * \brief Constructor.
     * \param msgType The message type.
     * \param transactId The transaction ID.
     */
    Dhcp6Header(uint8_t msgType, uint32_t transactId);

    /**
     * \brief Get the type of message.
     * \return integer corresponding to the message type.
     */
    uint8_t GetMessageType();

    /**
     * \brief Set the message type.
     * \param msgType integer corresponding to the message type.
     */
    void SetMessageType(uint8_t msgType);

    /**
     * \brief Get the transaction ID.
     * \return the 32-bit transaction ID
     */
    uint32_t GetTransactId();

    /**
     * \brief Set the transaction ID.
     * \param transactId A 32-bit transaction ID.
     */
    void SetTransactId(uint32_t transactId);

    /**
     * \brief Reset all options.
     */
    void ResetOptions();

    /**
     * \brief Enum to identify the message type.
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

    /**
     * \brief Enum to identify the option type.
     */
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

  private:
    /**
     * \brief The message type.
     */
    uint8_t m_msgType;

    /**
     * \brief The transaction ID calculated by the client or the server.
     */
    uint32_t m_transactId;

    /**
     * \brief Options present in the header, indexed by option code.
     */
    bool m_options[65536];

    /**
     * \brief The client identifier option.
     */
    IdentifierOption clientIdentifier;

    /**
     * \brief The server identifier option.
     */
    IdentifierOption serverIdentifier;

    /**
     * \brief (optional) The status code of the operation just performed.
     */
    StatusCodeOption statusCode;

    /**
     * \brief List of additional options requested.
     */
    RequestOptions optionRequest;

    /**
     * \brief The preference value for the server.
     */
    IntegerOptions<uint16_t> preference;

    /**
     * \brief The amount of time since the client began the transaction.
     */
    IntegerOptions<uint8_t> elapsedTime;
};

} // namespace ns3

#endif
