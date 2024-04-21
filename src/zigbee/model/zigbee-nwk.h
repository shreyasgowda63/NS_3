/*
 * Copyright (c) 2024 Tokushima University, Japan
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
 * Authors:
 *
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 *  Ryo Okuda <c611901200@tokushima-u.ac.jp>
 */

#ifndef ZIGBEE_NWK_H
#define ZIGBEE_NWK_H

#include "zigbee-nwk-fields.h"
#include "zigbee-nwk-header.h"
#include "zigbee-payload-header.h"
#include "zigbee-tables.h"

#include <ns3/event-id.h>
#include <ns3/lr-wpan-mac-base.h>
#include <ns3/mac16-address.h>
#include <ns3/mac64-address.h>
#include <ns3/object.h>
#include <ns3/random-variable-stream.h>
#include <ns3/sequence-number.h>
#include <ns3/traced-callback.h>
#include <ns3/traced-value.h>

#include <cstdint>
#include <iomanip>

namespace ns3
{

namespace zigbee
{

/**
 * \defgroup zigbee ZIGBEE models
 *
 * This section documents the API of the Zigbee Specification related models. For a generic
 * functional description, please refer to the ns-3 manual.
 */

/**
 * \ingroup zigbee
 *
 * Indicates a pending NWK primitive
 */
enum PendingPrimitiveNwk
{
    NLDE_NLME_NONE = 0,         //!< No pending primitive
    NLME_NETWORK_FORMATION = 1, //!< Pending NLME-NETWORK-FORMATION.request primitive
    NLME_DIRECT_JOIN = 2,       //!< Pending NLME-DIRECT-JOIN.request primitive
    NLME_JOIN = 3,              //!< Pending NLME-JOIN.request primitive
    NLME_START_ROUTER = 4,      //!< Pending NLME-START-ROUTER.request primitive
    NLME_NET_DISCV = 5,         //!< Pending NLME-NETWORK-DISCOVERY.request primitive
    NLME_JOIN_INDICATION = 6,   //!< Pending NLME-JOIN.indication primitive
    NLME_ROUTE_DISCOVERY = 7    //!< Pending NLME-ROUTE-DISCOVERY.request primitive
};

/**
 * \ingroup zigbee
 *
 * table 3.2 (Address Mode) NLDE-DATA-Request parameters
 */
enum ZigbeeAddressMode
{
    NO_ADDRESS = 0x00, //!< No destination address
    MCST = 0x01,       //!< Multicast Address mode
    UCST_BCST = 0x02   //!< Unicast or Broadcast address mode
};

/**
 * Use to describe the method used to assign addresses in the NWK layer.
 * See Zigbee specification r22.1.0, Table 3-58
 */
enum AddrAllocMethod
{
    DISTRIBUTED_ALLOC = 0x00, //!< Distributed address allocation
                              //!< (Zigbee Specification r22.1.0 Section 3.6.1.6)
    STOCHASTIC_ALLOC = 0x02   //!< Stochastic address allocation
                              //!< (Zigbee Specification r22.1.0 Section 3.6.1.7)
};

/**
 * Use to describe the identifier of the zigbee stack profile.
 */
enum StackProfile
{
    ZIGBEE = 0x01,    //!< Zigbee stack profile 0x01 (a.k.a. Zigbee 2006, Zigbee 2007, Zigbee)
    ZIGBEE_PRO = 0x02 //!< Zigbee stack profile 0x02 (Zigbee Pro, also known as r22.1.0, 3.0)
};

/**
 * Use to describe the parameter that controls the method of joining  the network.
 * See Zigbee specification r22.1.0, Table 3-21
 */
enum JoiningMethod
{
    ASSOCIATION = 0x00,      //!< The device is requesting to join a network through association.
    DIRECT_OR_REJOIN = 0x01, //!< The device is joining directly or rejoining using the
                             //!< orphaning procedure.
    REJOINING = 0x02,        //!< The device is joining the network using the rejoining procedure.
    CHANGE_CHANNEL = 0x03    //!< The device is to change the operational network channel to
                             //!< that identified in the ScanChannel parameter.
};

/**
 * \ingroup zigbee
 *
 *  Network layer status values
 *  Combines Zigbee Specification r22.1.0 Table 3-73 and
 *  and IEEE 802.15.4-2006 Table 78
 *
 *  Used to report the resulting status of various NWK operations.
 *
 *  Note: UNSUPPORTED_ATTRIBUTE, LIMIT_REACHED, SCAN_IN_PROGRESS
 */
enum ZigbeeNwkStatus : std::uint8_t
{
    // MAC defined status:
    SUCCESS = 0,          //!< The operation was completed successfully.
    FULL_CAPACITY = 0x01, //!< PAN at capacity. Association Status field (std. 2006, Table 83).
    ACCESS_DENIED = 0x02, //!< PAN access denied. Association Status field (std. 2006, Table 83).
    COUNTER_ERROR = 0xdb, //!< The frame counter of the received frame is invalid.
    IMPROPER_KEY_TYPE = 0xdc,       //!< The key is not allowed to be used with that frame type.
    IMPROPER_SECURITY_LEVEL = 0xdd, //!< Insufficient security level expected by the recipient.
    UNSUPPORTED_LEGACY = 0xde,      //!< Deprecated security used in IEEE 802.15.4-2003
    UNSUPPORTED_SECURITY = 0xdf,    //!< The security applied is not supported.
    BEACON_LOSS = 0xe0,             //!< The beacon was lost following a synchronization request.
    CHANNEL_ACCESS_FAILURE = 0xe1,  //!< A Tx could not take place due to activity in the CH.
    DENIED = 0xe2,                  //!< The GTS request has been denied by the PAN coordinator.
    DISABLE_TRX_FAILURE = 0xe3,     //!< The attempt to disable the transceier has failed.
    SECURITY_ERROR = 0xe4,   //!< Cryptographic process of the frame failed(FAILED_SECURITY_CHECK).
    FRAME_TOO_LONG = 0xe5,   //!< Frame more than aMaxPHYPacketSize or too large for CAP or GTS.
    INVALID_GTS = 0xe6,      //!< Missing GTS transmit or undefined direction.
    INVALID_HANDLE = 0xe7,   //!< When purging from TX queue handle was not found.
    NO_ACK = 0xe9,           //!< No acknowledgment was received after macMaxFrameRetries.
    NO_BEACON = 0xea,        //!< A scan operation failed to find any network beacons.
    NO_DATA = 0xeb,          //!<  No response data were available following a request.
    NO_SHORT_ADDRESS = 0xec, //!< Failure due to unallocated 16-bit short address.
    OUT_OF_CAP = 0xed,       //!< (Deprecated) See IEEE 802.15.4-2003
    PAN_ID_CONFLICT = 0xee,  //!<  PAN id conflict detected and informed to the coordinator.
    REALIGMENT = 0xef,       //!< A coordinator realigment command has been received.
    TRANSACTION_EXPIRED = 0xf0,  //!< The transaction expired and its information discarded.
    TRANSACTION_OVERFLOW = 0xf1, //!< There is no capacity to store the transaction.
    TX_ACTIVE = 0xf2,            //!< The transceiver was already enabled.
    UNAVAILABLE_KEY = 0xf3,      //!< Unavailable key, unknown or blacklisted.
    INVALID_ADDRESS = 0xf5,      //!< Invalid source or destination address.
    ON_TIME_TOO_LONG = 0xf6,     //!< RX enable request fail due to syms. longer than Bcn. interval
    PAST_TIME = 0xf7,            //!< Rx enable request fail due to lack of time in superframe.
    TRACKING_OFF = 0xf8,         //!< This device is currently not tracking beacons.
    INVALID_INDEX = 0xf9,      //!< A MAC PIB write failed because specified index is out of range.
    READ_ONLY = 0xfb,          //!< SET/GET request issued for a read only attribute.
    SUPERFRAME_OVERLAP = 0xfd, //!< Coordinator sperframe and this device superframe tx overlap.
    // Zigbee Specification defined status:
    INVALID_PARAMETER = 0xc1,     //!< Invalid Parameter (Zigbee specification r22.1.0)
    INVALID_REQUEST = 0xc2,       //!< Invalid request (Zigbee specification r22.1.0)
    NOT_PERMITED = 0xc3,          //!< Not permitted (Zigbee specification r22.1.0)
    STARTUP_FAILURE = 0xc4,       //!< Startup failure (Zigbee specification r22.1.0)
    ALREADY_PRESENT = 0xc5,       //!< Already present (Zigbee specification r22.1.0)
    SYNC_FAILURE = 0xc6,          //!< Sync Failure (Zigbee specification r22.1.0)
    NEIGHBOR_TABLE_FULL = 0xc7,   //!< Neighbor table full (Zigbee specification r22.1.0)
    UNKNOWN_DEVICE = 0xc8,        //!< Unknown device (Zigbee specification r22.1.0)
    UNSUPPORTED_ATTRIBUTE = 0xc9, //!< Unsupported attribute (Zigbee specification r22.1.0)
    NO_NETWORKS = 0xca,           //!< No network (Zigbee specification r22.1.0)
    MAX_FRM_COUNTER = 0xcc, //!< Max Frame counter (IEEE 802.15.4, Zigbee specification r22.1.0)
    NO_KEY = 0xcd,          //!< No Key (Zigbee specification r22.1.0)
    BAD_CCM_OUTPUT = 0xce,  //!< Bad ccm output (IEEE 802.15.4, Zigbee specification r22.1.0)
    ROUTE_DISCOVERY_FAILED = 0xd0, //!< Route discovery failed (Zigbee specification r22.1.0)
    ROUTE_ERROR = 0xd1,            //!< Route error (Zigbee specification r22.1.0)
    BT_TABLE_FULL = 0xd2,          //!< Bt table full (Zigbee specification r22.1.0)
    FRAME_NOT_BUFFERED = 0xd3,     //!< Frame not buffered (Zigbee specification r22.1.0)
    INVALID_INTERFACE = 0xd5,      //!< Invalid interface (Zigbee specification r22.1.0)
    LIMIT_REACHED = 0xd6,          //!< Limit reached during network scan (IEEE 802.15.4-2011)
    SCAN_IN_PROGRESS = 0xd7        //!< The dev was scanning during this call (IEEE 802.5.4)
};

/**
 *  \ingroup zigbee
 *
 *  Channel List Structure. See Zigbee Specification 3.2.2.2.1
 */
struct ChannelList
{
    uint8_t channelPageCount;            //!< The number of the channel page structures contained in
                                         //!< the channel list structure.
    std::vector<uint32_t> channelsField; //!< The set of channels for a given page.
                                         //!< Default = 0x07FFF800 = (CH 11-26)
                                         //!< LSB b0-b26, b27-b31 MSB = Page 0

