#include "csma-net-device-anim.h"

#include "csma-net-device.h"

#include "ns3/animation-interface.h"
#include "ns3/config.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CsmaNetDeviceAnim");

NS_OBJECT_ENSURE_REGISTERED(CsmaNetDeviceAnim);

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
    m_anim.IncrementAnimUid();
    uint64_t animUid = m_anim.GetAnimUid();
    NS_LOG_INFO("CsmaPhyTxBeginTrace for packet:" << animUid);
    m_anim.AddByteTag(animUid, p);
    m_anim.UpdatePosition(ndev);
    AnimPacketInfo pktInfo(ndev, Simulator::Now());
    m_anim.AddPendingPacket(AnimationInterface::CSMA, animUid, pktInfo);
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
    uint64_t animUid = m_anim.GetAnimUidFromPacket(p);
    NS_LOG_INFO("CsmaPhyTxEndTrace for packet:" << animUid);
    if (!m_anim.IsPacketPending(animUid, AnimationInterface::CSMA))
    {
        NS_LOG_WARN("CsmaPhyTxEndTrace: unknown Uid");
        NS_FATAL_ERROR("CsmaPhyTxEndTrace: unknown Uid");
        AnimPacketInfo pktInfo(ndev, Simulator::Now());
        m_anim.AddPendingPacket(AnimationInterface::CSMA, animUid, pktInfo);
        NS_LOG_WARN("Unknown Uid, but adding Csma Packet anyway");
    }
    /// \todo NS_ASSERT (IsPacketPending (AnimUid) == true);
    std::map<uint64_t, AnimPacketInfo>& m_pendingCsmaPackets = m_anim.GetPendingCsmaPacketsMap();
    AnimPacketInfo& pktInfo = m_pendingCsmaPackets[animUid];
    pktInfo.m_lbTx = Simulator::Now().GetSeconds();
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
    uint64_t animUid = m_anim.GetAnimUidFromPacket(p);
    if (!m_anim.IsPacketPending(animUid, AnimationInterface::CSMA))
    {
        NS_LOG_WARN("CsmaPhyRxEndTrace: unknown Uid");
        return;
    }
    /// \todo NS_ASSERT (CsmaPacketIsPending (AnimUid) == true);
    std::map<uint64_t, AnimPacketInfo>& m_pendingCsmaPackets = m_anim.GetPendingCsmaPacketsMap();
    AnimPacketInfo& pktInfo = m_pendingCsmaPackets[animUid];
    pktInfo.ProcessRxBegin(ndev, Simulator::Now().GetSeconds());
    NS_LOG_INFO("CsmaPhyRxEndTrace for packet:" << animUid);
    NS_LOG_INFO("CsmaPhyRxEndTrace for packet:" << animUid << " complete");
    m_anim.OutputCsmaPacket(p, pktInfo);
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
    uint64_t animUid = m_anim.GetAnimUidFromPacket(p);
    if (!m_anim.IsPacketPending(animUid, AnimationInterface::CSMA))
    {
        NS_LOG_WARN("CsmaMacRxTrace: unknown Uid");
        return;
    }
    /// \todo NS_ASSERT (CsmaPacketIsPending (AnimUid) == true);
    std::map<uint64_t, AnimPacketInfo>& m_pendingCsmaPackets = m_anim.GetPendingCsmaPacketsMap();
    AnimPacketInfo& pktInfo = m_pendingCsmaPackets[animUid];
    NS_LOG_INFO("MacRxTrace for packet:" << animUid << " complete");
    m_anim.OutputCsmaPacket(p, pktInfo);
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
} // namespace ns3
