#include "lte-net-device-anim.h"

#include "lte-enb-net-device.h"
#include "lte-enb-phy.h"
#include "lte-net-device.h"
#include "lte-ue-net-device.h"
#include "lte-ue-phy.h"

#include "ns3/animation-interface.h"
#include "ns3/config.h"
#include "ns3/proxy.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("LteNetDeviceAnim");

NS_OBJECT_ENSURE_REGISTERED(LteNetDeviceAnim);

LteNetDeviceAnim::LteAnimUidPacketInfoMap LteNetDeviceAnim::m_pendingLtePackets;
uint64_t LteNetDeviceAnim::lteAnimUid = 0;
EventId LteNetDeviceAnim::m_purgeLteAnimPendingPacketsEventId = EventId();
Time LteNetDeviceAnim::ltePurgeInterval = Seconds(5);
Time LteNetDeviceAnim::schedulePurgePendingPackets = MilliSeconds(25);

TypeId
LteNetDeviceAnim::GetTypeId()
{
    static TypeId tid = TypeId("ns3::LteNetDeviceAnim")
                            .SetParent<Object>()
                            .AddConstructor<LteNetDeviceAnim>()
                            .SetGroupName("LteNetDeviceAnim");

    return tid;
}

void
LteNetDeviceAnim::ConnectCallbacks()
{
    if (!m_netDev)
    {
        m_netDev = GetObject<LteNetDevice>();
        NS_ASSERT_MSG(true, "Failed to retrieve net-device");
    }

    for (auto i = NodeList::Begin(); i != NodeList::End(); ++i)
    {
        Ptr<Node> n = *i;
        NS_ASSERT(n);
        uint32_t nDevices = n->GetNDevices();
        for (uint32_t devIndex = 0; devIndex < nDevices; ++devIndex)
        {
            Ptr<NetDevice> nd = n->GetDevice(devIndex);
            if (!nd)
            {
                continue;
            }
            Ptr<LteUeNetDevice> lteUeNetDevice = DynamicCast<LteUeNetDevice>(nd);
            if (lteUeNetDevice)
            {
                ConnectLteUe(n, lteUeNetDevice, devIndex);
                continue;
            }
            Ptr<LteEnbNetDevice> lteEnbNetDevice = DynamicCast<LteEnbNetDevice>(nd);
            if (lteEnbNetDevice)
            {
                ConnectLteEnb(n, lteEnbNetDevice, devIndex);
            }
        }
    }
}

void
LteNetDeviceAnim::ConnectLteEnb(Ptr<Node> n, Ptr<LteEnbNetDevice> nd, uint32_t devIndex)
{
    Ptr<LteEnbPhy> lteEnbPhy = nd->GetPhy();
    Ptr<LteSpectrumPhy> dlPhy = lteEnbPhy->GetDownlinkSpectrumPhy();
    Ptr<LteSpectrumPhy> ulPhy = lteEnbPhy->GetUplinkSpectrumPhy();
    std::ostringstream oss;
    // NodeList/*/DeviceList/*/
    oss << "NodeList/" << n->GetId() << "/DeviceList/" << devIndex << "/";
    if (dlPhy)
    {
        dlPhy->TraceConnectWithoutContext(
            "TxStart",
            MakeCallback(&ns3::LteNetDeviceAnim::LteSpectrumPhyTxStart, this));
        dlPhy->TraceConnectWithoutContext(
            "RxStart",
            MakeCallback(&ns3::LteNetDeviceAnim::LteSpectrumPhyRxStart, this));
    }
    if (ulPhy)
    {
        ulPhy->TraceConnectWithoutContext(
            "TxStart",
            MakeCallback(&ns3::LteNetDeviceAnim::LteSpectrumPhyTxStart, this));
        ulPhy->TraceConnectWithoutContext(
            "RxStart",
            MakeCallback(&ns3::LteNetDeviceAnim::LteSpectrumPhyRxStart, this));
    }
}

