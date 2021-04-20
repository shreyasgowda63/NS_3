/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Philip Hönnecke
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
 * Author: Philip Hönnecke <p.hoennecke@tu-braunschweig.de>
 */

#ifndef LR_WPAN_ROUTE_H
#define LR_WPAN_ROUTE_H

#include <ns3/callback.h>
#include <ns3/object.h>
#include <ns3/log.h>
#include <ns3/packet.h>

#include <ns3/mac48-address.h>

namespace ns3 {

class LrWpanNetDevice;

class LrWpanRoute : public Object
{
public:
  /**
   * \brief Set destination address.
   * \param dest Destination address
   */
  void SetDestination (Address dest);

  /**
   * \brief Get destination address.
   * \return destination address
   */
  Address GetDestination () const;

  /**
   * \brief Set source address.
   * \param src Source address
   */
  void SetSource (Address src);

  /**
   * \brief Get source address.
   * \return source address
   */
  Address GetSource () const;

  /**
   * \brief Set gateway address.
   * \param gw Gateway address
   */
  void SetGateway (Address gw);

  /**
   * \brief Get gateway address.
   * \return gateway address
   */
  Address GetGateway () const;

  /**
   * \brief Converts the input addr to either Mac16 or Mac64 address depending on the type.
   * Asserts if no compatible address was given.
   * 
   * \param addr The address to convert
   * \return Address Mac16Address or Mac64Address
   */
  static Address ConvertAddress (Address addr);

private:
  /**
   * \brief Destination address.
   */
  Address m_dest;

  /**
   * \brief source address.
   */
  Address m_source;

  /**
   * \brief Gateway address.
   */
  Address m_gateway;
};

} // namespace ns3

#endif