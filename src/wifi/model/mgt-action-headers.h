/*
 * Copyright (c) 2006 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#ifndef MGT_ACTION_HEADERS_H
#define MGT_ACTION_HEADERS_H

#include "ctrl-headers.h"
#include "status-code.h"

#include "ns3/header.h"

#include <list>
#include <optional>

namespace ns3
{

class Packet;

/**
 * \ingroup wifi
 *
 * See IEEE 802.11 chapter 7.3.1.11
 * Header format: | category: 1 | action value: 1 |
 *
 */
class WifiActionHeader : public Header
{
  public:
    WifiActionHeader();
    ~WifiActionHeader() override;

    /*
     * Compatible with table 8-38 IEEE 802.11, Part11, (Year 2012)
     * Category values - see 802.11-2012 Table 8-38
     */

    /// CategoryValue enumeration
    enum CategoryValue : uint8_t // table 9-51 of IEEE 802.11-2020
    {
        SPECTRUM_MANAGEMENT = 0,
        QOS = 1,
        BLOCK_ACK = 3,
        PUBLIC = 4,
        RADIO_MEASUREMENT = 5, // Category: Radio Measurement
        MESH = 13,             // Category: Mesh
        MULTIHOP = 14,         // not used so far
        SELF_PROTECTED = 15,   // Category: Self Protected
        DMG = 16,              // Category: DMG
        FST = 18,              // Category: Fast Session Transfer
        UNPROTECTED_DMG = 20,  // Category: Unprotected DMG
        HE = 30,               // Category: He
        PROTECTED_EHT = 37,    // Category: Protected EHT
        // Since vendor specific action has no stationary Action value,the parse process is not
        // here. Refer to vendor-specific-action in wave module.
        VENDOR_SPECIFIC_ACTION = 127,
        // values 128 to 255 are illegal
    };

    /// HeActionValue enumeration
    enum HeActionValue
    {
        HE_COMPRESSED_BEAMFORMING_CQI = 0,
        QUIET_TIME_PERIOD = 1,
        OPS = 2,
    };

    /// QosActionValue enumeration
    enum QosActionValue : uint8_t
    {
        ADDTS_REQUEST = 0,
        ADDTS_RESPONSE = 1,
        DELTS = 2,
        SCHEDULE = 3,
        QOS_MAP_CONFIGURE = 4,
    };

    /**
     * Block Ack Action field values
     * See 802.11 Table 8-202
     */
    enum BlockAckActionValue : uint8_t
    {
        BLOCK_ACK_ADDBA_REQUEST = 0,
        BLOCK_ACK_ADDBA_RESPONSE = 1,
        BLOCK_ACK_DELBA = 2
    };

    /// PublicActionValue enumeration
    enum PublicActionValue : uint8_t
    {
        QAB_REQUEST = 16,
        QAB_RESPONSE = 17,
    };

    /// RadioMeasurementActionValue enumeration
    enum RadioMeasurementActionValue : uint8_t
    {
        RADIO_MEASUREMENT_REQUEST = 0,
        RADIO_MEASUREMENT_REPORT = 1,
        LINK_MEASUREMENT_REQUEST = 2,
        LINK_MEASUREMENT_REPORT = 3,
        NEIGHBOR_REPORT_REQUEST = 4,
        NEIGHBOR_REPORT_RESPONSE = 5
    };

    /// MeshActionValue enumeration
    enum MeshActionValue : uint8_t
    {
        LINK_METRIC_REPORT = 0,              // Action Value:0 in Category 13: Mesh
        PATH_SELECTION = 1,                  // Action Value:1 in Category 13: Mesh
        PORTAL_ANNOUNCEMENT = 2,             // Action Value:2 in Category 13: Mesh
        CONGESTION_CONTROL_NOTIFICATION = 3, // Action Value:3 in Category 13: Mesh
        MDA_SETUP_REQUEST =
            4, // Action Value:4 in Category 13: Mesh MCCA-Setup-Request (not used so far)
        MDA_SETUP_REPLY =
            5, // Action Value:5 in Category 13: Mesh MCCA-Setup-Reply (not used so far)
        MDAOP_ADVERTISEMENT_REQUEST =
            6, // Action Value:6 in Category 13: Mesh MCCA-Advertisement-Request (not used so far)
        MDAOP_ADVERTISEMENTS = 7,      // Action Value:7 in Category 13: Mesh (not used so far)
        MDAOP_SET_TEARDOWN = 8,        // Action Value:8 in Category 13: Mesh (not used so far)
        TBTT_ADJUSTMENT_REQUEST = 9,   // Action Value:9 in Category 13: Mesh (not used so far)
        TBTT_ADJUSTMENT_RESPONSE = 10, // Action Value:10 in Category 13: Mesh (not used so far)
    };

