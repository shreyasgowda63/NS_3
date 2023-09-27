
#include "lte-enb-net-device.h"
#include "lte-net-device.h"
#include "lte-ue-net-device.h"

#include "ns3/animation-interface.h"
#include "ns3/ptr.h"

namespace ns3
{

class LteNetDeviceAnim;

class LteNetDeviceAnim : public Object

{
  public:
    /**
     * LteAnimPacketInfo class
     */
    class LteAnimPacketInfo

    {
      public:
        /**
         * Constructor
         */
        LteAnimPacketInfo();
        /**
         * Constructor
         *
         * \param pInfo anim packet info
         */
        LteAnimPacketInfo(const LteAnimPacketInfo& pInfo);
        /**
         * Constructor
         * \param txNodeId transmit node ID
         * \param firstBitTxTime time of the first bit being transmitted
         */
        LteAnimPacketInfo(uint32_t txNodeId, const Time firstBitTxTime);
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
     * LTE Spectrum Phy transmit start function
     * \param pb the packet burst
     */
    void LteSpectrumPhyTxStart(Ptr<const PacketBurst> pb);
    /**
     * LTE Spectrum Phy receive start function
     * \param pb the packet burst
     */
    void LteSpectrumPhyRxStart(Ptr<const PacketBurst> pb);
    /**
     * Connect LTE ue function
     * \param n the node
     * \param nd the device
     * \param devIndex the device index
     */
    void ConnectLteUe(Ptr<Node> n, Ptr<LteUeNetDevice> nd, uint32_t devIndex);
    /**
     * Connect LTE ENB function
     * \param n the node
     * \param nd the device
     * \param devIndex the device index
     */
    void ConnectLteEnb(Ptr<Node> n, Ptr<LteEnbNetDevice> nd, uint32_t devIndex);
    /**
     * Output wireless packet transmit info
     * \param p the packet
     * \param pktInfo the packet info
     * \param animUid the UID
     */
    void OutputWirelessPacketTxInfo(Ptr<const Packet> p,
                                    LteAnimPacketInfo& pktInfo,
                                    uint64_t animUid);
    /**
     * Output wireless packet receive info
     * \param p the packet
     * \param pktInfo the packet info
     * \param animUid the UID
     */
    void OutputWirelessPacketRxInfo(Ptr<const Packet> p,
                                    LteAnimPacketInfo& pktInfo,
                                    uint64_t animUid);
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
    typedef std::map<uint64_t, LteAnimPacketInfo>
        LteAnimUidPacketInfoMap;                        ///< LteAnimUidPacketInfoMap typedef
    static LteAnimUidPacketInfoMap m_pendingLtePackets; ///< pending CSMA packets
    static uint64_t lteAnimUid;                         ///< Lte AnimUid
    static EventId m_purgeLteAnimPendingPacketsEventId; ///< PurgeLteAnimPackets EventId
    Ptr<LteNetDevice> m_netDev{nullptr};                ///< Pointer to NetDevice
    static Time ltePurgeInterval; ///< Minimum time interval to purge pending packets
    static Time
        schedulePurgePendingPackets; ///< Scheduled time interval to call PurgePendingPackets
};

} // namespace ns3
