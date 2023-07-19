#include "csma-net-device-anim.h"

#include "csma-net-device.h"

#include "ns3/animation-interface.h"
#include "ns3/config.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CsmaNetDeviceAnim");

NS_OBJECT_ENSURE_REGISTERED(CsmaNetDeviceAnim);

CsmaNetDeviceAnim::CsmaAnimUidPacketInfoMap CsmaNetDeviceAnim::m_pendingCsmaPackets;
uint64_t CsmaNetDeviceAnim::csmaAnimUid = 0;

TypeId
CsmaNetDeviceAnim::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CsmaNetDeviceAnim")
                            .SetParent<NetDeviceAnim>()
                            .AddConstructor<CsmaNetDeviceAnim>()
                            .SetGroupName("CsmaNetDeviceAnim");

    return tid;
}

TypeId
CsmaNetDeviceAnim::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
CsmaNetDeviceAnim::ConnectCallbacks()
{
    GetObject<CsmaNetDevice>()->TraceConnectWithoutContext(
        "PhyTxBegin",
        MakeCallback(&ns3::CsmaNetDeviceAnim::CsmaPhyTxBeginTrace, this));
    GetObject<CsmaNetDevice>()->TraceConnectWithoutContext(
        "PhyTxEnd",
        MakeCallback(&ns3::CsmaNetDeviceAnim::CsmaPhyTxEndTrace, this));
    GetObject<CsmaNetDevice>()->TraceConnectWithoutContext(
        "PhyRxEnd",
        MakeCallback(&ns3::CsmaNetDeviceAnim::CsmaPhyRxEndTrace, this));
    GetObject<CsmaNetDevice>()->TraceConnectWithoutContext(
        "MacRx",
        MakeCallback(&ns3::CsmaNetDeviceAnim::CsmaMacRxTrace, this));
    GetObject<CsmaNetDevice>()->TraceConnectWithoutContext(
        "TxQueue/Enqueue",
        MakeCallback(&ns3::CsmaNetDeviceAnim::EnqueueTrace, this));
    GetObject<CsmaNetDevice>()->TraceConnectWithoutContext(
        "TxQueue/Dequeue",
        MakeCallback(&ns3::CsmaNetDeviceAnim::DequeueTrace, this));
    GetObject<CsmaNetDevice>()->TraceConnectWithoutContext(
        "TxQueue/Drop",
        MakeCallback(&ns3::CsmaNetDeviceAnim::QueueDropTrace, this));
}

void
CsmaNetDeviceAnim::CsmaPhyTxBeginTrace(Ptr<const Packet> p)
{
    NS_LOG_FUNCTION(this);
    if (!m_anim.IsStarted() || !m_anim.IsInTimeWindow() || !m_anim.IsTracking())
    {
        return;
    }
    Ptr<NetDevice> ndev = this->GetObject<CsmaNetDevice>();
    NS_ASSERT(ndev);
    m_anim.UpdatePosition(ndev);
    csmaAnimUid++;
    NS_LOG_INFO("CsmaPhyTxBeginTrace for packet:" << csmaAnimUid);
    m_anim.AddByteTag(csmaAnimUid, p);
    m_anim.UpdatePosition(ndev);
    CsmaAnimPacketInfo pktInfo(ndev, Simulator::Now());
    m_pendingCsmaPackets.insert(CsmaAnimUidPacketInfoMap::value_type(csmaAnimUid, pktInfo));
}

void
CsmaNetDeviceAnim::CsmaPhyTxEndTrace(Ptr<const Packet> p)
{
    NS_LOG_FUNCTION(this);
    if (!m_anim.IsStarted() || !m_anim.IsInTimeWindow() || !m_anim.IsTracking())
    {
        return;
    }
    Ptr<NetDevice> ndev = this->GetObject<CsmaNetDevice>();
    NS_ASSERT(ndev);
    m_anim.UpdatePosition(ndev);
    uint64_t csmaAnimUid = m_anim.GetAnimUidFromPacket(p);
    NS_LOG_INFO("CsmaPhyTxEndTrace for packet:" << csmaAnimUid);
    if (m_pendingCsmaPackets.find(csmaAnimUid) == m_pendingCsmaPackets.end())
    {
        NS_LOG_WARN("CsmaPhyTxEndTrace: unknown Uid");
        NS_FATAL_ERROR("CsmaPhyTxEndTrace: unknown Uid");
        CsmaAnimPacketInfo pktInfo(ndev, Simulator::Now());
        m_pendingCsmaPackets.insert(CsmaAnimUidPacketInfoMap::value_type(csmaAnimUid, pktInfo));
        NS_LOG_WARN("Unknown Uid, but adding Csma Packet anyway");
    }
    /// \todo NS_ASSERT (IsPacketPending (AnimUid) == true);
    CsmaAnimPacketInfo& pktInfo = m_pendingCsmaPackets[csmaAnimUid];
    pktInfo.m_lastBitTxTime = Simulator::Now().GetSeconds();
}

