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

#ifndef B_LIST_H
#define B_LIST_H

#include <list>
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/ipv6-address.h"
#include "ns3/ptr.h"
#include "ns3/timer.h"
#include "ns3/node.h"

namespace ns3 {

class BList : public Object
{
public:
  /**
   * \brief Get the type identifier.
   * \return type identifier
   */
  static TypeId GetTypeId ();
  /**
   * \brief constructor.
   * \param haalist home agent address list
   */
  BList (std::list<Ipv6Address> haalist);
  /**
   * \brief destructor
   */
  ~BList ();

  /**
   * \brief get the node pointer.
   * \returns the node pointer 
   */
  Ptr<Node> GetNode () const;

  /**
   * \brief set the node pointer.
   * \param node the node pointer 
   */
  void SetNode (Ptr<Node> node);


  /**
   * \brief whether the home agent is unreachable.
   * \returns the unreachability status of the home agent 
   */
  bool IsHomeUnreachable () const;

  /**
   * \brief whether the home agent is updating now.
   * \returns the updating status of the home agent 
   */
  bool IsHomeUpdating () const;

  /**
   * \brief whether the MN currently performing BU process.
   * \returns the status of the home refreshment 
   */
  bool IsHomeRefreshing () const;

  /**
   * \brief whether the home agent is reachable.
   * \returns the reachability status of the home agent 
   */
  bool IsHomeReachable () const;

  /**
   * \brief mark the reachability status of the home agent as unreachable.
   */
  void MarkHomeUnreachable ();

  /**
   * \brief mark the updating status of the home agent as updating.
   */
  void MarkHomeUpdating ();

  /**
   * \brief mark the refreshing status of the home agent as refreshing.
   */
  void MarkHomeRefreshing ();

  /**
   * \brief mark the reachability status of the home agent as reachable.
   */
  void MarkHomeReachable ();

  //timer processing

  /**
   * \brief If BU transmission failed it starts the BU transmission process.
   */
  void StartHomeRetransTimer ();

  /**
   * \brief If BU transmission succeded or, time out happens, it stops the retransmission timer.
   */
  void StopHomeRetransTimer ();

  /**
   * \brief not used.
   */
  void StartHomeReachableTimer ();

  /**
   * \brief not used.
   */
  void StopHomeReachableTimer ();

  /**
   * \brief not used.
   */
  void StartHomeRefreshTimer ();

  /**
   * \brief not used.
   */
  void StopHomeRefreshTimer ();

  /**
   * \brief not used.
   */
  void FunctionHomeRetransTimeout ();

  /**
   * \brief not used.
   */
  void FunctionHomeReachableTimeout ();

  /**
   * \brief not used.
   */
  void FunctionHomeRefreshTimeout ();

  /**
   * \brief not used.
   */
  Time GetHomeReachableTime () const;

  /**
   * \brief not used.
   */
  void SetHomeReachableTime (Time tm);


  /**
   * \brief no of retransmission tried.
   * \return home BU retry count
   */
  uint8_t GetHomeRetryCount () const;

  /**
   * \brief increase home retransmission counter.
   */
  void IncreaseHomeRetryCount ();

  /**
   * \brief reset home retransmission counter.
   */
  void ResetHomeRetryCount ();

  /**
   * \brief get home bu initial lifetime.
   * \return home bu initial lifetime
   */
  Time GetHomeInitialLifeTime () const;
  /**
   * \brief set home bu initial lifetime.
   * \param tm home bu initial lifetime
   */
  void SetHomeInitialLifeTime (Time tm);

  /**
   * \brief get home bu remaining lifetime.
   * \return home bu remaining lifetime
   */
  Time GetHomeRemainingLifeTime () const;

  /**
   * \brief get last home bu time.
   * \return last home bu lifetime
   */
  Time GetLastHomeBindingUpdateTime () const;
  /**
   * \brief set last home bu time.
   * \param tm last home bu time
   */
  void SetLastHomeBindingUpdateTime (Time tm);

  /**
   * \brief get max home bu sequence.
   * \return max home bu sequence
   */
  uint16_t GetHomeMaxBindingUpdateSequence () const;
  /**
   * \brief set max home bu sequence.
   * \param seq max home bu sequence
   */
  void SetHomeMaxBindingUpdateSequence (uint16_t seq);

  /**
   * \brief get last home bu sequence.
   * \return last home bu sequence
   */
  uint16_t GetHomeLastBindingUpdateSequence () const;

