#ifndef CSMANETDEVICE_H
#define CSMANETDEVICE_H

#include "ns3/animation-interface.h"
#include "ns3/csma-net-device.h"
#include "ns3/ptr.h"

namespace ns3
{

class CsmaNetDeviceAnim;

class CsmaNetDeviceAnim : public Object

{
  public:
    /**
     * CsmaAnimPacketInfo class
     */
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
         * \param txNodeId transmit node ID
         * \param firstBitTxTime time of the first bit being transmitted
         */
        CsmaAnimPacketInfo(uint32_t txNodeId, const Time firstBitTxTime);
        uint32_t m_txNodeId;   ///< node ID
        Time m_firstBitTxTime; ///< time of the first bit being transmitted (when the packet did
                               ///< start the Tx)
        Time m_lastBitTxTime;  ///< time of the last bit being transmitted (when the packet did
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
     * CSMA Phy transmit begin trace function
     * \param p the packet
     */
    void CsmaPhyTxBeginTrace(Ptr<const Packet> p);
    /**
     * CSMA Phy transmit end trace function
     * \param p the packet
     */
    void CsmaPhyTxEndTrace(Ptr<const Packet> p);
    /**
     * CSMA Phy receive end trace function
     * \param p the packet
     */
    void CsmaPhyRxEndTrace(Ptr<const Packet> p);
    /**
     * CSMA MAC receive trace function
     * \param p the packet
     */
    void CsmaMacRxTrace(Ptr<const Packet> p);
    /**
     * Enqueue trace function
     * \param p the packet
     */
    void EnqueueTrace(Ptr<const Packet> p);
    /**
     * Dequeue trace function
     * \param p the packet
     */
    void DequeueTrace(Ptr<const Packet> p);
    /**
     * Queue trace function
     * \param p the packet
     */
    void QueueDropTrace(Ptr<const Packet> p);
    /**
     * Output CSMA packet function
     * \param p the packet
     * \param pktInfo the packet info
     */
    void OutputCsmaPacket(Ptr<const Packet> p, CsmaAnimPacketInfo& pktInfo);
    /**
     * Purge pending packets function
     */
    static void PurgePendingPackets();
    // Inherited from Object base class.
    void DoDispose() override;
    // inherited from Object
    void DoInitialize() override;
    // Checks if Animation Interface has been enabled
    /**
     * \brief Checks if Animation Interface has been enabled
     * \return If Trace is enabled
     */
    bool IsEnabled();

  private:
    /**
     * \brief AnimationInterface object
     */
    Ptr<NetAnimWriter> m_anim{nullptr};
    double m_firstBitRxTime; ///< time of the first bit being received (when the packet did start
                             ///< the Rx)
    double m_lastBitRxTime;  ///< time of the last bit being received (when the packet did start
                             ///< the Rx)
    typedef std::map<uint64_t, CsmaAnimPacketInfo>
        CsmaAnimUidPacketInfoMap;                         ///< CsmaAnimUidPacketInfoMap typedef
    static CsmaAnimUidPacketInfoMap m_pendingCsmaPackets; ///< pending CSMA packets
    static uint64_t csmaAnimUid;                          ///< Csma AnimUid
    static EventId m_purgeCsmaAnimPendingPacketsEventId;  ///< PurgeCsmaAnimPackets EventId
    Ptr<CsmaNetDevice> m_netDev{nullptr};                 ///< Pointer to NetDevice
    static Time csmaPurgeInterval; ///< Minimum time interval to purge pending packets
    static Time
        schedulePurgePendingPackets; ///< Scheduled time interval to call PurgePendingPackets
};

} // namespace ns3

#endif
