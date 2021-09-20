/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 Jadavpur University, India
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
 * Author: Manoj Kumar Rana <manoj24.rana@gmail.com>
 */

#ifndef B_CACHE_H
#define B_CACHE_H



#include <list>
#include "ns3/nstime.h"
#include "ns3/node.h"
#include "ns3/ipv6-address.h"
#include "ns3/ptr.h"
#include "ns3/timer.h"
#include <unordered_map>

namespace ns3 {

/**
 * \class BCache
 * \brief BCache class is associated with Mipv6Ha class. It contain data
 * members: CoA, HoA, lifetime, HA address, tunnel interface index, sequence
 * number and BU state as defined in RFC 6275. To handle the
 * information of multiple MNs and HAs, each entry in the BCache is keyed by
 * the HoA.
*/
class BCache : public Object
{
public:
  class Entry;

  /**
   * \brief Get the type identifier.
   * \return type identifier
   */
  static TypeId GetTypeId ();

  /**
   * \brief constructor
   */
  BCache ();

  /**
   * \brief destructor
   */
  ~BCache ();

  /**
   * \brief Lookup entry for an MN.
   * \param mnhoa MN's home address
   * \returns the entry of an MN
   */
  BCache::Entry * Lookup (Ipv6Address mnhoa);

  /**
   * \brief Add an entry for an MN.
   * \param bce binding cache entry
   */
  void Add (BCache::Entry *bce);

  /**
   * \brief remove the entry of an MN.
   * \param entry MN's entry
   */
  void Remove (BCache::Entry *entry);

  /**
   * \brief Lookup whether the HoA cached in it.
   * \param shoa solicited home address
   * \returns status
   */
  BCache::Entry * LookupSHoa (Ipv6Address shoa);

  /**
   * \brief delete all entries in the cache
   */
  void Flush ();

  /**
   * \brief get node.
   * \returns the node pointer
   */
  Ptr<Node> GetNode () const;

  /**
   * \brief set node.
   * \param node the node pointer
   */
  void SetNode (Ptr<Node> node);

  /**
   * \brief stores the home prefixes. It makes zero last 64 bit if HA addresses
   * \param halist the list of Ipv6 addresses of all HA's links
   */
  void SetHomePrefixes (std::list<Ipv6Address> halist);


  /**
   * Entry for an MN
   */
  class Entry
  {
  public:
    /**
     * \enum State_e
     * \brief state of the address
     */
    enum State_e
    {
      TENTATIVE, /**< Address is tentative, no packet can be sent unless DAD finished */
      DEPRECATED, /**< Address is deprecated and should not be used */
      PREFERRED, /**< Preferred address */
      PERMANENT, /**< Permanent address */
      HOMEADDRESS, /**< Address is a HomeAddress */
      TENTATIVE_OPTIMISTIC, /**< Address is tentative but we are optimistic so we can send packet even if DAD is not yet finished */
      INVALID, /**< Invalid state (after a DAD failed) */
    };
    /**
     * \brief constructor
     * \param bcache binding cache
     */
    Entry (Ptr<BCache> bcache);

    /**
     * \brief copy of an entry object
     * \returns an entry object
     */
    Entry * Copy ();


    /**
     * \brief whether MN is unreachable
     * \returns status
     */
    bool IsUnreachable () const;

    /**
     * \brief whether MN is reachable
     * \returns status
     */
    bool IsReachable () const;

    /**
     * \brief mark MN's reachability status as reachable
     */
    void MarkReachable ();

    /**
     * \brief check against MN's HoA
     * \param mnhoa MN HoA
     * \returns whether MN's hoa is matched
     */
    bool Match (Ipv6Address mnhoa) const;

    /**
     * \brief get the CoA of MN
     * \returns care-of-address of MN
     */
    Ipv6Address GetCoa () const;

    /**
     * \brief set CoA of MN
     * \param mnhoa MN HoA
     */
    void SetCoa (Ipv6Address mnhoa);

    /**
     * \brief get the HA address of MN
     * \returns status
     */
    Ipv6Address GetHA () const;

    /**
     * \brief Set the HA address of MN
     * \param haa HA address
     */
    void SetHA (Ipv6Address haa);

