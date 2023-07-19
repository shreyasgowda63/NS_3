#ifndef CSMANETDEVICE_H
#define CSMANETDEVICE_H

#include "ns3/animation-interface.h"
#include "ns3/net-device-anim.h"

namespace ns3
{

class CsmaNetDeviceAnim : public NetDeviceAnim

{
  public:
    class CsmaAnimPacketInfo

    {
      public:
        /**
         * Constructor
         */
        CsmaAnimPacketInfo();
        /**
         * Constructor
         *
         * \param pInfo anim packet info
         */
        CsmaAnimPacketInfo(const CsmaAnimPacketInfo& pInfo);
        /**
         * Constructor
         *
         * \param tx_nd transmit device
         * \param fbTx fb transmit
         * \param txNodeId transmit node ID
         */
        CsmaAnimPacketInfo(Ptr<const NetDevice> tx_nd, const Time fbTx, uint32_t txNodeId = 0);
        Ptr<const NetDevice> m_txnd; ///< transmit device
        uint32_t m_txNodeId;         ///< node ID
        double m_firstBitTxTime; ///< time of the first bit being transmitted (when the packet did
                                 ///< start the Tx)
        double m_lastBitTxTime;  ///< time of the last bit being transmitted (when the packet did
                                 ///< start the Tx)
    };

    /// Connect callbacks function
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
    void EnqueueTrace(Ptr<const Packet> p);
    /**
     * Dequeue trace function
     * \param context the context
     * \param p the packet
     */
    void DequeueTrace(Ptr<const Packet> p);
    /**
     * Queue trace function
     * \param context the context
     * \param p the packet
     */
    void QueueDropTrace(Ptr<const Packet> p);
    /**
     * Output CSMA packet function
     * \param p the packet
     * \param pktInfo the packet info
     */
    void OutputCsmaPacket(Ptr<const Packet> p, CsmaAnimPacketInfo& pktInfo);

  private:
    /**
     * \brief AnimationInterface object
     */
    AnimationInterface m_anim;
    double m_firstBitRxTime; ///< time of the first bit being received (when the packet did start
                             ///< the Rx)
    double m_lastBitRxTime;  ///< time of the last bit being received (when the packet did start
                             ///< the Rx)
    typedef std::map<uint64_t, CsmaAnimPacketInfo>
        CsmaAnimUidPacketInfoMap;                         ///< CsmaAnimUidPacketInfoMap typedef
    static CsmaAnimUidPacketInfoMap m_pendingCsmaPackets; ///< pending CSMA packets
    static uint64_t csmaAnimUid;                          ///< Csma AnimUid
};

} // namespace ns3

#endif
