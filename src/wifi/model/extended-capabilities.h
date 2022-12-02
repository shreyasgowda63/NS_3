/*
 * Copyright (c) 2017
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
 * Authors: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef EXTENDED_CAPABILITIES_H
#define EXTENDED_CAPABILITIES_H

#include "wifi-information-element.h"

namespace ns3
{

/**
 * \brief The Extended Capabilities Information Element
 * \ingroup wifi
 *
 * This class knows how to serialise and deserialise the Extended Capabilities Information Element
 */
class ExtendedCapabilities : public WifiInformationElement
{
  public:
    ExtendedCapabilities();

    // Implementations of pure virtual methods of WifiInformationElement
    WifiInformationElementId ElementId() const override;
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
    /**
     * Set the HT Supported flag.
     *
     * \param htSupported flag whether HT is supported
     */
    void SetHtSupported(uint8_t htSupported);
    /**
     * Set the VHT Supported flag.
     *
     * \param vhtSupported flag whether VHT is supported
     */
    void SetVhtSupported(uint8_t vhtSupported);

    /**
     * Set the first byte in the Extended Capabilities information element.
     *
     * \param ctrl the first byte in the Extended Capabilities information element
     */
    void SetExtendedCapabilitiesByte1(uint8_t ctrl);
    /**
     * Set the second byte in the Extended Capabilities information element.
     *
     * \param ctrl the second byte in the Extended Capabilities information element
     */
    void SetExtendedCapabilitiesByte2(uint8_t ctrl);
    /**
     * Set the third byte in the Extended Capabilities information element.
     *
     * \param ctrl the third byte in the Extended Capabilities information element
     */
    void SetExtendedCapabilitiesByte3(uint8_t ctrl);
    /**
     * Set the fourth byte in the Extended Capabilities information element.
     *
     * \param ctrl the fourth byte in the Extended Capabilities information element
     */
    void SetExtendedCapabilitiesByte4(uint8_t ctrl);
    /**
     * Set the fifth byte in the Extended Capabilities information element.
     *
     * \param ctrl the fifth byte in the Extended Capabilities information element
     */
    void SetExtendedCapabilitiesByte5(uint8_t ctrl);
    /**
     * Set the sixth byte in the Extended Capabilities information element.
     *
     * \param ctrl the sixth byte in the Extended Capabilities information element
     */
    void SetExtendedCapabilitiesByte6(uint8_t ctrl);
    /**
     * Set the seventh byte in the Extended Capabilities information element.
     *
     * \param ctrl the seventh byte in the Extended Capabilities information element
     */
    void SetExtendedCapabilitiesByte7(uint8_t ctrl);
    /**
     * Set the eighth byte in the Extended Capabilities information element.
     *
     * \param ctrl the eighth byte in the Extended Capabilities information element
     */
    void SetExtendedCapabilitiesByte8(uint8_t ctrl);

    /**
     * Return the first byte in the Extended Capabilities information element.
     *
     * \return the first byte in the Extended Capabilities information element
     */
    uint8_t GetExtendedCapabilitiesByte1() const;
    /**
     * Return the second byte in the Extended Capabilities information element.
     *
     * \return the second byte in the Extended Capabilities information element
     */
    uint8_t GetExtendedCapabilitiesByte2() const;
    /**
     * Return the third byte in the Extended Capabilities information element.
     *
     * \return the third byte in the Extended Capabilities information element
     */
    uint8_t GetExtendedCapabilitiesByte3() const;
    /**
     * Return the fourth byte in the Extended Capabilities information element.
     *
     * \return the fourth byte in the Extended Capabilities information element
     */
    uint8_t GetExtendedCapabilitiesByte4() const;
    /**
     * Return the fifth byte in the Extended Capabilities information element.
     *
     * \return the fifth byte in the Extended Capabilities information element
     */
    uint8_t GetExtendedCapabilitiesByte5() const;
    /**
     * Return the sixth byte in the Extended Capabilities information element.
     *
     * \return the sixth byte in the Extended Capabilities information element
     */
    uint8_t GetExtendedCapabilitiesByte6() const;
    /**
     * Return the seventh byte in the Extended Capabilities information element.
     *
     * \return the seventh byte in the Extended Capabilities information element
     */
    uint8_t GetExtendedCapabilitiesByte7() const;
    /**
     * Return the eighth byte in the Extended Capabilities information element.
     *
     * \return the eighth byte in the Extended Capabilities information element
     */
    uint8_t GetExtendedCapabilitiesByte8() const;

  private:
    // fields if HT supported
    uint8_t m_20_40_bssCoexistenceManagementSupport{
        0};                                ///< 20/40 BSS Coexistence Management Support
    uint8_t m_extendedChannelSwitching{0}; ///< Extended Channel Switching
    uint8_t m_psmpCapability{0};           ///< PSMP Capability
    uint8_t m_spsmpSupport{0};             ///< S-PSMP Support

