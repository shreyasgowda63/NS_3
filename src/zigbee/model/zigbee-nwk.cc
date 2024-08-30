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

#include "zigbee-nwk.h"

#include "zigbee-tables.h"

#include <ns3/log.h>
#include <ns3/packet.h>
#include <ns3/simulator.h>

using namespace ns3::lrwpan;

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT                                                                      \
    std::clog << "[" << m_nwkNetworkAddress << " | " << m_nwkIeeeAddress << "] ";

namespace ns3
{
namespace zigbee
{

NS_LOG_COMPONENT_DEFINE("ZigbeeNwk");
NS_OBJECT_ENSURE_REGISTERED(ZigbeeNwk);

TypeId
ZigbeeNwk::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ZigbeeNwk")
            .SetParent<Object>()
            .SetGroupName("Zigbee")
            .AddConstructor<ZigbeeNwk>()
            .AddAttribute("nwkcCoordinatorCapable",
                          "[Constant] Indicates whether the device is capable of becoming a"
                          "Zigbee coordinator.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&ZigbeeNwk::m_nwkcCoordinatorCapable),
                          MakeBooleanChecker())
            .AddAttribute("nwkcProtocolVersion",
                          "[Constant] The version of the Zigbee NWK protocol in the device",
                          UintegerValue(0x02),
                          MakeUintegerAccessor(&ZigbeeNwk::m_nwkcProtocolVersion),
                          MakeUintegerChecker<uint8_t>())

            .AddAttribute("nwkcRouteDiscoveryTime",
                          "[Constant] The duration until a route discovery expires",
                          TimeValue(MilliSeconds(0x2710)),
                          MakeTimeAccessor(&ZigbeeNwk::m_nwkcRouteDiscoveryTime),
                          MakeTimeChecker())

            .AddAttribute("nwkcInitialRREQRetries",
                          "[Constant] The number of times the first broadcast transmission"
                          "of a RREQ cmd frame is retried.",
                          UintegerValue(0x03),
                          MakeUintegerAccessor(&ZigbeeNwk::m_nwkcInitialRREQRetries),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("nwkcRREQRetries",
                          "[Constant] The number of times the broadcast transmission of a"
                          "RREQ cmd frame is retried on relay by intermediate router or"
                          "coordinator.",
                          UintegerValue(0x02),
                          MakeUintegerAccessor(&ZigbeeNwk::m_nwkcRREQRetries),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("nwkcRREQRetryInterval",
                          "[Constant] The duration between retries of a broadcast RREQ "
                          "cmd frame.",
                          TimeValue(MilliSeconds(0xFE)),
                          MakeTimeAccessor(&ZigbeeNwk::m_nwkcRREQRetryInterval),
                          MakeTimeChecker())
            .AddAttribute("nwkcMinRREQJitter",
                          "[Constant] The minimum jitter for broadcast retransmission "
                          "of a RREQ (msec)",
                          DoubleValue(2),
                          MakeDoubleAccessor(&ZigbeeNwk::m_nwkcMinRREQJitter),
                          MakeDoubleChecker<double>())
            .AddAttribute("nwkcMaxRREQJitter",
                          "[Constant] The duration between retries of a broadcast RREQ (msec)",
                          DoubleValue(128),
                          MakeDoubleAccessor(&ZigbeeNwk::m_nwkcMaxRREQJitter),
                          MakeDoubleChecker<double>())
            .AddAttribute("maxPendingTxQueueSize",
                          "The maximum size of the table storing pending packets awaiting "
                          "to be transmitted after discovering a route to the destination.",
                          UintegerValue(10),
                          MakeUintegerAccessor(&ZigbeeNwk::m_maxPendingTxQueueSize),
                          MakeUintegerChecker<uint32_t>())
            .AddTraceSource("rreqRetriesExhausted",
                            "Trace source indicating when a node has "
                            "reached the maximum allowed number of RREQ retries during a "
                            "route discovery request",
                            MakeTraceSourceAccessor(&ZigbeeNwk::m_rreqRetriesExhaustedTrace),
                            "ns3::ZigbeeNwk::RreqRetriesExhaustedTracedCallback");
    return tid;
}

ZigbeeNwk::ZigbeeNwk()
{
    m_scanEnergyThreshold = 127;
    m_pendPrimitiveNwk = NLDE_NLME_NONE;
    m_netFormParams = {};
    m_netFormParamsGen = nullptr;
    m_nwkNetworkAddress = Mac16Address("ff:ff");
    m_nwkPanId = 0xffff;
    m_nwkExtendedPanId = 0xffffffffffffffff;
    m_nwkCapabilityInformation = 0;

    m_nwkStackProfile = ZIGBEE_PRO;
    m_nwkAddrAlloc = STOCHASTIC_ALLOC;
    m_nwkMaxDepth = 5;
    m_nwkMaxChildren = 20;
    m_nwkMaxRouters = 6;
    m_nwkEndDeviceTimeoutDefault = 8;
    m_nwkUseTreeRouting = false;

    m_nwkReportConstantCost = false;
    m_nwkSymLink = false;

    m_nwkMaxBroadcastRetries = 0x03;

    m_countRREQRetries = 0;

    m_nwkIsConcentrator = false;
    m_nwkConcentratorRadius = 5;
    m_nwkConcentratorDiscoveryTime = 0x00;

    m_uniformRandomVariable = CreateObject<UniformRandomVariable>();
    m_uniformRandomVariable->SetAttribute("Min", DoubleValue(0.0));
    m_uniformRandomVariable->SetAttribute("Max", DoubleValue(255.0));
    m_nwkSequenceNumber = SequenceNumber8(m_uniformRandomVariable->GetValue());
    m_routeRequestId = SequenceNumber8(m_uniformRandomVariable->GetValue());
    m_dataHandle = SequenceNumber8(m_uniformRandomVariable->GetValue());

    m_rreqJitter = CreateObject<UniformRandomVariable>();
    m_rreqJitter->SetAttribute("Min", DoubleValue(m_nwkcMinRREQJitter));
    m_rreqJitter->SetAttribute("Max", DoubleValue(m_nwkcMaxRREQJitter));
}

ZigbeeNwk::~ZigbeeNwk()
{
}

void
ZigbeeNwk::DoInitialize()
{
    Object::DoInitialize();
}

void
ZigbeeNwk::DoDispose()
{
    m_panIdTable.Dispose();
    m_nwkNeighborTable.Dispose();
    m_nwkRoutingTable.Dispose();
    m_nwkRouteDiscoveryTable.Dispose();
    m_rreqRetryTable.Dispose();

    m_nlmeDirectJoinConfirmCallback = MakeNullCallback<void, NlmeDirectJoinConfirmParams>();
    m_nlmeJoinConfirmCallback = MakeNullCallback<void, NlmeJoinConfirmParams>();
    m_nlmeJoinIndicationCallback = MakeNullCallback<void, NlmeJoinIndicationParams>();
    m_nlmeNetworkDiscoveryConfirmCallback =
        MakeNullCallback<void, NlmeNetworkDiscoveryConfirmParams>();
    m_nlmeNetworkFormationConfirmCallback =
        MakeNullCallback<void, NlmeNetworkFormationConfirmParams>();
    m_nlmeRouteDiscoveryConfirmCallback = MakeNullCallback<void, NlmeRouteDiscoveryConfirmParams>();
    m_nlmeStartRouterConfirmCallback = MakeNullCallback<void, NlmeStartRouterConfirmParams>();

    m_nldeDataConfirmCallback = MakeNullCallback<void, NldeDataConfirmParams>();
    m_nldeDataIndicationCallback = MakeNullCallback<void, NldeDataIndicationParams, Ptr<Packet>>();

    m_mac = nullptr;

    Object::DoDispose();
}

void
ZigbeeNwk::SetMac(Ptr<LrWpanMacBase> mac)
{
    m_mac = mac;
    // Update IEEE Nwk Address
    m_mac->MlmeGetRequest(MacPibAttributeIdentifier::macExtendedAddress);
}

Ptr<LrWpanMacBase>
ZigbeeNwk::GetMac()
{
    return m_mac;
}

void
ZigbeeNwk::PrintRoutingTable(Ptr<OutputStreamWrapper> stream) const
{
    std::ostream* os = stream->GetStream();
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    std::ostringstream nwkAddr;
    std::ostringstream ieeeAddr;

    nwkAddr << m_nwkNetworkAddress;
    ieeeAddr << m_nwkIeeeAddress;

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);
    *os << "[" << ieeeAddr.str() << " | " << nwkAddr.str() << "] | ";
    *os << "Time: " << Simulator::Now().As(Time::S) << " | ";
    m_nwkRoutingTable.Print(stream);
}

void
ZigbeeNwk::PrintRouteDiscoveryTable(Ptr<OutputStreamWrapper> stream)
{
    std::ostream* os = stream->GetStream();
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    std::ostringstream nwkAddr;
    std::ostringstream ieeeAddr;

    nwkAddr << m_nwkNetworkAddress;
    ieeeAddr << m_nwkIeeeAddress;

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);
    *os << "[" << ieeeAddr.str() << " | " << nwkAddr.str() << "] | ";
    *os << "Time: " << Simulator::Now().As(Time::S) << " | ";
    m_nwkRouteDiscoveryTable.Print(stream);
}

void
ZigbeeNwk::PrintNeighborTable(Ptr<OutputStreamWrapper> stream) const
{
    std::ostream* os = stream->GetStream();
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    std::ostringstream nwkAddr;
    std::ostringstream ieeeAddr;

    nwkAddr << m_nwkNetworkAddress;
    ieeeAddr << m_nwkIeeeAddress;

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);
    *os << "[" << ieeeAddr.str() << " | " << nwkAddr.str() << "] | ";
    *os << "Time: " << Simulator::Now().As(Time::S) << " | ";
    m_nwkNeighborTable.Print(stream);
}

void
ZigbeeNwk::PrintRREQRetryTable(Ptr<OutputStreamWrapper> stream) const
{
    std::ostream* os = stream->GetStream();
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    std::ostringstream nwkAddr;
    std::ostringstream ieeeAddr;

    nwkAddr << m_nwkNetworkAddress;
    ieeeAddr << m_nwkIeeeAddress;

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);
    *os << "[" << ieeeAddr.str() << " | " << nwkAddr.str() << "] | ";
    *os << "Time: " << Simulator::Now().As(Time::S) << " | ";
    m_rreqRetryTable.Print(stream);
}

Mac16Address
ZigbeeNwk::FindRoute(Mac16Address dst, bool& neighbor)
{
    Ptr<NeighborTableEntry> neighborEntry;
    if (m_nwkNeighborTable.LookUpEntry(dst, neighborEntry))
    {
        neighbor = true;
        return dst;
    }

    Ptr<RoutingTableEntry> entry;
    if (m_nwkRoutingTable.LookUpEntry(dst, entry))
    {
        if (entry->GetStatus() == ROUTE_ACTIVE)
        {
            // std::cout <<"IN FIND ROUTE at " << m_nwkNetworkAddress<<", next hop
            // "<<entry->GetNextHopAddr()<<"\n";
            neighbor = false;
            return entry->GetNextHopAddr();
        }
    }

    neighbor = false;
    return Mac16Address("FF:FF"); // route not found
}

Mac16Address
ZigbeeNwk::GetNetworkAddress()
{
    return m_nwkNetworkAddress;
}

Mac64Address
ZigbeeNwk::GetIeeeAddress()
{
    return m_nwkIeeeAddress;
}

void
ZigbeeNwk::McpsDataIndication(McpsDataIndicationParams params, Ptr<Packet> msdu)
{
    NS_LOG_FUNCTION(this);

    ZigbeeNwkHeader nwkHeader;
    msdu->RemoveHeader(nwkHeader);

    // Decrease the radius in the network header as it might be retransmitted
    // to a next hop.
    uint8_t radius = nwkHeader.GetRadius();
    nwkHeader.SetRadius(radius - 1);

    // Check if the received frame is from a neighbor and update LQI if necessary
    Ptr<NeighborTableEntry> neighborEntry;
    if (m_nwkNeighborTable.LookUpEntry(nwkHeader.GetSrcAddr(), neighborEntry))
    {
        neighborEntry->SetLqi(params.m_mpduLinkQuality);
        neighborEntry->SetOutgoingCost(GetLQINonLinearValue(params.m_mpduLinkQuality));
    }

    switch (nwkHeader.GetFrameType())
    {
    case DATA:
        if (nwkHeader.IsMulticast())
        {
            NS_LOG_ERROR("Multicast DATA transmission not supported");
        }
        else
        {
            if (IsBroadcastAddress(nwkHeader.GetDstAddr()))
            {
                // TODO check if you can rebroadcast.

                if (!m_nldeDataIndicationCallback.IsNull())
                {
                    NldeDataIndicationParams dataParams;
                    dataParams.m_srcAddr = nwkHeader.GetSrcAddr();
                    dataParams.m_dstAddr = nwkHeader.GetDstAddr();
                    dataParams.m_dstAddrMode = UCST_BCST;
                    dataParams.m_linkQuality = params.m_mpduLinkQuality;
                    dataParams.m_nsduLength = msdu->GetSize();
                    dataParams.m_rxTime = Simulator::Now();
                    dataParams.m_securityUse = false;
                    m_nldeDataIndicationCallback(dataParams, msdu);
                }
            }
            else
            {
                if (nwkHeader.GetDstAddr() == m_nwkNetworkAddress)
                {
                    // Zigbee specification r22.1.0 Sections 3.6.2.2 and 3.6.3.3
                    if (!m_nldeDataIndicationCallback.IsNull())
                    {
                        NldeDataIndicationParams dataParams;
                        dataParams.m_srcAddr = nwkHeader.GetSrcAddr();
                        dataParams.m_dstAddr = nwkHeader.GetDstAddr();
                        dataParams.m_dstAddrMode = UCST_BCST;
                        dataParams.m_linkQuality = params.m_mpduLinkQuality;
                        dataParams.m_nsduLength = msdu->GetSize();
                        dataParams.m_rxTime = Simulator::Now();
                        dataParams.m_securityUse = false;
                        m_nldeDataIndicationCallback(dataParams, msdu);
                    }
                }
                else
                {
                    // Add again the network header to the DATA packet and
                    // route the packet to the next hop
                    msdu->AddHeader(nwkHeader);
                    SendUnicast(msdu, m_dataHandle.GetValue());
                    m_dataHandle++;
                }
            }
        }
        break;
    case NWK_COMMAND: {
        ZigbeePayloadType payloadType;
        msdu->RemoveHeader(payloadType);

        if (payloadType.GetCmdType() == ROUTE_REQ_CMD || payloadType.GetCmdType() == ROUTE_REP_CMD)
        {
            CapabilityInformation capability;
            capability.SetCapability(m_nwkCapabilityInformation);
            if (capability.GetDeviceType() != ROUTER)
            {
                // Received RREQ  or RREP but the device
                // has no routing capabilities
                return;
            }
        }
        // NOTE: this cover the cases for MESH routing
        // TREE routing is not supported
        uint8_t linkCost = GetLinkCost(params.m_mpduLinkQuality);

        if (payloadType.GetCmdType() == ROUTE_REQ_CMD)
        {
            ZigbeePayloadRouteRequestCommand payload;
            msdu->RemoveHeader(payload);
            // Zigbee specification r22.1.0 Section 3.6.3.5.2
            ReceiveRREQ(params.m_srcAddr, linkCost, nwkHeader, payload);
        }
        else if (payloadType.GetCmdType() == ROUTE_REP_CMD)
        {
            ZigbeePayloadRouteReplyCommand payload;
            msdu->RemoveHeader(payload);
            // Zigbee specification r22.1.0 Section 3.6.3.5.3
            ReceiveRREP(params.m_srcAddr, linkCost, nwkHeader, payload);
        }
        break;
    }
    case INTER_PAN:
        NS_LOG_DEBUG("Inter PAN frame received but not supported");
        break;
    default:
        NS_LOG_ERROR("Unknown frame received in NWK layer");
    }
}