    ChannelList()
        : channelPageCount(1),
          channelsField{0x07FFF800}
    {
    }
};

/**
 * \ingroup zigbee
 *
 * NLDE-DATA.confirm params. See Zigbee Specification 3.2.1.2
 */
struct NldeDataConfirmParams
{
    ZigbeeNwkStatus m_status{ZigbeeNwkStatus::INVALID_PARAMETER}; //!< The status of
                                                                  //!< the corresponding request.
    uint8_t m_nsduHandle{0};   //!< The handle associated with the NSDU being confirmed.
    Time m_txTime{Seconds(0)}; //!< The time indication for the transmitted packet
                               //!< based on the local clock.
};

/**
 * \ingroup lr-wpan
 *
 * NLDE-DATA.indication params. See Zigbee Specification 3.2.1.3.1
 */
struct NldeDataIndicationParams
{
    ZigbeeAddressMode m_dstAddrMode{UCST_BCST}; //!< Destination address mode.
                                                //!< 0x01=MCST, 0x02=BCST or UCST
    Mac16Address m_dstAddr;    //!< The destination address to which the NSDU was sent
    Mac16Address m_srcAddr;    //!< The individual device address from which the NSDU originated
    uint32_t m_nsduLength{0};  //!< The number of octets comprising the NSDU being indicated
    uint8_t m_linkQuality{0};  //!< LQI value delivered by the MAC on receipt of this frame
    Time m_rxTime{Seconds(0)}; //!< A time indication for the received packet
                               //!< based on the local clock
    bool m_securityUse{false}; //!< An indication of whether the received data is using security
};

/**
 * \ingroup zigbee
 *
 * NLDE-DATA.request params. See Zigbee Specification 3.2.1.1
 */
struct NldeDataRequestParams
{
    ZigbeeAddressMode m_dstAddrMode{UCST_BCST}; //!< Destination address mode.
                                                //!< 0x01=MCST, 0x02=BCST or UCST
    Mac16Address m_dstAddr;                     //!< The destination address.
    uint32_t m_nsduLength{0}; //!< The number of octets comprising the NSDU to be transferred.
    uint8_t m_nsduHandle{0};  //!< The NSDU handle
    bool m_useAlias{false}; //!< Indicates if next higher layer use an alias for the current frame.
    Mac16Address m_aliasSrcAddr;      //!< The source address to be used by this NSDU
                                      //!< (ignored ifuseAlias = false).
    SequenceNumber8 m_aliasSeqNumber; //!< The sequence number used by this NSDU
                                      //!< (ignored if useAlias = false).
    uint8_t m_radius{0};              //!< Distance in hops that the frame is allowed to
                                      //!< travel through the network.
    uint8_t m_nonMemberRadius{0}; //!< Distance in hops that a multicast frame will be relayed by
                                  //!< nodes not a member of the group. 0x07 = Infinity.
    bool m_discoverRoute{false};  //!< T: Enable Route Discovery | F: Suppress Route discovery
    bool m_securityEnable{false}; //!< Enable NWK layer security for the current frame.
};

/**
 *  \ingroup zigbee
 *
 *  NLME-NETWORK-FORMATION.request params. See Zigbee Specification 3.2.2.5.1
 */
struct NlmeNetworkFormationRequestParams
{
    ChannelList m_scanChannelList; //!< A structure
                                   //!< that contain a description on the pages and
                                   //!< their channels to be scanned.
    uint8_t m_scanDuration;        //!< The time spent of each channel in symbols:
                                   //!< aBaseSuperframeDuriantion * (2n+1). n=0-14
    uint8_t m_beaconOrder;         //!< The beacon order
    uint8_t m_superFrameOrder;     //!< The superframe order
    bool m_batteryLifeExtension;   //!< True: The zigbee coordinator is started supporting
                                   //!< battery extension mode.
    bool m_distributedNetwork;     //!< Indicates that distributed security will be used.
    Mac16Address m_distributedNetworkAddress; //!< The address of the device in a
                                              //!< distributed network.