    /// MultihopActionValue enumeration
    enum MultihopActionValue : uint8_t
    {
        PROXY_UPDATE = 0,              // not used so far
        PROXY_UPDATE_CONFIRMATION = 1, // not used so far
    };

    /// SelfProtectedActionValue enumeration
    enum SelfProtectedActionValue : uint8_t // Category: 15 (Self Protected)
    {
        PEER_LINK_OPEN = 1,    // Mesh Peering Open
        PEER_LINK_CONFIRM = 2, // Mesh Peering Confirm
        PEER_LINK_CLOSE = 3,   // Mesh Peering Close
        GROUP_KEY_INFORM = 4,  // Mesh Group Key Inform
        GROUP_KEY_ACK = 5,     // Mesh Group Key Acknowledge
    };

    /**
     * DMG Action field values
     * See 802.11ad Table 8-281b
     */
    enum DmgActionValue : uint8_t
    {
        DMG_POWER_SAVE_CONFIGURATION_REQUEST = 0,
        DMG_POWER_SAVE_CONFIGURATION_RESPONSE = 1,
        DMG_INFORMATION_REQUEST = 2,
        DMG_INFORMATION_RESPONSE = 3,
        DMG_HANDOVER_REQUEST = 4,
        DMG_HANDOVER_RESPONSE = 5,
        DMG_DTP_REQUEST = 6,
        DMG_DTP_RESPONSE = 7,
        DMG_RELAY_SEARCH_REQUEST = 8,
        DMG_RELAY_SEARCH_RESPONSE = 9,
        DMG_MULTI_RELAY_CHANNEL_MEASUREMENT_REQUEST = 10,
        DMG_MULTI_RELAY_CHANNEL_MEASUREMENT_REPORT = 11,
        DMG_RLS_REQUEST = 12,
        DMG_RLS_RESPONSE = 13,
        DMG_RLS_ANNOUNCEMENT = 14,
        DMG_RLS_TEARDOWN = 15,
        DMG_RELAY_ACK_REQUEST = 16,
        DMG_RELAY_ACK_RESPONSE = 17,
        DMG_TPA_REQUEST = 18,
        DMG_TPA_RESPONSE = 19,
        DMG_TPA_REPORT = 20,
        DMG_ROC_REQUEST = 21,
        DMG_ROC_RESPONSE = 22
    };

    /**
     * FST Action field values
     * See 802.11ad Table 8-281x
     */
    enum FstActionValue : uint8_t
    {
        FST_SETUP_REQUEST = 0,
        FST_SETUP_RESPONSE = 1,
        FST_TEAR_DOWN = 2,
        FST_ACK_REQUEST = 3,
        FST_ACK_RESPONSE = 4,
        ON_CHANNEL_TUNNEL_REQUEST = 5
    };

    /**
     * Unprotected DMG action field values
     * See 802.11ad Table 8-281ae
     */
    enum UnprotectedDmgActionValue : uint8_t
    {
        UNPROTECTED_DMG_ANNOUNCE = 0,
        UNPROTECTED_DMG_BRP = 1,
        UNPROTECTED_MIMO_BF_SETUP = 2,
        UNPROTECTED_MIMO_BF_POLL = 3,
        UNPROTECTED_MIMO_BF_FEEDBACK = 4,
        UNPROTECTED_MIMO_BF_SELECTION = 5,
    };