void
ZigbeeNwk::ReceiveRREQ(Mac16Address macSrcAddr,
                       uint8_t linkCost,
                       ZigbeeNwkHeader nwkHeader,
                       ZigbeePayloadRouteRequestCommand payload)
{
    NS_LOG_FUNCTION(this);

    if (nwkHeader.GetSrcAddr() == m_nwkNetworkAddress)
    {
        // I am the original initiator of the RREQ, ignore request
        return;
    }

    // Calculate the pathcost on the RREQ receiving device
    uint8_t pathCost = linkCost + payload.GetPathCost();

    // Many-to-one routing

    if (payload.GetCmdOptManyToOneField() != ManyToOne::NO_MANY_TO_ONE)
    {
        RouteDiscoveryStatus routeStatus =
            ProcessManyToOneRoute(macSrcAddr, pathCost, nwkHeader, payload);

        // Update the path cost of the RREQ
        payload.SetPathCost(pathCost);

        // Note: At this point we already have the updated radius, which was updated as soon
        // as the frame was received (i.e. In the MCPS-DATA.indication).

        if (routeStatus == MANY_TO_ONE_ROUTE || routeStatus == ROUTE_UPDATED)
        {
            Simulator::Schedule(MilliSeconds(m_rreqJitter->GetValue()),
                                &ZigbeeNwk::SendRREQ,
                                this,
                                nwkHeader,
                                payload,
                                0);
            m_nwkSequenceNumber++;
            m_routeRequestId++;
        }
        return;
    }

    // Mesh Routing

    Mac16Address nextHop;
    RouteDiscoveryStatus nextHopStatus =
        FindNextHop(macSrcAddr, pathCost, nwkHeader, payload, nextHop);

    if (payload.GetDstAddr() == m_nwkNetworkAddress || nextHopStatus == ROUTE_FOUND)
    {
        // RREQ is for this device or its children
        NS_LOG_DEBUG("RREQ is for me or my children, sending a RREP to [" << macSrcAddr << "]");

        SendRREP(macSrcAddr,
                 nwkHeader.GetSrcAddr(),
                 payload.GetDstAddr(),
                 payload.GetRouteReqId(),
                 pathCost);
    }
    else if (nextHopStatus == ROUTE_NOT_FOUND || nextHopStatus == ROUTE_UPDATED)
    {
        NS_LOG_DEBUG("Route for device [" << payload.GetDstAddr()
                                          << "] not found, forwarding RREQ");

        // Update path cost and resend the RREQ
        payload.SetPathCost(pathCost);
        Simulator::Schedule(MilliSeconds(m_rreqJitter->GetValue()),
                            &ZigbeeNwk::SendRREQ,
                            this,
                            nwkHeader,
                            payload,
                            m_nwkcRREQRetries);
    }
}

void
ZigbeeNwk::ReceiveRREP(Mac16Address macSrcAddr,
                       uint8_t linkCost,
                       ZigbeeNwkHeader nwkHeader,
                       ZigbeePayloadRouteReplyCommand payload)
{
    NS_LOG_FUNCTION(this);

    // RREP received, cancel any ongoing RREQ retry events for that
    // RREQ ID and remove entry from RREQ retry table.
    Ptr<RreqRetryTableEntry> rreqRetryTableEntry;
    if (m_rreqRetryTable.LookUpEntry(payload.GetRouteReqId(), rreqRetryTableEntry))
    {
        rreqRetryTableEntry->GetRreqEventId().Cancel();
        m_rreqRetryTable.Delete(payload.GetRouteReqId());
    }

    uint8_t pathCost = linkCost + payload.GetPathCost();

    if (payload.GetOrigAddr() == m_nwkNetworkAddress)
    {
        // The RREP is destined for this device
        Ptr<RouteDiscoveryTableEntry> discEntry;
        if (m_nwkRouteDiscoveryTable.LookUpEntry(payload.GetRouteReqId(),
                                                 payload.GetOrigAddr(),
                                                 discEntry))
        {
            Ptr<RoutingTableEntry> routeEntry;
            if (m_nwkRoutingTable.LookUpEntry(payload.GetRespAddr(), routeEntry))
            {
                if (routeEntry->GetStatus() == ROUTE_DISCOVERY_UNDERWAY)
                {
                    if (routeEntry->IsGroupIdPresent())
                    {
                        routeEntry->SetStatus(ROUTE_VALIDATION_UNDERWAY);
                    }
                    else
                    {
                        routeEntry->SetStatus(ROUTE_ACTIVE);
                    }
                    routeEntry->SetNextHopAddr(macSrcAddr);
                    discEntry->SetResidualCost(pathCost);
                }
                else if (routeEntry->GetStatus() == ROUTE_VALIDATION_UNDERWAY ||
                         routeEntry->GetStatus() == ROUTE_ACTIVE)
                {
                    if (pathCost < discEntry->GetResidualCost())
                    {
                        routeEntry->SetNextHopAddr(macSrcAddr);
                        discEntry->SetResidualCost(pathCost);
                    }
                }

                NS_LOG_DEBUG("RREP from source [" << payload.GetRespAddr()
                                                  << "] is for me; received from last hop ["
                                                  << macSrcAddr << "]");

                if (m_pendPrimitiveNwk == NLME_ROUTE_DISCOVERY)
                {
                    // We only report the result of the route discovery request
                    // with the first RREP received.
                    m_pendPrimitiveNwk = NLDE_NLME_NONE;
                    if (!m_nlmeRouteDiscoveryConfirmCallback.IsNull())
                    {
                        NlmeRouteDiscoveryConfirmParams routeDiscConfirmParams;
                        routeDiscConfirmParams.m_status = ZigbeeNwkStatus::SUCCESS;
                        m_nlmeRouteDiscoveryConfirmCallback(routeDiscConfirmParams);
                    }
                }

                Ptr<PendingTxPkt> pendingTxPkt = Create<PendingTxPkt>();
                if (!m_pendingTxQueue.empty() && DequeueTx(payload.GetRespAddr(), pendingTxPkt))
                {
                    std::cout << " In Zigbee " << m_nwkNetworkAddress
                              << " sending pending data to next hop "
                              << routeEntry->GetNextHopAddr() << "\n";
                    // There is a pending packet awaiting to be transmitted
                    // to the next hop, send it.
                    McpsDataRequestParams mcpsDataparams;
                    mcpsDataparams.m_txOptions = 0x01; // Acknowledment on.
                    mcpsDataparams.m_dstPanId = m_nwkPanId;
                    mcpsDataparams.m_msduHandle = pendingTxPkt->nsduHandle;
                    mcpsDataparams.m_srcAddrMode = SHORT_ADDR;
                    mcpsDataparams.m_dstAddrMode = SHORT_ADDR;
                    mcpsDataparams.m_dstAddr = routeEntry->GetNextHopAddr();
                    m_mac->McpsDataRequest(mcpsDataparams, pendingTxPkt->txPkt);
                }
            }
            else
            {
                m_nwkRouteDiscoveryTable.Delete(payload.GetRouteReqId(), payload.GetOrigAddr());
                return;
            }
        }
        else
        {
            return;
        }
    }
    else
    {
        // The RREP is NOT destined for this device
        Ptr<RouteDiscoveryTableEntry> discEntry;
        if (m_nwkRouteDiscoveryTable.LookUpEntry(payload.GetRouteReqId(),
                                                 payload.GetOrigAddr(),
                                                 discEntry))
        {
            if (payload.GetPathCost() < discEntry->GetResidualCost())
            {
                Ptr<RoutingTableEntry> routeEntry;
                if (m_nwkRoutingTable.LookUpEntry(payload.GetRespAddr(), routeEntry))
                {
                    routeEntry->SetNextHopAddr(macSrcAddr);
                    routeEntry->SetStatus(ROUTE_ACTIVE);
                    discEntry->SetResidualCost(pathCost);
                    // Forward route reply to the next hop back to the original route requester
                    SendRREP(discEntry->GetSenderAddr(),
                             payload.GetOrigAddr(),
                             payload.GetRespAddr(),
                             payload.GetRouteReqId(),
                             pathCost);
                }
                else
                {
                    // error here! Route discovery entry detected but no corresponding Routing table
                    // entry found
                }
            }
        }
    }
}

bool
ZigbeeNwk::IsBroadcastAddress(Mac16Address address)
{
    return address == "FF:FF" || address == "FF:FD" || address == "FF:FC" || address == "FF:FB";
}

RouteDiscoveryStatus
ZigbeeNwk::FindNextHop(Mac16Address macSrcAddr,
                       uint8_t pathCost,
                       ZigbeeNwkHeader nwkHeader,
                       ZigbeePayloadRouteRequestCommand payload,
                       Mac16Address& nextHop)
{
    NS_LOG_FUNCTION(this);

    // Mesh routing

    // Check if the destination is our neighbor
    Ptr<NeighborTableEntry> neighborEntry;
    if (m_nwkNeighborTable.LookUpEntry(payload.GetDstAddr(), neighborEntry))
    {
        nextHop = payload.GetDstAddr();
        return ROUTE_FOUND;
    }

    Ptr<RoutingTableEntry> entry;
    if (m_nwkRoutingTable.LookUpEntry(payload.GetDstAddr(), entry))
    {
        if (!(entry->GetStatus() == ROUTE_ACTIVE) &&
            !(entry->GetStatus() == ROUTE_VALIDATION_UNDERWAY))
        {
            // Entry found but is not valid
            entry->SetStatus(ROUTE_DISCOVERY_UNDERWAY);
        }
        else
        {
            // Entry found
            nextHop = entry->GetNextHopAddr();
            return ROUTE_FOUND;
        }
    }
    else if (nwkHeader.GetDiscoverRoute() == DiscoverRouteType::ENABLE_ROUTE_DISCOVERY)
    {
        // Check that the max routing capacity has not been reached.
        if (m_nwkRoutingTable.GetSize() == m_nwkRoutingTable.GetMaxTableSize())
        {
            if (!m_nlmeRouteDiscoveryConfirmCallback.IsNull())
            {
                NlmeRouteDiscoveryConfirmParams confirmParams;
                confirmParams.m_status = ROUTE_ERROR;
                confirmParams.m_networkStatusCode = NO_ROUTING_CAPACITY;
                m_nlmeRouteDiscoveryConfirmCallback(confirmParams);
            }
            return TABLE_FULL;
        }

        // Entry not found
        Ptr<RoutingTableEntry> newRoutingEntry =
            Create<RoutingTableEntry>(payload.GetDstAddr(),
                                      ROUTE_DISCOVERY_UNDERWAY,
                                      true,  // TODO no route cache
                                      false, // TODO: Many to one
                                      false, // TODO: Route record
                                      false, // TODO: Group id
                                      Mac16Address("FF:FF"));
        m_nwkRoutingTable.AddEntry(newRoutingEntry);
    }
    else
    {
        if (!m_nlmeRouteDiscoveryConfirmCallback.IsNull())
        {
            NlmeRouteDiscoveryConfirmParams confirmParams;
            confirmParams.m_status = ROUTE_ERROR;
            confirmParams.m_networkStatusCode = NO_ROUTE_AVAILABLE;
            m_nlmeRouteDiscoveryConfirmCallback(confirmParams);
        }
        return NO_DISCOVER_ROUTE;
    }

    // 2- Find entry in DISCOVERY TABLE
    Ptr<RouteDiscoveryTableEntry> discEntry;
    if (m_nwkRouteDiscoveryTable.LookUpEntry(payload.GetRouteReqId(),
                                             nwkHeader.GetSrcAddr(),
                                             discEntry))
    {
        // Entry Found
        if (pathCost < discEntry->GetForwardCost())
        {
            // More optimal route found, update route discovery values.
            discEntry->SetSenderAddr(macSrcAddr);
            discEntry->SetForwardCost(pathCost);
            discEntry->SetExpTime(Simulator::Now() + m_nwkcRouteDiscoveryTime);
            return ROUTE_UPDATED;
        }
        else
        {
            return DISCOVER_UNDERWAY;
        }
    }
    else
    {
        // Entry NOT found, add NEW entry to route discovery table.
        Ptr<RouteDiscoveryTableEntry> newDiscEntry =
            Create<RouteDiscoveryTableEntry>(payload.GetRouteReqId(),
                                             nwkHeader.GetSrcAddr(),
                                             macSrcAddr, // macSrcAddr,
                                             pathCost,   // payload.GetPathCost(), // Forward cost
                                             0xff,       // Residual cost
                                             (Simulator::Now() + m_nwkcRouteDiscoveryTime));

        if (!m_nwkRouteDiscoveryTable.AddEntry(newDiscEntry))
        {
            return TABLE_FULL;
        }
    }
    return ROUTE_NOT_FOUND;
}