    NlmeNetworkFormationRequestParams()
        : m_scanChannelList{},
          m_scanDuration(14),
          m_beaconOrder(15),
          m_superFrameOrder(15),
          m_batteryLifeExtension(false),
          m_distributedNetwork(false),
          m_distributedNetworkAddress("FF:F7")
    {
    }
};

/**
 * \ingroup zigbee
 *
 * A group of pending parameters arranged into a structure during the execution of
 * a NLME-NETWORK-FORMATION.request primitive.
 */
struct NetFormPendingParamsGen : public SimpleRefCount<NetFormPendingParamsGen>
{
    uint8_t channel{0}; //!< The channel selected during the initial steps of a network formation
    uint8_t page{0};    //!< The page selected during the initial steps of a network formation.
    uint16_t panId{0};  //!< The PAN id selected during the initial steps of a network formation.
};

/**
 *  \ingroup zigbee
 *
 *  NLME-NETWORK-FORMATION.confirm params. See Zigbee Specification 3.2.2.6.1
 */
struct NlmeNetworkFormationConfirmParams
{
    ZigbeeNwkStatus m_status{ZigbeeNwkStatus::INVALID_PARAMETER}; //!< The status as a result of
                                                                  //!< this request
};

/**
 *  \ingroup zigbee
 *
 *  NLME-ROUTE-DISCOVERY.request params. See Zigbee Specification 3.2.2.33
 */
struct NlmeRouteDiscoveryRequestParams
{
    ZigbeeAddressMode m_dstAddrMode{UCST_BCST}; //!< Specifies the kind of destination address.
    Mac16Address m_dstAddr;                     //!< The destination of the route discovery.
    uint16_t m_radius{0};       //!< Optional parameter that describes the number of hops that the
                                //!< route request will travel through the network.
    bool m_noRouteCache{false}; //!< This flag determines whether the NWK should establish a
                                //!< route record table.
};

/**
 *  \ingroup zigbee
 *
 *  NLME-ROUTE-DISCOVERY.confirm params. See Zigbee Specification r22.1.0, 3.2.2.34
 */
struct NlmeRouteDiscoveryConfirmParams
{
    ZigbeeNwkStatus m_status{ZigbeeNwkStatus::INVALID_PARAMETER}; //!< The status as a result of
                                                                  //!< this request.
    // TODO: here   NetworkStatusCode
};

/**
 *  \ingroup zigbee
 *
 *  NLME-DIRECT-JOIN.request params.
 *  See Zigbee Specification r22.1.0, 3.2.2.16
 */
struct NlmeDirectJoinRequestParams
{
    Mac64Address m_deviceAddr; //!< The EUI-64 bit address of the device directly joined.
    uint8_t m_capabilityInfo;  //!< The operating capabilities of the device
                               //!< being directly joined
};

/**
 *  \ingroup zigbee
 *
 *  NLME-DIRECT-JOIN.confirm params.
 *  See Zigbee Specification r22.1.0, 3.2.2.17
 */
struct NlmeDirectJoinConfirmParams
{
    ZigbeeNwkStatus m_status{ZigbeeNwkStatus::INVALID_PARAMETER}; //!< The status
                                                                  //!< the corresponding request.
    Mac64Address m_deviceAddr; //!< The IEEE EUI-64 address in the request to
                               //!< which this is a confirmation.
};

/**
 *  \ingroup zigbee
 *
 *  NLME-NETWORK-DISCOVERY.request params.
 *  See Zigbee Specification r22.1.0, 3.2.2.3
 */
struct NlmeNetworkDiscoveryRequestParams
{
    ChannelList m_scanChannelList; //!< The list of all channel pages and the associated
                                   //!< channels that shall be scanned.
    uint8_t m_scanDuration{0};     //!< A value used to calculate the length of time to spend
};

/**
 * \ingroup lr-wpan
 *
 * Network Descriptor, Zigbee Specification  r22.1.0, 3.2.2.4, Table 3-12
 */
struct NetworkDescriptor
{
    uint64_t m_extPanId{0xFFFFFFFFFFFFFFFE}; //!< The 64-bit PAN identifier of the network.
    uint16_t m_panId{0xFFFF};                //!< The 16-bit PAN identifier of the network.
    uint8_t m_updateId{0};                   //!< The value of the UpdateID from the NIB.
    uint8_t m_logCh{11}; //!< The current channel number occupied by the network.
    StackProfile m_stackProfile{ZIGBEE_PRO}; //!< The Zigbee stack profile identifier in use in
                                             //!< the discovered network.
    uint8_t m_zigbeeVersion;                 //!< The version of the zigbee protocol in use in
                                             //!< the discovered network.
    uint8_t m_beaconOrder{15};               //!< The beacon order value of the underlying MAC
                               //!< (Determinates the beacon frequency, 15 = No beacon)
    uint8_t m_superframeOrder{15};  //!< The superframe order value of the underlying MAC
                                    //!< (Determinates the value of the active period)
    bool m_permitJoining{true};     //!< TRUE = Indicates that at least one zigbee router on the
                                    //!< network currently permits joining.
    bool m_routerCapacity{true};    //!< TRUE = The device is able to accept join requests from
                                    //!< router-capable devices.
    bool m_endDeviceCapacity{true}; //!< TRUE= The device is able to accept join request from
                                    //!< end devices.
};

/**
 *  \ingroup zigbee
 *
 *  NLME-NETWORK-DISCOVERY.confirm params. See Zigbee Specification r22.1.0, 3.2.2.4
 */
struct NlmeNetworkDiscoveryConfirmParams
{
    ZigbeeNwkStatus m_status{ZigbeeNwkStatus::INVALID_PARAMETER}; //!< The status of
                                                                  //!< the corresponding request.
    uint8_t m_networkCount{0}; //!< Gives the number of networks discovered by the search
    std::vector<NetworkDescriptor> m_netDescList; //!< A list of descriptors,
                                                  //!< one for each of the networks discovered.
};

/**
 * \ingroup zigbee
 *
 * NLME-JOIN.request params.
 * See Zigbee Specification r22.1.0, 3.2.2.13
 */
struct NlmeJoinRequestParams
{
    uint64_t m_extendedPanId{1};   //!< The 64 bit PAN identifier of the
                                   //!< the network to join.
    JoiningMethod m_rejoinNetwork; //!< This parameter controls the method of joining the
                                   //!< network.
    ChannelList m_scanChannelList; //!< The list of all channel pages and the associated
                                   //!< channels that shall be scanned.
    uint8_t m_scanDuration{0};     //!< A value used to calculate the length of time to spend
                                   //!< scanning each channel.
    uint8_t m_capabilityInfo;      //!< The operating capabilities of the device
                                   //!< being directly joined (Bit map).
    bool m_securityEnable{false};  //!< If the value of RejoinNetwork is REJOINING and this
                                   //!< value is true, the device will rejoin securely.
};

/**
 *  \ingroup zigbee
 *
 *  NLME-JOIN.confirm params.
 *  See Zigbee Specification r22.1.0, 3.2.2.15
 */
struct NlmeJoinConfirmParams
{
    ZigbeeNwkStatus m_status;      //!< The status of
                                   //!< the corresponding request.
    Mac16Address m_networkAddress; //!< The 16 bit network address that was allocated
                                   //!< to this device. Equal to 0xFFFF if association
                                   //!< was unsuccessful.
    uint64_t m_extendedPanId;      //!< The extended 64 bit PAN ID for the network of which the
                                   //!< device is now a member.
    ChannelList m_channelList;     //!< The structure indicating the current channel of the
                                   //!< network that has been joined.
    bool m_enhancedBeacon;         //!< True if using enhanced beacons.
    uint8_t m_macInterfaceIndex;   //!< The value of the MAC index from nwkMacInterfaceTable.

