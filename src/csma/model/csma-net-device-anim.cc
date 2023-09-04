#include "csma-net-device-anim.h"

#include "csma-net-device.h"

#include "ns3/animation-interface.h"
#include "ns3/config.h"
#include "ns3/proxy.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CsmaNetDeviceAnim");

NS_OBJECT_ENSURE_REGISTERED(CsmaNetDeviceAnim);

CsmaNetDeviceAnim::CsmaAnimUidPacketInfoMap CsmaNetDeviceAnim::m_pendingCsmaPackets;
uint64_t CsmaNetDeviceAnim::csmaAnimUid = 0;
EventId CsmaNetDeviceAnim::m_purgeCsmaAnimPendingPacketsEventId = EventId();
Time CsmaNetDeviceAnim::csmaPurgeInterval = Seconds(5);
Time CsmaNetDeviceAnim::schedulePurgePendingPackets = MilliSeconds(25);

TypeId
CsmaNetDeviceAnim::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CsmaNetDeviceAnim")
                            .SetParent<Object>()
                            .AddConstructor<CsmaNetDeviceAnim>()
                            .SetGroupName("CsmaNetDeviceAnim");

    return tid;
}

void
CsmaNetDeviceAnim::ConnectCallbacks()
{
    if (!m_netDev)
    {
        m_netDev = GetObject<CsmaNetDevice>();
        NS_ASSERT_MSG(true, "Failed to retrieve net-device");
    }
    m_netDev->TraceConnectWithoutContext(
        "PhyTxBegin",
        MakeCallback(&ns3::CsmaNetDeviceAnim::CsmaPhyTxBeginTrace, this));
    m_netDev->TraceConnectWithoutContext(
        "PhyTxEnd",
        MakeCallback(&ns3::CsmaNetDeviceAnim::CsmaPhyTxEndTrace, this));
    m_netDev->TraceConnectWithoutContext(
        "PhyRxEnd",
        MakeCallback(&ns3::CsmaNetDeviceAnim::CsmaPhyRxEndTrace, this));
    m_netDev->TraceConnectWithoutContext(
        "MacRx",
        MakeCallback(&ns3::CsmaNetDeviceAnim::CsmaMacRxTrace, this));
    m_netDev->TraceConnectWithoutContext("TxQueue/Enqueue",
                                         MakeCallback(&ns3::CsmaNetDeviceAnim::EnqueueTrace, this));
    m_netDev->TraceConnectWithoutContext("TxQueue/Dequeue",
                                         MakeCallback(&ns3::CsmaNetDeviceAnim::DequeueTrace, this));
    m_netDev->TraceConnectWithoutContext(
        "TxQueue/Drop",
        MakeCallback(&ns3::CsmaNetDeviceAnim::QueueDropTrace, this));
}

void
CsmaNetDeviceAnim::CsmaPhyTxBeginTrace(Ptr<const Packet> p)
{
    NS_LOG_FUNCTION(this);
    if (!IsEnabled())
    {
        return;
    }
    m_anim->UpdatePosition(m_netDev->GetNode());
    csmaAnimUid++;
    NS_LOG_INFO("CsmaPhyTxBeginTrace for packet:" << csmaAnimUid);
    m_anim->AddByteTag(csmaAnimUid, p);
    m_anim->UpdatePosition(m_netDev->GetNode());
    CsmaAnimPacketInfo pktInfo(m_netDev->GetNode()->GetId(), Simulator::Now());
    m_pendingCsmaPackets.insert(CsmaAnimUidPacketInfoMap::value_type(csmaAnimUid, pktInfo));
    if (!m_purgeCsmaAnimPendingPacketsEventId.IsRunning())
    {
        Simulator::Schedule(schedulePurgePendingPackets, &CsmaNetDeviceAnim::PurgePendingPackets);
    }
}

void
CsmaNetDeviceAnim::CsmaPhyTxEndTrace(Ptr<const Packet> p)
{
    NS_LOG_FUNCTION(this);
    if (!IsEnabled())
    {
        return;
    }
    m_anim->UpdatePosition(m_netDev->GetNode());
    uint64_t csmaAnimUid = m_anim->GetAnimUidFromPacket(p);
    NS_LOG_INFO("CsmaPhyTxEndTrace for packet:" << csmaAnimUid);
    if (m_pendingCsmaPackets.find(csmaAnimUid) == m_pendingCsmaPackets.end())
    {
        NS_LOG_WARN("CsmaPhyTxEndTrace: unknown Uid");
        NS_FATAL_ERROR("CsmaPhyTxEndTrace: unknown Uid");
        CsmaAnimPacketInfo pktInfo(m_netDev->GetNode()->GetId(), Simulator::Now());
        m_pendingCsmaPackets.insert(CsmaAnimUidPacketInfoMap::value_type(csmaAnimUid, pktInfo));
        NS_LOG_WARN("Unknown Uid, but adding Csma Packet anyway");
    }
    CsmaAnimPacketInfo& pktInfo = m_pendingCsmaPackets[csmaAnimUid];
    pktInfo.m_lastBitTxTime = Simulator::Now();
}

