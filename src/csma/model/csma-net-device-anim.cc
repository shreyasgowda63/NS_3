#include "csma-net-device-anim.h"

#include "csma-net-device.h"

#include "ns3/animation-interface.h"
#include "ns3/config.h"

namespace ns3
{

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
CsmaNetDeviceAnim::CsmaPhyTxBeginTrace(std::string context, Ptr<const Packet> p)
{
    // NS_LOG_FUNCTION(this);
    if (!m_anim.IsStarted() || !m_anim.IsInTimeWindow() || !m_anim.IsTracking())
    {
        return;
    }
    Ptr<NetDevice> ndev = m_anim.GetNetDeviceFromContext(context);
    NS_ASSERT(ndev);
    m_anim.UpdatePosition(ndev);
    m_anim.IncrementAnimUid();
    // NS_LOG_INFO("CsmaPhyTxBeginTrace for packet:" << m_anim.GetAnimUid());
    m_anim.AddByteTag(m_anim.GetAnimUid(), p);
    m_anim.UpdatePosition(ndev);
    AnimPacketInfo pktInfo(ndev, Simulator::Now());
    m_anim.AddPendingPacket(AnimationInterface::CSMA, m_anim.GetAnimUid(), pktInfo);
}

void
CsmaNetDeviceAnim::CsmaPhyTxEndTrace(std::string context, Ptr<const Packet> p)
{
    // NS_LOG_FUNCTION(this);
    if (!m_anim.IsStarted() || !m_anim.IsInTimeWindow() || !m_anim.IsTracking())
    {
        return;
    }
    Ptr<NetDevice> ndev = m_anim.GetNetDeviceFromContext(context);
    NS_ASSERT(ndev);
    m_anim.UpdatePosition(ndev);
    uint64_t animUid = m_anim.GetAnimUidFromPacket(p);
    // NS_LOG_INFO("CsmaPhyTxEndTrace for packet:" << animUid);
    if (!m_anim.IsPacketPending(animUid, AnimationInterface::CSMA))
    {
        // NS_LOG_WARN("CsmaPhyTxEndTrace: unknown Uid");
        // NS_FATAL_ERROR("CsmaPhyTxEndTrace: unknown Uid");
        AnimPacketInfo pktInfo(ndev, Simulator::Now());
        m_anim.AddPendingPacket(AnimationInterface::CSMA, animUid, pktInfo);
        // NS_LOG_WARN("Unknown Uid, but adding Csma Packet anyway");
    }
    /// \todo NS_ASSERT (IsPacketPending (AnimUid) == true);
    AnimPacketInfo& pktInfo = m_anim.GetPendingCsmaPacketsMap()[animUid];
    pktInfo.m_lbTx = Simulator::Now().GetSeconds();
}

void
CsmaNetDeviceAnim::CsmaPhyRxEndTrace(std::string context, Ptr<const Packet> p)
{
    // NS_LOG_FUNCTION(this);
    if (!m_anim.IsStarted() || !m_anim.IsInTimeWindow() || !m_anim.IsTracking())
    {
        return;
    }
    Ptr<NetDevice> ndev = m_anim.GetNetDeviceFromContext(context);
    NS_ASSERT(ndev);
    m_anim.UpdatePosition(ndev);
    uint64_t animUid = m_anim.GetAnimUidFromPacket(p);
    if (!m_anim.IsPacketPending(animUid, AnimationInterface::CSMA))
    {
        // NS_LOG_WARN("CsmaPhyRxEndTrace: unknown Uid");
        return;
    }
    /// \todo NS_ASSERT (CsmaPacketIsPending (AnimUid) == true);
    AnimPacketInfo& pktInfo = m_anim.GetPendingCsmaPacketsMap()[animUid];
    pktInfo.ProcessRxBegin(ndev, Simulator::Now().GetSeconds());
    // NS_LOG_INFO("CsmaPhyRxEndTrace for packet:" << animUid);
    // NS_LOG_INFO("CsmaPhyRxEndTrace for packet:" << animUid << " complete");
    m_anim.OutputCsmaPacket(p, pktInfo);
}

void
CsmaNetDeviceAnim::CsmaMacRxTrace(std::string context, Ptr<const Packet> p)
{
    // NS_LOG_FUNCTION(this);
    if (!m_anim.IsStarted() || !m_anim.IsInTimeWindow() || !m_anim.IsTracking())
    {
        return;
    }
    Ptr<NetDevice> ndev = m_anim.GetNetDeviceFromContext(context);
    NS_ASSERT(ndev);
    uint64_t animUid = m_anim.GetAnimUidFromPacket(p);
    if (!m_anim.IsPacketPending(animUid, AnimationInterface::CSMA))
    {
        // NS_LOG_WARN("CsmaMacRxTrace: unknown Uid");
        return;
    }
    /// \todo NS_ASSERT (CsmaPacketIsPending (AnimUid) == true);
    AnimPacketInfo& pktInfo = m_anim.GetPendingCsmaPacketsMap()[animUid];
    // NS_LOG_INFO("MacRxTrace for packet:" << animUid << " complete");
    m_anim.OutputCsmaPacket(p, pktInfo);
}

void
CsmaNetDeviceAnim::EnqueueTrace(std::string context, Ptr<const Packet> p)
{
    const Ptr<const Node> node = m_anim.GetNodeFromContext(context);
    m_anim.AddNodeToNodeEnqueueMap(node->GetId());
}

void
CsmaNetDeviceAnim::DequeueTrace(std::string context, Ptr<const Packet> p)
{
    const Ptr<const Node> node = m_anim.GetNodeFromContext(context);
    m_anim.AddNodeToNodeDequeueMap(node->GetId());
}

void
CsmaNetDeviceAnim::QueueDropTrace(std::string context, Ptr<const Packet> p)
{
    const Ptr<const Node> node = m_anim.GetNodeFromContext(context);
    m_anim.AddNodeToNodeDropMap(node->GetId());
}
} // namespace ns3