RouteDiscoveryStatus
ZigbeeNwk::ProcessManyToOneRoute(Mac16Address macSrcAddr,
                                 uint8_t pathCost,
                                 ZigbeeNwkHeader nwkHeader,
                                 ZigbeePayloadRouteRequestCommand payload)
{
    Ptr<RouteDiscoveryTableEntry> discEntry;
    if (m_nwkRouteDiscoveryTable.LookUpEntry(payload.GetRouteReqId(),
                                             nwkHeader.GetSrcAddr(),
                                             discEntry))
    {
        Ptr<RoutingTableEntry> routeEntry;
        if (m_nwkRoutingTable.LookUpEntry(nwkHeader.GetSrcAddr(), routeEntry))
        {
            if (routeEntry->GetStatus() == ROUTE_VALIDATION_UNDERWAY ||
                routeEntry->GetStatus() == ROUTE_ACTIVE)
            {
                if (pathCost < discEntry->GetForwardCost())
                {
                    // Update with a better route.
                    routeEntry->SetNextHopAddr(macSrcAddr);
                    discEntry->SetForwardCost(pathCost);
                    discEntry->SetExpTime(Simulator::Now() + m_nwkcRouteDiscoveryTime);
                    return ROUTE_UPDATED;
                }
                return NO_ROUTE_CHANGE;
            }
        }
        else
        {
            NS_LOG_ERROR("Error: Entry found in the discovery table but not the routing table");
            return NO_ROUTE_CHANGE;
        }
    }
    else
    {
        // Entry NOT found, add NEW entry to route discovery table.
        Ptr<RouteDiscoveryTableEntry> newDiscEntry =
            Create<RouteDiscoveryTableEntry>(payload.GetRouteReqId(),
                                             nwkHeader.GetSrcAddr(),
                                             macSrcAddr, // previous hop address
                                             pathCost,   // Forward cost
                                             0xff,       // Residual cost (not used by Many-to-One)
                                             (Simulator::Now() + m_nwkcRouteDiscoveryTime));

        // TODO: what to do if route discovery table is full?
        m_nwkRouteDiscoveryTable.AddEntry(newDiscEntry);

        // Define the type of Many-To-One routing (with or without route record)
        bool routeRecord = false;
        if (payload.GetCmdOptManyToOneField() == ManyToOne::ROUTE_RECORD)
        {
            routeRecord = true;
        }

        Ptr<RoutingTableEntry> routeEntry;
        if (m_nwkRoutingTable.LookUpEntry(nwkHeader.GetSrcAddr(), routeEntry))
        {
            if (routeEntry->GetStatus() == ROUTE_VALIDATION_UNDERWAY ||
                routeEntry->GetStatus() == ROUTE_ACTIVE)
            {
                // The entry exist in routing table but it was not in discovery table
                // Refresh the Route
                routeEntry->SetNextHopAddr(macSrcAddr);
                // TODO: other parameters
                return ROUTE_UPDATED;
            }
            return NO_ROUTE_CHANGE;
        }
        else
        {
            // New routing table entry
            if (m_nwkRoutingTable.GetSize() == m_nwkRoutingTable.GetMaxTableSize())
            {
                if (!m_nlmeRouteDiscoveryConfirmCallback.IsNull())
                {
                    NlmeRouteDiscoveryConfirmParams confirmParams;
                    confirmParams.m_status = ROUTE_ERROR;
                    confirmParams.m_networkStatusCode = NO_ROUTING_CAPACITY;
                    m_nlmeRouteDiscoveryConfirmCallback(confirmParams);
                }
                return TABLE_FULL;
            }

            Ptr<RoutingTableEntry> newRoutingEntry =
                Create<RoutingTableEntry>(nwkHeader.GetSrcAddr(),
                                          ROUTE_ACTIVE,
                                          true,        // TODO no route cache
                                          true,        // TODO: Many to one
                                          routeRecord, // TODO: Route record
                                          false,       // TODO: Group id
                                          macSrcAddr);
            m_nwkRoutingTable.AddEntry(newRoutingEntry);
            return MANY_TO_ONE_ROUTE;
        }
    }
    return NO_ROUTE_CHANGE;
}

void
ZigbeeNwk::SendUnicast(Ptr<Packet> packet, uint8_t handle)
{
    NS_LOG_FUNCTION(this);

    // Obtain information from the DATA packet and construct the RREQ
    ZigbeeNwkHeader nwkHeaderData;
    packet->PeekHeader(nwkHeaderData);

    ZigbeeNwkHeader nwkHeader;
    nwkHeader.SetFrameType(NWK_COMMAND);
    nwkHeader.SetProtocolVer(m_nwkcProtocolVersion);
    nwkHeader.SetDiscoverRoute(nwkHeaderData.GetDiscoverRoute());
    // See r22.1.0, Table 3-69
    // Set destination to broadcast (all routers and coordinator)
    nwkHeader.SetDstAddr(Mac16Address("FF:FC"));
    nwkHeader.SetSrcAddr(m_nwkNetworkAddress);
    nwkHeader.SetSeqNum(m_nwkSequenceNumber.GetValue());
    // see Zigbee specification 3.2.2.33.3
    if (nwkHeaderData.GetRadius() == 0)
    {
        nwkHeader.SetRadius(m_nwkMaxDepth * 2);
    }
    else
    {
        nwkHeader.SetRadius(nwkHeaderData.GetRadius());
    }

    ZigbeePayloadRouteRequestCommand payload;
    payload.SetRouteReqId(m_routeRequestId.GetValue());
    payload.SetDstAddr(nwkHeaderData.GetDstAddr());
    payload.SetPathCost(0);

    Mac16Address nextHop;
    RouteDiscoveryStatus nextHopStatus =
        FindNextHop(m_nwkNetworkAddress, 0, nwkHeader, payload, nextHop);

    if (nextHopStatus == ROUTE_FOUND)
    {
        // Parameters as described in Section 3.6.3.3
        McpsDataRequestParams mcpsDataparams;
        mcpsDataparams.m_dstPanId = m_nwkPanId;
        mcpsDataparams.m_msduHandle = handle;
        mcpsDataparams.m_txOptions = 0x01; // Acknowledment on.
        mcpsDataparams.m_srcAddrMode = SHORT_ADDR;
        mcpsDataparams.m_dstAddrMode = SHORT_ADDR;
        mcpsDataparams.m_dstAddr = nextHop;
        m_mac->McpsDataRequest(mcpsDataparams, packet);
    }
    else if (nextHopStatus == ROUTE_NOT_FOUND)
    {
        // Route not found. Route marked as DISCOVER UNDERWAY,
        // packet added to pending Tx Queue and we initiate route
        // discovery
        EnqueueTx(packet, handle);

        Simulator::Schedule(MilliSeconds(m_rreqJitter->GetValue()),
                            &ZigbeeNwk::SendRREQ,
                            this,
                            nwkHeader,
                            payload,
                            m_nwkcInitialRREQRetries);

        m_nwkSequenceNumber++;
        m_routeRequestId++;
    }
}

void
ZigbeeNwk::McpsDataConfirm(McpsDataConfirmParams params)
{
    // TODO
    /* if (m_pendPrimitiveNwk == NLME_ROUTE_DISCOVERY)
     {
         //m_pendPrimitiveNwk = NLDE_NLME_NONE;
     }
     else
     {
         // Zigbee Specification r22.1.0, End of Section 3.2.1.1.3
         // Report the the results of a request to a transmission of a packet
         if (!m_nldeDataConfirmCallback.IsNull())
         {
             NldeDataConfirmParams nldeDataConfirmParams;
             nldeDataConfirmParams.m_status = static_cast<ZigbeeNwkStatus>(params.m_status);
             m_nldeDataConfirmCallback(nldeDataConfirmParams);
         }
     }*/
}