    NlmeJoinConfirmParams()
        : m_status(ZigbeeNwkStatus::INVALID_REQUEST),
          m_networkAddress("FF:FF"),
          m_extendedPanId(1),
          m_channelList{},
          m_enhancedBeacon(false),
          m_macInterfaceIndex(0)
    {
    }
};

/**
 *  \ingroup zigbee
 *
 *  NLME-JOIN.indication params.
 *  See Zigbee Specification r22.1.0, 3.2.2.14
 */
struct NlmeJoinIndicationParams
{
    Mac16Address m_networkAddress{0xFFFF}; //!< The 16 bit network address of an entity that
                                           //!< has been added to the network.
    Mac64Address m_extendedAddress; //!< The EUI-64 bit address of an entity that has been added
                                    //!< to the network.
    uint8_t m_capabilityInfo;       //!< Specifies the operational capabilities of the
                                    //!< joining device.
    JoiningMethod m_rejoinNetwork;  //!< This parameter indicates the method used to
                                    //!< join the network.
    bool m_secureRejoin{false};     //!< True if the rejoin was performed in a secure manner.
};

/**
 * \ingroup zigbee
 *
 * Structure to store parameters used during associate process.
 */
struct AssociateParams
{
    Mac64Address extAddress; //!< The extended address of the device to which the request is send.
    uint16_t panId; //!< The PAN id used by the device to which the association request is send.
};

/**
 * \ingroup zigbee
 *
 * NLME-START-ROUTER.request params.
 * See Zigbee Specification r22.1.0, 3.2.2.13
 */
struct NlmeStartRouterRequestParams
{
    uint8_t m_beaconOrder{15};     //!< The beacon order of the network
    uint8_t m_superframeOrder{15}; //!< The superframe order of the network
    bool m_batteryLifeExt{false};  //!< True if the router supports battery life extension mode.
};

/**
 * \ingroup zigbee
 *
 * NLME-START-ROUTER.confirm params.
 * See Zigbee Specification r22.1.0, 3.2.2.10
 */
struct NlmeStartRouterConfirmParams
{
    ZigbeeNwkStatus m_status{ZigbeeNwkStatus::INVALID_REQUEST}; //!< The status of
                                                                //!< the corresponding request.
};

//////////////////////
//     Callbacks    //
//////////////////////

/**
 * \ingroup zigbee
 *
 * This callback is called after a NSDU has successfully received and
 *  NWK push it to deliver it to the next higher layer.
 *
 */
typedef Callback<void, NldeDataIndicationParams, Ptr<Packet>> NldeDataIndicationCallback;

/**
 * \ingroup zigbee
 *
 * This callback is used to notify the next higher layer with a confirmation in response to
 * a previously issued NLDE-DATA.request.
 *
 */
typedef Callback<void, NldeDataConfirmParams> NldeDataConfirmCallback;

/**
 *  \ingroup zigbee
 *
 *  This callback is used to notify the next higher layer with a confirmation in response to
 *  a previously issued NLME-NETWORK-FORMATION.request.
 */
typedef Callback<void, NlmeNetworkFormationConfirmParams> NlmeNetworkFormationConfirmCallback;

/**
 *  \ingroup zigbee
 *
 *  This callback is used to notify the next higher layer with a confirmation in response to
 *  a previously issued NLME-NETWORK-DISCOVERY.request.
 */
typedef Callback<void, NlmeNetworkDiscoveryConfirmParams> NlmeNetworkDiscoveryConfirmCallback;

/**
 *  \ingroup zigbee
 *
 *  This callback is used to notify the next higher layer with a confirmation in response to
 *  a previously issued NLME-ROUTE-DISCOVERY.request.
 */
typedef Callback<void, NlmeRouteDiscoveryConfirmParams> NlmeRouteDiscoveryConfirmCallback;

/**
 *  \ingroup zigbee
 *
 *  This callback is used to notify the next higher layer with a confirmation in response to
 *  a previously issued NLME-DIRECT-JOIN.request.
 */
typedef Callback<void, NlmeDirectJoinConfirmParams> NlmeDirectJoinConfirmCallback;

/**
 *  \ingroup zigbee
 *
 *  This callback is used to notify the next higher layer with a confirmation in response to
 *  a previously issued NLME-JOIN.request.
 */
typedef Callback<void, NlmeJoinConfirmParams> NlmeJoinConfirmCallback;

/**
 *  \ingroup zigbee
 *
 *  This callback is used to notify the next higher layer with an indication  that a new
 *  device has successfully joined its network by association or rejoining.
 */
typedef Callback<void, NlmeJoinIndicationParams> NlmeJoinIndicationCallback;

/**
 *  \ingroup zigbee
 *
 *  This callback is used to notify the next higher layer with a confirmation in response to
 *  a previously issued NLME-START-ROUTER.request.
 */
typedef Callback<void, NlmeStartRouterConfirmParams> NlmeStartRouterConfirmCallback;

/**
 * \ingroup zigbee
 *
 * Class that implements the Zigbee Specification Network Layer
 */
class ZigbeeNwk : public Object
{
  public:
    /**
     * Get the type ID.
     *
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Default constructor.
     */
    ZigbeeNwk();
    ~ZigbeeNwk() override;