    /**
     * Protected EHT action field values
     * See 802.11be D3.0 Table 9-623c
     */
    enum ProtectedEhtActionValue : uint8_t
    {
        PROTECTED_EHT_TID_TO_LINK_MAPPING_REQUEST = 0,
        PROTECTED_EHT_TID_TO_LINK_MAPPING_RESPONSE = 1,
        PROTECTED_EHT_TID_TO_LINK_MAPPING_TEARDOWN = 2,
        PROTECTED_EHT_EPCS_PRIORITY_ACCESS_ENABLE_REQUEST = 3,
        PROTECTED_EHT_EPCS_PRIORITY_ACCESS_ENABLE_RESPONSE = 4,
        PROTECTED_EHT_EPCS_PRIORITY_ACCESS_TEARDOWN = 5,
        PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION = 6,
        PROTECTED_EHT_LINK_RECOMMENDATION = 7,
        PROTECTED_EHT_MULTI_LINK_OPERATION_UPDATE_REQUEST = 8,
        PROTECTED_EHT_MULTI_LINK_OPERATION_UPDATE_RESPONSE = 9,
    };

    /**
     * typedef for union of different ActionValues
     */
    typedef union {
        QosActionValue qos;                                 ///< qos
        BlockAckActionValue blockAck;                       ///< block ack
        RadioMeasurementActionValue radioMeasurementAction; ///< radio measurement
        PublicActionValue publicAction;                     ///< public
        SelfProtectedActionValue selfProtectedAction;       ///< self protected
        MultihopActionValue multihopAction;                 ///< multi hop
        MeshActionValue meshAction;                         ///< mesh
        DmgActionValue dmgAction;                           ///< dmg
        FstActionValue fstAction;                           ///< fst
        UnprotectedDmgActionValue unprotectedDmgAction;     ///< unprotected dmg
        ProtectedEhtActionValue protectedEhtAction;         ///< protected eht
        HeActionValue he;                                   ///< he
    } ActionValue;                                          ///< the action value

    /**
     * Set action for this Action header.
     *
     * \param type category
     * \param action action
     */
    void SetAction(CategoryValue type, ActionValue action);

    /**
     * Return the category value.
     *
     * \return CategoryValue
     */
    CategoryValue GetCategory() const;
    /**
     * Return the action value.
     *
     * \return ActionValue
     */
    ActionValue GetAction() const;

    /**
     * Peek an Action header from the given packet.
     *
     * \param pkt the given packet
     * \return the category value and the action value in the peeked Action header
     */
    static std::pair<CategoryValue, ActionValue> Peek(Ptr<const Packet> pkt);

    /**
     * Remove an Action header from the given packet.
     *
     * \param pkt the given packet
     * \return the category value and the action value in the removed Action header
     */
    static std::pair<CategoryValue, ActionValue> Remove(Ptr<Packet> pkt);

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    uint8_t m_category;    //!< Category of the action
    uint8_t m_actionValue; //!< Action value
};

/**
 * \ingroup wifi
 * Implement the header for management frames of type Add Block Ack request.
 */
class MgtAddBaRequestHeader : public Header
{
  public:
    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * Enable delayed BlockAck.
     */
    void SetDelayedBlockAck();
    /**
     * Enable immediate BlockAck
     */
    void SetImmediateBlockAck();
    /**
     * Set Traffic ID (TID).
     *
     * \param tid traffic ID
     */
    void SetTid(uint8_t tid);
    /**
     * Set timeout.
     *
     * \param timeout timeout
     */
    void SetTimeout(uint16_t timeout);
    /**
     * Set buffer size.
     *
     * \param size buffer size
     */
    void SetBufferSize(uint16_t size);
    /**
     * Set the starting sequence number.
     *
     * \param seq the starting sequence number
     */
    void SetStartingSequence(uint16_t seq);
    /**
     * Enable or disable A-MSDU support.
     *
     * \param supported enable or disable A-MSDU support
     */
    void SetAmsduSupport(bool supported);

