

#ifndef MULTI_MODEL_SPECTRUM_CHANNEL_SI_H
#define MULTI_MODEL_SPECTRUM_CHANNEL_SI_H

#include <ns3/spatial-index.h>
#include <ns3/multi-model-spectrum-channel.h>

namespace ns3 {

/**
 * \ingroup spectrum
 *
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
