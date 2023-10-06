#include "wifi-net-device-anim.h"

#include "wifi-net-device.h"

#include "ns3/animation-interface.h"
#include "ns3/config.h"
#include "ns3/proxy.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-psdu.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("WifiNetDeviceAnim");

NS_OBJECT_ENSURE_REGISTERED(WifiNetDeviceAnim);

WifiNetDeviceAnim::WifiAnimUidPacketInfoMap WifiNetDeviceAnim::m_pendingWifiPackets;
uint64_t WifiNetDeviceAnim::wifiAnimUid = 0;
EventId WifiNetDeviceAnim::m_purgeWifiAnimPendingPacketsEventId = EventId();
Time WifiNetDeviceAnim::wifiPurgeInterval = Seconds(5);
Time WifiNetDeviceAnim::schedulePurgePendingPackets = MilliSeconds(25);

TypeId
WifiNetDeviceAnim::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WifiNetDeviceAnim")
                            .SetParent<Object>()
                            .AddConstructor<WifiNetDeviceAnim>()
                            .SetGroupName("WifiNetDeviceAnim");

    return tid;
}

void
WifiNetDeviceAnim::ConnectCallbacks()
{
    if (!m_netDev)
    {
        m_netDev = GetObject<WifiNetDevice>();
        NS_ASSERT_MSG(true, "Failed to retrieve net-device");
    }
    // std::cout << m_netDev << std::endl;
    // Wifi Phy
    m_netDev->TraceConnectWithoutContext(
        "Phy/PhyTxPsduBegin",
        MakeCallback(&ns3::WifiNetDeviceAnim::WifiPhyTxBeginTrace, this));
    m_netDev->TraceConnectWithoutContext(
        "Phy/PhyRxBegin",
        MakeCallback(&ns3::WifiNetDeviceAnim::WifiPhyRxBeginTrace, this));
    // Wifi Mac
    m_netDev->TraceConnectWithoutContext(
        "Mac/MacTx",
        MakeCallback(&ns3::WifiNetDeviceAnim::WifiMacTxTrace, this));
    m_netDev->TraceConnectWithoutContext(
        "Mac/MacTxDrop",
        MakeCallback(&ns3::WifiNetDeviceAnim::WifiMacTxDropTrace, this));
    m_netDev->TraceConnectWithoutContext(
        "Mac/MacRx",
        MakeCallback(&ns3::WifiNetDeviceAnim::WifiMacRxTrace, this));
    m_netDev->TraceConnectWithoutContext(
        "Mac/MacRxDrop",
        MakeCallback(&ns3::WifiNetDeviceAnim::WifiMacRxDropTrace, this));

    // Wifi Phy
    m_netDev->TraceConnectWithoutContext(
        "Phy/PhyTxDrop",
        MakeCallback(&ns3::WifiNetDeviceAnim::WifiPhyTxDropTrace, this));
    m_netDev->TraceConnectWithoutContext(
        "Phy/PhyRxDrop",
        MakeCallback(&ns3::WifiNetDeviceAnim::WifiPhyRxDropTrace, this));
}

void
WifiNetDeviceAnim::WifiPhyTxBeginTrace(WifiConstPsduMap psduMap,
                                       WifiTxVector /* txVector */,
                                       double /* txPowerW */)
{
    std::cout << "trace called" << std::endl;
    NS_LOG_FUNCTION(this);
    if (!IsEnabled())
    {
        return;
    }
    m_anim->UpdatePosition(m_netDev->GetNode());

    WifiAnimPacketInfo pktInfo(m_netDev->GetNode()->GetId(), Simulator::Now());
    for (auto& psdu : psduMap)
    {
        for (auto& mpdu : *PeekPointer(psdu.second))
        {
            ++wifiAnimUid;
            NS_LOG_INFO("WifiPhyTxTrace for MPDU:" << wifiAnimUid);
            m_anim->AddByteTag(
                wifiAnimUid,
                mpdu->GetPacket()); // the underlying MSDU/A-MSDU should be handed off
            m_pendingWifiPackets.insert(WifiAnimUidPacketInfoMap::value_type(wifiAnimUid, pktInfo));
            if (!m_purgeWifiAnimPendingPacketsEventId.IsRunning())
            {
                Simulator::Schedule(schedulePurgePendingPackets,
                                    &WifiNetDeviceAnim::PurgePendingPackets);
            }
            OutputWirelessPacketTxInfo(
                mpdu->GetProtocolDataUnit(),
                m_pendingWifiPackets.at(wifiAnimUid),
                wifiAnimUid); // PDU should be considered in order to have header
        }
    }

    Ptr<WifiNetDevice> netDevice = DynamicCast<WifiNetDevice>(m_netDev);
    if (netDevice)
    {
        Mac48Address nodeAddr = netDevice->GetMac()->GetAddress();
        std::ostringstream oss;
        oss << nodeAddr;
        Ptr<Node> n = netDevice->GetNode();
        NS_ASSERT(n);
        std::map<std::string, uint32_t>& m_macToNodeIdMap = m_anim->GetMacToNodeIdMap();
        m_macToNodeIdMap[oss.str()] = n->GetId();
        NS_LOG_INFO("Added Mac" << oss.str() << " node:" << m_macToNodeIdMap[oss.str()]);
    }
    else
    {
        NS_ABORT_MSG("This NetDevice should be a Wi-Fi network device");
    }
}

