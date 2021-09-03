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

#ifndef MIPV6_MN_H
#define MIPV6_MN_H

#include "mipv6-agent.h"
#include "blist.h"
#include "ns3/traced-callback.h"
#include "ns3/ipv6-route.h"
#include "ns3/ipv6-header.h"

namespace ns3 {

/**
 * \class Mipv6Mn
 * \brief Mobile IPv6 class defining mobile node behaviour
 */
class Mipv6Mn : public Mipv6Agent
{
public:
  /**
   * \brief Get the type identifier.
   * \return type identifier
   */
  static TypeId GetTypeId (void);
  /**
   * \brief constructor.
   * \param haalist list of home agent addresses
   */
  Mipv6Mn (std::list<Ipv6Address> haalist);

  virtual ~Mipv6Mn ();
  
  /**
   * \brief get home BU seq. no.
   * \return seq no.
   */
  uint16_t GetHomeBUSequence ();

  /**
   * \brief build Home BU
   * \param flagA set A flag
   * \param flagH set H flag
   * \param flagL set L flag
   * \param flagK set K flag
   * \param lifetime set lifetime for BU
   * \param extn add extension or not
   * \return home BU packet
   */
  Ptr<Packet> BuildHomeBU (bool flagA, bool flagH, bool flagL, bool flagK, uint16_t lifetime, bool extn);
  
  /**
   * \brief setup tunnel to transmit packet to CN
   * \return status
   */
  bool SetupTunnelAndRouting ();

  /**
   * \brief clear tunnel
   */
  void ClearTunnelAndRouting ();

  /**
   * \brief set route optimization field
   * \param roflag route optimization flag
   */
  void SetRouteOptimizationRequiredField (bool roflag);

  /**
   * \brief Check whether route optimization field is set or, not.
   * \return status
   */
  bool IsRouteOptimizationRequired ();

  /**
   * \brief check an address whether matched with any of its home agent address
   * \param addr an address
   * \return if address becomes successful
   */
  bool IsHomeMatch (Ipv6Address addr);

  /**
   * \brief set the address of connected AR as its default router
   * \param addr address
   * \param index the interface index of its connected default router
   */
  void SetDefaultRouterAddress (Ipv6Address addr, uint32_t index);

  /**
   * \brief check for match
   * \param ha the home agent address
   * \param hoa the home address
   * \return whether these two addresses match
   */
  bool CheckAddresses (Ipv6Address ha, Ipv6Address hoa);

  /**
   * \brief Return the home address
   * \return HoA
   */
  Ipv6Address GetHomeAddress ();

  /**
   * \brief Return the Care of address
   * \return CoA
   */
  Ipv6Address GetCoA ();

  /**
   * \brief Set if node is in homelink
   * \param prefix the address to check with
   * \param mask the prefix to check with
   * \return CoA
   */
  bool SetHomeLink (Ipv6Address prefix, Ipv6Prefix mask);

  /**
   * TracedCallback signature for BA reception event.
   *
   * \param [in] packet The ba packet.
   * \param [in] src The source address
   * \param [in] dst The destination address
   * \param [in] interface the interface in which the bu received
   */
  typedef void (* RxBaTracedCallback)
    (Ptr<Packet> packet, Ipv6Address src, Ipv6Address dst, Ptr<Ipv6Interface> interface);


  /**
   * TracedCallback signature for BU sent event.
   *
   * \param [in] packet The bu packet.
   * \param [in] src The source address
   * \param [in] dst The destination address
   */
  typedef void (* TxBuTracedCallback)
    (Ptr<Packet> packet, Ipv6Address src, Ipv6Address dst);

protected:
  /**
   * \brief This method is called by AddAgregate and completes the aggregation
   */
  virtual void NotifyNewAggregate ();

  /**
   * \brief handle attachment with a network, called from ICMPv6L4Protocol
   * \param ipr the CoA currently configured at ICMPv6 layer
   */
  virtual void HandleNewAttachment (Ipv6Address ipr);

  /**
   * \brief Handle recieved BA from HA/CN.
   * \param packet BA packet
   * \param src address of HA/CN
   * \param dst CoA of MN
   * \param interface IPv6 interface which recieves the BA
   * \return status
   */
  virtual uint8_t HandleBA (Ptr<Packet> packet, const Ipv6Address &src, const Ipv6Address &dst, Ptr<Ipv6Interface> interface);

  /**
   * \brief Send Data packet from tunnel interface only.
   * \param packet data packet to be sent
   * \param source source address
   * \param dest destination address
   * \param protocol protocol used to send
   * \param route route chosen to send packet
   */
  void SendData (Ptr<Packet> packet, Ipv6Address source, Ipv6Address destination, uint8_t protocol, Ptr<Ipv6Route> route);

  /**
   * \brief Construct an IPv6 header.
   * \param src source IPv6 address
   * \param dst destination IPv6 address
   * \param protocol L4 protocol
   * \param payloadSize payload size
   * \param hopLimit Hop limit
   * \param tclass Tclass
   * \return newly created IPv6 header
   */
  Ipv6Header BuildHeader (Ipv6Address src, Ipv6Address dst, uint8_t protocol, uint16_t payloadSize, uint8_t ttl, uint8_t tclass);
private:

  /**
   * \brief Binding information list of the MN.
   */
  Ptr<BList> m_buinf;

  /**
   * \brief home binding update sequence no.
   */
  uint16_t m_hsequence;

  /**
   * \brief home agent address list.
   */
  std::list<Ipv6Address> m_Haalist;

  /**
   * \brief route optimization flag.
   */
  bool m_roflag;

  /**
   * \brief mobile node in home link.
   */
  bool m_homelink;

  /**
   * \brief default router (i.e. connected AR) address .
   */
  Ipv6Address m_defaultrouteraddress;

  /**
   * \brief prefix of the previous default route before handoff.
   */
  Ipv6Address m_OldPrefixToUse;

  /**
   * \brief interface index of the previous default route before handoff.
   */
  uint32_t m_OldinterfaceIndex;

  /**
   * \brief current interface index of the MN.
   */
  uint32_t m_IfIndex;

  /**
   * \brief Callback to trace RX (reception) ba packets.
   */ 
  TracedCallback<Ptr<Packet>, Ipv6Address, Ipv6Address, Ptr<Ipv6Interface> > m_rxbaTrace;

  /**
   * \brief Callback to trace TX (transmission) bu packets.
   */ 
  TracedCallback<Ptr<Packet>, Ipv6Address, Ipv6Address> m_txbuTrace;

};

} /* namespace ns3 */

#endif /* MIPV6_MN_H */