  /**
   * \brief set last home bu sequence.
   * \param seq last home bu sequence
   */
  void SetHomeLastBindingUpdateSequence (uint16_t seq);

  /**
   * \brief get last home bu time.
   * \return last home bu time
   */
  Time GetHomeLastBindingUpdateTime () const;

  /**
   * \brief set last home bu time.
   * \param tm last home bu time
   */
  void SetHomeLastBindingUpdateTime (Time tm);

  /**
   * \brief get tunnel interface index.
   * \return tunnel interface index
   */
  int16_t GetTunnelIfIndex () const;

  /**
   * \brief set tunnel interface index.
   * \param tunnelif tunnel interface index
   */
  void SetTunnelIfIndex (int16_t tunnelif);

  /**
   * \brief set home address.
   * \param hoa home address
   */
  void SetHoa (Ipv6Address hoa);
  /**
   * \brief get home address.
   * \return home address
   */
  Ipv6Address GetHoa () const;
  /**
   * \brief get care-of-address.
   * \return care-of-address
   */
  Ipv6Address GetCoa () const;
  /**
   * \brief set care-of-address.
   * \param addr care-of-address
   */
  void SetCoa (Ipv6Address addr);
  /**
   * \brief set home agent address.
   * \param ha home agent address
   */
  void SetHA (Ipv6Address ha);
  /**
   * \brief get home agent address.
   * \return home agent address
   */
  Ipv6Address GetHA () const;

  /**
   * \brief get home agent address list.
   * \return home agent address list
   */
  std::list<Ipv6Address> GetHomeAgentList () const;
  /**
   * \brief set home agent address list.
   * \param haalist home agent address list
   */
  void SetHomeAgentList (std::list<Ipv6Address> haalist);

  /**
   * \brief get home BU flag.
   * \return home BU flag
   */
  bool GetHomeBUFlag () const;
  /**
   * \brief set home BU flag.
   * \param f home BU flag
   */
  void SetHomeBUFlag (bool f);

  /**
   * \brief get home BU packet.
   * \return home BU packet
   */
  Ptr<Packet> GetHomeBUPacket () const;
  /**
   * \brief set home BU packet.
   * \param pkt home BU packet
   */
  void SetHomeBUPacket (Ptr<Packet> pkt);

  /**
   * \brief flush
   */
  void Flush ();

  /**
   * \brief set home address registered flag.
   * \param flag home address registered flag
   */
  void SetHomeAddressRegistered (bool flag);

  /**
   * \brief check home address registered flag.
   * \return home address registered flag
   */
  bool IsHomeAddressRegistered ();



private:

  /**
   * \enum BindingUpdateState_e
   * \brief binding update state
   */
  enum BindingUpdateState_e
  {
    UNREACHABLE,
    UPDATING,
    REFRESHING,
    REACHABLE,
  };

  /**
   * \brief binding update state variable
   */
  BindingUpdateState_e m_hstate;

  /**
   * \brief tunnel interface index
   */
  int16_t m_tunnelIfIndex;

  /**
   * \brief home bu packet
   */
  Ptr<Packet> m_hpktbu;

  /**
   * \brief home initial lifetime
   */
  Time m_hinitiallifetime;

  /**
   * \brief home last binding update sequence
   */
  uint16_t m_hlastBindingUpdateSequence;

  /**
   * \brief home bu flag
   */
  bool m_hflag;

  /**
   * \brief home bu last sent
   */
  Time m_hbulastsent;

  /**
   * \brief home address
   */
  Ipv6Address m_hoa;

  /**
   * \brief CoA
   */
  Ipv6Address m_coa;

  /**
   * \brief home agent address
   */
  Ipv6Address m_ha;

  /**
   * \brief home agent address list
   */
  std::list<Ipv6Address> m_HaaList;

  /**
   * \brief home reachable time
   */
  Time m_hreachableTime;

  /**
   * \brief home retransmission timer
   */
  Timer m_hretransTimer;

  /**
   * \brief home reachable timer
   */
  Timer m_hreachableTimer;

  /**
   * \brief home refresh timer
   */
  Timer m_hrefreshTimer;

  /**
   * \brief home bu retry count
   */
  uint8_t m_hretryCount;

  /**
   * \brief home address registered flag
   */
  bool m_HomeAddressRegisteredFlag;

  /**
   * \brief Dispose this object.
   */
  void DoDispose ();

  /**
   * \brief the MN.
   */
  Ptr<Node> m_node;
};

} /* ns3 */

#endif /* BINDING_UPDATE_LIST_H */