void
ZigbeeNwk::MlmeScanConfirm(MlmeScanConfirmParams params)
{
    NS_LOG_FUNCTION(this);

    if (m_pendPrimitiveNwk == NLME_NETWORK_FORMATION && params.m_scanType == MLMESCAN_ED)
    {
        if (params.m_status != MacStatus::SUCCESS)
        {
            m_pendPrimitiveNwk = NLDE_NLME_NONE;
            m_netFormParams = {};
            m_netFormParamsGen = nullptr;

            if (!m_nlmeNetworkFormationConfirmCallback.IsNull())
            {
                NlmeNetworkFormationConfirmParams confirmParams;
                confirmParams.m_status = ZigbeeNwkStatus::STARTUP_FAILURE;
                m_nlmeNetworkFormationConfirmCallback(confirmParams);
            }
        }
        else
        {
            // TODO: continue energy scan in other interfaces if supported.

            // Filter the channels with unacceptable energy level (channel, energy)
            std::vector<uint8_t> energyList = params.m_energyDetList;
            uint32_t channelMask = m_netFormParams.m_scanChannelList.channelsField[0];
            uint32_t channelMaskFiltered = 0;
            uint32_t countAcceptableChannels = 0;

            for (uint32_t i = 11; i <= 26; i++)
            {
                if ((channelMask >> i) & 1)
                {
                    // Channel found in mask, check energy channel and mark it if acceptable
                    if (energyList[0] <= m_scanEnergyThreshold)
                    {
                        // energy is acceptable, register to filtered list
                        channelMaskFiltered |= (1 << i);
                        energyList.erase(energyList.begin());
                        countAcceptableChannels++;
                    }
                }
            }

            NS_LOG_DEBUG("[NLME-NETWORK-FORMATION.request]: Energy scan complete, "
                         << countAcceptableChannels << " acceptable channels found : 0x" << std::hex
                         << channelMaskFiltered << std::dec);

            if (countAcceptableChannels == 0)
            {
                m_pendPrimitiveNwk = NLDE_NLME_NONE;
                m_netFormParams = {};
                m_netFormParamsGen = nullptr;

                if (!m_nlmeNetworkFormationConfirmCallback.IsNull())
                {
                    NlmeNetworkFormationConfirmParams confirmParams;
                    confirmParams.m_status = ZigbeeNwkStatus::STARTUP_FAILURE;
                    m_nlmeNetworkFormationConfirmCallback(confirmParams);
                }
            }
            else
            {
                MlmeScanRequestParams mlmeParams;
                mlmeParams.m_chPage = (channelMaskFiltered >> 27) & (0x01F);
                mlmeParams.m_scanChannels = channelMaskFiltered;
                mlmeParams.m_scanDuration = m_netFormParams.m_scanDuration;
                mlmeParams.m_scanType = MLMESCAN_ACTIVE;
                m_mac->MlmeScanRequest(mlmeParams);
            }
        }
    }
    else if (m_pendPrimitiveNwk == NLME_NETWORK_FORMATION && params.m_scanType == MLMESCAN_ACTIVE)
    {
        // See Zigbee specification r22.1.0, Section 3.2.2.5.3,
        if (params.m_status == MacStatus::NO_BEACON || params.m_status == MacStatus::SUCCESS)
        {
            uint8_t channel = 0;
            uint8_t page = 0;
            uint16_t panId = 0;

            // TODO: We should  scanned channels on each interface
            //       (only possible when more interfaces are supported.)

            if (params.m_status == MacStatus::NO_BEACON)
            {
                // All channels provided in the active scan were acceptable
                // (No coordinators found). Take the first channel in the list an
                // a random panid.
                for (uint8_t j = 11; j <= 26; j++)
                {
                    if ((m_netFormParams.m_scanChannelList.channelsField[0] & (1 << j)) != 0)
                    {
                        channel = j;
                        page = (m_netFormParams.m_scanChannelList.channelsField[0] >> 27) & (0x01F);
                        break;
                    }
                }
                //
                // Choose a random PAN ID  (3.2.2.5.3 , d.ii.)
                panId = m_uniformRandomVariable->GetInteger(1, 0xFFF7);
            }
            else
            {
                // At least 1 coordinator was found in X channel
                /* uint32_t channelMask = m_netFormParams.m_scanChannelList.channelsField[0];
                 for (uint32_t i=11; i<=26; i++)
                 {
                     if ((channelMask >> i) & 1)
                     {
                         // Channel found in mask, check energy channel and mark it if acceptable
                         if (energyList[0] <= m_scanEnergyThreshold)
                         {
                             // energy is acceptable, register to filtered list
                             channelMaskFiltered |= (1 << i) & (1 << i);
                             energyList.erase(energyList.begin());
                             countAcceptableChannels++;
                         }
                     }
                 }*/
            }

            // store the chosen page, channel and pan Id.
            m_netFormParamsGen = Create<NetFormPendingParamsGen>();
            m_netFormParamsGen->page = page;
            m_netFormParamsGen->channel = channel;
            m_netFormParamsGen->panId = panId;

            NS_LOG_DEBUG("[NLME-NETWORK-FORMATION.request]: Active scan complete, page "
                         << std::dec << static_cast<uint32_t>(page) << ", channel " << std::dec
                         << static_cast<uint32_t>(channel) << " and PAN ID 0x" << std::hex << panId
                         << std::dec << " chosen.");

            // Set the device short address (3.2.2.5.3 , 6.f)
            Ptr<MacPibAttributes> pibAttr = Create<MacPibAttributes>();
            if (m_netFormParams.m_distributedNetwork)
            {
                pibAttr->macShortAddress = m_netFormParams.m_distributedNetworkAddress;
                m_nwkNetworkAddress = m_netFormParams.m_distributedNetworkAddress;
            }
            else
            {
                pibAttr->macShortAddress = Mac16Address("00:00");
                m_nwkNetworkAddress = Mac16Address("00:00");
            }
            // Set Short Address and continue with beacon payload afterwards.
            m_mac->MlmeSetRequest(MacPibAttributeIdentifier::macShortAddress, pibAttr);
        }
        else
        {
            // Error occurred during network formation active scan
            // report to higher layer (Section 3.2.2.5.3, 6.d)

            m_pendPrimitiveNwk = NLDE_NLME_NONE;
            m_netFormParams = {};
            m_netFormParamsGen = nullptr;

            if (!m_nlmeNetworkFormationConfirmCallback.IsNull())
            {
                NlmeNetworkFormationConfirmParams confirmParams;
                switch (params.m_status)
                {
                case MacStatus::COUNTER_ERROR:
                    confirmParams.m_status = ZigbeeNwkStatus::COUNTER_ERROR;
                    break;
                case MacStatus::FRAME_TOO_LONG:
                    confirmParams.m_status = ZigbeeNwkStatus::FRAME_TOO_LONG;
                    break;
                case MacStatus::UNAVAILABLE_KEY:
                    confirmParams.m_status = ZigbeeNwkStatus::UNAVAILABLE_KEY;
                    break;
                case MacStatus::UNSUPPORTED_SECURITY:
                    confirmParams.m_status = ZigbeeNwkStatus::UNSUPPORTED_SECURITY;
                    break;
                default:
                    confirmParams.m_status = ZigbeeNwkStatus::INVALID_PARAMETER;
                }
                m_nlmeNetworkFormationConfirmCallback(confirmParams);
            }
        }
    }
    else if (m_pendPrimitiveNwk == NLME_NET_DISCV && params.m_scanType == MLMESCAN_ACTIVE)
    {
        NlmeNetworkDiscoveryConfirmParams netDiscConfirmParams;
        m_pendPrimitiveNwk = NLDE_NLME_NONE;

        if (params.m_status == MacStatus::SUCCESS)
        {
            NS_LOG_DEBUG("[NLME-NETWORK-DISCOVERY.request]: Active scan, "
                         << m_networkDescriptorList.size() << " PARENT capable device(s) found");

            netDiscConfirmParams.m_netDescList = m_networkDescriptorList;
            netDiscConfirmParams.m_networkCount = m_networkDescriptorList.size();
            netDiscConfirmParams.m_status = ZigbeeNwkStatus::SUCCESS;
            m_networkDescriptorList = {};
        }
        else
        {
            NS_LOG_DEBUG("[NLME-NETWORK-DISCOVERY.request]: Active scan failed with"
                         " status: "
                         << static_cast<uint32_t>(params.m_status));

            switch (params.m_status)
            {
            case MacStatus::LIMIT_REACHED:
                netDiscConfirmParams.m_status = ZigbeeNwkStatus::LIMIT_REACHED;
                break;
            case MacStatus::NO_BEACON:
                netDiscConfirmParams.m_status = ZigbeeNwkStatus::NO_NETWORKS;
                break;
            case MacStatus::SCAN_IN_PROGRESS:
                netDiscConfirmParams.m_status = ZigbeeNwkStatus::SCAN_IN_PROGRESS;
                break;
            case MacStatus::COUNTER_ERROR:
                netDiscConfirmParams.m_status = ZigbeeNwkStatus::COUNTER_ERROR;
                break;
            case MacStatus::FRAME_TOO_LONG:
                netDiscConfirmParams.m_status = ZigbeeNwkStatus::FRAME_TOO_LONG;
                break;
            case MacStatus::UNAVAILABLE_KEY:
                netDiscConfirmParams.m_status = ZigbeeNwkStatus::UNAVAILABLE_KEY;
                break;
            case MacStatus::UNSUPPORTED_SECURITY:
                netDiscConfirmParams.m_status = ZigbeeNwkStatus::UNSUPPORTED_SECURITY;
                break;
            default:
                netDiscConfirmParams.m_status = ZigbeeNwkStatus::INVALID_PARAMETER;
            }
        }

        if (!m_nlmeNetworkDiscoveryConfirmCallback.IsNull())
        {
            m_nlmeNetworkDiscoveryConfirmCallback(netDiscConfirmParams);
        }
    }
    else if (m_pendPrimitiveNwk == NLME_JOIN && params.m_scanType == MLMESCAN_ORPHAN)
    {
        // TODO: Add macInterfaceIndex and channelListStructure params when supported
        if (params.m_status == MacStatus::SUCCESS)
        {
            // Orphan scan was successful (Join success), first update the extended
            // PAN id and the capability information, then the
            // the nwkNetworkAddress with the macShortAddress, this
            // will be followed by an update of the m_nwkPanId with macPanId
            // and finally the join confirmation
            m_nwkExtendedPanId = m_joinParams.m_extendedPanId;
            m_nwkCapabilityInformation = m_joinParams.m_capabilityInfo;
            m_mac->MlmeGetRequest(MacPibAttributeIdentifier::macShortAddress);
        }
        else
        {
            m_pendPrimitiveNwk = NLDE_NLME_NONE;
            m_joinParams = {};
            NS_LOG_DEBUG("[NLME-JOIN.request]: Orphan scan completed but no networks found");

            if (!m_nlmeJoinConfirmCallback.IsNull())
            {
                NlmeJoinConfirmParams joinConfirmParams;
                joinConfirmParams.m_status = ZigbeeNwkStatus::NO_NETWORKS;
            }
        }
    }
}

void
ZigbeeNwk::MlmeAssociateConfirm(MlmeAssociateConfirmParams params)
{
    NS_LOG_FUNCTION(this);

    if (m_pendPrimitiveNwk == NLME_JOIN)
    {
        NlmeJoinConfirmParams joinConfirmParams;
        joinConfirmParams.m_extendedPanId = m_joinParams.m_extendedPanId;
        joinConfirmParams.m_enhancedBeacon = false; // hardcoded, no support
        joinConfirmParams.m_macInterfaceIndex = 0;  // hardcoded, no support
        joinConfirmParams.m_networkAddress = params.m_assocShortAddr;

        Ptr<NeighborTableEntry> entry;

        if (params.m_status == MacStatus::SUCCESS)
        {
            joinConfirmParams.m_status = ZigbeeNwkStatus::SUCCESS;
            joinConfirmParams.m_networkAddress = params.m_assocShortAddr;

            // Update NWK NIB values
            m_nwkNetworkAddress = params.m_assocShortAddr;
            m_nwkExtendedPanId = m_joinParams.m_extendedPanId;
            m_nwkPanId = m_associateParams.panId;

            // Update relationship
            if (m_nwkNeighborTable.LookUpEntry(m_associateParams.extAddress, entry))
            {
                entry->SetRelationship(NBR_PARENT);
                // m_nwkNeighborTable.Update(m_associateParams.extAddress, entry);
                NS_LOG_DEBUG("Associated SUCCESSFULLY to PAN ID and Ext PAN ID: "
                             << "(0x" << std::hex << m_nwkPanId << " | 0x" << m_nwkExtendedPanId
                             << ")" << std::dec);
            }
            else
            {
                NS_LOG_ERROR("Entry not found while updating relationship");
            }
            // TODO:m_nwkUpdateId
        }
        else
        {
            switch (params.m_status)
            {
            case MacStatus::ACCESS_DENIED:
            case MacStatus::FULL_CAPACITY:
                // Discard neighbor as potential parent
                if (m_nwkNeighborTable.LookUpEntry(m_associateParams.extAddress, entry))
                {
                    entry->SetPotentialParent(false);
                    // m_nwkNeighborTable.Update(m_associateParams.extAddress, entry);
                }
                else
                {
                    NS_LOG_ERROR("Neighbor not found when discarding as potential parent");
                }
                joinConfirmParams.m_status = ZigbeeNwkStatus::NOT_PERMITED;
                break;
            case MacStatus::NO_ACK:
                joinConfirmParams.m_status = ZigbeeNwkStatus::NO_ACK;
                break;
            case MacStatus::CHANNEL_ACCESS_FAILURE:
                joinConfirmParams.m_status = ZigbeeNwkStatus::CHANNEL_ACCESS_FAILURE;
                break;
            default:
                joinConfirmParams.m_status = ZigbeeNwkStatus::INVALID_REQUEST;
                break;
            }
        }

        m_pendPrimitiveNwk = NLDE_NLME_NONE;
        m_joinParams = {};
        m_associateParams = {};

        if (!m_nlmeJoinConfirmCallback.IsNull())
        {
            m_nlmeJoinConfirmCallback(joinConfirmParams);
        }
    }
}

void
ZigbeeNwk::MlmeStartConfirm(MlmeStartConfirmParams params)
{
    NS_LOG_FUNCTION(this);

    ZigbeeNwkStatus nwkConfirmStatus;
    switch (params.m_status)
    {
    case MacStatus::SUCCESS:
        nwkConfirmStatus = ZigbeeNwkStatus::SUCCESS;
        break;
    case MacStatus::NO_SHORT_ADDRESS:
        nwkConfirmStatus = ZigbeeNwkStatus::NO_SHORT_ADDRESS;
        break;
    case MacStatus::SUPERFRAME_OVERLAP:
        nwkConfirmStatus = ZigbeeNwkStatus::SUPERFRAME_OVERLAP;
        break;
    case MacStatus::TRACKING_OFF:
        nwkConfirmStatus = ZigbeeNwkStatus::TRACKING_OFF;
        break;
    case MacStatus::INVALID_PARAMETER:
        nwkConfirmStatus = ZigbeeNwkStatus::INVALID_PARAMETER;
        break;
    case MacStatus::COUNTER_ERROR:
        nwkConfirmStatus = ZigbeeNwkStatus::COUNTER_ERROR;
        break;
    case MacStatus::UNAVAILABLE_KEY:
        nwkConfirmStatus = ZigbeeNwkStatus::UNAVAILABLE_KEY;
        break;
    case MacStatus::UNSUPPORTED_SECURITY:
        nwkConfirmStatus = ZigbeeNwkStatus::UNSUPPORTED_SECURITY;
        break;
    case MacStatus::CHANNEL_ACCESS_FAILURE:
        nwkConfirmStatus = ZigbeeNwkStatus::CHANNEL_ACCESS_FAILURE;
        break;
    case MacStatus::FRAME_TOO_LONG:
        nwkConfirmStatus = ZigbeeNwkStatus::FRAME_TOO_LONG;
        break;
    default:
        nwkConfirmStatus = ZigbeeNwkStatus::STARTUP_FAILURE;
        m_pendPrimitiveNwk = NLDE_NLME_NONE;
        m_netFormParams = {};
        m_netFormParamsGen = nullptr;
        m_nwkExtendedPanId = 0xffffffffffffffed;
        m_nwkNetworkAddress = Mac16Address("ff:ff");
        m_nwkPanId = 0xffff;
        break;
    }

    if (m_pendPrimitiveNwk == NLME_NETWORK_FORMATION)
    {
        NS_LOG_DEBUG("[NLME-NETWORK-FORMATION.request]: Complete, Status "
                     << nwkConfirmStatus << " | Pan Id and ExtPanId: (0x" << std::hex << m_nwkPanId
                     << " | 0x" << m_nwkExtendedPanId << ")" << std::dec);

        m_pendPrimitiveNwk = NLDE_NLME_NONE;
        m_netFormParams = {};
        m_netFormParamsGen = nullptr;

        if (!m_nlmeNetworkFormationConfirmCallback.IsNull())
        {
            NlmeNetworkFormationConfirmParams confirmParams;
            confirmParams.m_status = nwkConfirmStatus;
            m_nlmeNetworkFormationConfirmCallback(confirmParams);
        }
    }
    else if (m_pendPrimitiveNwk == NLME_START_ROUTER)
    {
        NS_LOG_DEBUG("[NLME-START-ROUTER.request]: Complete, Status "
                     << nwkConfirmStatus << " | Pan Id and ExtPanId: (0x" << std::hex << m_nwkPanId
                     << " | 0x" << m_nwkExtendedPanId << ")" << std::dec);

        if (nwkConfirmStatus != ZigbeeNwkStatus::SUCCESS)
        {
            m_pendPrimitiveNwk = NLDE_NLME_NONE;
            if (!m_nlmeStartRouterConfirmCallback.IsNull())
            {
                NlmeStartRouterConfirmParams confirmParams;
                confirmParams.m_status = nwkConfirmStatus;
                m_nlmeStartRouterConfirmCallback(confirmParams);
            }
        }
        else
        {
            UpdateBeaconPayload();
        }
    }
}

