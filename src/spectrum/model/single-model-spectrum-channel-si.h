
#ifndef SINGLE_MODEL_SPECTRUM_CHANNEL_SI_H
#define SINGLE_MODEL_SPECTRUM_CHANNEL_SI_H

#include <ns3/spatial-index.h>
#include <ns3/single-model-spectrum-channel.h>

namespace ns3 {



/**
 * \ingroup spectrum
 *
 * \brief SpectrumChannel implementation which handles a single spectrum model
 *
 * All SpectrumPhy layers attached to this SpectrumChannel
 */
class SingleModelSpectrumChannelSpatialIndex : public SingleModelSpectrumChannel
{

public:
  SingleModelSpectrumChannelSpatialIndex ();

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  // inherited from SpectrumChannel
  void AddRx (Ptr<SpectrumPhy> phy) override;

  // inherited from SingleModelSpectrumChannel
  bool ProcessTxParams (Ptr<SpectrumSignalParameters> txParams) override;
  bool CheckValidPhy (Ptr<SpectrumPhy> phy) override;

  /// Container: SpectrumPhy objects
  using SingleModelSpectrumChannel::PhyList;

protected:
  bool                 m_spatialIndexingEnabled;
  double               m_receive_clip_range;
  Ptr<SpatialIndexing> m_spatialIndex;
  std::vector<Ptr<const Node>> m_nodesInRange;
};

}

#endif /* SINGLE_MODEL_SPECTRUM_CHANNEL_H */