void
LteNetDeviceAnim::ConnectLteUe(Ptr<Node> n, Ptr<LteUeNetDevice> nd, uint32_t devIndex)
{
    Ptr<LteUePhy> lteUePhy = nd->GetPhy();
    Ptr<LteSpectrumPhy> dlPhy = lteUePhy->GetDownlinkSpectrumPhy();
    Ptr<LteSpectrumPhy> ulPhy = lteUePhy->GetUplinkSpectrumPhy();
    std::ostringstream oss;
    // NodeList/*/DeviceList/*/
    oss << "NodeList/" << n->GetId() << "/DeviceList/" << devIndex << "/";
    if (dlPhy)
    {
        dlPhy->TraceConnectWithoutContext(
            "TxStart",
            MakeCallback(&ns3::LteNetDeviceAnim::LteSpectrumPhyTxStart, this));
        dlPhy->TraceConnectWithoutContext(
            "RxStart",
            MakeCallback(&ns3::LteNetDeviceAnim::LteSpectrumPhyRxStart, this));
    }
    if (ulPhy)
    {
        ulPhy->TraceConnectWithoutContext(
            "TxStart",
            MakeCallback(&ns3::LteNetDeviceAnim::LteSpectrumPhyTxStart, this));
        ulPhy->TraceConnectWithoutContext(
            "RxStart",
            MakeCallback(&ns3::LteNetDeviceAnim::LteSpectrumPhyRxStart, this));
    }
}

void
LteNetDeviceAnim::LteSpectrumPhyTxStart(Ptr<const PacketBurst> pb)
{
    NS_LOG_FUNCTION(this);
    if (!IsEnabled())
    {
        return;
    }
    if (!pb)
    {
        NS_LOG_WARN("pb == 0. Not yet supported");
        return;
    }
    NS_ASSERT(m_netDev);
    m_anim->UpdatePosition(m_netDev->GetNode());

    std::list<Ptr<Packet>> pbList = pb->GetPackets();
    for (std::list<Ptr<Packet>>::iterator i = pbList.begin(); i != pbList.end(); ++i)
    {
        Ptr<Packet> p = *i;
        ++lteAnimUid;
        NS_LOG_INFO("LteSpectrumPhyTxTrace for packet:" << lteAnimUid);
        LteAnimPacketInfo pktInfo(m_netDev, Simulator::Now());
        m_anim->AddByteTag(lteAnimUid, p);
        m_pendingLtePackets.insert(LteAnimUidPacketInfoMap::value_type(lteAnimUid, pktInfo));
        OutputWirelessPacketTxInfo(p, pktInfo, lteAnimUid);
    }
}

void
LteNetDeviceAnim::LteSpectrumPhyRxStart(Ptr<const PacketBurst> pb)
{
    NS_LOG_FUNCTION(this);
    if (!IsEnabled())
    {
        return;
    }
    if (!pb)
    {
        NS_LOG_WARN("pb == 0. Not yet supported");
        return;
    }
    NS_ASSERT(m_netDev);
    m_anim->UpdatePosition(m_netDev->GetNode());
    std::list<Ptr<Packet>> pbList = pb->GetPackets();
    for (std::list<Ptr<Packet>>::iterator i = pbList.begin(); i != pbList.end(); ++i)
    {
        Ptr<Packet> p = *i;
        uint64_t animUid = m_anim->GetAnimUidFromPacket(p);
        // check the below line it was ganimUid but should be animUid
        NS_LOG_INFO("LteSpectrumPhyRxTrace for packet:" << animUid);
        if (m_pendingLtePackets.find(animUid) == m_pendingLtePackets.end())
        {
            NS_LOG_WARN("LteSpectrumPhyRxTrace: unknown Uid");
            return;
        }
        LteAnimPacketInfo& pktInfo = m_pendingLtePackets[animUid];
        pktInfo.ProcessRxBegin(m_netDev, Simulator::Now().GetSeconds());
        OutputWirelessPacketRxInfo(p, pktInfo, animUid);
    }
}