    /**
     * Return the starting sequence number.
     *
     * \return the starting sequence number
     */
    uint16_t GetStartingSequence() const;
    /**
     * Return the Traffic ID (TID).
     *
     * \return TID
     */
    uint8_t GetTid() const;
    /**
     * Return whether the Block Ack policy is immediate Block Ack.
     *
     * \return true if immediate Block Ack is being used, false otherwise
     */
    bool IsImmediateBlockAck() const;
    /**
     * Return the timeout.
     *
     * \return timeout
     */
    uint16_t GetTimeout() const;
    /**
     * Return the buffer size.
     *
     * \return the buffer size.
     */
    uint16_t GetBufferSize() const;
    /**
     * Return whether A-MSDU capability is supported.
     *
     * \return true is A-MSDU is supported, false otherwise
     */
    bool IsAmsduSupported() const;

  private:
    /**
     * Return the raw parameter set.
     *
     * \return the raw parameter set
     */
    uint16_t GetParameterSet() const;
    /**
     * Set the parameter set from the given raw value.
     *
     * \param params raw parameter set value
     */
    void SetParameterSet(uint16_t params);
    /**
     * Return the raw sequence control.
     *
     * \return the raw sequence control
     */
    uint16_t GetStartingSequenceControl() const;
    /**
     * Set sequence control with the given raw value.
     *
     * \param seqControl the raw sequence control
     */
    void SetStartingSequenceControl(uint16_t seqControl);

    uint8_t m_dialogToken{1};   //!< Not used for now
    uint8_t m_amsduSupport{1};  //!< Flag if A-MSDU is supported
    uint8_t m_policy{1};        //!< Block Ack policy
    uint8_t m_tid{0};           //!< Traffic ID
    uint16_t m_bufferSize{0};   //!< Buffer size
    uint16_t m_timeoutValue{0}; //!< Timeout
    uint16_t m_startingSeq{0};  //!< Starting sequence number
};

/**
 * \ingroup wifi
 * Implement the header for management frames of type Add Block Ack response.
 */
class MgtAddBaResponseHeader : public Header
{
  public:
    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * Enable delayed BlockAck.
     */
    void SetDelayedBlockAck();
    /**
     * Enable immediate BlockAck.
     */
    void SetImmediateBlockAck();
    /**
     * Set Traffic ID (TID).
     *
     * \param tid traffic ID
     */
    void SetTid(uint8_t tid);
    /**
     * Set timeout.
     *
     * \param timeout timeout
     */
    void SetTimeout(uint16_t timeout);
    /**
     * Set buffer size.
     *
     * \param size buffer size
     */
    void SetBufferSize(uint16_t size);
    /**
     * Set the status code.
     *
     * \param code the status code
     */
    void SetStatusCode(StatusCode code);
    /**
     * Enable or disable A-MSDU support.
     *
     * \param supported enable or disable A-MSDU support
     */
    void SetAmsduSupport(bool supported);

    /**
     * Return the status code.
     *
     * \return the status code
     */
    StatusCode GetStatusCode() const;
    /**
     * Return the Traffic ID (TID).
     *
     * \return TID
     */
    uint8_t GetTid() const;
    /**
     * Return whether the Block Ack policy is immediate Block Ack.
     *
     * \return true if immediate Block Ack is being used, false otherwise
     */
    bool IsImmediateBlockAck() const;
    /**
     * Return the timeout.
     *
     * \return timeout
     */
    uint16_t GetTimeout() const;
    /**
     * Return the buffer size.
     *
     * \return the buffer size.
     */
    uint16_t GetBufferSize() const;
    /**
     * Return whether A-MSDU capability is supported.
     *
     * \return true is A-MSDU is supported, false otherwise
     */
    bool IsAmsduSupported() const;

  private:
    /**
     * Return the raw parameter set.
     *
     * \return the raw parameter set
     */
    uint16_t GetParameterSet() const;
    /**
     * Set the parameter set from the given raw value.
     *
     * \param params raw parameter set value
     */
    void SetParameterSet(uint16_t params);

    uint8_t m_dialogToken{1};   //!< Not used for now
    StatusCode m_code{};        //!< Status code
    uint8_t m_amsduSupport{1};  //!< Flag if A-MSDU is supported
    uint8_t m_policy{1};        //!< Block ACK policy
    uint8_t m_tid{0};           //!< Traffic ID
    uint16_t m_bufferSize{0};   //!< Buffer size
    uint16_t m_timeoutValue{0}; //!< Timeout
};

