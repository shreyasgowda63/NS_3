/*
 * Copyright (c) 2016
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef HE_CAPABILITIES_H
#define HE_CAPABILITIES_H

#include "ns3/wifi-information-element.h"

namespace ns3
{

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ax HE Capabilities
 */
class HeCapabilities : public WifiInformationElement
{
  public:
    HeCapabilities();

    // Implementations of pure virtual methods of WifiInformationElement
    WifiInformationElementId ElementId() const override;
    WifiInformationElementId ElementIdExt() const override;

    /**
     * Set the HE MAC Capabilities Info field in the HE Capabilities information element.
     *
     * \param ctrl1 the HE MAC Capabilities Info field 1 in the HE Capabilities information element
     * \param ctrl2 the HE MAC Capabilities Info field 2 in the HE Capabilities information element
     */
    void SetHeMacCapabilitiesInfo(uint32_t ctrl1, uint16_t ctrl2);
    /**
     * Set the HE PHY Capabilities Info field in the HE Capabilities information element.
     *
     * \param ctrl1 the HE PHY Capabilities Info field 1 in the HE Capabilities information element
     * \param ctrl2 the HE PHY Capabilities Info field 2 in the HE Capabilities information element
     * \param ctrl3 the HE PHY Capabilities Info field 3 in the HE Capabilities information element
     */
    void SetHePhyCapabilitiesInfo(uint64_t ctrl1, uint16_t ctrl2, uint8_t ctrl3);
    /**
     * Set the MCS and NSS field in the HE Capabilities information element.
     *
     * \param ctrl the MCS and NSS field in the HE Capabilities information element
     */
    void SetSupportedMcsAndNss(uint16_t ctrl);

    /**
     * Return the 4 first octets of the HE MAC Capabilities Info field in the HE Capabilities
     * information element.
     *
     * \return the 4 first octets of the HE MAC Capabilities Info field in the HE Capabilities
     * information element
     */
    uint32_t GetHeMacCapabilitiesInfo1() const;
    /**
     * Return the last 2 octets of the HE MAC Capabilities Info field in the HE Capabilities
     * information element.
     *
     * \return the last 2 octets of the HE MAC Capabilities Info field in the HE Capabilities
     * information element
     */
    uint16_t GetHeMacCapabilitiesInfo2() const;
    /**
     * Return the 8 first octets of the HE PHY Capabilities Info field in the HE Capabilities
     * information element.
     *
     * \return the 8 first octets of the HE PHY Capabilities Info field in the HE Capabilities
     * information element
     */
    uint64_t GetHePhyCapabilitiesInfo1() const;
    /**
     * Return the octets 9-10 of the HE PHY Capabilities Info field in the HE Capabilities
     * information element.
     *
     * \return the octets 9-10 of the HE PHY Capabilities Info field in the HE Capabilities
     * information element
     */
    uint16_t GetHePhyCapabilitiesInfo2() const;
    /**
     * Return the last octet of the HE PHY Capabilities Info field in the HE Capabilities
     * information element.
     *
     * \return the last octet of the HE PHY Capabilities Info field in the HE Capabilities
     * information element
     */
    uint8_t GetHePhyCapabilitiesInfo3() const;
    /**
     * Return the MCS and NSS field in the HE Capabilities information element.
     *
     * \return the MCS and NSS field in the HE Capabilities information element
     */
    uint16_t GetSupportedMcsAndNss() const;

