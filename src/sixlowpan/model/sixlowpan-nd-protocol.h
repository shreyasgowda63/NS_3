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

#ifndef SIXLOWPAN_ND_PROTOCOL_H
#define SIXLOWPAN_ND_PROTOCOL_H

#include "ns3/ipv6-address.h"
#include "ns3/ip-l4-protocol.h"
#include "ns3/icmpv6-l4-protocol.h"

namespace ns3 {

class SixLowPanNdiscCache;
class SixLowPanNdPrefix;
class SixLowPanNdContext;
class SixLowPanNetDevice;

/**
 * \ingroup sixlowpan
 * \class SixLowPanNdProtocol
 * \brief An optimization of the ND protocol for 6LoWPANs.
 */
class SixLowPanNdProtocol : public Icmpv6L4Protocol
{
public:
  /**
   * \brief 6LBR constants : min context change delay.
   */
  static const uint16_t MIN_CONTEXT_CHANGE_DELAY;

  /**
   * \brief 6LR constants : max RA transmission.
   */
  static const uint8_t MAX_RTR_ADVERTISEMENTS;

  /**
   * \brief 6LR constants : min delay between RA.
   */
  static const uint8_t MIN_DELAY_BETWEEN_RAS;

  /**
   * \brief 6LR constants : max delay between RA.
   */
  static const uint8_t MAX_RA_DELAY_TIME;

  /**
   * \brief 6LR constants : tentative neighbor cache entry lifetime.
   */
  static const uint8_t TENTATIVE_NCE_LIFETIME;

  /**
   * \brief router constants : multihop hoplimit.
   */
  static const uint8_t MULTIHOP_HOPLIMIT;

  /**
   * \brief host constants : RS interval.
   */
  static const uint8_t RTR_SOLICITATION_INTERVAL;

  /**
   * \brief host constants : max RS transmission.
   */
  static const uint8_t MAX_RTR_SOLICITATIONS;

  /**
   * \brief host constants : max RS interval.
   */
  static const uint8_t MAX_RTR_SOLICITATION_INTERVAL;

  /**
   * \brief Constructor.
   */
  SixLowPanNdProtocol ();

  /**
   * \brief Destructor.
   */
  virtual ~SixLowPanNdProtocol ();

  /**
   * \brief Interface ID
   */
  static TypeId GetTypeId ();

  virtual TypeId GetInstanceTypeId (void) const;
  virtual void DoInitialize (void);

  /**
   * \brief Forge a Neighbor Advertisement.
   * \param src source IPv6 address
   * \param dst destination IPv6 address
   * \param target target IPv6 address
   * \param hardwareAddress our mac address
   * \param flags flags (bitfield => R (4), S (2), O (1))
   * \return NA packet (with IPv6 header)
   */
  Ptr<Packet> ForgeNA (Ipv6Address src, Ipv6Address dst, Ipv6Address target, Address hardwareAddress, uint8_t flags);

  /**
   * \brief Send a NS for 6LoWPAN ND (+ SLLAO).
   * \param src source IPv6 address
   * \param dst destination IPv6 address
   * \param target target IPv6 address
   * \param linkAddr link-layer address (SLLAO)
   */
  virtual void SendNS (Ipv6Address src, Ipv6Address dst, Ipv6Address target, Address linkAddr);

  /**
   * \brief Send a NS for 6LoWPAN ND (+ ARO, SLLAO).
   * \param src source IPv6 address
   * \param dst destination IPv6 address
   * \param time registration lifetime (ARO)
   * \param eui EUI-64 (ARO)
   * \param linkAddr link-layer address (SLLAO)
   */
  void SendSixLowPanARO (Ipv6Address src, Ipv6Address dst, uint16_t time, Mac64Address eui,
                         Address linkAddr);

