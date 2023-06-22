#ifndef CSMANETDEVICE_H
#define CSMANETDEVICE_H

#include "ns3/animation-interface.h"
#include "ns3/net-device-anim.h"

namespace ns3
{

class CsmaNetDeviceAnim : public NetDeviceAnim

{
  public:
    void ConnectCallbacks();
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const;
    AnimationInterface m_anim;
    /**
     * CSMA Phy transmit begin trace function
     * \param context the context
     * \param p the packet
     */
    void CsmaPhyTxBeginTrace(std::string context, Ptr<const Packet> p);
    /**
     * CSMA Phy transmit end trace function
     *
     * \param context the context
     * \param p the packet
     */
    void CsmaPhyTxEndTrace(std::string context, Ptr<const Packet> p);
    /**
     * CSMA Phy receive end trace function
     *
     * \param context the context
     * \param p the packet
     */
    void CsmaPhyRxEndTrace(std::string context, Ptr<const Packet> p);
    /**
     * CSMA MAC receive trace function
     *
     * \param context the context
     * \param p the packet
     */
    void CsmaMacRxTrace(std::string context, Ptr<const Packet> p);
    /**
     * Enqueue trace function
     * \param context the context
     * \param p the packet
     */
    void EnqueueTrace(std::string context, Ptr<const Packet>);
    /**
     * Dequeue trace function
     * \param context the context
     * \param p the packet
     */
    void DequeueTrace(std::string context, Ptr<const Packet>);
    /**
     * Queue trace function
     * \param context the context
     * \param p the packet
     */
    void QueueDropTrace(std::string context, Ptr<const Packet>);
};

} // namespace ns3

#endif
