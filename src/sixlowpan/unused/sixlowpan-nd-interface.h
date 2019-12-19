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
 */

#ifndef SIXLOWPAN_RADVD_INTERFACE_H
#define SIXLOWPAN_RADVD_INTERFACE_H

#include <list>
#include "ns3/simple-ref-count.h"
#include "ns3/ipv6-address.h"

namespace ns3
{

class SixLowPanNdDadEntry;
class SixLowPanNdContext;

/**
 * \ingroup sixlowpan-nd
 * \brief Interface configuration for 6LoWPAN ND.
 */
class SixLowPanNdInterface : public SimpleRefCount<SixLowPanNdInterface>
{
public:
  /// Container: Ptr to SixLowPanNdContext
  typedef std::list<Ptr<SixLowPanNdContext> > SixLowPanNdContextList;
  /// Container Iterator: Ptr to SixLowPanNdContext
  typedef std::list<Ptr<SixLowPanNdContext> >::iterator SixLowPanNdContextListI;
  /// Container Const Iterator: Ptr to SixLowPanNdContext
  typedef std::list<Ptr<SixLowPanNdContext> >::const_iterator SixLowPanNdContextListCI;

  ///Container: Ptr to SixLowPanRadvdDadEntry
  typedef std::list<Ptr<SixLowPanNdDadEntry> > DadTable;
  /// Container Iterator: Ptr to SixLowPanRadvdDadEntry
  typedef std::list<Ptr<SixLowPanNdDadEntry> >::iterator DadTableI;
  /// Container Const Iterator: Ptr to SixLowPanRadvdDadEntry
  typedef std::list<Ptr<SixLowPanNdDadEntry> >::const_iterator DadTableCI;

  /**
   * \brief Constructor.
   * \param interface interface index
   */
  SixLowPanNdInterface (uint32_t interface);

  /**
   * \brief Destructor.
   */
  ~SixLowPanNdInterface ();

  /**
   * \brief Get interface index for this configuration.
   * \return interface index
   */
  uint32_t GetInterface () const;

  /**
   * \brief Get reachable time.
   * \return reachable time
   */
  uint32_t GetReachableTime () const;

  /**
   * \brief Set reachable time.
   * \param reachableTime reachable time
   */
  void SetReachableTime (uint32_t reachableTime);

  /**
   * \brief Get default lifetime.
   * \return default lifetime
   */
  uint32_t GetDefaultLifeTime () const;

  /**
   * \brief Set default lifetime.
   * \param defaultLifeTime default lifetime
   */
  void SetDefaultLifeTime (uint32_t defaultLifeTime);

  /**
   * \brief Get PIO network prefix.
   * \return PIO network prefix
   */
  Ipv6Address GetPioNetwork () const;

  /**
   * \brief Set PIOnetwork prefix.
   * \param network PIO network prefix
   */
  void SetPioNetwork (Ipv6Address network);

  /**
   * \brief Get PIO prefix length.
   * \return PIO prefix length
   */
  uint8_t GetPioPrefixLength () const;

  /**
   * \brief Set PIO prefix length.
   * \param prefixLength PIO prefix length
   */
  void SetPioPrefixLength (uint8_t prefixLength);

  /**
   * \brief Get PIO preferred lifetime.
   * \return PIO lifetime
   */
  uint32_t GetPioPreferredLifeTime () const;

  /**
   * \brief Set PIO preferred lifetime.
   * \param preferredLifeTime PIO lifetime
   */
  void SetPioPreferredLifeTime (uint32_t preferredLifeTime);

  /**
   * \brief Get PIO valid lifetime.
   * \return PIO lifetime
   */
  uint32_t GetPioValidLifeTime () const;

  /**
   * \brief Set PIO valid lifetime.
   * \param validLifeTime PIO lifetime
   */
  void SetPioValidLifeTime (uint32_t validLifeTime);

  /**
   * \brief Get list of 6LoWPAN contexts advertised for this interface.
   * \return list of 6LoWPAN contexts
   */
  SixLowPanNdContextList GetContexts () const;

  /**
   * \brief Add a 6LoWPAN context to advertise on interface.
   * \param routerContext 6LoWPAN context to advertise
   */
  void AddContext (Ptr<SixLowPanNdContext> routerContext);
  
  /**
   * \brief Get the DAD Table for this interface.
   * \return DAD Table
   */
  DadTable GetDadTable () const;

  /**
   * \brief Add an entry to DAD Table on interface.
   * \param entry entry to add
   */
  void AddDadEntry (Ptr<SixLowPanNdDadEntry> entry);

  /**
   * \brief Get version value (ABRO).
   * \return the version value
   */
  uint32_t GetAbroVersion () const;

  /**
   * \brief Set version value (ABRO).
   * \param version the version value
   */
  void SetAbroVersion (uint32_t version);

  /**
   * \brief Get valid lifetime value (ABRO).
   * \return the valid lifetime (units of 60 seconds)
   */
  uint16_t GetAbroValidLifeTime () const;

  /**
   * \brief Set valid lifetime value (ABRO).
   * \param time the valid lifetime (units of 60 seconds)
   */
  void SetAbroValidLifeTime (uint16_t time);

private:

  uint32_t m_interface; //!< Interface to advertise RA.

  uint32_t m_defaultLifeTime; //!< Default life time in seconds.
  uint32_t m_reachableTime; //!< Reachable time in milliseconds.

  Ipv6Address m_pioNetwork; //!< PIO network.
  uint32_t m_pioPreferredLifeTime; //!< PIO preferred time.
  uint32_t m_pioValidLifeTime; //!< PIO valid time.


  SixLowPanNdContextList m_contexts; //!< List of 6LoWPAN contexts to advertise.
  DadTable m_dadTable;  //!< A list of DAD entry (IPv6 Address, EUI-64, Lifetime).
  uint32_t m_abroVersion; //!< Version value for ABRO.
  uint16_t m_abroValidLifeTime; //!< Valid lifetime value for ABRO (units of 60 seconds).
};

} /* namespace ns3 */

#endif /* SIXLOWPAN_RADVD_INTERFACE_H */