    // PHY Capabilities Info fields
    /**
     * Set channel width set.
     *
     * \param channelWidthSet the channel width set
     */
    void SetChannelWidthSet(uint8_t channelWidthSet);
    /**
     * Set indication whether the transmission and reception of LDPC encoded packets is supported.
     *
     * \param ldpcCodingInPayload indication whether the transmission and reception of LDPC encoded
     * packets is supported
     */
    void SetLdpcCodingInPayload(uint8_t ldpcCodingInPayload);
    /**
     * Set 1xHE-LTF and 800ns GI in HE SU PPDU reception support
     *
     * \param heSuPpdu1xHeLtf800nsGi 1xHE-LTF and 800ns GI in HE SU PPDU reception support
     */
    void SetHeSuPpdu1xHeLtf800nsGi(bool heSuPpdu1xHeLtf800nsGi);
    /**
     * Set 4xHE-LTF and 800ns GI in HE SU PPDU and HE MU PPDU reception support
     *
     * \param heSuPpdu4xHeLtf800nsGi 4xHE-LTF and 800ns GI in HE SU PPDU and HE MU PPDU reception
     * support
     */
    void SetHePpdu4xHeLtf800nsGi(bool heSuPpdu4xHeLtf800nsGi);
    /**
     * Get channel width set.
     *
     * \returns the channel width set
     */
    uint8_t GetChannelWidthSet() const;
    /**
     * Indicates support for the transmission and reception of LDPC encoded packets.
     *
     * \returns indication whether the transmission and reception of LDPC encoded packets is
     * supported
     */
    uint8_t GetLdpcCodingInPayload() const;
    /**
     * Get 1xHE-LTF and 800ns GI in HE SU PPDU reception support
     *
     * \returns true if 1xHE-LTF and 800ns GI in HE SU PPDU reception is supported, false otherwise
     */
    bool GetHeSuPpdu1xHeLtf800nsGi() const;
    /**
     * Get 4xHE-LTF and 800ns GI in HE SU PPDU and HE MU PPDU reception support
     *
     * \returns true if 4xHE-LTF and 800ns GI in HE SU PPDU and HE MU PPDU reception is supported,
     *          false otherwise
     */
    bool GetHePpdu4xHeLtf800nsGi() const;
    /**
     * Get highest MCS supported.
     *
     * \returns the highest MCS is supported
     */
    uint8_t GetHighestMcsSupported() const;
    /**
     * Get highest NSS supported.
     *
     * \returns the highest supported NSS
     */
    uint8_t GetHighestNssSupported() const;

    // MAC Capabilities Info fields
    /**
     * Set the maximum AMPDU length.
     *
     * \param maxAmpduLength 2^(20 + x) - 1, x in the range 0 to 3
     */
    void SetMaxAmpduLength(uint32_t maxAmpduLength);

    // MCS and NSS field information
    /**
     * Set highest MCS supported.
     *
     * \param mcs the MCS
     */
    void SetHighestMcsSupported(uint8_t mcs);
    /**
     * Set highest NSS supported.
     *
     * \param nss the NSS
     */
    void SetHighestNssSupported(uint8_t nss);

    /**
     * Is RX MCS supported.
     *
     * \param mcs the MCS
     * \returns true if MCS transmit supported
     */
    bool IsSupportedTxMcs(uint8_t mcs) const;
    /**
     * Is RX MCS supported.
     *
     * \param mcs the MCS
     * \returns true if MCS receive supported
     */
    bool IsSupportedRxMcs(uint8_t mcs) const;

    /**
     * Return the maximum A-MPDU length.
     *
     * \return the maximum A-MPDU length
     */
    uint32_t GetMaxAmpduLength() const;

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    // MAC Capabilities Info fields
    // IEEE 802.11ax-2021 9.4.2.248.2 HE MAC Capabilities Information field
    uint8_t m_plusHtcHeSupport{0};               //!< HTC HE support
    uint8_t m_twtRequesterSupport{0};            //!< TWT requester support
    uint8_t m_twtResponderSupport{0};            //!< TWT responder support
    uint8_t m_fragmentationSupport{0};           //!< fragmentation support
    uint8_t m_maximumNumberOfFragmentedMsdus{0}; //!< maximum number of fragmentation MSDUs
    uint8_t m_minimumFragmentSize{0};            //!< minimum fragment size
    uint8_t m_triggerFrameMacPaddingDuration{0}; //!< trigger frame MAC padding duration
    uint8_t m_multiTidAggregationRxSupport{0};   //!< multi-TID aggregation Rx support
    uint8_t m_heLinkAdaptation{0};               //!< HE link adaptation
    uint8_t m_allAckSupport{0};                  //!< all Ack support
    uint8_t m_trsSupport{0};                     //!< TRS support
    uint8_t m_bsrSupport{0};                     //!< BSR support
    uint8_t m_broadcastTwtSupport{0};            //!< broadcast TXT support
    uint8_t m_32bitBaBitmapSupport{0};           //!< 32-bit BA bitmap support
    uint8_t m_muCascadeSupport{0};               //!< MU cascade support
    uint8_t m_ackEnabledAggregationSupport{0};   //!< ack enabled aggregation support
    uint8_t m_omControlSupport{0};               //!< operation mode control support
    uint8_t m_ofdmaRaSupport{0};                 //!< OFDMA RA support
    uint8_t m_maxAmpduLengthExponent{0};         //!< maximum A-MPDU length exponent extension
    uint8_t m_amsduFragmentationSupport{0};      //!< A-MSDU fragmentation support
    uint8_t m_flexibleTwtScheduleSupport{0};     //!< flexible TWT schedule support
    uint8_t m_rxControlFrameToMultiBss{0};       //!< receive control frame to multi-BSS
    uint8_t m_bsrpBqrpAmpduAggregation{0};       //!< BSRP BQRP A-MPDU aggregation
    uint8_t m_qtpSupport{0};                     //!< QTP support
    uint8_t m_bqrSupport{0};                     //!< BQR support
    uint8_t m_psrResponder{0};                   //!< PSR responder
    uint8_t m_ndpFeedbackReportSupport{0};       //!< NDP feedback report support
    uint8_t m_opsSupport{0};                     //!< OPS support
    uint8_t m_amsduNotUnderBaInAmpduSupport{
        0};                                    //!< AMSDU not under BA in Ack enabled A-MPDU support
    uint8_t m_multiTidAggregationTxSupport{0}; //!< Multi-TID aggregation TX support
    uint8_t m_heSubchannelSelectiveTxSupport{0}; //!< HE subchannel selective transmission support
    uint8_t m_ul2x996ToneRuSupport{0};           //!< UL 2x996 tone RU support
    uint8_t m_omControlUlMuDataDisableRxSupport{0}; //!< OM control UL MU data disable RX support
    uint8_t m_heDynamicSmPowerSave{0};              //!< HE dynamic SM power save
    uint8_t m_puncturedSoundingSupport{0};          //!< punctured sounding support
    uint8_t m_heVhtTriggerFrameRxSupport{0};        //!< HE and VHT trigger frame RX support

