/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006,2007 INRIA
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
 * Author: Mathieu Lacage, <mathieu.lacage@sophia.inria.fr>
 */

#ifndef YANS_WIFI_CHANNEL_SI_H
#define YANS_WIFI_CHANNEL_SI_H

#include <ns3/yans-wifi-channel.h>

#include <ns3/spatial-index.h>

namespace ns3 {
class YansWifiChannelSpatialIndex : public YansWifiChannel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  YansWifiChannelSpatialIndex ();
  ~YansWifiChannelSpatialIndex ();


  /**
   * Adds the given YansWifiPhy to the PHY list
   *
   * \param phy the YansWifiPhy to be added to the PHY list
   */
  void Add (Ptr<YansWifiPhy> phy) override;
  const PhyList getPhyList (Ptr<YansWifiPhy> sender) override;
  /**
   * \param sender the phy object from which the packet is originating.
   * \param packet the packet to send
   * \param txPowerDbm the tx power associated to the packet, in dBm
   * \param duration the transmission duration associated with the packet
   *
   * This method should not be invoked by normal users. It is
   * currently invoked only from YansWifiPhy::StartTx.  The channel
   * attempts to deliver the packet to all other YansWifiPhy objects
   * on the channel (except for the sender).
   */
  void Send (Ptr<YansWifiPhy> sender, Ptr<const Packet> packet, double txPowerDbm, Time duration);

  //void SetupCallbacks (Ptr<MobilityModel> mobility, Ptr<PositionAware> position_aware);
  //void NotifyConstructionCompleted () override;
  static void GetPhysForNodes (std::vector<Ptr<const Node>> &nodes, std::vector<Ptr<YansWifiPhy>> &phys);

protected:
  bool                 m_spatialIndexingEnabled;
  double               m_receive_clip_range;
  Ptr<SpatialIndexing> m_spatialIndex;
};

} // namespace ns3

#endif /* YANS_WIFI_CHANNEL_H */