void
LteNetDeviceAnim::OutputWirelessPacketTxInfo(Ptr<const Packet> p,
                                             LteAnimPacketInfo& pktInfo,
                                             uint64_t animUid)
{
    m_anim->CheckMaxPktsPerTraceFile();
    uint32_t nodeId = 0;
    if (pktInfo.m_txnd)
    {
        nodeId = pktInfo.m_txnd->GetNode()->GetId();
    }
    else
    {
        nodeId = pktInfo.m_txNodeId;
    }
    m_anim->WriteXmlPRef(animUid,
                         nodeId,
                         pktInfo.m_fbTx,
                         m_anim->IsEnablePacketMetadata() ? m_anim->GetPacketMetadata(p) : "");
}

void
LteNetDeviceAnim::OutputWirelessPacketRxInfo(Ptr<const Packet> p,
                                             LteAnimPacketInfo& pktInfo,
                                             uint64_t animUid)
{
    m_anim->CheckMaxPktsPerTraceFile();
    uint32_t rxId = pktInfo.m_rxnd->GetNode()->GetId();
    m_anim->WriteXmlP(animUid, "wpr", rxId, pktInfo.m_fbRx, pktInfo.m_lbRx);
}

LteNetDeviceAnim::LteAnimPacketInfo::LteAnimPacketInfo()
    : m_txnd(nullptr),
      m_txNodeId(0),
      m_fbTx(0),
      m_lbTx(0),
      m_lbRx(0)
{
}

LteNetDeviceAnim::LteAnimPacketInfo::LteAnimPacketInfo(const LteAnimPacketInfo& pInfo)
{
    m_txnd = pInfo.m_txnd;
    m_txNodeId = pInfo.m_txNodeId;
    m_fbTx = pInfo.m_fbTx;
    m_lbTx = pInfo.m_lbTx;
    m_lbRx = pInfo.m_lbRx;
}

LteNetDeviceAnim::LteAnimPacketInfo::LteAnimPacketInfo(Ptr<const NetDevice> txnd,
                                                       const Time fbTx,
                                                       uint32_t txNodeId)
    : m_txnd(txnd),
      m_txNodeId(0),
      m_fbTx(fbTx.GetSeconds()),
      m_lbTx(0),
      m_lbRx(0)
{
    if (!m_txnd)
    {
        m_txNodeId = txNodeId;
    }
}

void
LteNetDeviceAnim::LteAnimPacketInfo::ProcessRxBegin(Ptr<const NetDevice> nd, const double fbRx)
{
    Ptr<Node> n = nd->GetNode();
    m_fbRx = fbRx;
    m_rxnd = nd;
}

void
LteNetDeviceAnim::PurgePendingPackets()
{
    std::vector<uint64_t> purgeList;
    for (LteAnimUidPacketInfoMap::iterator i = m_pendingLtePackets.begin();
         i != m_pendingLtePackets.end();
         ++i)
    {
        LteAnimPacketInfo pktInfo = i->second;
        Time delta = (Simulator::Now() - Seconds(pktInfo.m_fbTx));
        if (delta > ltePurgeInterval)
        {
            purgeList.push_back(i->first);
        }
    }
    for (std::vector<uint64_t>::iterator i = purgeList.begin(); i != purgeList.end(); ++i)
    {
        m_pendingLtePackets.erase(*i);
    }
}

void
LteNetDeviceAnim::DoDispose()
{
    m_netDev = nullptr;
    m_anim = nullptr;
    Object::DoDispose();
}

void
LteNetDeviceAnim::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    m_netDev = GetObject<LteNetDevice>();
    m_anim = DynamicCast<NetAnimWriter>(
        GetObject<LteNetDevice>()->GetNode()->GetObject<Proxy<NetAnimWriter>>());
    ConnectCallbacks();
    Object::DoInitialize();
}

bool
LteNetDeviceAnim::IsEnabled()
{
    return (m_anim->IsStarted() && m_anim->IsInTimeWindow() && m_anim->IsTracking());
}

} // namespace ns3