void
ZigbeeNwk::MlmeSetConfirm(MlmeSetConfirmParams params)
{
    if (m_pendPrimitiveNwk == NLME_NETWORK_FORMATION)
    {
        if (params.m_status == MacStatus::SUCCESS &&
            params.id == MacPibAttributeIdentifier::macShortAddress)
        {
            // Section (3.2.2.5.3 , 6.g)
            // Getting this device MAC extended address using MLME-GET
            m_mac->MlmeGetRequest(MacPibAttributeIdentifier::macExtendedAddress);
        }
        else if (params.m_status == MacStatus::SUCCESS &&
                 params.id == MacPibAttributeIdentifier::macBeaconPayload)
        {
            // Finalize Network Formation (Start network)
            MlmeStartRequestParams startParams;
            startParams.m_logCh = m_netFormParamsGen->channel;
            startParams.m_logChPage = m_netFormParamsGen->page;
            startParams.m_PanId = m_netFormParamsGen->panId;
            startParams.m_bcnOrd = m_netFormParams.m_beaconOrder;
            startParams.m_sfrmOrd = m_netFormParams.m_superFrameOrder;
            startParams.m_battLifeExt = m_netFormParams.m_batteryLifeExtension;
            startParams.m_coorRealgn = false;
            startParams.m_panCoor = true;
            m_mac->MlmeStartRequest(startParams);
        }
        else
        {
            m_pendPrimitiveNwk = NLDE_NLME_NONE;
            m_netFormParams = {};
            m_netFormParamsGen = nullptr;

            if (!m_nlmeNetworkFormationConfirmCallback.IsNull())
            {
                NlmeNetworkFormationConfirmParams confirmParams;
                confirmParams.m_status = ZigbeeNwkStatus::STARTUP_FAILURE;
                m_nlmeNetworkFormationConfirmCallback(confirmParams);
            }
        }
    }
    else if (m_pendPrimitiveNwk == NLME_JOIN_INDICATION)
    {
        NlmeJoinIndicationParams joinIndParams = m_joinIndParams;

        m_pendPrimitiveNwk = NLDE_NLME_NONE;
        m_joinIndParams = {};

        if (!m_nlmeJoinIndicationCallback.IsNull())
        {
            m_nlmeJoinIndicationCallback(joinIndParams);
        }
    }
    else if (m_pendPrimitiveNwk == NLME_START_ROUTER)
    {
        if (params.m_status == MacStatus::SUCCESS &&
            params.id == MacPibAttributeIdentifier::macBeaconPayload)
        {
            m_pendPrimitiveNwk = NLDE_NLME_NONE;
            if (!m_nlmeStartRouterConfirmCallback.IsNull())
            {
                NlmeStartRouterConfirmParams confirmParams;
                confirmParams.m_status = ZigbeeNwkStatus::SUCCESS;
                m_nlmeStartRouterConfirmCallback(confirmParams);
            }
        }
        else
        {
            NS_LOG_ERROR("Beacon payload update failed during a NLME-START-ROUTER.request");
        }
    }
}

void
ZigbeeNwk::MlmeGetConfirm(MacStatus status,
                          MacPibAttributeIdentifier id,
                          Ptr<MacPibAttributes> attribute)
{
    if (m_pendPrimitiveNwk == NLME_NETWORK_FORMATION)
    {
        if (id == MacPibAttributeIdentifier::macExtendedAddress && status == MacStatus::SUCCESS)
        {
            // Section (3.2.2.5.3 , 6.g)
            // Set nwkExtendedPanId and m_nwkIeeeAddress and nwkPanId
            m_nwkIeeeAddress = attribute->macExtendedAddress;
            m_nwkExtendedPanId = m_nwkIeeeAddress.ConvertToInt();
            m_nwkPanId = m_netFormParamsGen->panId;

            // Configure the capability information of the PAN coordinator
            CapabilityInformation capaInfo;
            capaInfo.SetDeviceType(zigbee::MacDeviceType::ROUTER);
            m_nwkCapabilityInformation = capaInfo.GetCapability();

            // Set Beacon payload before starting a network
            // See Figure 3-37 Establishing a Network
            // See also 3.6.7.
            UpdateBeaconPayload();
        }
        else
        {
            m_pendPrimitiveNwk = NLDE_NLME_NONE;
            m_netFormParams = {};
            m_netFormParamsGen = nullptr;

            if (!m_nlmeNetworkFormationConfirmCallback.IsNull())
            {
                NlmeNetworkFormationConfirmParams confirmParams;
                confirmParams.m_status = ZigbeeNwkStatus::STARTUP_FAILURE;
                m_nlmeNetworkFormationConfirmCallback(confirmParams);
            }
        }
    }
    else if (m_pendPrimitiveNwk == NLME_JOIN && status == MacStatus::SUCCESS)
    {
        if (id == MacPibAttributeIdentifier::macShortAddress)
        {
            m_nwkNetworkAddress = attribute->macShortAddress;
            m_mac->MlmeGetRequest(MacPibAttributeIdentifier::macPanId);
        }
        else if (id == MacPibAttributeIdentifier::macPanId)
        {
            m_nwkPanId = attribute->macPanId;

            NlmeJoinConfirmParams joinConfirmParams;
            joinConfirmParams.m_channelList = m_joinParams.m_scanChannelList;
            joinConfirmParams.m_status = ZigbeeNwkStatus::SUCCESS;
            joinConfirmParams.m_networkAddress = m_nwkNetworkAddress;
            joinConfirmParams.m_extendedPanId = m_nwkExtendedPanId;
            joinConfirmParams.m_enhancedBeacon = false;

            m_pendPrimitiveNwk = NLDE_NLME_NONE;
            m_joinParams = {};

            if (!m_nlmeJoinConfirmCallback.IsNull())
            {
                m_nlmeJoinConfirmCallback(joinConfirmParams);
            }
        }
    }
    else if (status == MacStatus::SUCCESS)
    {
        if (id == MacPibAttributeIdentifier::macExtendedAddress)
        {
            m_nwkIeeeAddress = attribute->macExtendedAddress;
        }
        else if (id == MacPibAttributeIdentifier::macShortAddress)
        {
            m_nwkNetworkAddress = attribute->macShortAddress;
        }
        else if (id == MacPibAttributeIdentifier::macPanId)
        {
            m_nwkPanId = attribute->macPanId;
        }
        else if (id == MacPibAttributeIdentifier::pCurrentChannel)
        {
            m_currentChannel = attribute->pCurrentChannel;
        }
    }
}

void
ZigbeeNwk::MlmeOrphanIndication(MlmeOrphanIndicationParams params)
{
    Ptr<NeighborTableEntry> entry;
    MlmeOrphanResponseParams respParams;

    if (m_nwkNeighborTable.LookUpEntry(params.m_orphanAddr, entry))
    {
        respParams.m_assocMember = true;
        respParams.m_orphanAddr = params.m_orphanAddr;
        respParams.m_shortAddr = entry->GetNwkAddr();

        // Temporally store the NLME-JOIN.indications parameters that will be
        // returned after the DIRECT_JOIN process concludes.
        // (after MLME-COMM-STATUS.indication is received)
        CapabilityInformation capability;
        capability.SetReceiverOnWhenIdle(entry->IsRxOnWhenIdle());

        if (entry->GetDeviceType() == NwkDeviceType::ZIGBEE_ROUTER)
        {
            capability.SetDeviceType(MacDeviceType::ROUTER);
        }
        else if (entry->GetDeviceType() == NwkDeviceType::ZIGBEE_ENDDEVICE)
        {
            capability.SetDeviceType(MacDeviceType::ENDDEVICE);
        }
        m_joinIndParams.m_capabilityInfo = capability.GetCapability();
        m_joinIndParams.m_extendedAddress = params.m_orphanAddr;
        m_joinIndParams.m_networkAddress = entry->GetNwkAddr();
        m_joinIndParams.m_rejoinNetwork = DIRECT_OR_REJOIN;

        NS_LOG_DEBUG("[NLME-JOIN.request]: ["
                     << params.m_orphanAddr << " | " << entry->GetNwkAddr()
                     << "] found in neighbor table, responding to orphaned device");

        m_mac->MlmeOrphanResponse(respParams);
    }
}

void
ZigbeeNwk::MlmeCommStatusIndication(MlmeCommStatusIndicationParams params)
{
    // Return the results to the next layer of the router or coordinator
    // only after a SUCCESSFUL join to the network.
    if (params.m_status == MacStatus::SUCCESS)
    {
        if (params.m_dstExtAddr == m_joinIndParams.m_extendedAddress &&
            m_joinIndParams.m_rejoinNetwork == DIRECT_OR_REJOIN)
        {
            NlmeJoinIndicationParams joinIndParams = m_joinIndParams;
            m_joinIndParams = {};

            if (!m_nlmeJoinIndicationCallback.IsNull())
            {
                m_nlmeJoinIndicationCallback(joinIndParams);
            }
        }
        else if (params.m_dstExtAddr == m_joinIndParams.m_extendedAddress &&
                 m_joinIndParams.m_rejoinNetwork == ASSOCIATION)
        {
            m_pendPrimitiveNwk = NLME_JOIN_INDICATION;
            UpdateBeaconPayload();
        }
        else
        {
            NS_LOG_ERROR("MLME-COMM-Status.Indication: params do not match");
        }
    }
}

void
ZigbeeNwk::MlmeBeaconNotifyIndication(MlmeBeaconNotifyIndicationParams params)
{
    NS_LOG_FUNCTION(this);

    // Zigbee specification  Section 3.6.1.3
    // Update Neighbor Table with information of the beacon payload
    // during a network-discovery

    if ((params.m_sdu->GetSize() == 0) ||
        (params.m_panDescriptor.m_coorAddrMode != lrwpan::AddressMode::SHORT_ADDR))
    {
        // The beacon do not contain beacon payload or is for a different network
        // stop any further process.
        return;
    }

    ZigbeeBeaconPayload beaconPayload;
    params.m_sdu->RemoveHeader(beaconPayload);

    if (beaconPayload.GetProtocolId() != 0)
    {
        return;
    }

    // TODO: Add a Permit to join, stack profile , update id and capability  check

    if (m_pendPrimitiveNwk == NLME_NET_DISCV)
    {
        // Keep a network descriptor list from the information in the beacon
        // to later on pass to the next higher layer when the network-discovery
        // process is over (NLME-NETWORK-DISCOVERY.confirm)
        NetworkDescriptor descriptor;
        descriptor.m_extPanId = beaconPayload.GetExtPanId();
        descriptor.m_panId = params.m_panDescriptor.m_coorPanId;
        descriptor.m_updateId = 0; // TODO: unknown
        descriptor.m_logCh = params.m_panDescriptor.m_logCh;
        descriptor.m_stackProfile = static_cast<StackProfile>(beaconPayload.GetStackProfile());
        descriptor.m_zigbeeVersion = beaconPayload.GetProtocolId();

        SuperframeInformation superframe(params.m_panDescriptor.m_superframeSpec);
        descriptor.m_beaconOrder = superframe.GetBeaconOrder();
        descriptor.m_superframeOrder = superframe.GetFrameOrder();
        descriptor.m_permitJoining = superframe.IsAssocPermit();

        descriptor.m_routerCapacity = beaconPayload.GetRouterCapacity();
        descriptor.m_endDeviceCapacity = beaconPayload.GetEndDevCapacity();
        m_networkDescriptorList.emplace_back(descriptor);

        // Keep track of the pan id (16 bits) and the extended PAN id for
        // future join (association) procedures.
        m_panIdTable.AddEntry(descriptor.m_extPanId, descriptor.m_panId);
        // NOTE:  In Zigbee all PAN coordinators or routers work with a
        //        SOURCE short address addressing mode, therefore the PAN descriptors only
        //        contain the short address.
        NS_LOG_DEBUG("Received beacon frame from [" << params.m_panDescriptor.m_coorShortAddr
                                                    << "]");
    }

    Ptr<NeighborTableEntry> entry;
    if (m_nwkNeighborTable.LookUpEntry(params.m_panDescriptor.m_coorShortAddr, entry))
    {
        // Update Neighbor table with the info of the received beacon

        entry->SetNwkAddr(params.m_panDescriptor.m_coorShortAddr);
        entry->SetTimeoutCounter(Seconds(15728640));
        entry->SetDevTimeout(Minutes(RequestedTimeoutField[m_nwkEndDeviceTimeoutDefault]));
        entry->SetLqi(params.m_panDescriptor.m_linkQuality);
        entry->SetOutgoingCost(GetLQINonLinearValue(params.m_panDescriptor.m_linkQuality));
        // m_nwkNeighborTable.Update(params.m_panDescriptor.m_coorShortAddr, entry);
        //  TODO: Update other fields if necessary and
        //        Additional and optional fields.
    }
    else
    {
        // Add a new entry to the neighbor table, information comes from
        // the MAC PAN descriptor and the beacon payload received.
        NwkDeviceType devType;
        if (params.m_panDescriptor.m_coorShortAddr == Mac16Address("00:00"))
        {
            devType = NwkDeviceType::ZIGBEE_COORDINATOR;
        }
        else
        {
            devType = NwkDeviceType::ZIGBEE_ROUTER;
        }

        // Create neighbor table entry with the basic fields
        Ptr<NeighborTableEntry> newEntry =
            Create<NeighborTableEntry>(Mac64Address("FF:FF:FF:FF:FF:FF:FF:FF"),
                                       params.m_panDescriptor.m_coorShortAddr,
                                       devType,
                                       true,
                                       0,
                                       Seconds(15728640),
                                       Minutes(RequestedTimeoutField[m_nwkEndDeviceTimeoutDefault]),
                                       NBR_NONE,
                                       0,
                                       params.m_panDescriptor.m_linkQuality,
                                       GetLQINonLinearValue(params.m_panDescriptor.m_linkQuality),
                                       0,
                                       false,
                                       0);

        // If necessary add information to the
        // additional and optional fields. Currently only 2 additional fields are added:
        newEntry->SetExtPanId(beaconPayload.GetExtPanId());
        newEntry->SetLogicalCh(params.m_panDescriptor.m_logCh);

        m_nwkNeighborTable.AddEntry(newEntry);
    }
}

