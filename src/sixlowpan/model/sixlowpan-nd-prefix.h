/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Università di Firenze, Italy
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
 * Author: Alessio Bonadio <alessio.bonadio@gmail.com>
 *         Tommaso Pecorella <tommaso.pecorella@unifi.it>
 *         Adnan Rashid <adnanrashidpk@gmail.com>
 */

#ifndef SIXLOW_ND_PREFIX_H
#define SIXLOW_ND_PREFIX_H

#include <stdint.h>

#include "ns3/nstime.h"
#include "ns3/ipv6-address.h"
#include "ns3/simple-ref-count.h"
#include "ns3/output-stream-wrapper.h"

namespace ns3
{

/**
 * \ingroup sixlowpan
 * \brief Router prefix container for 6LoWPAN ND.
 */
class SixLowPanNdPrefix : public SimpleRefCount<SixLowPanNdPrefix>
{
public:
  /**
   * \brief Constructor.
   */
  SixLowPanNdPrefix ();

  /**
   * \brief Constructor.
   * \param prefix network prefix advertised
   * \param prefixLen prefix length ( 0 < x <= 128)
   * \param prefTime preferred life time (default 7 days)
   * \param validTime valid life time (default 30 days)
   */
  SixLowPanNdPrefix (Ipv6Address prefix, uint8_t prefixLen, Time prefTime, Time validTime);

  /**
   * \brief Destructor.
   */
  ~SixLowPanNdPrefix ();

  /**
   * \brief Get network prefix.
   * \return network prefix
   */
  Ipv6Address GetPrefix () const;

  /**
   * \brief Set network prefix.
   * \param prefix network prefix
   */
  void SetPrefix (Ipv6Address prefix);

  /**
   * \brief Get prefix length.
   * \return prefix length
   */
  uint8_t GetPrefixLength () const;

  /**
   * \brief Set prefix length.
   * \param prefixLen prefix length
   */
  void SetPrefixLength (uint8_t prefixLen);

  /**
   * \brief Get preferred lifetime.
   * \return lifetime
   */
  Time GetPreferredLifeTime () const;

  /**
   * \brief Set preferred lifetime.
   * \param prefTime lifetime
   */
  void SetPreferredLifeTime (Time prefTime);

  /**
   * \brief Get valid lifetime.
   * \return lifetime
   */
  Time GetValidLifeTime () const;

  /**
   * \brief Set valid lifetime.
   * \param validTime lifetime
   */
  void SetValidLifeTime (Time validTime);

  /**
   * \brief Print the prefix
   * \param stream the ostream the prefix is printed to
   */
  void PrintPrefix (Ptr<OutputStreamWrapper> stream);

private:
  /**
   * \brief Network prefix.
   */
  Ipv6Address m_prefix;

  /**
   * \brief Prefix length.
   */
  uint8_t m_prefixLength;

  /**
   * \brief Preferred time.
   */
  Time m_preferredLifeTime;

  /**
   * \brief Valid time.
   */
  Time m_validLifeTime;
};

} /* namespace ns3 */

#endif /* SIXLOW_ND_PREFIX_H */
