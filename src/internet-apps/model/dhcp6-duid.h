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
/**
 * \ingroup dhcp6
 *
 * \class Duid
 * \brief Implements the unique identifier for DHCPv6.
 */
class Duid
{
  public:
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
     * \brief Get the link-layer address.
     * \return the link layer address of the node.
     */
    Address GetDuid() const;

    /**
     * \brief Set the identifier as the DUID.
     * \param linkLayerAddress the link layer address of the node.
     * \param idLen the length of the identifier.
     */
    void SetDuid(uint8_t linkLayerAddress[16], uint8_t idLen);

    /**
     * \brief Get the time at which the DUID is generated.
     * \return the timestamp.
     */
    Time GetTime() const;

    /**
     * \brief Set the time at which DUID is generated.
     * \param time the timestamp.
     */
    void SetTime(Time time);

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
     * Identifier of the node in bytes. At most 128 bits.
     */
    uint8_t m_linkLayerAddress[16];

    /**
     * Length of the identifier.
     */
    uint8_t m_idLen;
};
} // namespace ns3

#endif