  /**
   * \brief Send a NA for 6LoWPAN ND (+ ARO).
   * \param src source IPv6 address
   * \param dst destination IPv6 address
   * \param status status (ARO)
   * \param time registration lifetime (ARO)
   * \param eui EUI-64 (ARO)
   */
  void SendSixLowPanARO (Ipv6Address src, Ipv6Address dst, uint8_t status, uint16_t time,
                         Mac64Address eui);

  /**
   * \brief Send a RA for 6LoWPAN ND (+ PIO, 6CO, ABRO, SLLAO).
   * \param src source IPv6 address
   * \param dst destination IPv6 address
   * \param interface the interface from which the packet will be sent
   */
  void SendSixLowPanRA (Ipv6Address src, Ipv6Address dst, Ptr<Ipv6Interface> interface);

  /**
   * \brief Send a DAR for 6LoWPAN ND.
   * \param src source IPv6 address
   * \param dst destination IPv6 address
   * \param time registration lifetime
   * \param eui EUI-64
   * \param registered registered IPv6 address
   */
  void SendSixLowPanDAR (Ipv6Address src, Ipv6Address dst, uint16_t time, Mac64Address eui,
                         Ipv6Address registered);

  /**
   * \brief Receive method.
   * \param p the packet
   * \param header the IPv6 header
   * \param interface the interface from which the packet is coming
   * \returns the receive status
   */
  virtual enum IpL4Protocol::RxStatus Receive (Ptr<Packet> p,
                                               Ipv6Header const &header,
                                               Ptr<Ipv6Interface> interface);

  virtual Ptr<NdiscCache> CreateCache (Ptr<NetDevice> device, Ptr<Ipv6Interface> interface);

  /**
   * \brief Function called to send NS + ARO + SLLAO.
   * \param src source IPv6 address
   * \param dst destination IPv6 address
   * \param time registration lifetime (ARO)
   * \param eui EUI-64 (ARO)
   * \param linkAddr link-layer address (SLLAO)
   */
  void RetransmitARO (Ipv6Address src, Ipv6Address dst, uint16_t time, Mac64Address eui,
                      Address linkAddr);

  /**
   * \brief Function called to send RS + SLLAO.
   * \param src source IPv6 address
   * \param dst destination IPv6 address
   * \param linkAddress link-layer address (SLLAO)
   */
  void RetransmitRS (Ipv6Address src, Ipv6Address dst, Address linkAddr);

  void AddAdvertisedPrefix (Ptr<SixLowPanNetDevice>, Ptr<SixLowPanNdPrefix>);

  void AddAdvertisedContext (Ptr<SixLowPanNetDevice>, Ptr<SixLowPanNdContext>);

protected:
  /**
   * \brief Dispose this object.
   */
  virtual void DoDispose ();

private:


  /**
   * \class SixLowPanRaEntry
   * \brief RA advertised from routers for 6LoWPAN ND.
   */
  class SixLowPanRaEntry : public SimpleRefCount<SixLowPanRaEntry>
  {
  public:
    SixLowPanRaEntry ();

    ~SixLowPanRaEntry ();

    /**
     * \brief Get list of prefixes advertised for this interface.
     * \return list of IPv6 prefixes
     */
    std::map<Ipv6Address, Ptr<SixLowPanNdPrefix> > GetPrefixes () const;

    /**
     * \brief Add a prefix to advertise on interface.
     * \param prefix prefix to advertise
     */
    void AddPrefix (Ptr<SixLowPanNdPrefix> prefix);

    /**
     * \brief Remove a prefix.
     * \param prefix prefix to remove
     */
    void RemovePrefix (Ptr<SixLowPanNdPrefix> prefix);

    /**
     * \brief Get list of 6LoWPAN contexts advertised for this interface.
     * \return list of 6LoWPAN contexts
     */
    std::map<uint8_t, Ptr<SixLowPanNdContext> > GetContexts () const;

    /**
     * \brief Add a 6LoWPAN context to advertise on interface.
     * \param context 6LoWPAN context to advertise
     */
    void AddContext (Ptr<SixLowPanNdContext> context);