void
ZigbeeNwk::MlmeAssociateIndication(MlmeAssociateIndicationParams params)
{
    NS_LOG_FUNCTION(this);

    // Joining Procedure through Association (Parent procedure)
    // Zigbee Specification 3.6.1.4.1

    CapabilityInformation receivedCapability(params.capabilityInfo);
    auto devType = static_cast<NwkDeviceType>(receivedCapability.GetDeviceType());

    Ptr<NeighborTableEntry> entry;
    if (m_nwkNeighborTable.LookUpEntry(params.m_extDevAddr, entry))
    {
        if (entry->GetDeviceType() == devType)
        {
            MlmeAssociateResponseParams responseParams;
            responseParams.m_status = MacStatus::SUCCESS;
            responseParams.m_assocShortAddr = entry->GetNwkAddr();
            responseParams.m_extDevAddr = entry->GetExtAddr();
            m_mac->MlmeAssociateResponse(responseParams);
        }
        else
        {
            m_nwkNeighborTable.Delete(params.m_extDevAddr);
            MlmeAssociateIndication(params);
        }
    }
    else
    {
        // Device currently do not exist in coordinator,
        // allocate an address and add to neighbor table.

        Mac16Address allocatedAddr;
        if (receivedCapability.IsAllocateAddrOn())
        {
            allocatedAddr = AllocateNetworkAddress();
        }
        else
        {
            // The device is associated but it will only use its
            // extended address (EUI-64 also known as IEEE Address)
            allocatedAddr = Mac16Address("FF:FE");
        }

        CapabilityInformation capability(params.capabilityInfo);

        Ptr<NeighborTableEntry> newEntry =
            Create<NeighborTableEntry>(params.m_extDevAddr,
                                       allocatedAddr,
                                       devType,
                                       capability.IsReceiverOnWhenIdle(),
                                       0,
                                       Seconds(15728640),
                                       Minutes(RequestedTimeoutField[m_nwkEndDeviceTimeoutDefault]),
                                       NBR_CHILD,
                                       0,
                                       params.lqi,
                                       0,
                                       0,
                                       true,
                                       0);
        // Optional parameters
        newEntry->SetExtPanId(m_nwkExtendedPanId);

        MlmeAssociateResponseParams responseParams;
        responseParams.m_extDevAddr = params.m_extDevAddr;

        if (m_nwkNeighborTable.AddEntry(newEntry))
        {
            responseParams.m_status = MacStatus::SUCCESS;
            responseParams.m_assocShortAddr = allocatedAddr;

            // Temporally store the NLME-JOIN.indications parameters that will be
            // returned after the association process concludes.
            // (after MLME-COMM-STATUS.indication received and beacon payload updated)
            m_joinIndParams.m_capabilityInfo = receivedCapability.GetCapability();
            m_joinIndParams.m_extendedAddress = params.m_extDevAddr;
            m_joinIndParams.m_networkAddress = allocatedAddr;
            m_joinIndParams.m_rejoinNetwork = ASSOCIATION;
        }
        else
        {
            responseParams.m_status = MacStatus::FULL_CAPACITY;
            responseParams.m_assocShortAddr = Mac16Address("FF:FF");
        }

        NS_LOG_DEBUG("Storing an Associate response command with the allocated"
                     " address "
                     << responseParams.m_assocShortAddr);

        m_mac->MlmeAssociateResponse(responseParams);
    }
}

void
ZigbeeNwk::NldeDataRequest(NldeDataRequestParams params, Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);

    if (params.m_dstAddr == m_nwkNetworkAddress)
    {
        NS_LOG_DEBUG("The source and the destination of the route request are the same!");
        return;
    }

    // Zigbee specification r22.1.0, Section 3.2.1.1.3 and Section 3.6.2.1
    // check that we are associated
    if (m_nwkNetworkAddress == "FF:FF")
    {
        NS_LOG_DEBUG("Cannot send data, the device is not currently associated");

        if (!m_nldeDataConfirmCallback.IsNull())
        {
            NldeDataConfirmParams confirmParams;
            confirmParams.m_status = ZigbeeNwkStatus::INVALID_REQUEST;
            confirmParams.m_txTime = Simulator::Now();
            confirmParams.m_nsduHandle = params.m_nsduHandle;
            m_nldeDataConfirmCallback(confirmParams);
        }
        return;
    }

    // Constructing the NPDU (Zigbee specification r22.1.0, Section 3.2.1.1.3 and Section 3.6.2.1)
    ZigbeeNwkHeader nwkHeader;
    nwkHeader.SetFrameType(DATA);
    nwkHeader.SetProtocolVer(3);
    nwkHeader.SetDiscoverRoute(static_cast<DiscoverRouteType>(params.m_discoverRoute));
    nwkHeader.SetDstAddr(params.m_dstAddr);

    if (params.m_useAlias)
    {
        nwkHeader.SetSrcAddr(params.m_aliasSrcAddr);
        nwkHeader.SetSeqNum(params.m_aliasSeqNumber.GetValue());
    }
    else
    {
        nwkHeader.SetSrcAddr(m_nwkNetworkAddress);
        nwkHeader.SetSeqNum(m_nwkSequenceNumber.GetValue());
    }

    if (params.m_radius == 0)
    {
        nwkHeader.SetRadius(m_nwkMaxDepth * 2);
    }
    else
    {
        nwkHeader.SetRadius(params.m_radius);
    }

    if (params.m_securityEnable)
    {
        // TODO: Secure processing (Section 3.6.2.1)
        NS_ABORT_MSG("Security processing is currently not supported");
    }

    // Check the current device capabilities
    CapabilityInformation capability;
    capability.SetCapability(m_nwkCapabilityInformation);

    if (capability.GetDeviceType() == ENDDEVICE)
    {
        nwkHeader.SetEndDeviceInitiator();
    }

    if (params.m_dstAddrMode == MCST)
    {
        nwkHeader.SetMulticast();
        // TODO:
        // set the nwkHeader multicast control according to
        // the values of the non-member radios parameter
        // See 3.2.1.1.3
    }

    packet->AddHeader(nwkHeader);

    if (capability.GetDeviceType() == ROUTER)
    {
        if (params.m_dstAddrMode == MCST)
        {
            // The destination is MULTICAST (See 3.6.2)
            NS_ABORT_MSG("Multicast is currently not supported");
            // TODO
            return;
        }
        else if (IsBroadcastAddress(params.m_dstAddr))
        {
            // The destination is BROADCAST (See 3.6.5)
            NS_ABORT_MSG("Broadcast is currently not supported");
            // SendBroadcast(packet);
            // TODO
            return;
        }
        else
        {
            // The destination is UNICAST (See 3.6.3.3)
            SendUnicast(packet, params.m_nsduHandle);
        }
    }
    else
    {
        // The device is an END DEVICE
        // direct message to its Parent device (Coordinator)
        Ptr<NeighborTableEntry> entry;
        if (m_nwkNeighborTable.GetParent(entry))
        {
            McpsDataRequestParams mcpsDataparams;
            mcpsDataparams.m_txOptions = 0x01; // Acknowledment on.
            mcpsDataparams.m_dstPanId = m_nwkPanId;
            mcpsDataparams.m_msduHandle = params.m_nsduHandle;
            mcpsDataparams.m_srcAddrMode = SHORT_ADDR;
            mcpsDataparams.m_dstAddrMode = SHORT_ADDR;
            mcpsDataparams.m_dstAddr = entry->GetNwkAddr();
            m_mac->McpsDataRequest(mcpsDataparams, packet);
        }
        else
        {
            // Section 3.6.3.7.1
            // Link failure with Parent device
            // TODO
            /*
            if (!m_nlmeNwkStatusIndicationCallback.IsNull())
            {
                NlmeNetworkStatusIndication indicationParams;

                 m_networkStatusCode = PARENT_LINK_FAILURE;
                m_nlmeNwkStatusIndicationCallback(confirmParams);
            }
            */
        }
    }
    m_pendPrimitiveNwk = NLDE_NLME_NONE;
}

void
ZigbeeNwk::NlmeNetworkFormationRequest(NlmeNetworkFormationRequestParams params)
{
    NS_LOG_FUNCTION(this);

    NS_ASSERT_MSG(m_netFormParams.m_scanChannelList.channelPageCount ==
                      m_netFormParams.m_scanChannelList.channelsField.size(),
                  "channelsField and its channelPageCount size do not match "
                  "in networkFormationParams");

    if (!m_nwkcCoordinatorCapable)
    {
        m_pendPrimitiveNwk = NLDE_NLME_NONE;
        m_netFormParams = {};
        m_netFormParamsGen = nullptr;

        if (!m_nlmeNetworkFormationConfirmCallback.IsNull())
        {
            NlmeNetworkFormationConfirmParams confirmParams;
            confirmParams.m_status = ZigbeeNwkStatus::INVALID_REQUEST;
            m_nlmeNetworkFormationConfirmCallback(confirmParams);
        }
        return;
    }

    if (params.m_distributedNetwork)
    {
        // Zigbee Specification r22.1.0, 3.2.2.5 , 3)
        // Verify Distributed Network Address is in a valid range.
        // TODO: Verify the address is not > 0xFFF7
        if (params.m_distributedNetwork == Mac16Address("00:00"))
        {
            m_pendPrimitiveNwk = NLDE_NLME_NONE;
            m_netFormParams = {};
            m_netFormParamsGen = nullptr;

            if (!m_nlmeNetworkFormationConfirmCallback.IsNull())
            {
                NlmeNetworkFormationConfirmParams confirmParams;
                confirmParams.m_status = ZigbeeNwkStatus::INVALID_REQUEST;
                m_nlmeNetworkFormationConfirmCallback(confirmParams);
            }
            return;
        }
    }

    // 4. On receipt of this primitive the NLME shall first validate the
    //  ChannelListStructure parameter according to section 3.2.2.2.2.
    // (if nwkMacInterfaceTable support is added)
    // If validation fails the NLME-NETWORK-FORMATION.confirm
    // primitive shall be issued with a Status parameter set to INVALID_PARAMETER.

    if (params.m_scanChannelList.channelPageCount != 1)
    {
        NS_LOG_ERROR("Multi page scanning not supported");
    }

    // Only page 0 is supported (O-QPSK 250 kbps)
    // Take 5 MSB bits: b27-b31 to check the page
    uint32_t page = (params.m_scanChannelList.channelsField[0] >> 27) & (0x01F);

    if (page != 0)
    {
        NS_LOG_ERROR("PHY band not supported (Only page 0 is supported)");
    }

    uint8_t channelsCount = 0;
    for (int i = 11; i <= 26; i++)
    {
        channelsCount += (params.m_scanChannelList.channelsField[0] >> i) & 1;
    }

    m_pendPrimitiveNwk = NLME_NETWORK_FORMATION;
    m_netFormParams = params;

    if (channelsCount == 1)
    {
        // There is only 1 channel, skip energy scan and go directly to
        // active scan instead
        MlmeScanRequestParams mlmeParams;
        mlmeParams.m_chPage = page;
        mlmeParams.m_scanChannels = params.m_scanChannelList.channelsField[0];
        mlmeParams.m_scanDuration = params.m_scanDuration;
        mlmeParams.m_scanType = MLMESCAN_ACTIVE;
        m_mac->MlmeScanRequest(mlmeParams);
    }
    else if (channelsCount > 1)
    {
        MlmeScanRequestParams mlmeParams;
        mlmeParams.m_chPage = page;
        mlmeParams.m_scanChannels = params.m_scanChannelList.channelsField[0];
        mlmeParams.m_scanDuration = params.m_scanDuration;
        mlmeParams.m_scanType = MLMESCAN_ED;
        m_mac->MlmeScanRequest(mlmeParams);
    }
}