void
WifiNetDeviceAnim::WifiPhyRxBeginTrace(Ptr<const Packet> p, RxPowerWattPerChannelBand rxPowersW)
{
    std::cout << "trace 1called" << std::endl;
    NS_LOG_FUNCTION(this);
    if (!IsEnabled())
    {
        return;
    }
    m_anim->UpdatePosition(m_netDev->GetNode());
    uint64_t animUid = m_anim->GetAnimUidFromPacket(p);
    NS_LOG_INFO("Wifi RxBeginTrace for packet: " << animUid);
    if (m_pendingWifiPackets.find(wifiAnimUid) == m_pendingWifiPackets.end())
    {
        NS_ASSERT_MSG(false, "WifiPhyRxBeginTrace: unknown Uid");
        std::ostringstream oss;
        WifiMacHeader hdr;
        if (!p->PeekHeader(hdr))
        {
            NS_LOG_WARN("WifiMacHeader not present");
            return;
        }
        oss << hdr.GetAddr2();
        std::map<std::string, uint32_t>& m_macToNodeIdMap = m_anim->GetMacToNodeIdMap();
        if (m_macToNodeIdMap.find(oss.str()) == m_macToNodeIdMap.end())
        {
            NS_LOG_WARN("Transmitter Mac address " << oss.str() << " never seen before. Skipping");
            return;
        }
        Ptr<Node> txNode = NodeList::GetNode(m_macToNodeIdMap[oss.str()]);
        m_anim->UpdatePosition(txNode);
        WifiAnimPacketInfo pktInfo(m_macToNodeIdMap[oss.str()], Simulator::Now());
        m_pendingWifiPackets.insert(WifiAnimUidPacketInfoMap::value_type(animUid, pktInfo));
        NS_LOG_WARN("WifiPhyRxBegin: unknown Uid, but we are adding a wifi packet");
    }
    /// \todo NS_ASSERT (WifiPacketIsPending (animUid) == true);

    // Talk to Tommaso about this
    m_pendingWifiPackets[animUid].ProcessRxBegin(m_netDev, Simulator::Now());
    OutputWirelessPacketRxInfo(p, m_pendingWifiPackets[animUid], animUid);
}

void
WifiNetDeviceAnim::OutputWirelessPacketRxInfo(Ptr<const Packet> p,
                                              WifiAnimPacketInfo& pktInfo,
                                              uint64_t animUid)
{
    std::cout << "trace2 called" << std::endl;
    m_anim->CheckMaxPktsPerTraceFile();
    uint32_t rxId = m_netDev->GetNode()->GetId();
    m_anim->WriteXmlP(animUid,
                      "wpr",
                      rxId,
                      pktInfo.m_firstBitRxTime.GetSeconds(),
                      pktInfo.m_lastBitRxTime.GetSeconds());
}

void
WifiNetDeviceAnim::WifiMacTxTrace(Ptr<const Packet> p)
{
    const Ptr<const Node> node = m_netDev->GetNode();
    // ++m_nodeWifiMacTx[node->GetId()];
    m_anim->AddNodeToNodeWifiMacTxMap(node->GetId());
}

void
WifiNetDeviceAnim::WifiMacTxDropTrace(Ptr<const Packet> p)
{
    const Ptr<const Node> node = m_netDev->GetNode();
    m_anim->AddNodeToNodeWifiMacTxDropMap(node->GetId());
}

