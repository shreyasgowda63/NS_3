#ifndef WIFINETDEVICE_H
#define WIFINETDEVICE_H

#include "ns3/animation-interface.h"
#include "ns3/ptr.h"
#include "ns3/wifi-net-device.h"

namespace ns3
{

class WifiNetDeviceAnim;

class WifiNetDeviceAnim : public Object

{
  public:
    /**
     * WifiAnimPacketInfo class
     */
    class WifiAnimPacketInfo

    {
      public:
        /**
         * Constructor
         */
        WifiAnimPacketInfo();
        /**
         * Constructor
         *
         * \param pInfo anim packet info
         */
        WifiAnimPacketInfo(const WifiAnimPacketInfo& pInfo);
        /**
         * Constructor
         * \param txNodeId transmit node ID
         * \param firstBitTxTime time of the first bit being transmitted
         */
        WifiAnimPacketInfo(uint32_t txNodeId, const Time firstBitTxTime);
        uint32_t m_txNodeId;   ///< node ID
        Time m_firstBitTxTime; ///< time of the first bit being transmitted (when the packet did
                               ///< start the Tx)
        Time m_lastBitTxTime;  ///< time of the last bit being transmitted (when the packet did
                               ///< start the Tx)
        Time m_firstBitRxTime; ///< time of the first bit being received (when the packet did
                               ///< start the Rx)
        Time m_lastBitRxTime;  ///< time of the last bit being received (when the packet did start
                               ///< the Rx)
        /**
         * Process receive begin
         * \param nd the device
         * \param fbRx
         */
        void ProcessRxBegin(Ptr<const NetDevice> nd, const Time fbRx);
    };

    /// Connect callbacks function
    void ConnectCallbacks();
    /**
     * \brief Get the type identificator.
     * \return type identificator
     */
    static TypeId GetTypeId();
    /**
     * wifi Phy transmit PSDU begin trace function
     * \param psduMap the PSDU map
     * \param txVector the TXVECTOR
     * \param txPowerW the tx power in Watts
     */
    void WifiPhyTxBeginTrace(WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW);
    /**
     * wifi Phy receive begin trace function
     *
     * \param p the packet
     * \param rxPowersW the receive power per channel band in Watts
     */
    void WifiPhyRxBeginTrace(Ptr<const Packet> p, RxPowerWattPerChannelBand rxPowersW);
    /**
     * wifi MAC transmit trace function
     * \param p the packet
     */
    void WifiMacTxTrace(Ptr<const Packet> p);
    /**
     * wifi MAC transmit drop trace function
     * \param p the packet
     */
    void WifiMacTxDropTrace(Ptr<const Packet> p);
    /**
     * wifi MAC receive trace function
     * \param p the packet
     */
    void WifiMacRxTrace(Ptr<const Packet> p);
    /**
     * wifi MAC receive drop trace function
     * \param p the packet
     */
    void WifiMacRxDropTrace(Ptr<const Packet> p);
    /**
     * wifi Phy transmit drop trace function
     * \param p the packet
     */
    void WifiPhyTxDropTrace(Ptr<const Packet> p);
    /**
     * wifi Phy receive drop trace function
     * \param p the packet
     * \param reason the reason
     */
    void WifiPhyRxDropTrace(Ptr<const Packet> p, WifiPhyRxfailureReason reason);
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
    /**
     * Output wireless packet transmit info
     * \param p the packet
     * \param pktInfo the packet info
     * \param animUid the UID
     */
    void OutputWirelessPacketTxInfo(Ptr<const Packet> p,
                                    WifiAnimPacketInfo& pktInfo,
                                    uint64_t animUid);
    /**
     * Output wireless packet receive info
     * \param p the packet
     * \param pktInfo the packet info
     * \param animUid the UID
     */
    void OutputWirelessPacketRxInfo(Ptr<const Packet> p,
                                    WifiAnimPacketInfo& pktInfo,
                                    uint64_t animUid);

  private:
    /**
     * \brief AnimationInterface object
     */
    Ptr<NetAnimWriter> m_anim{nullptr};
    typedef std::map<uint64_t, WifiAnimPacketInfo>
        WifiAnimUidPacketInfoMap;                         ///< WifiAnimUidPacketInfoMap typedef
    static WifiAnimUidPacketInfoMap m_pendingWifiPackets; ///< pending Wifi packets
    static uint64_t wifiAnimUid;                          ///< Wifi AnimUid
    static EventId m_purgeWifiAnimPendingPacketsEventId;  ///< PurgeWifiAnimPackets EventId
    Ptr<WifiNetDevice> m_netDev{nullptr};                 ///< Pointer to NetDevice
    static Time wifiPurgeInterval; ///< Minimum time interval to purge pending packets
    static Time
        schedulePurgePendingPackets; ///< Scheduled time interval to call PurgePendingPackets
};

} // namespace ns3

#endif