void
CsmaNetDeviceAnim::CsmaPhyRxEndTrace(Ptr<const Packet> p)
{
    NS_LOG_FUNCTION(this);
    if (!m_anim.IsStarted() || !m_anim.IsInTimeWindow() || !m_anim.IsTracking())
    {
        return;
    }
    Ptr<NetDevice> ndev = this->GetObject<CsmaNetDevice>();
    NS_ASSERT(ndev);
    m_anim.UpdatePosition(ndev);
    uint64_t csmaAnimUid = m_anim.GetAnimUidFromPacket(p);
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
    if (!m_anim.IsStarted() || !m_anim.IsInTimeWindow() || !m_anim.IsTracking())
    {
        return;
    }
    Ptr<NetDevice> ndev = this->GetObject<CsmaNetDevice>();
    NS_ASSERT(ndev);
    uint64_t csmaAnimUid = m_anim.GetAnimUidFromPacket(p);
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
    const Ptr<const Node> node = this->GetObject<CsmaNetDevice>()->GetNode();
    m_anim.AddNodeToNodeEnqueueMap(node->GetId());
}

void
CsmaNetDeviceAnim::DequeueTrace(Ptr<const Packet> p)
{
    const Ptr<const Node> node = this->GetObject<CsmaNetDevice>()->GetNode();
    m_anim.AddNodeToNodeDequeueMap(node->GetId());
}

void
CsmaNetDeviceAnim::QueueDropTrace(Ptr<const Packet> p)
{
    const Ptr<const Node> node = this->GetObject<CsmaNetDevice>()->GetNode();
    m_anim.AddNodeToNodeDropMap(node->GetId());
}

void
CsmaNetDeviceAnim::OutputCsmaPacket(Ptr<const Packet> p, CsmaAnimPacketInfo& pktInfo)
{
    m_anim.CheckMaxPktsPerTraceFile();
    NS_ASSERT(pktInfo.m_txnd);
    uint32_t nodeId = pktInfo.m_txnd->GetNode()->GetId();
    uint32_t rxId = this->GetObject<CsmaNetDevice>()->GetNode()->GetId();

    m_anim.WriteXmlP("p",
                     nodeId,
                     pktInfo.m_firstBitTxTime,
                     pktInfo.m_lastBitTxTime,
                     rxId,
                     m_firstBitRxTime,
                     m_lastBitRxTime,
                     m_anim.IsEnablePacketMetadata() ? m_anim.GetPacketMetadata(p) : "");
}

CsmaNetDeviceAnim::CsmaAnimPacketInfo::CsmaAnimPacketInfo()
    : m_txnd(nullptr),
      m_txNodeId(0),
      m_firstBitTxTime(0),
      m_lastBitTxTime(0)
//   m_lbRx(0)
{
}

CsmaNetDeviceAnim::CsmaAnimPacketInfo::CsmaAnimPacketInfo(const CsmaAnimPacketInfo& pInfo)
{
    m_txnd = pInfo.m_txnd;
    m_txNodeId = pInfo.m_txNodeId;
    m_firstBitTxTime = pInfo.m_firstBitTxTime;
    m_lastBitTxTime = pInfo.m_firstBitTxTime;
    // m_lastBitRxTime = m_lastBitTRxTime;
}

CsmaNetDeviceAnim::CsmaAnimPacketInfo::CsmaAnimPacketInfo(Ptr<const NetDevice> txnd,
                                                          const Time fbTx,
                                                          uint32_t txNodeId)
    : m_txnd(txnd),
      m_txNodeId(0),
      m_firstBitTxTime(fbTx.GetSeconds()),
      m_lastBitTxTime(0)
//   m_lbRx(0)
{
    if (!m_txnd)
    {
        m_txNodeId = txNodeId;
    }
}
} // namespace ns3