    // fields if VHT supported
    uint8_t m_event{0};                           ///< Event
    uint8_t m_diagnostics{0};                     ///< Diagnostics
    uint8_t m_multicastDiagnostics{0};            ///< Multicast Diagnostics
    uint8_t m_locationTracking{0};                ///< Location Tracking
    uint8_t m_fms{0};                             ///< FMS
    uint8_t m_proxyArpService{0};                 ///< Proxy ARP Service
    uint8_t m_collocatedInterferenceReporting{0}; ///< Collocated Interference Reporting
    uint8_t m_civicLocation{0};                   ///< Civic Location
    uint8_t m_geospatialLocation{0};              ///< Geospatial Location

    uint8_t m_tfs{0};                  ///< TFS
    uint8_t m_wnmSleepMode{0};         ///< WNM Sleep Mode
    uint8_t m_timBroadcast{0};         ///< TIM Broadcast
    uint8_t m_bssTransition{0};        ///< BSS Transition
    uint8_t m_qosTrafficCapability{0}; ///< QoS Traffic Capability
    uint8_t m_acStationCount{0};       ///< AC Station Count
    uint8_t m_multipleBssid{0};        ///< Multiple BSSID
    uint8_t m_timingMeasurement{0};    ///< Timing Measurement

    uint8_t m_channelUsage{0};         ///< Channel Usage
    uint8_t m_ssidList{0};             ///< SSID List
    uint8_t m_dms{0};                  ///< DMS
    uint8_t m_utcTsfOffset{0};         ///< UTC TSF Offset
    uint8_t m_tpuBufferStaSupport{0};  ///< TPU Buffer STA Support
    uint8_t m_tdlsPeerPsmSupport{0};   ///< TDLS Peer PSM Support
    uint8_t m_tdlsChannelSwitching{0}; ///< TDLS Channel Switching
    uint8_t m_interworking{0};         ///< Interworking

    uint8_t m_qosMap{0};                         ///< QoS Map
    uint8_t m_ebr{0};                            ///< EBR
    uint8_t m_sspnInterface{0};                  ///< SSPN Interface
    uint8_t m_msgcfCapability{0};                ///< MSGCF Capability
    uint8_t m_tdlsSupport{0};                    ///< TDLS Support
    uint8_t m_tdlsProhibited{0};                 ///< TDLS Prohibited
    uint8_t m_tdlsChannelSwitchingProhibited{0}; ///< TDLS Channel Switching Prohibited

    uint8_t m_rejectUnadmittedFrame{0};      ///< Reject Unadmitted Frame
    uint8_t m_serviceIntervalGranularity{0}; ///< Service Interval Granularity
    uint8_t m_identifierLocation{0};         ///< Identifier Location
    uint8_t m_uapsdCoexistence{0};           ///< U-APSD Coexistence
    uint8_t m_wnmNotification{0};            ///< WNM Notification
    uint8_t m_qabCapability{0};              ///< QAB Capability

    uint8_t m_utf8Ssid{0};                    ///< UTF-8 SSID
    uint8_t m_qmfActivated{0};                ///< QMFActivated
    uint8_t m_qmfReconfigurationActivated{0}; ///< QMFReconfigurationActivated
    uint8_t m_robustAvStreaming{0};           ///< Robust AV Streaming
    uint8_t m_advancedGcr{0};                 ///< Advanced GCR
    uint8_t m_meshGcr{0};                     ///< Mesh GCR
    uint8_t m_scs{0};                         ///< SCS
    uint8_t m_qloadReport{0};                 ///< QLoad Report

    uint8_t m_alternateEdca{0};              ///< Alternate EDCA
    uint8_t m_unprotectedTxopNegotiation{0}; ///< Unprotected TXOP Negotiation
    uint8_t m_protectedTxopNegotiation{0};   ///< Protected TXOP Negotiation
    uint8_t m_protectedQloadReport{0};       ///< Protected QLoad Report
    uint8_t m_tdlsWiderBandwidth{0};         ///< TDLS Wider Bandwidth
    uint8_t m_operatingModeNotification{0};  ///< Operating Mode Notification
    uint8_t m_maxNumberOfMsdusInAmsdu{0};    ///< Max Number Of MSDUs In A-MSDU

    uint8_t m_htSupported{0};  ///< Flag to indicate HT is supported in order to decide whether this
                               ///< element should be added to the frame or not
    uint8_t m_vhtSupported{0}; ///< Flag to indicate VHT is supported in order to decide whether
                               ///< this element should be added to the frame or not
};

/**
 * output stream output operator
 *
 * \param os output stream
 * \param extendedCapabilities the extended capabilities
 *
 * \returns output stream
 */
std::ostream& operator<<(std::ostream& os, const ExtendedCapabilities& extendedCapabilities);

} // namespace ns3

#endif /* EXTENDED_CAPABILITIES_H */