void
ZigbeeNwk::NlmeRouteDiscoveryRequest(NlmeRouteDiscoveryRequestParams params)
{
    NS_LOG_FUNCTION(this);

    if (params.m_dstAddr == m_nwkNetworkAddress && params.m_dstAddrMode == UCST_BCST)
    {
        NS_LOG_DEBUG("The source and the destination of the route request are the same!");
        return;
    }

    // (See 3.2.2.33.3)
    // - Check the device has routing capacity
    // - Check the device dstAddrMode != NO_ADDRESS &&  dst != Broadcast address
    if (params.m_dstAddrMode != NO_ADDRESS && IsBroadcastAddress(params.m_dstAddr))
    {
        if (!m_nlmeRouteDiscoveryConfirmCallback.IsNull())
        {
            NlmeRouteDiscoveryConfirmParams confirmParams;
            confirmParams.m_status = ZigbeeNwkStatus::INVALID_REQUEST;
            m_nlmeRouteDiscoveryConfirmCallback(confirmParams);
        }
        return;
    }

    CapabilityInformation capability;
    capability.SetCapability(m_nwkCapabilityInformation);
    if (capability.GetDeviceType() != ROUTER && params.m_dstAddrMode != NO_ADDRESS)
    {
        if (!m_nlmeRouteDiscoveryConfirmCallback.IsNull())
        {
            NlmeRouteDiscoveryConfirmParams confirmParams;
            confirmParams.m_status = ZigbeeNwkStatus::ROUTE_ERROR;
            m_nlmeRouteDiscoveryConfirmCallback(confirmParams);
        }
        return;
    }

    m_pendPrimitiveNwk = NLME_ROUTE_DISCOVERY;

    ZigbeeNwkHeader nwkHeader;
    nwkHeader.SetFrameType(NWK_COMMAND);
    nwkHeader.SetProtocolVer(m_nwkcProtocolVersion);
    nwkHeader.SetDiscoverRoute(DiscoverRouteType::ENABLE_ROUTE_DISCOVERY);
    // See r22.1.0, Table 3-69
    // Set destination to broadcast (all routers and coordinator)
    nwkHeader.SetDstAddr(Mac16Address("FF:FC"));
    nwkHeader.SetSrcAddr(m_nwkNetworkAddress);
    nwkHeader.SetSeqNum(m_nwkSequenceNumber.GetValue());

    ZigbeePayloadRouteRequestCommand payload;
    payload.SetRouteReqId(m_routeRequestId.GetValue());
    payload.SetPathCost(0);

    if (params.m_dstAddrMode == UCST_BCST)
    {
        // Set the rest of the nwkHeader and command payload parameters
        // as described in Zigbee specification, Section 3.2.2.33.3
        if (params.m_radius == 0)
        {
            nwkHeader.SetRadius(m_nwkMaxDepth * 2);
        }
        else
        {
            nwkHeader.SetRadius(params.m_radius);
        }

        payload.SetDstAddr(params.m_dstAddr);

        Mac16Address nextHop;
        RouteDiscoveryStatus routeStatus =
            FindNextHop(m_nwkNetworkAddress, 0, nwkHeader, payload, nextHop);

        if (routeStatus == ROUTE_FOUND)
        {
            if (!m_nlmeRouteDiscoveryConfirmCallback.IsNull())
            {
                NlmeRouteDiscoveryConfirmParams confirmParams;
                confirmParams.m_status = ZigbeeNwkStatus::SUCCESS;
                m_nlmeRouteDiscoveryConfirmCallback(confirmParams);
            }
        }
        else if (routeStatus == ROUTE_NOT_FOUND)
        {
            // Route not found. Route marked as DISCOVER UNDERWAY,
            // we initiate route discovery.
            Simulator::Schedule(MilliSeconds(m_rreqJitter->GetValue()),
                                &ZigbeeNwk::SendRREQ,
                                this,
                                nwkHeader,
                                payload,
                                m_nwkcInitialRREQRetries);

            m_nwkSequenceNumber++;
            m_routeRequestId++;
        }
    }
    else if (params.m_dstAddrMode == MCST)
    {
        NS_ABORT_MSG("Multicast Route discovery not supported");
    }
    else if (params.m_dstAddrMode == NO_ADDRESS)
    {
        // Many-to-one route discovery.
        // (See Last paragraph of Zigbee Specification, Section 3.6.3.5.1)
        m_nwkIsConcentrator = true;

        nwkHeader.SetRadius(m_nwkConcentratorRadius);

        payload.SetDstAddr(Mac16Address("FF:FF"));
        if (params.m_noRouteCache)
        {
            payload.SetCmdOptManyToOneField(ManyToOne::NO_ROUTE_RECORD);
        }
        else
        {
            payload.SetCmdOptManyToOneField(ManyToOne::ROUTE_RECORD);
        }

        RouteDiscoveryStatus routeStatus =
            ProcessManyToOneRoute(m_nwkNetworkAddress, 0, nwkHeader, payload);

        if (routeStatus == MANY_TO_ONE_ROUTE || routeStatus == ROUTE_UPDATED)
        {
            // TODO if nwkConcentratorDiscoveryTime != 0,  schedule
            // RREQ every nwkConcentratorDiscovery time.

            Simulator::Schedule(MilliSeconds(m_rreqJitter->GetValue()),
                                &ZigbeeNwk::SendRREQ,
                                this,
                                nwkHeader,
                                payload,
                                0);
            m_nwkSequenceNumber++;
            m_routeRequestId++;
        }
    }
}

void
ZigbeeNwk::NlmeNetworkDiscoveryRequest(NlmeNetworkDiscoveryRequestParams params)
{
    NS_LOG_FUNCTION(this);

    if (params.m_scanDuration > 14)
    {
        NS_LOG_ERROR("Scan duration must be an int between 0 and 14");
    }

    if (params.m_scanChannelList.channelPageCount != params.m_scanChannelList.channelsField.size())
    {
        NS_LOG_ERROR("In scanChannelList parameter, channelPageCount "
                     "and the channelsField structure size does not match");
    }

    // TODO: Add support to scan other MAC interfaces, for the moment
    // only a single interface and only Page 0 is supported (PHY O-QPSK 250 kbps)

    if (params.m_scanChannelList.channelsField.size() != 1)
    {
        NS_LOG_ERROR("Only a single MAC interface supported");
    }

    uint8_t page = (params.m_scanChannelList.channelsField[0] >> 27) & (0x01F);
    if (page != 0)
    {
        NS_LOG_ERROR("Only Page 0 (O-QPSK 250 kbps) is supported.");
    }

    m_pendPrimitiveNwk = NLME_NET_DISCV;

    MlmeScanRequestParams scanParams;
    scanParams.m_chPage = 0; // Only page 0 is supported.
    scanParams.m_scanChannels = params.m_scanChannelList.channelsField[0];
    scanParams.m_scanDuration = params.m_scanDuration;
    scanParams.m_scanType = MLMESCAN_ACTIVE;

    NS_LOG_DEBUG("Active scanning started, "
                 << " on page " << static_cast<uint32_t>(page) << " and channels 0x" << std::hex
                 << params.m_scanChannelList.channelsField[0] << std::dec);
    m_mac->MlmeScanRequest(scanParams);
}

void
ZigbeeNwk::NlmeDirectJoinRequest(NlmeDirectJoinRequestParams params)
{
    NS_LOG_FUNCTION(this);

    // TODO: Check the device is router or coordinator, send invalid_request
    //       status otherwise. See 3.6.1.4.3.

    Ptr<NeighborTableEntry> entry;
    if (m_nwkNeighborTable.LookUpEntry(params.m_deviceAddr, entry))
    {
        NS_LOG_DEBUG("[NLME-DIRECT-JOIN.request]: "
                     "Device already present in neighbor table. ");

        if (!m_nlmeDirectJoinConfirmCallback.IsNull())
        {
            NlmeDirectJoinConfirmParams confirmParams;
            confirmParams.m_status = ZigbeeNwkStatus::ALREADY_PRESENT;
            confirmParams.m_deviceAddr = params.m_deviceAddr;
            m_nlmeDirectJoinConfirmCallback(confirmParams);
        }
    }
    else
    {
        CapabilityInformation capaInfo;
        capaInfo.SetCapability(params.m_capabilityInfo);

        Mac16Address allocatedAddr;
        if (capaInfo.IsAllocateAddrOn())
        {
            allocatedAddr = AllocateNetworkAddress();
        }
        else
        {
            // The device is associated but it will only use its
            // extended address (EUI-64 also known as IEEE Address)
            allocatedAddr = Mac16Address("FF:FE");
        }

        NwkDeviceType devType;
        if (capaInfo.GetDeviceType() == MacDeviceType::ROUTER)
        {
            devType = NwkDeviceType::ZIGBEE_ROUTER;
        }
        else
        {
            devType = NwkDeviceType::ZIGBEE_ENDDEVICE;
        }

        Ptr<NeighborTableEntry> newEntry =
            Create<NeighborTableEntry>(params.m_deviceAddr,
                                       allocatedAddr,
                                       devType,
                                       capaInfo.IsReceiverOnWhenIdle(),
                                       0,
                                       Seconds(15728640),
                                       Minutes(RequestedTimeoutField[m_nwkEndDeviceTimeoutDefault]),
                                       NBR_CHILD,
                                       0,
                                       255,
                                       0,
                                       0,
                                       true,
                                       0);

        NlmeDirectJoinConfirmParams confirmParams;

        if (m_nwkNeighborTable.AddEntry(newEntry))
        {
            NS_LOG_DEBUG("Device added to neighbor table (" << m_nwkNeighborTable.GetSize()
                                                            << ") with address [" << allocatedAddr
                                                            << " | " << params.m_deviceAddr << "]");
            if (!m_nlmeDirectJoinConfirmCallback.IsNull())
            {
                confirmParams.m_status = ZigbeeNwkStatus::SUCCESS;
                confirmParams.m_deviceAddr = params.m_deviceAddr;
                m_nlmeDirectJoinConfirmCallback(confirmParams);
            }
        }
        else
        {
            NS_LOG_DEBUG("Error, neighbor table is full");
            if (!m_nlmeDirectJoinConfirmCallback.IsNull())
            {
                confirmParams.m_status = ZigbeeNwkStatus::NEIGHBOR_TABLE_FULL;
                confirmParams.m_deviceAddr = params.m_deviceAddr;
                m_nlmeDirectJoinConfirmCallback(confirmParams);
            }
        }
    }
}

void
ZigbeeNwk::NlmeJoinRequest(NlmeJoinRequestParams params)
{
    NS_LOG_FUNCTION(this);

    if (params.m_scanDuration > 14)
    {
        NS_LOG_ERROR("Scan duration must be an int between 0 and 14");
    }

    if (params.m_scanChannelList.channelPageCount != params.m_scanChannelList.channelsField.size())
    {
        NS_LOG_ERROR("In scanChannelList parameter, channelPageCount "
                     "and the channelsField structure size does not match");
    }

    // TODO: Add support to scan other MAC interfaces, for the moment
    // only a single interface and only Page 0 is supported (PHY O-QPSK 250 kbps)

    // TODO: Only devices who have not join another network can call JOIN.

    if (params.m_scanChannelList.channelsField.size() != 1)
    {
        NS_LOG_ERROR("Only a single MAC interface supported");
    }

    uint8_t page = (params.m_scanChannelList.channelsField[0] >> 27) & (0x01F);
    if (page != 0)
    {
        NS_LOG_ERROR("Only Page 0 (O-QPSK 250 kbps) is supported.");
    }

    m_pendPrimitiveNwk = NLME_JOIN;
    m_joinParams = params;

    if (params.m_rejoinNetwork == DIRECT_OR_REJOIN)
    {
        // Zigbee specification r22.1.0 Section 3.6.1.4.3.1,
        // Child procedure for joining or re-joining a network through
        // orphaning (DIRECT JOIN procedure).

        MlmeScanRequestParams scanParams;
        scanParams.m_chPage = page;
        scanParams.m_scanChannels = params.m_scanChannelList.channelsField[0];

        // Note: Scan duration is fixed to a macResponseWaitTime in an Orphan scan
        //       (i.e. It does not use the scanDuration parameter)
        scanParams.m_scanType = MLMESCAN_ORPHAN;
        NS_LOG_DEBUG("Orphan scanning started, "
                     << "sending orphan notifications on page " << static_cast<uint32_t>(page)
                     << " and channels " << std::hex << params.m_scanChannelList.channelsField[0]);
        m_mac->MlmeScanRequest(scanParams);
    }
    else if (params.m_rejoinNetwork == ASSOCIATION)
    {
        // Check if we have the MAC pan id info recorded during the discovery process
        uint16_t panId;
        if (!m_panIdTable.GetEntry(params.m_extendedPanId, panId))
        {
            NS_LOG_ERROR("Error PAN id of neighbor device not found");
        }

        // Zigbee specification r22.1.0 Section 3.6.1.4.1
        // Child procedure for joining a network through ASSOCIATION.

        NlmeJoinConfirmParams joinConfirmParams;
        Ptr<NeighborTableEntry> bestParentEntry;

        if (m_nwkNeighborTable.LookUpForBestParent(params.m_extendedPanId, bestParentEntry))
        {
            MlmeAssociateRequestParams assocParams;
            m_nwkCapabilityInformation = params.m_capabilityInfo;

            assocParams.m_chNum = bestParentEntry->GetLogicalCh();
            assocParams.m_chPage = 0; // Zigbee assumes Page is always 0
            assocParams.m_capabilityInfo = params.m_capabilityInfo;
            assocParams.m_coordPanId = panId;

            if (bestParentEntry->GetNwkAddr() != Mac16Address("FF:FE"))
            {
                assocParams.m_coordAddrMode = lrwpan::AddressMode::SHORT_ADDR;
                assocParams.m_coordShortAddr = bestParentEntry->GetNwkAddr();
                NS_LOG_DEBUG("Send Assoc. Req. to [" << bestParentEntry->GetNwkAddr()
                                                     << "] in PAN id and Ext PAN id: " << std::hex
                                                     << "(0x" << panId << " | 0x"
                                                     << params.m_extendedPanId << ")" << std::dec);
            }
            else
            {
                assocParams.m_coordAddrMode = lrwpan::AddressMode::EXT_ADDR;
                assocParams.m_coordExtAddr = bestParentEntry->GetExtAddr();
                NS_LOG_DEBUG("Send Assoc. Req. to [" << bestParentEntry->GetNwkAddr()
                                                     << "] in \nPAN id and Ext PAN id: " << std::hex
                                                     << "(0x" << panId << " | 0x"
                                                     << params.m_extendedPanId << ")" << std::dec);
            }

            m_nwkParentInformation = 0;
            m_nwkCapabilityInformation = params.m_capabilityInfo;

            // Temporally store some associate values until the process concludes
            m_associateParams.panId = panId;
            m_associateParams.extAddress = bestParentEntry->GetExtAddr();

            m_mac->MlmeAssociateRequest(assocParams);
        }
        else
        {
            m_pendPrimitiveNwk = NLDE_NLME_NONE;
            m_joinParams = {};

            if (!m_nlmeJoinConfirmCallback.IsNull())
            {
                joinConfirmParams.m_extendedPanId = params.m_extendedPanId;
                joinConfirmParams.m_networkAddress = Mac16Address("FF:FF");
                joinConfirmParams.m_enhancedBeacon = false;
                joinConfirmParams.m_macInterfaceIndex = 0;
                joinConfirmParams.m_status = ZigbeeNwkStatus::NOT_PERMITED;
                m_nlmeJoinConfirmCallback(joinConfirmParams);
            }
        }
    }
    else
    {
        NS_LOG_ERROR("Joining method not supported");
    }
}