/**
 * \ingroup wifi
 * Implement the header for management frames of type Delete Block Ack.
 */
class MgtDelBaHeader : public Header
{
  public:
    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();

    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * Check if the initiator bit in the DELBA is set.
     *
     * \return true if the initiator bit in the DELBA is set,
     *         false otherwise
     */
    bool IsByOriginator() const;
    /**
     * Return the Traffic ID (TID).
     *
     * \return TID
     */
    uint8_t GetTid() const;
    /**
     * Set Traffic ID (TID).
     *
     * \param tid traffic ID
     */
    void SetTid(uint8_t tid);
    /**
     * Set the initiator bit in the DELBA.
     */
    void SetByOriginator();
    /**
     * Un-set the initiator bit in the DELBA.
     */
    void SetByRecipient();

  private:
    /**
     * Return the raw parameter set.
     *
     * \return the raw parameter set
     */
    uint16_t GetParameterSet() const;
    /**
     * Set the parameter set from the given raw value.
     *
     * \param params raw parameter set value
     */
    void SetParameterSet(uint16_t params);

    uint16_t m_initiator{0};  //!< initiator
    uint16_t m_tid{0};        //!< Traffic ID
    uint16_t m_reasonCode{1}; //!< Not used for now. Always set to 1: "Unspecified reason"
};

/**
 * \ingroup wifi
 * Implement the header for Action frames of type EML Operating Mode Notification.
 */
class MgtEmlOmn : public Header
{
  public:
    MgtEmlOmn() = default;

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * EML Control field.
     */
    struct EmlControl
    {
        uint8_t emlsrMode : 1;                  //!< EMLSR Mode
        uint8_t emlmrMode : 1;                  //!< EMLMR Mode
        uint8_t emlsrParamUpdateCtrl : 1;       //!< EMLSR Parameter Update Control
        uint8_t : 5;                            //!< reserved
        std::optional<uint16_t> linkBitmap;     //!< EMLSR/EMLMR Link Bitmap
        std::optional<uint8_t> mcsMapCountCtrl; //!< MCS Map Count Control
        // TODO Add EMLMR Supported MCS And NSS Set subfield when EMLMR is supported
    };

    /**
     * EMLSR Parameter Update field.
     */
    struct EmlsrParamUpdate
    {
        uint8_t paddingDelay : 3;    //!< EMLSR Padding Delay
        uint8_t transitionDelay : 3; //!< EMLSR Transition Delay
    };

    /**
     * Set the bit position in the link bitmap corresponding to the given link.
     *
     * \param linkId the ID of the given link
     */
    void SetLinkIdInBitmap(uint8_t linkId);
    /**
     * \return the ID of the links whose bit position in the link bitmap is set to 1
     */
    std::list<uint8_t> GetLinkBitmap() const;

    uint8_t m_dialogToken{0};                             //!< Dialog Token
    EmlControl m_emlControl{};                            //!< EML Control field
    std::optional<EmlsrParamUpdate> m_emlsrParamUpdate{}; //!< EMLSR Parameter Update field
};

/**
 * \ingroup wifi
 *
 * Implement HE MIMO Control field
 */
class HeMimoControlHeader : public Header
{
  public:
    HeMimoControlHeader();
    /**
     * Constructoe
     * \param ndpaHeader NDPA header from the beamformer
     * \param aid11 AID11 of the beamformee
     */
    HeMimoControlHeader(CtrlNdpaHeader ndpaHeader, uint16_t aid11);
    HeMimoControlHeader(const HeMimoControlHeader& heMimoControlHeader);
    ~HeMimoControlHeader() override;

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    enum CsType
    {
        SU = 0,  //!< single-user channel sounding
        MU = 1,  //!< multi-user channel sounding
        CQI = 2, //!< CQI feedback
    };

    /**
     * Set Nc subfield
     * \param nc Nc subfield (the number of columns in the compressed beamforming matrix minus 1)
     */
    void SetNc(uint8_t nc);

    /**
     * Get Nc subfield
     * \return Nc subfield (the number of columns in the compressed beamforming matrix minus 1)
     */
    uint8_t GetNc() const;