    /**
     * \brief Remove a 6LoWPAN context.
     * \param context 6LoWPAN context to remove
     */
    void RemoveContext (Ptr<SixLowPanNdContext> context);

    /**
     * \brief Is managed flag enabled ?
     * \return managed flag
     */
    bool IsManagedFlag () const;

    /**
     * \brief Set managed flag
     * \param managedFlag value
     */
    void SetManagedFlag (bool managedFlag);

    /**
     * \brief Is "other config" flag enabled ?
     * \return other config flag
     */
    bool IsOtherConfigFlag () const;

    /**
     * \brief Is "home agent" flag enabled ?
     * \return "home agent" flag
     */
    bool IsHomeAgentFlag () const;

    /**
     * \brief Set "home agent" flag.
     * \param homeAgentFlag value
     */
    void SetHomeAgentFlag (bool homeAgentFlag);

    /**
     * \brief Set "other config" flag
     * \param otherConfigFlag value
     */
    void SetOtherConfigFlag (bool otherConfigFlag);

    /**
     * \brief Get reachable time.
     * \return reachable time
     */
    uint32_t GetReachableTime () const;

    /**
     * \brief Set reachable time.
     * \param time reachable time
     */
    void SetReachableTime (uint32_t itme);

    /**
     * \brief Get router lifetime.
     * \return router lifetime
     */
    uint32_t GetRouterLifeTime () const;

    /**
     * \brief Set router lifetime.
     * \param time router lifetime
     */
    void SetRouterLifeTime (uint32_t time);

    /**
     * \brief Get retransmission timer.
     * \return retransmission timer
     */
    uint32_t GetRetransTimer () const;

    /**
     * \brief Set retransmission timer.
     * \param timer retransmission timer
     */
    void SetRetransTimer (uint32_t timer);

    /**
     * \brief Get current hop limit.
     * \return current hop limit for the link
     */
    uint8_t GetCurHopLimit () const;

    /**
     * \brief Set current hop limit.
     * \param curHopLimit current hop limit for the link
     */
    void SetCurHopLimit (uint8_t curHopLimit);

    /**
     * \brief Get version value (ABRO).
     * \return the version value
     */
    uint32_t GetVersion () const;

    /**
     * \brief Set version value (ABRO).
     * \param version the version value
     */
    void SetVersion (uint32_t version);

    /**
     * \brief Get valid lifetime value (ABRO).
     * \return the valid lifetime (units of 60 seconds)
     */
    uint16_t GetValidTime () const;

    /**
     * \brief Set valid lifetime value (ABRO).
     * \param time the valid lifetime (units of 60 seconds)
     */
    void SetValidTime (uint16_t time);

    /**
     * \brief Get Border Router address (ABRO).
     * \return the Border Router address
     */
    Ipv6Address GetBorderAddress () const;

    /**
     * \brief Set Border Router address (ABRO).
     * \param border the Border Router address
     */
    void SetBorderAddress (Ipv6Address border);

  private:
    /**
     * \brief List of prefixes advertised.
     */
    std::map<Ipv6Address, Ptr<SixLowPanNdPrefix> > m_prefixes;

    /**
     * \brief List of 6LoWPAN contexts advertised.
     */
    std::map<uint8_t, Ptr<SixLowPanNdContext> > m_contexts;

    /**
     * \brief Managed flag. If true host use the stateful protocol for address autoconfiguration.
     */
    bool m_managedFlag;

    /**
     * \brief Other configuration flag. If true host use stateful protocol for other (non-address) information.
     */
    bool m_otherConfigFlag;

    /**
     * \brief Flag to add HA (home agent) flag in RA.
     */
    bool m_homeAgentFlag;

    /**
     * \brief Reachable time in milliseconds.
     */
    uint32_t m_reachableTime;

    /**
     * \brief Retransmission timer in milliseconds.
     */
    uint32_t m_retransTimer;