    /**
     * Set the underlying MAC to use in this Zigbee NWK
     *
     * \param mac The pointer to the underlying LrWpan MAC to set to this Zigbee NWK
     */
    void SetMac(Ptr<lrwpan::LrWpanMacBase> mac);

    /**
     * Get the underlying MAC used by the current Zigbee NWK.
     *
     * \return The pointer to the underlying MAC object currently connected to the Zigbee NWK.
     */
    Ptr<lrwpan::LrWpanMacBase> GetMac();

    /**
     * Print the entries in the routing table
     *
     * \param stream The stream object used to print.
     */
    void PrintRoutingTable(Ptr<OutputStreamWrapper> stream) const;

    /**
     * Print the entries in the route discovery table
     *
     * \param stream The stream object used to print.
     */
    void PrintRouteDiscoveryTable(Ptr<OutputStreamWrapper> stream) const;

    /**
     * Print the entries in the neighbor table
     *
     * \param stream The stream object used to print.
     */
    void PrintNeighborTable(Ptr<OutputStreamWrapper> stream) const;

    /**
     *  IEEE 802.15.4-2011 section 6.3.3
     *  MCPS-DATA.indication
     *  Indicates the reception of an MSDU from MAC to NWK (receiving)
     *
     *  @param params The MCPS-DATA.indication parameters.
     *  @param msdu The set of octets forming the MSDU.
     */
    void McpsDataIndication(lrwpan::McpsDataIndicationParams params, Ptr<Packet> msdu);

    /**
     *  IEEE 802.15.4-2011 section 6.3.2
     *  MCPS-DATA.confirm
     *  Reports the results of a request to a transfer data to another device.
     *
     *  @param params The MCPS-DATA.confirm parameters.
     */
    void McpsDataConfirm(lrwpan::McpsDataConfirmParams params);

    /**
     *  IEEE 802.15.4-2011 section 6.2.10.2
     *  MLME-SCAN.confirm
     *  Reports the results of a scan request.
     *
     *  @param params The MLME-SCAN.confirm parameters.
     */
    void MlmeScanConfirm(lrwpan::MlmeScanConfirmParams params);

    /**
     * IEEE 802.15.4-2011 section
     * MlME-ASSOCIATE.confirm
     * Report the results of an associate request attempt.
     *
     *  @param params The MLME-ASSOCIATE.confirm parameters.
     */
    void MlmeAssociateConfirm(lrwpan::MlmeAssociateConfirmParams params);

    /**
     *  IEEE 802.15.4-2011 section 7.1.14.2
     *  MLME-START.confirm
     *  Reports the results of a network start request.
     *
     *  @param params The MLME-START.confirm parameters.
     */
    void MlmeStartConfirm(lrwpan::MlmeStartConfirmParams params);

    /**
     * IEEE 802.15.4-2011 section 6.2.11.2
     * MLME-SET.confirm
     * Reports the result of an attempt to change a MAC PIB attribute.
     *
     * @param params The MLME-SET.confirm params
     */
    void MlmeSetConfirm(lrwpan::MlmeSetConfirmParams params);

    /**
     * IEEE 802.15.4-2011 section 6.2.5.1
     * MLME-GET.confirm
     * Reports the result of an attempt to obtain a MAC PIB attribute.
     *
     * @param status The status as a result of a MLME-GET.request operation
     * @param id The identififier of the attribute requested
     * @param attribute The value of of the attribute requested
     */
    void MlmeGetConfirm(lrwpan::MacStatus status,
                        lrwpan::MacPibAttributeIdentifier id,
                        Ptr<lrwpan::MacPibAttributes> attribute);

    /**
     *  IEEE 802.15.4-2011 sections 6.2.7.1,
     *  Zigbee Specification r22.1.0 Section 3.6.1.4.3 (parent procedure)
     *  MLME-ORPHAN.indication
     *  Generated by the coordinator and issued to its next higher
     *  layer on receipt of an orphan notification command, as defined
     *  in 5.3.6.
     *
     *  @param params The MLME-ORPHAN.indication parameters
     */
    void MlmeOrphanIndication(lrwpan::MlmeOrphanIndicationParams params);

    /**
     *  IEEE 802.15.4-2011 section 6.2.4.2
     *  MLME-COMM-STATUS.indication
     *  Allows the MAC MLME to indicate a communication status.
     *
     *  @param params The MLME-COMM-STATUS.indication parameters
     */
    void MlmeCommStatusIndication(lrwpan::MlmeCommStatusIndicationParams params);

    /**
     *  IEEE 802.15.4-2011, Section 6.2.4.1
     *  MLME-BEACON-NOTIFY.indication
     *  Allows the MAC MLME to indicate the reception of a beacon with payload.
     *
     *  @param params The MLME-BEACON-NOTIFY.indication parameters
     */
    void MlmeBeaconNotifyIndication(lrwpan::MlmeBeaconNotifyIndicationParams params);

    /**
     *  IEEE 802.15.4-2011, Section 6.2.2.2.
     *  MLME-ASSOCIATE.indication
     *  Allows the MAC MLME to indicate the reception of an associate request
     *  on a PAN coordinator or router. In the Zigbee specification this implements
     *  the parent procedure when a device join a network through association
     *  (See Zigbee specification r22.1.0, Section 3.6.1.4.1)
     *
     *  @param params The MLME-ASSOCIATE.indication parameters
     */
    void MlmeAssociateIndication(lrwpan::MlmeAssociateIndicationParams params);

    /**
     *  Zigbee Specification r22.1.0, Section 3.2.1.1
     *  NLDE-DATA.request
     *  Request to transfer a NSDU.
     *
     *  @param params the request parameters
     *  @param nsdu the NSDU to be transmitted
     */
    void NldeDataRequest(NldeDataRequestParams params, Ptr<Packet> nsdu);

    /**
     *  Zigbee Specification r22.1.0, Section 3.2.2.5 and 3.6.1.1
     *  NLME-NETWORK-FORMATION.request
     *  Request the formation of a network in a capable device.
     *
     *  @param params the network formation request params
     */
    void NlmeNetworkFormationRequest(NlmeNetworkFormationRequestParams params);

    /**
     *  Zigbee Specification r22.1.0, section 3.2.2.33.3 and 3.6.3.5
     *  NLME-ROUTE-DISCOVERY.request
     *  Allows the next higher layer to initiate route discovery.
     *
     *  @param params the route discovery request params
     */
    void NlmeRouteDiscoveryRequest(NlmeRouteDiscoveryRequestParams params);