    /**
     * Set Nr subfield
     * \param nr Nr subfield (the number of rows in the compressed beamforming matrix minus 1)
     */
    void SetNr(uint8_t nr);

    /**
     * Get Nr subfield
     * \return Nr subfield (the number of rows in the compressed beamforming matrix minus 1)
     */
    uint8_t GetNr() const;

    /**
     * Set BW subfield given channel bandwidth
     * \param bw channel bandwidth (20, 40, 80, or 160)
     */
    void SetBw(uint16_t bw);

    /**
     * Get channel bandwidth given BW subfield
     * \return channel bandwidth (20, 40, 80, or 160)
     */
    uint16_t GetBw() const;

    /**
     * Set Grouping subfield given subcarrier grouping parameter Ng
     * \param ng subcarrier grouping parameter Ng (4 or 16)
     */
    void SetGrouping(uint8_t ng);

    /**
     * Get subcarrier grouping parameter Ng given Grouping subfield
     * \return subcarrier grouping parameter Ng (4 or 16)
     */
    uint8_t GetNg() const;

    /**
     * Set Codebook Information subfield
     * \param codebookInfo Codebook Information subfield (If codebookInfo = 0, then codebook size of
     * (4,2) and (7,5) is used for SU and MU channel sounding, respectively. If codebookInfo = 1,
     * then codebook size of (6,4) and (9,7) is used for SU and MU channel sounding, respectively.)
     */
    void SetCodebookInfo(uint8_t codebookInfo);

    /**
     * Get Codebook Information subfield
     * \return Codebook Information subfield (If codebookInfo = 0, then codebook size of (4,2) and
     * (7,5) is used for SU and MU channel sounding, respectively. If codebookInfo = 1, then
     * codebook size of (6,4) and (9,7) is used for SU and MU channel sounding, respectively.)
     */
    uint8_t GetCodebookInfo() const;

    /**
     * Set Feedback Type subfield
     * \param feedbackType Feedback Type subfield
     */
    void SetFeedbackType(CsType feedbackType);

    /**
     * Get Feedback Type subfield
     * \return Feedback Type subfield
     */
    CsType GetFeedbackType() const;

    /**
     * Set Ru Start subfield
     * \param ruStart Ru Start subfield
     */
    void SetRuStart(uint8_t ruStart);

    /**
     * Get Ru Start subfield
     * \return Ru Start subfield
     */
    uint8_t GetRuStart() const;

    /**
     * Set Ru End subfield
     * \param ruEnd Ru End subfield
     */
    void SetRuEnd(uint8_t ruEnd);

    /**
     * Get Ru End subfield
     * \return Ru End subfield
     */
    uint8_t GetRuEnd() const;

    /**
     * Set Remaining Feedback Segments subfield
     * \param remainingFeedback Remaining Feedback Segments subfield
     */
    void SetRemainingFeedback(uint8_t remainingFeedback);

    /**
     * Get Remaining Feedback Segments subfield
     * \return Remaining Feedback Segments subfield
     */
    uint8_t GetRemainingFeedback() const;

    /**
     * Set First Feedback Segments subfield
     * \param firstFeedback First Feedback Segments subfield
     */
    void SetFirstFeedback(bool firstFeedback);

    /**
     * Get First Feedback Segments subfield
     * \return First Feedback Segments subfield
     */
    uint8_t GetFirstFeedback() const;

    /**
     * Set Sounding Dialog oken subfield
     * \param soundingDialogToken Sounding Dialog Token subfield
     */
    void SetSoundingDialogToken(uint8_t soundingDialogToken);

    /**
     * Get Sounding Dialog Token subfield
     * \return Sounding Dialog Token subfield
     */
    uint8_t GetSoundingDialogToken() const;

    /**
     * Set Disallowed Subchannel Bitmap Present subfield
     * \param present Disallowed Subchannel Bitmap Present subfield
     */
    void SetDisallowedSubchannelBitmapPresent(bool present);

    /**
     * Get Disallowed Subchannel Bitmap Present subfield
     * \return Disallowed Subchannel Bitmap Present subfield
     */
    bool GetDisallowedSubchannelBitmapPresent() const;

