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
    // trace sources don't exisit for the below 2
    // m_netDev->TraceConnectWithoutContext("Tx",
    //                                      MakeCallback(&ns3::LteNetDeviceAnim::LteTxTrace, this));
    // m_netDev->TraceConnectWithoutContext("Rx",
    //                                      MakeCallback(&ns3::LteNetDeviceAnim::LteRxTrace, this));

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

// void
// LteNetDeviceAnim::LteTxTrace(std::string context, Ptr<const Packet> p, const Mac48Address& m)
// {
//     NS_LOG_FUNCTION(this);
//     return GenericWirelessTxTrace(context, p, AnimationInterface::LTE);
// }

// void
// LteNetDeviceAnim::LteRxTrace(std::string context, Ptr<const Packet> p, const Mac48Address& m)
// {
//     NS_LOG_FUNCTION(this);
//     return GenericWirelessRxTrace(context, p, AnimationInterface::LTE);
// }

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
        dlPhy->TraceConnect("TxStart",
                            oss.str(),
                            MakeCallback(&LteNetDeviceAnim::LteSpectrumPhyTxStart, this));
        dlPhy->TraceConnect("RxStart",
                            oss.str(),
                            MakeCallback(&LteNetDeviceAnim::LteSpectrumPhyRxStart, this));
    }
    if (ulPhy)
    {
        ulPhy->TraceConnect("TxStart",
                            oss.str(),
                            MakeCallback(&LteNetDeviceAnim::LteSpectrumPhyTxStart, this));
        ulPhy->TraceConnect("RxStart",
                            oss.str(),
                            MakeCallback(&LteNetDeviceAnim::LteSpectrumPhyRxStart, this));
    }
}

void
LteNetDeviceAnim::LteSpectrumPhyTxStart(Ptr<const PacketBurst> pb)
{
    std::cout << "a" << std::endl;
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
        LteAnimPacketInfo pktInfo(m_netDev->GetNode()->GetId(), Simulator::Now());
        m_anim->AddByteTag(lteAnimUid, p);
        m_pendingLtePackets.insert(LteAnimUidPacketInfoMap::value_type(lteAnimUid, pktInfo));
        OutputWirelessPacketTxInfo(p, pktInfo, lteAnimUid);
    }
}

void
LteNetDeviceAnim::LteSpectrumPhyRxStart(Ptr<const PacketBurst> pb)
{
    std::cout << "b" << std::endl;
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
        NS_LOG_INFO("LteSpectrumPhyRxTrace for packet:" << lteAnimUid);
        if (m_pendingLtePackets.find(lteAnimUid) == m_pendingLtePackets.end())
        {
            NS_LOG_WARN("LteSpectrumPhyRxTrace: unknown Uid");
            return;
        }
        // AnimPacketInfo& pktInfo = m_pendingLtePackets[animUid];
        LteAnimPacketInfo& pktInfo = m_pendingLtePackets[lteAnimUid];
        pktInfo.m_lastBitTxTime = Simulator::Now();
        // pktInfo.ProcessRxBegin(ndev, Simulator::Now().GetSeconds());
        OutputWirelessPacketRxInfo(p, pktInfo, animUid);
    }
}

void
LteNetDeviceAnim::OutputWirelessPacketTxInfo(Ptr<const Packet> p,
                                             LteAnimPacketInfo& pktInfo,
                                             uint64_t animUid)
{
    std::cout << "c" << std::endl;
    m_anim->CheckMaxPktsPerTraceFile();
    uint32_t nodeId = pktInfo.m_txNodeId;
    m_anim->WriteXmlPRef(animUid,
                         nodeId,
                         pktInfo.m_firstBitTxTime.GetSeconds(),
                         m_anim->IsEnablePacketMetadata() ? m_anim->GetPacketMetadata(p) : "");
}

void
LteNetDeviceAnim::OutputWirelessPacketRxInfo(Ptr<const Packet> p,
                                             LteAnimPacketInfo& pktInfo,
                                             uint64_t animUid)
{
    std::cout << "d" << std::endl;
    m_anim->CheckMaxPktsPerTraceFile();
    // uint32_t rxId = pktInfo.m_rxnd->GetNode()->GetId();
    uint32_t rxId = m_netDev->GetNode()->GetId();
    // std::cout << "r" << std::endl;
    m_anim->WriteXmlP(animUid,
                      "wpr",
                      rxId,
                      pktInfo.m_firstBitTxTime.GetSeconds(),
                      pktInfo.m_lastBitTxTime.GetSeconds());
}

LteNetDeviceAnim::LteAnimPacketInfo::LteAnimPacketInfo()
    : m_txNodeId(0),
      m_firstBitTxTime(0),
      m_lastBitTxTime(0)
{
}

LteNetDeviceAnim::LteAnimPacketInfo::LteAnimPacketInfo(const LteAnimPacketInfo& pInfo)
{
    m_txNodeId = pInfo.m_txNodeId;
    m_firstBitTxTime = pInfo.m_firstBitTxTime;
    m_lastBitTxTime = pInfo.m_firstBitTxTime;
}

LteNetDeviceAnim::LteAnimPacketInfo::LteAnimPacketInfo(uint32_t txNodeId, const Time firstBitTxTime)
    : m_txNodeId(txNodeId),
      m_firstBitTxTime(firstBitTxTime),
      m_lastBitTxTime(0)
{
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
        Time delta = (Simulator::Now() - pktInfo.m_firstBitTxTime);
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