    /**
     * \brief Current hop limit (TTL).
     */
    uint32_t m_curHopLimit;

    /**
     * \brief Router life time in seconds.
     */
    uint32_t m_routerLifeTime;

    /**
     * \brief Version value for ABRO.
     */
    uint32_t m_version;

    /**
     * \brief Valid lifetime value for ABRO (units of 60 seconds).
     */
    uint16_t m_validTime;

    /**
     * \brief Border Router address for ABRO.
     */
    Ipv6Address m_border;
  };

  /**
   *  Role of the node: 6LN, 6LR, 6LBR
   */
  enum SixLowPanNodeStatus_e
  {
    SixLowPanNode,
    SixLowPanRouter,
    SixLowPanBorderRouter
  };

  /**
   * \brief Use multihop DAD mechanism
   */
  bool m_multihopDad;

  /**
   * \brief Number of RS retransmission.
   */
  uint8_t m_rsRetransmit;

  /**
   * \brief Is an RA received.
   */
  bool m_receivedRA;

  /**
   * \brief Number of NS + ARO + SLLAO retransmission.
   */
  uint8_t m_aroRetransmit;

  /**
   * \brief The amount of time (units of 60 seconds) that the router should retain the NCE for the node.
   */
  uint16_t m_regTime;

  /**
   * \brief The advance to perform maintaining of RA's information and registration.
   */
  uint16_t m_advance;

  /**
   * \brief Is a 6LoWPAN Border router.
   */
  bool m_border;

  /**
   * \brief Status of the node
   */
  SixLowPanNodeStatus_e m_nodeRole;

  std::map<Ipv6Address, Ptr<SixLowPanRaEntry> > m_raCache; //!< Router Advertisement cached entries (if the node is a 6LR)
  std::map<Ptr<SixLowPanNetDevice>, Ptr<SixLowPanRaEntry> > m_raEntries; //!< Router Advertisement entries (if the node is a 6LBR)

  /**
   * \brief Receive NS for 6LoWPAN ND method.
   * \param p the packet
   * \param src source address
   * \param dst destination address
   * \param interface the interface from which the packet is coming
   */
  void HandleSixLowPanNS (Ptr<Packet> packet, Ipv6Address const &src, Ipv6Address const &dst,
                          Ptr<Ipv6Interface> interface);

  /**
   * \brief Receive NA for 6LoWPAN ND method.
   * \param p the packet
   * \param src source address
   * \param dst destination address
   * \param interface the interface from which the packet is coming
   */
  void HandleSixLowPanNA (Ptr<Packet> packet, Ipv6Address const &src, Ipv6Address const &dst,
                          Ptr<Ipv6Interface> interface);

  /**
   * \brief Receive RS for 6LoWPAN ND method.
   * \param p the packet
   * \param src source address
   * \param dst destination address
   * \param interface the interface from which the packet is coming
   */
  void HandleSixLowPanRS (Ptr<Packet> packet, Ipv6Address const &src, Ipv6Address const &dst,
                          Ptr<Ipv6Interface> interface);

  /**
   * \brief Receive RA for 6LoWPAN ND method.
   * \param p the packet
   * \param src source address
   * \param dst destination address
   * \param interface the interface from which the packet is coming
   */
  void HandleSixLowPanRA (Ptr<Packet> packet, Ipv6Address const &src, Ipv6Address const &dst,
                          Ptr<Ipv6Interface> interface);

  /**
   * \brief Receive DAC for 6LoWPAN ND method.
   * \param p the packet
   * \param src source address
   * \param dst destination address
   * \param interface the interface from which the packet is coming
   */
  void HandleSixLowPanDAC (Ptr<Packet> packet, Ipv6Address const &src, Ipv6Address const &dst,
                           Ptr<Ipv6Interface> interface);

  /**
   * \brief Set the m_receivedRA flag.
   * \param received value
   */
  void SetReceivedRA (bool received);
};

} /* namespace ns3 */

#endif
