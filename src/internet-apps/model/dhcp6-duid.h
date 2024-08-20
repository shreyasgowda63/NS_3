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

#ifndef DHCP6_DUID_H
#define DHCP6_DUID_H

#include "ns3/address.h"
#include "ns3/buffer.h"
#include "ns3/header.h"
#include "ns3/ipv6-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"

namespace ns3
{

namespace dhcp6
{
/**
 * \ingroup dhcp6
 *
 * \class Duid
 * \brief Implements the unique identifier for DHCPv6.
 */
class Duid : public Header
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
    Duid();

    /**
     * \brief Initialize the DUID for a client or server.
     * \param node The node for which the DUID is to be generated.
     */
    void Initialize(Ptr<Node> node);

    /**
     * \brief Check if the DUID is invalid.
     * \return true if the DUID is invalid.
     */
    bool IsInvalid() const;

    /**
     * \brief Get the DUID type
     * \return the DUID type.
     */
    uint16_t GetDuidType() const;

    /**
     * \brief Set the DUID type
     * \param duidType the DUID type.
     */
    void SetDuidType(uint16_t duidType);

    /**
     * \brief Get the hardware type.
     * \return the hardware type
     */
    uint16_t GetHardwareType() const;

    /**
     * \brief Set the hardware type.
     * \param hardwareType the hardware type.
     */
    void SetHardwareType(uint16_t hardwareType);

    /**
     * \brief Set the identifier as the DUID.
     * \param linkLayerAddress the link layer address of the node.
     */
    void SetDuid(std::vector<uint8_t> linkLayerAddress);

    /**
     * \brief Get the time at which the DUID is generated.
     * \return the timestamp.
     */
    Time GetTime() const;

    /**
     * \brief Get the length of the DUID.
     * \return the DUID length.
     */
    uint8_t GetLength() const;

    /**
     * \brief Set the time at which DUID is generated.
     * \param time the timestamp.
     */
    void SetTime(Time time);

    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * \brief Deserialize the identifier in the DUID.
     * \param start The buffer iterator.
     * \param len The number of bytes to be read.
     * \return The number of bytes read.
     */
    uint32_t DeserializeIdentifier(Buffer::Iterator start, uint32_t len);

    /**
     * \brief Copy the link layer address to a buffer.
     * \param buffer The buffer to which the link layer address is to be copied.
     * \return the updated buffer.
     */
    std::vector<uint8_t> CopyTo(std::vector<uint8_t> buffer) const;

    /**
     * \brief Comparison operator
     * \param duid header to compare
     * \return true if the headers are equal
     */
    bool operator==(const Duid& duid) const;

    /**
     * \brief Less than operator.
     *
     * \param a the first operand
     * \param b the first operand
     * \returns true if the operand a is less than operand b
     */
    friend bool operator<(const Duid& a, const Duid& b);

  private:
    /**
     * Type of the DUID.
     * Here, we implement only DUID type 3, based on the link-layer address.
     */
    uint16_t m_duidType;

    /**
     * Valid hardware type assigned by IANA.
     */
    uint16_t m_hardwareType;

    /**
     * Time at which DUID is generated. Used in DUID-LLT.
     */
    Time m_time;

    /**
     * Identifier of the node in bytes.
     */
    std::vector<uint8_t> m_linkLayerAddress;
};

/**
 * \brief Stream output operator
 * \param os output stream
 * \param duid The reference to the DUID object.
 * \return updated stream
 */
std::ostream& operator<<(std::ostream& os, const Duid& duid);

/**
 * Stream extraction operator
 * \param is input stream
 * \param duid The reference to the DUID object.
 * \return std::istream
 */
std::istream& operator>>(std::istream& is, Duid& duid);

/**
 * \ingroup dhcp6
 *
 * \brief Class providing an hash for DUIDs
 */
class DuidHash
{
  public:
    /**
     * \brief Returns the hash of a DUID.
     * \param x the DUID
     * \return the hash
     *
     * This method uses std::hash rather than class Hash
     * as speed is more important than cryptographic robustness.
     */
    size_t operator()(const Duid& x) const;
};
} // namespace dhcp6
} // namespace ns3

#endif