void
CsmaNetDeviceAnim::CsmaPhyRxEndTrace(Ptr<const Packet> p)
{
    NS_LOG_FUNCTION(this);
    if (!IsEnabled())
    {
        return;
    }
    m_anim->UpdatePosition(m_netDev->GetNode());
    uint64_t csmaAnimUid = m_anim->GetAnimUidFromPacket(p);
    if (m_pendingCsmaPackets.find(csmaAnimUid) == m_pendingCsmaPackets.end())
    {
        NS_LOG_WARN("CsmaPhyRxEndTrace: unknown Uid");
        return;
    }
    /// \todo NS_ASSERT (CsmaPacketIsPending (AnimUid) == true);
    CsmaAnimPacketInfo& pktInfo = m_pendingCsmaPackets[csmaAnimUid];
    m_firstBitRxTime = Simulator::Now().GetSeconds();
    NS_LOG_INFO("CsmaPhyRxEndTrace for packet:" << csmaAnimUid);
    NS_LOG_INFO("CsmaPhyRxEndTrace for packet:" << csmaAnimUid << " complete");
    OutputCsmaPacket(p, pktInfo);
}

void
CsmaNetDeviceAnim::CsmaMacRxTrace(Ptr<const Packet> p)
{
    NS_LOG_FUNCTION(this);
    if (!IsEnabled())
    {
        return;
    }
    uint64_t csmaAnimUid = m_anim->GetAnimUidFromPacket(p);
    if (m_pendingCsmaPackets.find(csmaAnimUid) == m_pendingCsmaPackets.end())
    {
        NS_LOG_WARN("CsmaMacRxTrace: unknown Uid");
        return;
    }
    /// \todo NS_ASSERT (CsmaPacketIsPending (AnimUid) == true);
    CsmaAnimPacketInfo& pktInfo = m_pendingCsmaPackets[csmaAnimUid];
    NS_LOG_INFO("MacRxTrace for packet:" << csmaAnimUid << " complete");
    OutputCsmaPacket(p, pktInfo);
}

void
CsmaNetDeviceAnim::EnqueueTrace(Ptr<const Packet> p)
{
    const Ptr<const Node> node = m_netDev->GetNode();
    m_anim->AddNodeToNodeEnqueueMap(node->GetId());
}

void
CsmaNetDeviceAnim::DequeueTrace(Ptr<const Packet> p)
{
    const Ptr<const Node> node = m_netDev->GetNode();
    m_anim->AddNodeToNodeDequeueMap(node->GetId());
}

void
CsmaNetDeviceAnim::QueueDropTrace(Ptr<const Packet> p)
{
    const Ptr<const Node> node = m_netDev->GetNode();
    m_anim->AddNodeToNodeDropMap(node->GetId());
}

void
CsmaNetDeviceAnim::OutputCsmaPacket(Ptr<const Packet> p, CsmaAnimPacketInfo& pktInfo)
{
    m_anim->CheckMaxPktsPerTraceFile();
    uint32_t nodeId = pktInfo.m_txNodeId;
    uint32_t rxId = m_netDev->GetNode()->GetId();

    m_anim->WriteXmlP("p",
                      nodeId,
                      pktInfo.m_firstBitTxTime.GetSeconds(),
                      pktInfo.m_lastBitTxTime.GetSeconds(),
                      rxId,
                      m_firstBitRxTime,
                      m_lastBitRxTime,
                      m_anim->IsEnablePacketMetadata() ? m_anim->GetPacketMetadata(p) : "");
}

CsmaNetDeviceAnim::CsmaAnimPacketInfo::CsmaAnimPacketInfo()
    : m_txNodeId(0),
      m_firstBitTxTime(0),
      m_lastBitTxTime(0)
{
}

CsmaNetDeviceAnim::CsmaAnimPacketInfo::CsmaAnimPacketInfo(const CsmaAnimPacketInfo& pInfo)
{
    m_txNodeId = pInfo.m_txNodeId;
    m_firstBitTxTime = pInfo.m_firstBitTxTime;
    m_lastBitTxTime = pInfo.m_firstBitTxTime;
}

CsmaNetDeviceAnim::CsmaAnimPacketInfo::CsmaAnimPacketInfo(uint32_t txNodeId,
                                                          const Time firstBitTxTime)
    : m_txNodeId(txNodeId),
      m_firstBitTxTime(firstBitTxTime),
      m_lastBitTxTime(0)
{
}

void
CsmaNetDeviceAnim::PurgePendingPackets()
{
    std::vector<uint64_t> purgeList;
    for (CsmaAnimUidPacketInfoMap::iterator i = m_pendingCsmaPackets.begin();
         i != m_pendingCsmaPackets.end();
         ++i)
    {
        CsmaAnimPacketInfo pktInfo = i->second;
        Time delta = (Simulator::Now() - pktInfo.m_firstBitTxTime);
        if (delta > csmaPurgeInterval)
        {
            purgeList.push_back(i->first);
        }
    }
    for (std::vector<uint64_t>::iterator i = purgeList.begin(); i != purgeList.end(); ++i)
    {
        m_pendingCsmaPackets.erase(*i);
    }
}

void
CsmaNetDeviceAnim::DoDispose()
{
    m_netDev = nullptr;
    m_anim = nullptr;
    Object::DoDispose();
}

void
CsmaNetDeviceAnim::DoInitialize()
{
    m_netDev = GetObject<CsmaNetDevice>();
    m_anim = DynamicCast<NetAnimWriter>(
        GetObject<CsmaNetDevice>()->GetNode()->GetObject<Proxy<NetAnimWriter>>());
    ConnectCallbacks();
    Object::DoInitialize();
}

bool
CsmaNetDeviceAnim::IsEnabled()
{
    return (m_anim->IsStarted() && m_anim->IsInTimeWindow() && m_anim->IsTracking());
}

} // namespace ns3