void
ZigbeeNwk::NlmeStartRouterRequest(NlmeStartRouterRequestParams params)
{
    NS_LOG_FUNCTION(this);

    NS_ASSERT_MSG(params.m_beaconOrder == 15, "Beacon mode not supported for zigbee");
    NS_ASSERT_MSG(params.m_superframeOrder == 15, "Beacon mode not supported for zigbee");

    CapabilityInformation capability;
    capability.SetCapability(m_nwkCapabilityInformation);

    if (capability.GetDeviceType() != MacDeviceType::ROUTER)
    {
        if (!m_nlmeStartRouterConfirmCallback.IsNull())
        {
            NlmeStartRouterConfirmParams confirmParams;
            confirmParams.m_status = ZigbeeNwkStatus::INVALID_REQUEST;
            m_nlmeStartRouterConfirmCallback(confirmParams);
        }
        NS_LOG_ERROR("This device is not a Zigbee Router or is not joined to this network");
    }
    else
    {
        m_pendPrimitiveNwk = NLME_START_ROUTER;

        // request an update of the current channel in use in the PHY
        m_mac->MlmeGetRequest(MacPibAttributeIdentifier::pCurrentChannel);

        // TODO: MLME-START.request should be issue to all the interfaces in the
        // nwkMacInterfaceTable (currently not supported), for the moment only a single
        // interface is supported.
        MlmeStartRequestParams startParams;
        startParams.m_logCh = m_currentChannel;
        startParams.m_logChPage = 0; // In zigbee, only page 0 is supported.
        startParams.m_PanId = m_nwkPanId;
        startParams.m_bcnOrd = params.m_beaconOrder;
        startParams.m_sfrmOrd = params.m_superframeOrder;
        startParams.m_battLifeExt = params.m_batteryLifeExt;
        startParams.m_coorRealgn = false;
        startParams.m_panCoor = false;
        m_mac->MlmeStartRequest(startParams);
    }
}

void
ZigbeeNwk::SetNldeDataIndicationCallback(NldeDataIndicationCallback c)
{
    m_nldeDataIndicationCallback = c;
}

void
ZigbeeNwk::SetNldeDataConfirmCallback(NldeDataConfirmCallback c)
{
    m_nldeDataConfirmCallback = c;
}

void
ZigbeeNwk::SetNlmeNetworkFormationConfirmCallback(NlmeNetworkFormationConfirmCallback c)
{
    m_nlmeNetworkFormationConfirmCallback = c;
}

void
ZigbeeNwk::SetNlmeNetworkDiscoveryConfirmCallback(NlmeNetworkDiscoveryConfirmCallback c)
{
    m_nlmeNetworkDiscoveryConfirmCallback = c;
}

void
ZigbeeNwk::SetNlmeRouteDiscoveryConfirmCallback(NlmeRouteDiscoveryConfirmCallback c)
{
    m_nlmeRouteDiscoveryConfirmCallback = c;
}

void
ZigbeeNwk::SetNlmeDirectJoinConfirmCallback(NlmeDirectJoinConfirmCallback c)
{
    m_nlmeDirectJoinConfirmCallback = c;
}

void
ZigbeeNwk::SetNlmeJoinConfirmCallback(NlmeJoinConfirmCallback c)
{
    m_nlmeJoinConfirmCallback = c;
}

void
ZigbeeNwk::SetNlmeJoinIndicationCallback(NlmeJoinIndicationCallback c)
{
    m_nlmeJoinIndicationCallback = c;
}

void
ZigbeeNwk::SetNlmeStartRouterConfirmCallback(NlmeStartRouterConfirmCallback c)
{
    m_nlmeStartRouterConfirmCallback = c;
}

void
ZigbeeNwk::EnqueueTx(Ptr<Packet> p, uint8_t nsduHandle)
{
    // TODO : PurgeTxQueue();
    if (m_pendingTxQueue.size() < m_maxPendingTxQueueSize)
    {
        ZigbeeNwkHeader peekedNwkHeader;
        p->PeekHeader(peekedNwkHeader);

        Ptr<PendingTxPkt> pendingTxPkt = Create<PendingTxPkt>();
        pendingTxPkt->dstAddr = peekedNwkHeader.GetDstAddr();
        pendingTxPkt->nsduHandle = nsduHandle;
        pendingTxPkt->txPkt = p;
        // TODO: expiration time here
        m_pendingTxQueue.emplace_back(pendingTxPkt);
        // TODO: pending trace here
    }
    else
    {
        // TODO: Drop trace here
    }
}

bool
ZigbeeNwk::DequeueTx(Mac16Address dst, Ptr<PendingTxPkt> entry)
{
    // TODO : PurgeTxQueue();

    for (auto iter = m_pendingTxQueue.begin(); iter != m_pendingTxQueue.end(); iter++)
    {
        if ((*iter)->dstAddr == dst)
        {
            *entry = **iter;
            // TODO: Dequeue trace if needed here.
            m_pendingTxQueue.erase(iter);
            return true;
        }
    }
    return false;
}

Mac16Address
ZigbeeNwk::AllocateNetworkAddress()
{
    if (m_nwkAddrAlloc == DISTRIBUTED_ALLOC)
    {
        NS_LOG_ERROR("Distruted Allocation not supported");
        return Mac16Address("FF:FF");
    }
    else if (m_nwkAddrAlloc == STOCHASTIC_ALLOC)
    {
        // See nwkNetworkAddress valid range Zigbee specification r22.1.0, 3.5.2
        // Valid values in the Zigbee specification goes from 1 to 0xFFF7,
        // However, the range 0x8000 to 0x9FFF is used for multicast in other networks
        // (i.e. IPV6 over IEEE 802.15.4) for this reason, we avoid this range as well.
        // See RFC 4944, Section 9
        uint16_t rndValue = m_uniformRandomVariable->GetInteger(1, 0x7FFF);
        uint16_t rndValue2 = m_uniformRandomVariable->GetInteger(0xA000, 0xFFF7);
        uint16_t rndValue3 = m_uniformRandomVariable->GetInteger(1, 2);

        Mac16Address allocAddr;
        if (rndValue3 == 1)
        {
            allocAddr = Mac16Address(rndValue);
        }
        else
        {
            allocAddr = Mac16Address(rndValue2);
        }
        return allocAddr;
    }
    else
    {
        NS_LOG_ERROR("Address allocation method not supported");
        return Mac16Address("FF:FF");
    }
}

uint8_t
ZigbeeNwk::GetLQINonLinearValue(uint8_t lqi) const
{
    uint8_t mappedValue;

    if (lqi > 50)
    {
        mappedValue = 1;
    }
    else if ((lqi <= 50) && (lqi > 45))
    {
        mappedValue = 2;
    }
    else if ((lqi <= 45) && (lqi > 40))
    {
        mappedValue = 3;
    }
    else if ((lqi <= 40) && (lqi > 38))
    {
        mappedValue = 4;
    }
    else if ((lqi <= 38) && (lqi > 35))
    {
        mappedValue = 5;
    }
    else if ((lqi <= 35) && (lqi > 24))
    {
        mappedValue = 6;
    }
    else
    {
        mappedValue = 7;
    }

    return mappedValue;
}

uint8_t
ZigbeeNwk::GetLinkCost(uint8_t lqi) const
{
    if (m_nwkReportConstantCost)
    {
        // Hop count based. Report constant value
        return 7;
    }
    else
    {
        // Based on non-linear mapping of LQI
        return GetLQINonLinearValue(lqi);
    }
}

void
ZigbeeNwk::SendRREQ(ZigbeeNwkHeader nwkHeader,
                    ZigbeePayloadRouteRequestCommand payload,
                    uint8_t rreqRetries)
{
    NS_LOG_FUNCTION(this);

    ZigbeePayloadType payloadType(ROUTE_REQ_CMD);

    Ptr<Packet> nsdu = Create<Packet>();
    nsdu->AddHeader(payload);
    nsdu->AddHeader(payloadType);
    nsdu->AddHeader(nwkHeader);

    if (payload.GetCmdOptManyToOneField() == ManyToOne::NO_MANY_TO_ONE &&
        nwkHeader.GetRadius() != 0)
    {
        // Set RREQ RETRIES
        Time rreqRetryTime =
            Simulator::Now() + m_nwkcRREQRetryInterval + MilliSeconds(m_rreqJitter->GetValue());

        Ptr<RreqRetryTableEntry> rreqRetryTableEntry;
        if (m_rreqRetryTable.LookUpEntry(payload.GetRouteReqId(), rreqRetryTableEntry))
        {
            if (rreqRetryTableEntry->GetRreqRetryCount() >= rreqRetries)
            {
                NS_LOG_DEBUG("Maximum RREQ retries reached for dst ["
                             << payload.GetDstAddr() << "] and rreq ID "
                             << static_cast<uint32_t>(payload.GetRouteReqId()));
                // Note: The value of the maximum number of retries (rreqRetries) is either
                // nwkcInitialRREQRetries or nwkcRREQRetries depending on where the RREQ is
                // transmitted. See Zigbee specification r22.1.0, Section 3.6.3.5.1 This trace here
                // is used to keep track when the maximum RREQ retries is reached.
                m_rreqRetriesExhaustedTrace(payload.GetRouteReqId(),
                                            payload.GetDstAddr(),
                                            rreqRetries);
            }
            else
            {
                // Schedule the next RREQ RETRY event and update entry.
                EventId rreqRetryEvent = Simulator::Schedule(rreqRetryTime,
                                                             &ZigbeeNwk::SendRREQ,
                                                             this,
                                                             nwkHeader,
                                                             payload,
                                                             rreqRetries);

                rreqRetryTableEntry->SetRreqRetryCount(rreqRetryTableEntry->GetRreqRetryCount() +
                                                       1);
                rreqRetryTableEntry->SetRreqEventId(rreqRetryEvent);
            }
        }
        else
        {
            // Schedule the next RREQ RETRY and add a new record of the event.
            EventId rreqRetryEvent = Simulator::Schedule(rreqRetryTime,
                                                         &ZigbeeNwk::SendRREQ,
                                                         this,
                                                         nwkHeader,
                                                         payload,
                                                         rreqRetries);

            Ptr<RreqRetryTableEntry> newEntry =
                Create<RreqRetryTableEntry>(payload.GetRouteReqId(), rreqRetryEvent, 0);

            m_rreqRetryTable.AddEntry(newEntry);
        }
    }

    // Send the RREQ
    // See Section 3.4.1.1 Mac Data Service Requirements for RREQ
    if (nwkHeader.GetRadius() != 0)
    {
        McpsDataRequestParams params;
        params.m_dstPanId = m_nwkPanId;
        params.m_srcAddrMode = SHORT_ADDR;
        params.m_dstAddrMode = SHORT_ADDR;
        params.m_dstAddr = Mac16Address::GetBroadcast();
        // params.m_txOptions
        m_mac->McpsDataRequest(params, nsdu);
    }
    else
    {
        NS_LOG_DEBUG("Maximum radius reached, dropping RREQ");
    }
}

void
ZigbeeNwk::SendRREP(Mac16Address nextHop,
                    Mac16Address originator,
                    Mac16Address responder,
                    uint8_t rreqId,
                    uint8_t pathcost)
{
    NS_LOG_FUNCTION(this);

    ZigbeeNwkHeader nwkHeader;
    nwkHeader.SetFrameType(NWK_COMMAND);
    nwkHeader.SetProtocolVer(m_nwkcProtocolVersion);
    nwkHeader.SetDiscoverRoute(DiscoverRouteType::ENABLE_ROUTE_DISCOVERY);

    nwkHeader.SetDstAddr(nextHop);
    nwkHeader.SetSrcAddr(m_nwkNetworkAddress);
    m_nwkSequenceNumber++;
    nwkHeader.SetSeqNum(m_nwkSequenceNumber.GetValue());
    // see Zigbee specification 3.4.2.2
    // Use the maximum possible radius
    nwkHeader.SetRadius(m_nwkMaxDepth * 2);

    ZigbeePayloadType payloadType(ROUTE_REP_CMD);

    ZigbeePayloadRouteReplyCommand payload;
    payload.SetRouteReqId(rreqId);
    payload.SetOrigAddr(originator);
    payload.SetRespAddr(responder);
    payload.SetPathCost(pathcost);

    // See Section 3.4.2 Mac Data Service Requirements for RREP
    McpsDataRequestParams params;
    params.m_dstPanId = m_nwkPanId;
    params.m_srcAddrMode = SHORT_ADDR;
    params.m_dstAddrMode = SHORT_ADDR;
    params.m_dstAddr = nextHop;
    // params.m_txOptions

    Ptr<Packet> nsdu = Create<Packet>();
    nsdu->AddHeader(payload);
    nsdu->AddHeader(payloadType);
    nsdu->AddHeader(nwkHeader);

    m_mac->McpsDataRequest(params, nsdu);
}

int64_t
ZigbeeNwk::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_uniformRandomVariable->SetStream(stream);
    return 1;
}

void
ZigbeeNwk::UpdateBeaconPayload()
{
    NS_LOG_FUNCTION(this);

    ZigbeeBeaconPayload beaconPayloadHeader;
    beaconPayloadHeader.SetStackProfile(static_cast<uint8_t>(m_nwkStackProfile));
    beaconPayloadHeader.SetRouterCapacity(m_nwkcCoordinatorCapable);
    beaconPayloadHeader.SetDeviceDepth(0); // Not used by stack profile (0x02 =ZIGBEE pro)
    beaconPayloadHeader.SetEndDevCapacity(true);
    beaconPayloadHeader.SetExtPanId(m_nwkExtendedPanId);
    beaconPayloadHeader.SetTxOffset(0xFFFFFF);
    // TODO: beaconPayload.SetNwkUpdateId(m_nwkUpdateId);
    Ptr<Packet> payload = Create<Packet>();
    payload->AddHeader(beaconPayloadHeader);

    // Extract octets from payload
    // uint32_t payloadSize = payload->GetSize();
    // uint8_t octets [payloadSize];
    // uint8_t* octetsPtr =  octets;
    auto octetsPtr = new uint8_t[payload->GetSize()];
    payload->CopyData(octetsPtr, payload->GetSize());

    // Add octets to macBeaconPayload vector
    Ptr<MacPibAttributes> pibAttr = Create<MacPibAttributes>();
    pibAttr->macBeaconPayload = std::vector<uint8_t>(octetsPtr, octetsPtr + payload->GetSize());
    // pibAttr->macBeaconPayloadLength = payload->GetSize();
    m_mac->MlmeSetRequest(MacPibAttributeIdentifier::macBeaconPayload, pibAttr);
}

} // namespace zigbee
} // namespace ns3
