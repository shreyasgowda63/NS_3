/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 CTTC
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#ifndef SINGLE_MODEL_SPECTRUM_CHANNEL_H
#define SINGLE_MODEL_SPECTRUM_CHANNEL_H


#include <ns3/spectrum-channel.h>
#include <ns3/spectrum-model.h>
#include <ns3/traced-callback.h>
#include <ns3/spatial-index.h>

namespace ns3 {



/**
 * \ingroup spectrum
 *
 * \brief SpectrumChannel implementation which handles a single spectrum model
 *
 * All SpectrumPhy layers attached to this SpectrumChannel
 */
class SingleModelSpectrumChannel : public SpectrumChannel
{
public:
  /// Container: SpectrumPhy objects
  typedef std::vector<Ptr<SpectrumPhy>> PhyList;
  SingleModelSpectrumChannel ();

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  // inherited from SpectrumChannel
  virtual void AddRx (Ptr<SpectrumPhy> phy);
  virtual void StartTx (Ptr<SpectrumSignalParameters> params);


  // inherited from Channel
  virtual std::size_t GetNDevices (void) const;
  virtual Ptr<NetDevice> GetDevice (std::size_t i) const;

private:
  virtual void DoDispose ();
protected:
  /** Useful method for child classes to preprocess txparams at start of tx while
   * reusing parent startTx
   * \param txParams The params that are being used
   * \return False aborts transmit */
  virtual bool ProcessTxParams (Ptr<SpectrumSignalParameters> txParams);
  /** Useful method for child classes to filter phys while reusing parent send
   * \param phy The phy to check
   * \return True if valid, false cancels reception scheduling */
  virtual bool CheckValidPhy (Ptr<SpectrumPhy> phy);

  /**
   * Used internally to reschedule transmission after the propagation delay.
   *
   * \param params
   * \param receiver
   */
  void StartRx (Ptr<SpectrumSignalParameters> params, Ptr<SpectrumPhy> receiver);

protected:
  /**
   * List of SpectrumPhy instances attached to the channel.
   */
  PhyList m_phyList;

  /**
   * SpectrumModel that this channel instance is supporting.
   */
  Ptr<const SpectrumModel> m_spectrumModel;

  bool                          m_spatialIndexingEnabled; ///< Enable clipping based on spatial indexing
  double                        m_receive_clip_range;     ///< Range to use for clipping
  Ptr<SpatialIndexing>          m_spatialIndex;           ///< pointer to spatial indexing to be used
  std::vector<ns3::SpatialIndexing::PointerType>  m_nodesInRange;           ///<Vector for storing nodes that are in range
};

}

#endif /* SINGLE_MODEL_SPECTRUM_CHANNEL_H */
