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
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * \brief Default constructor.
     */
    Dhcp6Header();

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

    /**
     * \brief Set the elapsed time option.
     * \param timestamp the time at which the client began the exchange.
     */
    void AddElapsedTime(uint16_t timestamp);

    /**
     * \brief Add the client identifier option.
     * \param hardwareType the hardware type of the DUID.
     * \param linkLayerAddress the link layer address of the node.
     */
    void AddClientIdentifier(uint16_t hardwareType, Address linkLayerAddress);

    /**
     * \brief Add the server identifier option.
     * \param hardwareType the hardware type of the DUID.
     * \param linkLayerAddress the link layer address of the node.
     */
    void AddServerIdentifier(uint16_t hardwareType, Address linkLayerAddress);

    /**
     * \brief Add IANA option.
     * \param iaid
     * \param t1
     * \param t2
     */
    void AddIanaOption(uint32_t iaid, uint32_t t1, uint32_t t2);

    /**
     * \brief Add IATA option.
     * \param iaid
     */
    void AddIataOption(uint32_t iaid);

    /**
     * \brief Add IA address option to the IANA or IATA.
     * \param iaid the unique identifier of the identity association.
     * \param address The IPv6 address to be offered.
     * \param prefLifetime the preferred lifetime in seconds.
     * \param validLifetime the valid lifetime in seconds.
     */
    void AddAddress(uint32_t iaid,
                    Ipv6Address address,
                    uint32_t prefLifetime,
                    uint32_t validLifetime);

  private:
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * \brief Update the message length.
     * \param len The length to be added to the total.
     */
    void AddMessageLength(uint32_t len);

    /**
     * \brief The message length;
     */
    uint32_t m_len;

    /**
     * \brief The message type.
     */
    uint32_t m_msgType : 8;

    /**
     * \brief The transaction ID calculated by the client or the server.
     * This is a 24-bit integer.
     */
    uint32_t m_transactId : 24;

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
    IntegerOptions<uint8_t> preference;

    /**
     * \brief The amount of time since the client began the transaction.
     */
    IntegerOptions<uint16_t> elapsedTime;

    /**
     * \brief List of IANA options.
     */
    std::list<IaOptions> m_ianaList;

    /**
     * \brief List of IATA options.
     */
    std::list<IaOptions> m_iataList;

    /**
     * \brief Add an identifier option to the header.
     * \param identifier the client or server identifier option object.
     * \param optionType identify whether to add a client or server identifier.
     * \param hardwareType the hardware type of the DUID.
     * \param linkLayerAddress the link layer address of the node.
     */
    void AddIdentifierOption(IdentifierOption& identifier,
                             uint16_t optionType,
                             uint16_t hardwareType,
                             Address linkLayerAddress);

    /**
     * \brief Add IANA or IATA option to the header.
     * \param optionType identify whether to add an IANA or IATA.
     * \param iaid
     * \param t1
     * \param t2
     */
    void AddIaOption(uint16_t optionType, uint32_t iaid, uint32_t t1 = 0, uint32_t t2 = 0);
};

} // namespace ns3

#endif