    /**
     * Set Disallowed Subchannel Bitmap subfield
     * \param bitmap Disallowed Subchannel Bitmap subfield
     */
    void SetDisallowedSubchannelBitmap(uint8_t bitmap);

    /**
     * Get Disallowed Subchannel Bitmap subfield
     * \return Disallowed Subchannel Bitmap subfield
     */
    uint8_t GetDisallowedSubchannelBitmap() const;

  private:
    uint8_t m_nc; //!< Indicate the number of columns Nc in a compressed beamforming feedback matrix
                  //!< and m_nc = Nc - 1
    uint8_t m_nr; //!< Indicate the number of rows Nr in a compressed beamforming feedback matrix
                  //!< and m_nr = Nr - 1
    uint8_t m_bw; //!< Channel width
    uint8_t m_grouping;     //!< Indicate subcarrier grouping. If m_grouping is 0, then Ng = 4. If
                            //!< m_grouping is 1, then Ng = 16;
    uint8_t m_codebookInfo; //!<  Codebook size subfield, (4,2) or (7,5) if m_codebookSize = 0,
                            //!<  (6,4) or (9,7) if m_codebookSize = 1
    uint8_t m_feedbackType; //!< Feedback Type: SU, MU or CQI
    uint8_t m_remainingFeedbackSegments; //!< Indicate the number of remaining feedback segments for
                                         //!< the associated HE compressed beamforming/CQI frame
    uint8_t
        m_firstFeedbackSegment; //!< Indicate whether it is the first or the only feedback segment
    uint8_t m_ruStart; //!< The starting RU index for which the HE beamformer is requesting feedback
    uint8_t m_ruEnd;   //!< The ending RU index for which the HE beamformer is requesting feedback
    uint8_t m_soundingDialogToken;               //!< Sounding Dialog Token Number
    uint8_t m_disallowedSubchannelBitmapPresent; //!< Whether the Disallowed Subchannel Bitmap
                                                 //!< subfield and a reserved field of 8 bits are
                                                 //!< present in the HE MIMO Control field
    uint8_t m_disallowedSubchannelBitmap;        //!< Disallowed Subchannel Bitmap
};

/**
 * \ingroup wifi
 *
 * Implement HE Compressed Beamforming Report field
 */
class HeCompressedBfReport : public Header
{
  public:
    HeCompressedBfReport();
    HeCompressedBfReport(const HeMimoControlHeader& heMimoControlHeader);
    ~HeCompressedBfReport() override;

    // Channel information
    struct ChannelInfo
    {
        std::vector<uint8_t> m_stStreamSnr; //!< Average SNR of space-time streams
        std::vector<std::vector<uint16_t>>
            m_phi; //!< Phi angle (number of subcarriers * number of angles)
        std::vector<std::vector<uint16_t>>
            m_psi; //!< Psi angle (number of subcarriers * number of angles)
    };

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    static std::tuple<uint8_t, uint8_t> GetAngleBits(
        const HeMimoControlHeader& heMimoControlHeader);

    /**
     * Set necessary parameters to build compressed beamforming report given HE MIMO Control header
     * \param heMimoControlHeader HE MIMO Control header
     */
    void SetHeMimoControlHeader(const HeMimoControlHeader& heMimoControlHeader);

    /**
     * Set channel information to build compressed beamforming
     * \param channelInfo channel information
     */
    void SetChannelInfo(ChannelInfo channelInfo);

    /**
     * Get channel information
     * \return channelInfo channel information
     */
    ChannelInfo GetChannelInfo();

    /**
     * Get the number of columns Nc in a compressed beamforming feedback matrix
     * \return the number of columns Nc in a compressed beamforming feedback matrix
     */
    uint8_t GetNc() const;

    /**
     * Get the number of rows Nr in a compressed beamforming feedback matrix
     * \return the number of rows Nr in a compressed beamforming feedback matrix
     */
    uint8_t GetNr() const;

    /**
     * Get the number of subcarriers in compressed beamforming report
     * \return the number of subcarriers
     */
    uint16_t GetNs() const;

