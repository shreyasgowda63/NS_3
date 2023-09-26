#include "point-to-point-net-device-anim.h"

#include "point-to-point-channel.h"
#include "point-to-point-net-device.h"

#include "ns3/animation-interface.h"
#include "ns3/config.h"
#include "ns3/proxy.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("PointToPointNetDeviceAnim");

NS_OBJECT_ENSURE_REGISTERED(PointToPointNetDeviceAnim);

TypeId
PointToPointNetDeviceAnim::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PointToPointNetDeviceAnim")
                            .SetParent<Object>()
                            .AddConstructor<PointToPointNetDeviceAnim>()
                            .SetGroupName("PointToPointNetDeviceAnim");

    return tid;
}

void
PointToPointNetDeviceAnim::ConnectCallbacks()
{
    if (!m_netDev)
    {
        m_netDev = GetObject<PointToPointNetDevice>();
        NS_ASSERT_MSG(m_netDev == nullptr, "Failed to retrieve net-device");
    }
    // Tx/Rx packets are traced through the channel, but both NetDevices will try to hook to the
    // same trace
    // Since in a P2P there are only two devices, it's enough to have only one of them trace
    // the packets
    // We arbitrarily use the 1st device.
    Ptr<PointToPointChannel> channel = DynamicCast<PointToPointChannel>(m_netDev->GetChannel());
    if (channel->GetDevice(0) == m_netDev)
    {
        channel->TraceConnectWithoutContext(
            "TxRxPointToPoint",
            MakeCallback(&ns3::PointToPointNetDeviceAnim::DevTxTrace, this));
    }
    m_netDev->TraceConnectWithoutContext(
        "TxQueue/Enqueue",
        MakeCallback(&ns3::PointToPointNetDeviceAnim::EnqueueTrace, this));
    m_netDev->TraceConnectWithoutContext(
        "TxQueue/Dequeue",
        MakeCallback(&ns3::PointToPointNetDeviceAnim::DequeueTrace, this));
    m_netDev->TraceConnectWithoutContext(
        "TxQueue/Drop",
        MakeCallback(&ns3::PointToPointNetDeviceAnim::QueueDropTrace, this));
}

void
PointToPointNetDeviceAnim::DevTxTrace(Ptr<const Packet> p,
                                      Ptr<NetDevice> tx,
                                      Ptr<NetDevice> rx,
                                      Time txTime,
                                      Time rxTime)
{
    NS_LOG_FUNCTION(this);
    if (!IsEnabled())
    {
        return;
    }
    NS_ASSERT(tx);
    NS_ASSERT(rx);
    Time now = Simulator::Now();
    double fbTx = now.GetSeconds();
    double lbTx = (now + txTime).GetSeconds();
    double fbRx = (now + rxTime - txTime).GetSeconds();
    double lbRx = (now + rxTime).GetSeconds();
    m_anim->CheckMaxPktsPerTraceFile();
    m_anim->WriteXmlP("p",
                      tx->GetNode()->GetId(),
                      fbTx,
                      lbTx,
                      rx->GetNode()->GetId(),
                      fbRx,
                      lbRx,
                      m_anim->IsEnablePacketMetadata() ? m_anim->GetPacketMetadata(p) : "");
}

void
PointToPointNetDeviceAnim::EnqueueTrace(Ptr<const Packet> p)
{
    const Ptr<const Node> node = m_netDev->GetNode();
    m_anim->AddNodeToNodeEnqueueMap(node->GetId());
}

void
PointToPointNetDeviceAnim::DequeueTrace(Ptr<const Packet> p)
{
    const Ptr<const Node> node = m_netDev->GetNode();
    m_anim->AddNodeToNodeDequeueMap(node->GetId());
}

void
PointToPointNetDeviceAnim::QueueDropTrace(Ptr<const Packet> p)
{
    const Ptr<const Node> node = m_netDev->GetNode();
    m_anim->AddNodeToNodeDropMap(node->GetId());
}

void
PointToPointNetDeviceAnim::DoDispose()
{
    m_netDev = nullptr;
    m_anim = nullptr;
    Object::DoDispose();
}

void
PointToPointNetDeviceAnim::DoInitialize()
{
    m_netDev = GetObject<PointToPointNetDevice>();
    m_anim = DynamicCast<NetAnimWriter>(
        GetObject<PointToPointNetDevice>()->GetNode()->GetObject<Proxy<NetAnimWriter>>());
    ConnectCallbacks();
    Object::DoInitialize();
}

bool
PointToPointNetDeviceAnim::IsEnabled()
{
    return (m_anim->IsStarted() && m_anim->IsInTimeWindow() && m_anim->IsTracking());
}

} // namespace ns3
