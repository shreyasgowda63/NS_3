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

#ifndef MULTI_MODEL_SPECTRUM_CHANNEL_SI_H
#define MULTI_MODEL_SPECTRUM_CHANNEL_SI_H

#include <ns3/spatial-index.h>
#include <ns3/multi-model-spectrum-channel.h>

namespace ns3 {

/** @brief Implementation that using spatial indexing to clip reception events based on range
 * This range can be varied to balance between fidelity and simulation scalability
 * \ingroup spectrum spatial-index
 */
class MultiModelSpectrumChannelSpatialIndex : public MultiModelSpectrumChannel
{

public:
  MultiModelSpectrumChannelSpatialIndex ();

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  // inherited from MultiModelSpectrumChannel
  void AddRx (Ptr<SpectrumPhy> phy) override;

  // inherited from SingleModelSpectrumChannel
  bool ProcessTxParams (Ptr<SpectrumSignalParameters> txParams) override;
  bool CheckValidPhy (Ptr<SpectrumPhy> phy) override;

protected:
  bool                 m_spatialIndexingEnabled;
  double               m_receive_clip_range;
  Ptr<SpatialIndexing> m_spatialIndex;
  std::vector<Ptr<const Node>> m_nodesInRange;
};

} // namespace ns3

#endif /* MULTI_MODEL_SPECTRUM_CHANNEL_H */