    /**
     * Get the number of angles (Phi and Psi) on each subcarrier in compressed beamforming report
     * \return the number of angles
     */
    uint8_t GetNa() const;

    /**
     * Get the number of bits to quantize Phi angles
     * \return the number of bits to quantize Phi angles
     */
    uint8_t GetBits1() const;

    /**
     * Get the number of bits to quantize Psi angles
     * \return the number of bits to quantize Psi angles
     */
    uint8_t GetBits2() const;

    /**
     * Prepare necessary information to serialize channel information into compressed beamforming
     * report \return the tuple containing information to serialize channel information: a vector of
     * values arranges in order that will be serialized, a vector of bits used to quantize each
     * value
     */
    std::tuple<std::vector<uint16_t>, std::vector<uint8_t>> PrepareWriteBfBuffer() const;

    /**
     * Write beamforming report information into a buffer
     * \param values a vector of values that will be written into a buffer
     * \param bits a vector of number of bits corresponding to each value in the "values" vector
     *
     * \return a buffer containing beamforming report
     */
    std::vector<uint8_t> WriteBfReportBuffer(std::vector<uint16_t> values,
                                             std::vector<uint8_t> bits) const;

    /**
     * Read beamforming report information and store channel information in m_channelInfo
     * \param buffer a vector containing channel information values
     *
     */
    void ReadChannelInfoFromBuffer(std::vector<uint8_t> buffer);

    /**
     * Read beamforming report information
     * \param buffer a vector containing beamforming report
     * \param bits a vector of number of bits arranged in order which is used to quantize each value
     * in the beamforming report \param idxBuffer the index of byte where to start reading the
     * buffer
     *
     * \return an vector of values read from the buffer
     */
    std::vector<uint16_t> ReadBfReportBuffer(std::vector<uint8_t> buffer,
                                             std::vector<uint8_t> bits,
                                             uint16_t idxBuffer) const;

    /**
     * Generate number of angles on each subcarrier in beamforming report matrix
     *
     * \param nc number of columns in a compressed beamforming feedback matrix
     * \param nr number of rows in a compressed beamforming feedback matrix
     */
    static uint8_t CalculateNa(uint8_t nc, uint8_t nr);

    /**
     * Generate number of subcarriers that are required for CSI feedback
     *
     * \param bandwidth bandwidth of the channel that are required for CSI feedback
     * \param ng subcarrier grouping parameter (4 or 16)
     */
    static uint16_t GetNSubcarriers(uint8_t ruStart, uint8_t ruEnd, uint8_t ng);

  private:
    uint8_t m_nc;    //!< The number of columns Nc in a compressed beamforming feedback matrix
    uint8_t m_nr;    //!< The number of rows Nr in a compressed beamforming feedback matrix
    uint8_t m_na;    //!< The number of compressed angles
    uint16_t m_ns;   //!< The number of subcarriers
    uint8_t m_bits1; //!< The `number of bits representing angle Phi
    uint8_t m_bits2; //!< The number of bits representing angle Psis
    ChannelInfo m_channelInfo; //!< Channel information
};

class HeMuExclusiveBfReport : public Header
{
  public:
    HeMuExclusiveBfReport();
    HeMuExclusiveBfReport(const HeMimoControlHeader& heMimoControlHeader);
    ~HeMuExclusiveBfReport() override;

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * Set Delta SNR information for the HE MU Exclusive beamforming report
     * \param deltaSnr Delta SNR information
     */
    void SetDeltaSnr(std::vector<std::vector<uint8_t>> deltaSnr);

    /**
     * Get Delta SNR information for the HE MU Exclusive beamforming report
     * \return deltaSnr Delta SNR information
     */
    std::vector<std::vector<uint8_t>> GetDeltaSnr();

  private:
    uint8_t m_nc;  //!< The number of columns Nc in a compressed beamforming feedback matrix
    uint16_t m_ns; //!< The number of subcarriers
    std::vector<std::vector<uint8_t>>
        m_deltaSnr; //!< Delta SNR information for each space-time stream
};

} // namespace ns3

#endif /* MGT_ACTION_HEADERS_H */