    /**
     *  Zigbee Specification r22.1.0, section 3.2.2.3
     *  NLME-NETWORK-DISCOVERY.request
     *  Allows the next higher layer to request that the NWK layer discover
     *  networks currently operating within the personal operating space (POS).
     *
     *  @param params the network discovery request params
     */
    void NlmeNetworkDiscoveryRequest(NlmeNetworkDiscoveryRequestParams params);

    /**
     *  Zigbee Specification r22.1.0, section 3.2.2.16 and 3.6.1.4.3
     *  NLME-DIRECT-JOIN.request
     *  Allows the next layer of a Zigbee coordinator or router to request to
     *  directly join another device to its network
     *
     *  @param params the direct join request params
     */
    void NlmeDirectJoinRequest(NlmeDirectJoinRequestParams params);

    /**
     *  Zigbee Specification r22.1.0, section 3.2.2.13
     *  NLME-JOIN.request
     *  This primitive allows the next higher layer to request to join or rejoin a
     *  network, or to change the operating channel for the device while within an
     *  operating network.
     *
     *  @param params the join request params
     */
    void NlmeJoinRequest(NlmeJoinRequestParams params);

    /**
     *  Zigbee Specification r22.1.0, section 3.2.2.9
     *  NLME-START-ROUTER.request
     *  This primitive allows the next higher layer of a Zigbee router to initiate
     *  the activities expected of a Zigbee router including the routing of data
     *  framaes, route discovery, and the accepting of request to join the network
     *  from other devices.
     *
     *  @param params the join request params
     */
    void NlmeStartRouterRequest(NlmeStartRouterRequestParams params);

    /**
     *  Set the callback for the end of a RX, as part of the
     *  interconnections between the NWK and the APS sublayer. The callback
     *  implements the callback used in a NLDE-DATA.indication.
     *  @param c the NldeDataIndication callback
     */
    void SetNldeDataIndicationCallback(NldeDataIndicationCallback c);

    /**
     *  Set the callback as part of the interconnections between the NWK and
     *  the APS sublayer (or any other higher layer). The callback
     *  implements the callback used in a NLDE-DATA.confirm
     *
     *  @param c the NldeDataConfirm callback
     */
    void SetNldeDataConfirmCallback(NldeDataConfirmCallback c);

    /**
     *  Set the callback as part of the interconnections between the NWK and the
     *  APS sublayer (or any other higher layer). The callback implements the callback
     *  used in a NLME-NETWORK-FORMATION.confirm
     *  @param c the NlmeNetworkFormationConfirm callback
     */
    void SetNlmeNetworkFormationConfirmCallback(NlmeNetworkFormationConfirmCallback c);

    /**
     *  Set the callback as part of the interconnections between the NWK and the
     *  APS sublayer (or any other higher layer). The callback implements the callback
     *  used in a NLME-NETWORK-DISCOVERY.confirm
     *  @param c the NlmeNetworkDiscoveryConfirm callback
     */
    void SetNlmeNetworkDiscoveryConfirmCallback(NlmeNetworkDiscoveryConfirmCallback c);

    /**
     *  Set the callback as part of the interconnections between the NWK and the
     *  APS sublayer (or any other higher layer). The callback implements the callback
     *  used in a NLME-ROUTE-DISCOVERY.confirm
     *
     *  @param c the NlmeRouteDiscoveryConfirm callback
     */
    void SetNlmeRouteDiscoveryConfirmCallback(NlmeRouteDiscoveryConfirmCallback c);

    /**
     *  Set the callback as part of the interconnections between the NWK and the
     *  APS sublayer (or any other higher layer). The callback implements the callback
     *  used in a NLME-DIRECT-JOIN.confirm
     *
     *  @param c the NlmeDirectJoinConfirm callback
     */
    void SetNlmeDirectJoinConfirmCallback(NlmeDirectJoinConfirmCallback c);

    /**
     *  Set the callback as part of the interconnections between the NWK and the
     *  APS sublayer (or any other higher layer). The callback implements the callback
     *  used in a NLME-JOIN.confirm
     *
     *  @param c the NlmeJoinConfirm callback
     */
    void SetNlmeJoinConfirmCallback(NlmeJoinConfirmCallback c);

    /**
     *  Set the callback as part of the interconnections between the NWK and the
     *  APS sublayer (or any other higher layer). The callback implements the callback
     *  used in a NLME-JOIN.indication
     *
     *  @param c the NlmeJoinIndication callback
     */
    void SetNlmeJoinIndicationCallback(NlmeJoinIndicationCallback c);

    /**
     *  Set the callback as part of the interconnections between the NWK and the
     *  APS sublayer (or any other higher layer). The callback implements the callback
     *  used in a NLME-START-ROUTER.confirm
     *
     *  @param c the NlmeStartRouterConfirm callback
     */
    void SetNlmeStartRouterConfirmCallback(NlmeStartRouterConfirmCallback c);

    /**
     *  Assign a fixed random variable stream number to the random variables
     *  used by this model.  Return the number of streams (possibly zero) that
     *  have been assigned.
     *
     *  \param stream first stream index to use
     *  \return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

  protected:
    void DoInitialize() override;
    void DoDispose() override;

  private:
    Ptr<lrwpan::LrWpanMacBase>
        m_mac; //!< Pointer to the underlying MAC connected to this Zigbee NWK.

    ///////////////
    // Callbacks //
    ///////////////

    /**
     *  This callback is used to notify incoming packets to the APS sublayer.
     *  See Zigbee Specification r22.1.0, section 6.2.1.3.
     */
    NldeDataIndicationCallback m_nldeDataIndicationCallback;

    /**
     *  This callback is used to respond to data PDU (NSDU) transfer
     *  request issued by APS sublayer to the NWK (or a layer higher to NWK).
     *  See Zigbee specification r22.1.0, section 3.2.1.2.
     */
    NldeDataConfirmCallback m_nldeDataConfirmCallback;

    /**
     *  This callback is used to to notify the results of a network
     *  formation to the APS sublayer making the request.
     *  See Zigbee specification r22.1.0, section 3.2.2.5
     */
    NlmeNetworkFormationConfirmCallback m_nlmeNetworkFormationConfirmCallback;

    /**
     *  This callback is used to to notify the results of a network
     *  formation to the APS sublayer making the request.
     *  See Zigbee specification r22.1.0, section 3.2.2.4
     */
    NlmeNetworkDiscoveryConfirmCallback m_nlmeNetworkDiscoveryConfirmCallback;

    /**
     *  This callback is used to to notify the results of a network
     *  formation to the APS sublayer making the request.
     *  See Zigbee specification r22.1.0, section 3.2.2.34
     */
    NlmeRouteDiscoveryConfirmCallback m_nlmeRouteDiscoveryConfirmCallback;

    /**
     *  This callback is used by the next layer of a zigbee coordinator or
     *  router to be notified of the result of its request to directly join
     *  another device to its network
     *  See Zigbee specification r22.1.0, section 3.2.2.17
     */
    NlmeDirectJoinConfirmCallback m_nlmeDirectJoinConfirmCallback;