    /**
     * \brief get solicited HoA of MN
     * \returns solicited HoA
     */
    Ipv6Address GetSolicitedHoA () const;

    /**
     * \brief set solicited HoA of MN
     * \param shoa solicited HoA
     */
    void SetSolicitedHoA (Ipv6Address shoa);

    /**
     * \brief get HoA of the MN
     * \return HoA
     */
    Ipv6Address GetHoa () const;

    /**
     * \brief set set HoA of MN
     * \param hoa home address
     */
    void SetHoa (Ipv6Address hoa);

    /**
     * \brief get tunnel interface index of an MN
     * \return interface index
     */
    int16_t GetTunnelIfIndex () const;

    /**
     * \brief set tunnel interface index
     * \param tunnelif tunnel interface index
     */
    void SetTunnelIfIndex (int16_t tunnelif);

    /**
     * \brief get last binding update time of MN
     * \returns bu time
     */
    Time GetLastBindingUpdateTime () const;

    /**
     * \brief set last binding update time of MN
     * \param tm last bu time
     */
    void SetLastBindingUpdateTime (Time tm);

    /**
     * \brief get last BU sequence of MN
     * \returns BU sequence
     */
    uint16_t GetLastBindingUpdateSequence () const;

    /**
     * \brief set last BU sequence
     * \param seq BU seq. no.
     */
    void SetLastBindingUpdateSequence (uint16_t seq);

    /**
     * \brief get next pointer
     * \return next pointer
     */
    Entry * GetNext () const;

    /**
     * \brief set next pointer
     * \param entry next pointer
     */
    void SetNext (Entry *entry);

    /**
     * \brief get the CoA MN hold before current BU request
     * \returns the old CoA
     */
    Ipv6Address GetOldCoa () const;

    /**
     * \brief set binding cache entry state of MN
     * \param state the state of that entry
     */
    void SetState (BCache::Entry::State_e state);

    /**
     * \brief get binding cache entry state of MN
     * \return cache entry state
     */
    BCache::Entry::State_e GetState ();

  private:

    /**
     * \brief The BCache object which holds this entry
     */
    Ptr<BCache> m_bCache;

    /**
     * \brief The enum definition to define the state of this entry
     */
    enum BCacheState_e
    {
      UNREACHABLE,
      DEREGISTERING,
      REGISTERING,
      REACHABLE
    };

    /**
     * \brief The state of this entry
     */
    BCacheState_e m_state;

    /**
     * \brief The home agent address of the MN
     */
    Ipv6Address m_haa;

    /**
     * \brief The CoA of the MN
     */
    Ipv6Address m_coa;

    /**
     * \brief The HoA of the MN
     */
    Ipv6Address m_hoa;

    /**
     * \brief The solicited HoA of the MN
     */
    Ipv6Address m_shoa;

    /**
     * \brief The tunnel interface index of the MN
     */
    int16_t m_tunnelIfIndex;

    /**
     * \brief The last binding update time of the MN
     */
    Time m_lastBindingUpdateTime;

    /**
     * \brief The last binding update sequence of the MN
     */
    uint16_t m_lastBindingUpdateSequence;

    /**
     * \brief The next link of the entry
     */
    Entry *m_next;

    //internal

    /**
     * \brief The Old CoA which was previosly bound with HoA
     */
    Ipv6Address m_oldCoa;

    /**
     * \brief The state of the address
     */
    State_e m_addrstate;

  };

protected:

private:

  /**
   * \brief The hashmap of entries keyed by home addresses
   */
  typedef std::unordered_map<Ipv6Address, BCache::Entry *, Ipv6AddressHash> bCache;

  /**
   * \brief The hashmap iterator of entries keyed by home addresses
   */
  typedef std::unordered_map<Ipv6Address, BCache::Entry *, Ipv6AddressHash>::iterator BCacheI;

  void DoDispose ();

  /**
   * \brief The BCache
   */
  bCache m_bCache;

  /**
   * \brief The home agent address list
   */
  std::list<Ipv6Address> m_HaaList;

  /**
   * \brief The home agent prefix list
   */
  std::list<Ipv6Address> m_HomePrefixList;

  /**
   * \brief The node pointer of the HA
   */
  Ptr<Node> m_node;
};

}

#endif /* BINDING_CACHE_H */