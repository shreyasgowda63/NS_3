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
    /**
     * \brief Get the type identificator.
     * \return type identificator
     */
    static TypeId GetTypeId();
    /**
     * \brief Get the instance type ID.
     * \return instance type ID
     */
    TypeId GetInstanceTypeId() const;
    /**
     * CSMA Phy transmit begin trace function
     * \param context the context
     * \param p the packet
     */
    void CsmaPhyTxBeginTrace(Ptr<const Packet> p);
    /**
     * CSMA Phy transmit end trace function
     *
     * \param context the context
     * \param p the packet
     */
    void CsmaPhyTxEndTrace(Ptr<const Packet> p);
    /**
     * CSMA Phy receive end trace function
     *
     * \param context the context
     * \param p the packet
     */
    void CsmaPhyRxEndTrace(Ptr<const Packet> p);
    /**
     * CSMA MAC receive trace function
     *
     * \param context the context
     * \param p the packet
     */
    void CsmaMacRxTrace(Ptr<const Packet> p);
    /**
     * Enqueue trace function
     * \param context the context
     * \param p the packet
     */
    void EnqueueTrace(Ptr<const Packet>);
    /**
     * Dequeue trace function
     * \param context the context
     * \param p the packet
     */
    void DequeueTrace(Ptr<const Packet>);
    /**
     * Queue trace function
     * \param context the context
     * \param p the packet
     */
    void QueueDropTrace(Ptr<const Packet>);

  private:
    /**
     * \brief AnimationInterface object
     */
    AnimationInterface m_anim;
};

} // namespace ns3

#endif