    /**
     *  This callback is used by the next layer of a zigbee router or device
     *  to be notified of the result of its request to join
     *  another device network.
     *  See Zigbee specification r22.1.0, section 3.2.2.15
     */
    NlmeJoinConfirmCallback m_nlmeJoinConfirmCallback;

    /**
     *  This callback is used by the next layer of a zigbee coordinator or
     *  router to be notified when a new device has successfully joined its
     *  network by association or rejoined procedures.
     *  See Zigbee specification r22.1.0, section 3.2.2.14
     */
    NlmeJoinIndicationCallback m_nlmeJoinIndicationCallback;

    /**
     *  This callback is used by the next layer of a zigbee router or device
     *  to be notified of the result of its request to initiate activities
     *  as a zigbee router
     *  See Zigbee specification r22.1.0, section 3.2.2.10
     */
    NlmeStartRouterConfirmCallback m_nlmeStartRouterConfirmCallback;

    /**
     *  The parameters used during a NLME-NETWORK-FORMATION.request. These parameters
     *  are stored here while the scanning operations and network initialization
     *  procedures take place.
     */
    NlmeNetworkFormationRequestParams m_netFormParams;

    /**
     *  The values temporally stored as a result of  the initial steps of a
     *  NLME-NETWORK-FORMATION.request (i.e. after the first energy scan)
     *  Page, Channel, PanId.
     */
    Ptr<NetFormPendingParamsGen> m_netFormParamsGen;

    /**
     *  The parameters used during a NLME-JOIN.request. These parameters
     *  are stored here while the scanning operations and network joining
     *  procedures take place.
     */
    NlmeJoinRequestParams m_joinParams;

    /**
     * Temporally store the NLME-JOIN.indication parameters while the
     * join operations (asocciation) conclude in the coordinator or router.
     */
    NlmeJoinIndicationParams m_joinIndParams;

    /**
     * Temporally store parameters during the associate process that take
     * place during a NLME-JOIN.request.
     */
    AssociateParams m_associateParams;

    /**
     *  The maximum acceptable energy level used in an energy scan
     */
    uint8_t m_scanEnergyThreshold;

    /**
     *  Indicates the current primitive in use in the NWK layer.
     */
    PendingPrimitiveNwk m_pendPrimitiveNwk;

    /**
     *  The network layer neighbor table
     *  See Zigbee specification r22.1.0, 3.6.1.5
     */
    NeighborTable m_nwkNeighborTable;

    /**
     * Use to keep track of neighboring 16 bit PAN id.
     * This information is used during the Join process (Association).
     */
    PanIdTable m_panIdTable;

    /**
     *  The network layer routing table
     *  See Zigbee specification r22.1.0, 3.6.3.2
     */
    RoutingTable m_nwkRoutingTable;

    /**
     *  The network route discovery table
     *  See Zigbee specification r22.1.0, 3.6.3.2
     */
    RouteDiscoveryTable m_nwkRouteDiscoveryTable;

    /**
     *  Used by a Zigbee coordinator or router to allocate a
     *  16 bit address (A.K.A short address or network address)
     *  to its associated device upon request.
     *
     *  \return The allocated 16 bit address by this router or Zigbee coordinator.
     */
    Mac16Address AllocateNetworkAddress();

    /**
     * Updates the contents of the MAC beacon payload with information of the NWK.
     */
    void UpdateBeaconPayload();

    /**
     * Obtain the link cost based on the value of the nwkReportConstantCost.
     * If nwkReportConstantCost is True, the link will use a constant value of 7,
     * if false, it will use the LQI to obtain the link cost.
     * When the LQI option is used, the link cost is based on a non linear mapping
     * of LQI values.
     * See Zigbee specification r22.1.0, 3.6.3.1
     * See NXP Zigbee 3.0 Stack User Guide (JN-UG-3113, revision 1.5), page 108.
     *
     * \param lqi The lqi value (1-255) used to calculate the link cost.
     * \return The link cost (1-7).
     */
    uint8_t GetLinkCost(uint8_t lqi) const;

    /**
     *  Construct and send a route request command.
     *  See Zigbee specification r22.1.0, Section 3.4.1
     *
     *  \param src The source address of the originator device of the first RREQ
     *  \param dst The destination address of the RREQ
     *  \param seq The sequence number of this route request
     *  \param rreqId The RREQ identifier
     *  \param pathcost The pathcost
     *  \param radius The Optional radius parameter supplied
     */
    void SendRREQ(Mac16Address src,
                  Mac16Address dst,
                  uint8_t seq,
                  uint8_t rreqId,
                  uint8_t pathcost,
                  uint8_t radius);
    /**
     * Handles the reception of a route request command.
     * See Zigbee specification r22.1.0, Section 3.6.3.5.2
     *
     * \param macSrcAddr The MAC header source address
     * \param nwkHeader The received network Header
     * \param payload The received route request command payload
     * \param linkCost The link cost associated to the received RREQ
     */
    void ReceiveRREQ(Mac16Address macSrcAddr,
                     uint8_t linkCost,
                     ZigbeeNwkHeader nwkHeader,
                     ZigbeePayloadRouteRequestCommand payload);

    /**
     *  Construct and send a route reply command.
     *  See Zigbee specification r22.1.0, Section 3.4.2
     *
     *  \param nextHop The address of the next hop in the path back to the RREQ originator.
     *  \param originator The address of the originator device of the first RREQ.
     *  \param responder The address of the first device responding to the RREQ with a RREP.
     *  \param rreqId The RREQ identifier of the originator RREQ.
     *  \param pathcost The sum value of link costs along the way.
     */
    void SendRREP(Mac16Address nextHop,
                  Mac16Address originator,
                  Mac16Address responder,
                  uint8_t rreqId,
                  uint8_t pathcost);

    /**
     * Handles the reception of a route reply command.
     * See Zigbee specification r22.1.0, Section 3.6.3.5.3
     *
     * \param macSrcAddr The MAC source address of this reply (a.k.a. previous hop)
     * \param nwkHeader The received network Header
     * \param payload The received route reply command payload
     * \param linkCost The link cost associated to the received RREP
     */
    void ReceiveRREP(Mac16Address macSrcAddr,
                     uint8_t linkCost,
                     ZigbeeNwkHeader nwkHeader,
                     ZigbeePayloadRouteReplyCommand payload);

    /**
     *  Provides uniform random values
     */
    Ptr<UniformRandomVariable> m_uniformRandomVariable;

    /**
     *  Provides uniform random values for the route request jitter
     */
    Ptr<UniformRandomVariable> m_rreqJitter;

    /////////////////////////////
    // Network layer constants //
    /////////////////////////////

    /**
     *  Indicates whether the device is capable of becoming the ZigBee coordinator
     *  Zigbee Specification r22.1.0, Table 3-57. Defined as a constant in the specification
     *  but here is defined as variable to be able to change the devices capabilities.
     */
    bool m_nwkcCoordinatorCapable;

