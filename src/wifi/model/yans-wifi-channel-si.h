/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2020 National Technology & Engineering Solutions of Sandia,
 * LLC (NTESS). Under the terms of Contract DE-NA0003525 with NTESS,
 * the U.S. Government retains certain rights in this software.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 *
 */

#ifndef YANS_WIFI_CHANNEL_SI_H
#define YANS_WIFI_CHANNEL_SI_H

#include <ns3/yans-wifi-channel.h>

#include <ns3/spatial-index.h>

namespace ns3 {
/** @ingroup spatial-index
 * @brief Implementation that using spatial indexing to clip reception events based on range
 * This range can be varied to balance between fidelity and simulation scalability
 */
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
   * @brief 
   *
   * @param phy the YansWifiPhy to be added to the PHY list
   */
  void Add (Ptr<YansWifiPhy> phy) override;
  /**
   * @brief Override to use spatial indexing to clip the list of nodes to schedule receives on
   * 
   * @param sender 
   * @return const PhyList 
   */
  const PhyList getPhyList (Ptr<YansWifiPhy> sender) override;
  /**
   * @brief Get applicable phys for a given node
   * 
   * @param nodes 
   * @param phys 
   */
  static void GetPhysForNodes (std::vector<Ptr<const Node>> &nodes, std::vector<Ptr<YansWifiPhy>> &phys);

protected:
  bool                 m_spatialIndexingEnabled;
  double               m_receive_clip_range;
  Ptr<SpatialIndexing> m_spatialIndex;
};

} // namespace ns3

#endif /* YANS_WIFI_CHANNEL_H */