    // PHY Capabilities Info fields
    // IEEE 802.11ax-2021 9.4.2.248.3 HE PHY Capabilities Information field
    uint8_t m_channelWidthSet{0};        //!< channel width set
    uint8_t m_puncturedPreambleRx{0};    //!< Punctured preamble Rx
    uint8_t m_deviceClass{0};            //!< device class
    uint8_t m_ldpcCodingInPayload{0};    //!< LDPC coding in payload
    uint8_t m_heSuPpdu1xHeLtf800nsGi{0}; //!< HE SU PPDU with 1x HE LTF and 0.8us GI
    uint8_t m_midambleRxMaxNsts{0};      //!< Midamble TX/RX max NSTS
    uint8_t m_ndp4xHeLtfAnd32msGi{0};    //!< NDP with 4x HE-LTF and 3.2us GI
    uint8_t m_stbcTxLeq80MHz{0};         //!< STBC TX <= 80MHz
    uint8_t m_stbcRxLeq80MHz{0};         //!< STBC RX <= 80Mhz
    uint8_t m_dopplerTx{0};              //!< Doppler Tx
    uint8_t m_dopplerRx{0};              //!< Doppler Rx
    uint8_t m_fullBwUlMuMimo{0};         //!< Full Bandwidth UL MU-MIMO
    uint8_t m_partialBwUlMuMimo{0};      //!< Partial Bandwidth UL MU-MIMO
    uint8_t m_dcmMaxConstellationTx{0};  //!< DCM Max Constellation Tx
    uint8_t m_dcmMaxNssTx{0};            //!< DCM Max NSS Tx
    uint8_t m_dcmMaxConstellationRx{0};  //!< DCM Max Constellation Rx
    uint8_t m_dcmMaxNssRx{0};            //!< DCM Max NSS Rx
    uint8_t m_rxPartialBwSuInHeMu{0};    //!< Rx Partial BW SU in 20 MHz HE MU PPDU
    uint8_t m_suBeamformer{0};           //!< SU beamformer
    uint8_t m_suBeamformee{0};           //!< SU beamformee
    uint8_t m_muBeamformer{0};           //!< MU beamformer
    uint8_t m_beamformeeStsForSmallerOrEqualThan80Mhz{0}; //!< beam formee STS for < 80 MHz
    uint8_t m_beamformeeStsForLargerThan80Mhz{0};         //!< beamformee STS for > 80MHz
    uint8_t m_numberOfSoundingDimensionsForSmallerOrEqualThan80Mhz{0}; //!< # of sounding dimensions
                                                                       //!< for < 80 MHz
    uint8_t m_numberOfSoundingDimensionsForLargerThan80Mhz{
        0};                                     //!< # of sounding dimensions for > 80 MHz
    uint8_t m_ngEqual16ForSuFeedbackSupport{0}; //!< equal 16 for SU feedback
    uint8_t m_ngEqual16ForMuFeedbackSupport{0}; //!< equal 16 for MU feedback
    uint8_t m_codebookSize42SuFeedback{0};      //!< Codebook Size = {4, 2} SU feedback
    uint8_t m_codebookSize75MuFeedback{0};      //!< Codebook Size = {7, 5} MU feedback
    uint8_t m_triggeredSuBfFeedback{0};         //!< Triggered SU beamforming feedback
    uint8_t m_triggeredMuBfFeedback{0};         //!< Triggered MU beamforming feedback
    uint8_t m_triggeredCqiFeedback{0};          //!< Triggered CQI feedback
    uint8_t m_erPartialBandwidth{0};            //!< Extended range partial bandwidth
    uint8_t m_dlMuMimoOnPartialBandwidth{0};    //!< DL MU-MIMO on partial bandwidth
    uint8_t m_ppeThresholdPresent{0};           //!< PPE threshold present
    uint8_t m_psrBasedSrSupport{0};             //!< PSR based SR support
    uint8_t m_powerBoostFactorAlphaSupport{0};  //!< power boost factor alpha support
    uint8_t m_hePpdu4xHeLtf800nsGi{0};    //!< 4 times HE-LFT and 800ns GI support for HE-PPDUs
    uint8_t m_maxNc{0};                   //!< Max Nc for HE compressed beamforming/CQI report
    uint8_t m_stbcTxGt80MHz{0};           //!< STBC Tx > 80MHz
    uint8_t m_stbcRxGt80MHz{0};           //!< STBC RX > 80MHz
    uint8_t m_heErSuPpdu4xHeLtf08sGi{0};  //!< HE ER SU PPDU with 4x HE LTF and 0.8us GI
    uint8_t m_hePpdu20MHzIn40MHz24GHz{0}; //!< 20MHz in 40MHz HE PPDU in 2.4GHz band
    uint8_t m_hePpdu20MHzIn160MHz{0};     //!< 20MHz in 160/80+80MHz HE PPDU
    uint8_t m_hePpdu80MHzIn160MHz{0};     //!< 80MHz in 160/80+80MHz HE PPDU
    uint8_t m_heErSuPpdu1xHeLtf08Gi{0};   //!< HE ER SU PPDU with 1x HE LTF and 0.8us GI
    uint8_t m_midamble2xAnd1xHeLtf{0};    //!< Midamble TX/RX 2x and 1x HE-LTF
    uint8_t m_dcmMaxRu{0};                //!< DCM Max RU
    uint8_t m_longerThan16HeSigbOfdm{0};  //!< Longer than 16 HE SIG-=B OFDM symbols support
    uint8_t m_nonTriggeredCqiFeedback{0}; //!< Non-Triggered CQI feedback
    uint8_t m_tx1024QamLt242Ru{0};        //!< TX 1024 QAM < 242 =-tone RU support
    uint8_t m_rx1024QamLt242Ru{0};        //!< TX 1024 QAM < 242 =-tone RU support
    uint8_t m_rxFullBwSuInHeMuCompressedSigB{
        0}; //!< RX full BW SU using HE MU PPDU with compressed SIGB
    uint8_t m_rxFullBwSuInHeMuNonCompressedSigB{0}; //!< RX full BW SU using HE MU PPDU with
                                                    //!< non-compressed SIGB
    uint8_t m_nominalPacketPadding{0};              //!< Nominal packet padding
    uint8_t m_maxHeLtfRxInHeMuMoreThanOneRu{0};     ///< max HE-LTF symbols STA can Rx in HE MU PPDU
                                                    ///< with more than one RU

    // MCS and NSS field information
    uint8_t m_highestNssSupportedM1{0}; //!< highest NSS support M1
    uint8_t m_highestMcsSupported{0};   //!< highest MCS support
    std::vector<uint8_t> m_txBwMap;     //!< transmit BW map
    std::vector<uint8_t> m_rxBwMap;     //!< receive BW map
};

/**
 * output stream output operator
 * \param os the output stream
 * \param HeCapabilities the HE capabilities
 * \returns the output stream
 */
std::ostream& operator<<(std::ostream& os, const HeCapabilities& HeCapabilities);

} // namespace ns3

#endif /* HE_CAPABILITY_H */
