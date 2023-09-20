
#include "point-to-point-net-device.h"

#include "ns3/animation-interface.h"
#include "ns3/ptr.h"

namespace ns3
{

class PointToPointNetDeviceAnim;

class PointToPointNetDeviceAnim : public Object

{
  public:
    /// Connect callbacks function
    void ConnectCallbacks();
    /**
     * \brief Get the type identificator.
     * \return type identificator
     */
    static TypeId GetTypeId();
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
     * Device transmit trace function
     * \param p the packet
     * \param tx the transmit device
     * \param rx the receive device
     * \param txTime the transmit time
     * \param rxTime the receive time
     */
    void DevTxTrace(Ptr<const Packet> p,
                    Ptr<NetDevice> tx,
                    Ptr<NetDevice> rx,
                    Time txTime,
                    Time rxTime);

  private:
    /**
     * \brief AnimationInterface object
     */
    Ptr<NetAnimWriter> m_anim{nullptr};
    Ptr<PointToPointNetDevice> m_netDev{nullptr}; ///< Pointer to NetDevice
};

} // namespace ns3