    /**
     *  Indicates the version of the ZigBee NWK protocol in the device.
     *  Zigbee Specification r22.1.0, Table 3-57. Defined as a constant in the specification
     *  but here is defined as variable to be able to change the devices capabilities.
     */
    uint8_t m_nwkcProtocolVersion;

    /**
     *  Indicates the duration until a route discovery expires.
     *  Zigbee Specification r22.1.0, Table 3-57. Defined as a constant in the specification
     *  but here is defined as variable to be able to change the devices capabilities.
     */
    Time m_nwkcRouteDiscoveryTime;

    /**
     * The number of times the first broadcast transmission of a route request command
     * frame is retried. Zigbee Specification r22.1.0, Table 3-57. Defined as a constant
     * in the specification but here is defined as variable to allow the selection of values
     * per device.
     */
    uint8_t m_nwkcInitialRREQRetries;

    /**
     * The number of times the broadcast transmission of a route request command frame is
     * retried on relay by an intermediate Zigbee router or coordinator.
     * Zigbee Specification r22.1.0, Table 3-57. Defined as a constant
     * in the specification but here is defined as variable to allow the selection of values
     * per device.
     */
    uint8_t m_nwkcRREQRetries;

    /**
     *  Count the number of retries this device has transmitted an RREQ
     */
    uint8_t m_countRREQRetries;

    /**
     * Duration between retries of a broadcast route request command frame.
     * Zigbee Specification r22.1.0, Table 3-57. Defined as a constant
     * in the specification but here is defined as variable to allow the selection of values
     * per device.
     */
    Time m_nwkcRREQRetryInterval;

    /**
     * Minimum Route request broadcast jitter time (msec).
     * Zigbee Specification r22.1.0, Table 3-57. Defined as a constant
     * in the specification but here is defined as variable to allow the selection of values
     * per device.
     */
    double m_nwkcMinRREQJitter;

    /**
     * Maximum Route request broadcast jitter time (msec).
     * Zigbee Specification r22.1.0, Table 3-57. Defined as a constant
     * in the specification but here is defined as variable to allow the selection of values
     * per device.
     */
    double m_nwkcMaxRREQJitter;

    //////////////////////////////
    // Network layer attributes //
    //////////////////////////////

    /**
     * A value that determines the method used to assign addresses.
     * See Zigbee specification r22.1.0, Table 3-58
     */
    AddrAllocMethod m_nwkAddrAlloc;

    /**
     * The depth a device can have.
     * Default value defined in the stack profile.
     * See Zigbee specification r22.1.0 Layer protocol implementation conformance statement (PICS)
     * and stack profiles (Section 10.4.2.1)
     */
    uint8_t m_nwkMaxDepth;

    /**
     * The number of children a device is allowed to have on its current network.
     * Default value defined in the stack profile.
     * See Zigbee PRO/2007 Layer protocol implementation conformance statement (PICS)
     * and stack profiles (Section 10.4.2.1)
     */
    uint16_t m_nwkMaxChildren;

    /**
     * The number of routers any one device is allowed to have as children. This is
     * determined by the zigbee coordinator for all devices in the network.
     * This value is not used if stochastic address allocation is used.
     * Default value defined in the stack profile.
     * See Zigbee PRO/2007 Layer protocol implementation conformance statement (PICS)
     * and stack profiles (Section 10.4.2.1)
     */
    uint16_t m_nwkMaxRouters;

    /**
     * Describes the current stack profile used in this NWK layer
     */
    StackProfile m_nwkStackProfile;

    /**
     * Indicates the index of the requested timeout field that contains the timeout
     * in minutes for any end device that does not negotiate a different timeout
     * value.
     * See Zigbee specification r22.1.0, Table 3-58 (NIB attributes)
     */
    uint16_t m_nwkEndDeviceTimeoutDefault;

    /**
     * The extended PAN identifier for the PAN of which the device is a member.
     * A value of 0 means that the extended PAN identifier is unknown.
     * See Zigbee specification r22.1.0, Table 3-58 (NIB attributes)
     */
    uint64_t m_nwkExtendedPanId;

    /**
     * The 16-bit address that the device uses to communicate with the PAN.
     * This attribute reflects the value of the MAC PIB attribute macShortAddress
     * See Zigbee specification r22.1.0, Table 3-58 (NIB attributes)
     */
    Mac16Address m_nwkNetworkAddress;

    /**
     * The EUI 64 bit IEEE address of the local device.
     * See Zigbee specification r22.1.0, Table 3-58 (NIB attributes)
     */
    Mac64Address m_nwkIeeeAddress;

    /**
     * This NIB attribute should, at all times, have the same value as macPANId .
     * See Zigbee specification r22.1.0, Table 3-58 (NIB attributes)
     */
    uint16_t m_nwkPanId;

    /**
     *
     * The behavior depends upon whether the device is a FFD or RFD.
     * For RFD, this records the information received in an End device timeout
     * response command indicating the parent information (Table 3-55).
     * For FFD, this records the device's local capabilities.
     * See Zigbee specification r22.1.0, Table 3-58 (NIB attributes)
     */
    uint8_t m_nwkParentInformation;

    /**
     * This NIB attribute contains the device capability information established
     * at network joining time.
     * See Zigbee specification r22.1.0, Table 3-58 (NIB attributes)
     */
    uint8_t m_nwkCapabilityInformation;

    /**
     * The sequence number used to identify outgoing frames
     * See Zigbee specification r22.1.0, Table 3-58 (NIB attributes)
     */
    SequenceNumber8 m_nwkSequenceNumber;

    /**
     * The counter used to identify route request commands
     */
    SequenceNumber8 m_routeRequestId;

    /**
     * This NIB attribute indicates whether the NWK layer should assume the ability to
     * use hierarchical routing.
     * True = Hierarchical routing  False = Never use hierarchical routing
     * See Zigbee specification r22.1.0, Table 3-58 (NIB attributes)
     */
    bool m_nwkUseTreeRouting;

    /**
     * If false, the NWK layer shall calculate the link cost from all neighbor nodes
     * using the LQI values reported by the MAC layer, otherwise it shall report a
     * constant value (7).
     */
    bool m_nwkReportConstantCost;

    /**
     * Describes the current route symmetry:
     * True: Routes are considered to be symmetric links. Backward and forward routes
     * are created during one-route discovery and they are identical.
     * False: Routes are not consider to be comprised of symmetric links. Only the forward
     * route is stored during route discovery.
     */
    bool m_nwkSymLink;

    /**
     * Temporarily store beacons information from POS routers and PAN coordinators
     * during a network-discovery process.
     */
    std::vector<NetworkDescriptor> m_networkDescriptorList;

    /**
     * Used to store the value of the PHY current channel.
     */
    uint8_t m_currentChannel;
};

} // namespace zigbee
} // namespace ns3

#endif /* ZIGBEE_NWK_H */