void
WifiNetDeviceAnim::WifiMacRxTrace(Ptr<const Packet> p)
{
    const Ptr<const Node> node = m_netDev->GetNode();
    m_anim->AddNodeToNodeWifiMacRxMap(node->GetId());
}

void
WifiNetDeviceAnim::WifiMacRxDropTrace(Ptr<const Packet> p)
{
    const Ptr<const Node> node = m_netDev->GetNode();
    m_anim->AddNodeToNodeWifiMacRxDropMap(node->GetId());
}

void
WifiNetDeviceAnim::WifiPhyTxDropTrace(Ptr<const Packet> p)
{
    const Ptr<const Node> node = m_netDev->GetNode();
    m_anim->AddNodeToNodeWifiPhyTxDropMap(node->GetId());
}

void
WifiNetDeviceAnim::WifiPhyRxDropTrace(Ptr<const Packet> p, WifiPhyRxfailureReason reason)
{
    const Ptr<const Node> node = m_netDev->GetNode();
    m_anim->AddNodeToNodeWifiPhyRxDropMap(node->GetId());
}

void
WifiNetDeviceAnim::OutputWirelessPacketTxInfo(Ptr<const Packet> p,
                                              WifiAnimPacketInfo& pktInfo,
                                              uint64_t animUid)
{
    std::cout << "trace3 called" << std::endl;
    m_anim->CheckMaxPktsPerTraceFile();
    uint32_t nodeId = 0;
    nodeId = pktInfo.m_txNodeId;
    m_anim->WriteXmlPRef(animUid,
                         nodeId,
                         pktInfo.m_firstBitTxTime.GetSeconds(),
                         m_anim->IsEnablePacketMetadata() ? m_anim->GetPacketMetadata(p) : "");
}

WifiNetDeviceAnim::WifiAnimPacketInfo::WifiAnimPacketInfo()
    : m_txNodeId(0),
      m_firstBitTxTime(0),
      m_lastBitTxTime(0),
      m_lastBitRxTime(0)
{
}

WifiNetDeviceAnim::WifiAnimPacketInfo::WifiAnimPacketInfo(const WifiAnimPacketInfo& pInfo)
{
    m_txNodeId = pInfo.m_txNodeId;
    m_firstBitTxTime = pInfo.m_firstBitTxTime;
    m_lastBitTxTime = pInfo.m_firstBitTxTime;
    m_lastBitRxTime = pInfo.m_lastBitRxTime;
}

WifiNetDeviceAnim::WifiAnimPacketInfo::WifiAnimPacketInfo(uint32_t txNodeId,
                                                          const Time firstBitTxTime)
    : m_txNodeId(txNodeId),
      m_firstBitTxTime(firstBitTxTime),
      m_lastBitTxTime(0),
      m_lastBitRxTime(0)
{
}

void
WifiNetDeviceAnim::WifiAnimPacketInfo::ProcessRxBegin(Ptr<const NetDevice> nd, const Time fbRx)
{
    m_firstBitRxTime = fbRx;
    m_txNodeId = nd->GetNode()->GetId();
}

void
WifiNetDeviceAnim::PurgePendingPackets()
{
    std::vector<uint64_t> purgeList;
    for (WifiAnimUidPacketInfoMap::iterator i = m_pendingWifiPackets.begin();
         i != m_pendingWifiPackets.end();
         ++i)
    {
        WifiAnimPacketInfo pktInfo = i->second;
        Time delta = (Simulator::Now() - pktInfo.m_firstBitTxTime);
        if (delta > wifiPurgeInterval)
        {
            purgeList.push_back(i->first);
        }
    }
    for (std::vector<uint64_t>::iterator i = purgeList.begin(); i != purgeList.end(); ++i)
    {
        m_pendingWifiPackets.erase(*i);
    }
}

void
WifiNetDeviceAnim::DoDispose()
{
    m_netDev = nullptr;
    m_anim = nullptr;
    Object::DoDispose();
}

void
WifiNetDeviceAnim::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    m_netDev = GetObject<WifiNetDevice>();
    m_anim = DynamicCast<NetAnimWriter>(
        GetObject<WifiNetDevice>()->GetNode()->GetObject<Proxy<NetAnimWriter>>());
    ConnectCallbacks();
    Object::DoInitialize();
}

bool
WifiNetDeviceAnim::IsEnabled()
{
    return (m_anim->IsStarted() && m_anim->IsInTimeWindow() && m_anim->IsTracking());
}
} // namespace ns3
