/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: George F. Riley<riley@ece.gatech.edu>
 * Author: John Abraham <john.abraham@gatech.edu>
 * Contributions: Eugene Kalishenko <ydginster@gmail.com> (Open Source and Linux Laboratory
 * http://dev.osll.ru/)
 */

// Interface between ns3 and the network animator

#ifndef ANIMATION_INTERFACE__H
#define ANIMATION_INTERFACE__H

#include "ns3/config.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4.h"
#include "ns3/log.h"
// #include "ns3/lte-enb-net-device.h"
// #include "ns3/lte-ue-net-device.h"
#include "ns3/mac48-address.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/node-list.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include "ns3/rectangle.h"
#include "ns3/simulator.h"
#include "ns3/singleton.h"
#include "ns3/uan-phy-gen.h"
#include "ns3/wifi-phy.h"

#include <cstdio>
#include <map>
#include <string>

namespace ns3
{

#define MAX_PKTS_PER_TRACE_FILE 100000
#define PURGE_INTERVAL 5
#define NETANIM_VERSION "netanim-3.109"
#define CHECK_STARTED_INTIMEWINDOW                                                                 \
    {                                                                                              \
        if (!m_started || !IsInTimeWindow())                                                       \
                                                                                                   \
        {                                                                                          \
            return;                                                                                \
        }                                                                                          \
    }
#define CHECK_STARTED_INTIMEWINDOW_TRACKPACKETS                                                    \
    {                                                                                              \
        if (!m_started || !IsInTimeWindow() || !m_trackPackets)                                    \
        {                                                                                          \
            return;                                                                                \
        }                                                                                          \
    }

struct NodeSize;
class WifiPsdu;

/**
 * \defgroup netanim Network Animation
 *
 * This section documents the API of the ns-3 netanim module. For a generic functional description,
 * please refer to the ns-3 manual.
 */

/**
 * \ingroup netanim
 *
 * \brief Interface to network animator
 *
 * Provides functions that facilitate communications with an
 * external or internal network animator.
 */

/**
 * AnimPacketInfo class
 */
class AnimPacketInfo

{
  public:
    AnimPacketInfo();
    /**
     * Constructor
     *
     * \param pInfo anim packet info
     */
    AnimPacketInfo(const AnimPacketInfo& pInfo);
    /**
     * Constructor
     *
     * \param tx_nd transmit device
     * \param fbTx fb transmit
     * \param txNodeId transmit node ID
     */
    AnimPacketInfo(Ptr<const NetDevice> tx_nd, const Time fbTx, uint32_t txNodeId = 0);
    Ptr<const NetDevice> m_txnd; ///< transmit device
    uint32_t m_txNodeId;         ///< node ID
    double m_fbTx;               ///< fb transmit
    double m_lbTx;               ///< lb transmit
    double m_fbRx;               ///< fb receive
    double m_lbRx;               ///< lb receive
    Ptr<const NetDevice> m_rxnd; ///< receive device
    /**
     * Process receive begin
     * \param nd the device
     * \param fbRx
     */
    void ProcessRxBegin(Ptr<const NetDevice> nd, const double fbRx);
};

class AnimationInterface
{
  public:
    /**
     * \brief Constructor
     * \param filename The Filename for the trace file used by the Animator
     *
     */
    AnimationInterface(const std::string& filename);
    AnimationInterface();

    /**
     * Counter Types
     */
    enum CounterType
    {
        UINT32_COUNTER,
        DOUBLE_COUNTER
    };

    /**
     * \brief typedef for WriteCallBack used for listening to AnimationInterfaceSingleton
     * write messages
     *
     */
    typedef void (*AnimWriteCallback)(const char* str);

    /**
     * \brief Enable tracking of Ipv4 L3 Protocol Counters such as Tx, Rx, Drop
     *
     * \param startTime Start Time for capturing values
     * \param stopTime Stop Time for capturing values
     * \param pollInterval The periodic interval at which the counters are written to the trace file
     *        Default: 1s
     */
    void EnableIpv4L3ProtocolCounters(Time startTime,
                                      Time stopTime,
                                      Time pollInterval = Seconds(1));

    /**
     * \brief Enable tracking of Queue Counters such as Enqueue, Dequeue, Queue Drops
     *
     * \param startTime Start Time for capturing values
     * \param stopTime Stop Time for capturing values
     * \param pollInterval The periodic interval at which the counters are written to the trace file
     *        Default: 1s
     */
    void EnableQueueCounters(Time startTime, Time stopTime, Time pollInterval = Seconds(1));

    /**
     * \brief Enable tracking of Wifi Mac Counters such as Tx, TxDrop, Rx, RxDrop
     *
     * \param startTime Start Time for capturing values
     * \param stopTime Stop Time for capturing values
     * \param pollInterval The periodic interval at which the counters are written to the trace file
     *        Default: 1s
     */
    void EnableWifiMacCounters(Time startTime, Time stopTime, Time pollInterval = Seconds(1));

    /**
     * \brief Enable tracking of Wifi Phy Counters such as TxDrop, RxDrop
     *
     * \param startTime Start Time for capturing values
     * \param stopTime Stop Time for capturing values
     * \param pollInterval The periodic interval at which the counters are written to the trace file
     *        Default: 1s
     */
    void EnableWifiPhyCounters(Time startTime, Time stopTime, Time pollInterval = Seconds(1));

    /**
     * \brief Enable tracking of the Ipv4 routing table for all Nodes
     *
     * \param fileName Trace file for storing routing table information
     * \param startTime Start time for capture
     * \param stopTime  End time for capture
     * \param pollInterval The periodic interval at which routing table information is polled
     *        Default: 5s
     *
     */
    void EnableIpv4RouteTracking(std::string fileName,
                                 Time startTime,
                                 Time stopTime,
                                 Time pollInterval = Seconds(5));

    /**
     * \brief Enable tracking of the Ipv4 routing table for a set of Nodes
     *
     * \param fileName Trace file for storing routing table information
     * \param startTime Start time for capture
     * \param stopTime  End time for capture
     * \param nc A NodeContainer containing nodes for which Routing table has to be tracked
     * \param pollInterval The periodic interval at which routing table information is polled
     *        Default: 5s
     *
     */
    void EnableIpv4RouteTracking(std::string fileName,
                                 Time startTime,
                                 Time stopTime,
                                 NodeContainer nc,
                                 Time pollInterval = Seconds(5));

    /**
     * \brief Check if AnimationInterfaceSingleton is initialized
     *
     * \returns true if AnimationInterfaceSingleton was already initialized
     *
     */
    static bool IsInitialized();

    /**
     * \brief Specify the time at which capture should start
     *
     * \param t The time at which AnimationInterfaceSingleton should begin capture of traffic info
     *
     */
    void SetStartTime(Time t);

    /**
     * \brief Specify the time at which capture should stop
     *
     * \param t The time at which AnimationInterfaceSingleton should stop capture of traffic info
     *
     */
    void SetStopTime(Time t);

    /**
     * \brief Set Max packets per trace file
     * \param maxPktsPerFile The maximum number of packets per trace file.
              AnimationInterfaceSingleton will create trace files with the following
              filenames : filename, filename-1, filename-2..., filename-N
              where each file contains packet info for 'maxPktsPerFile' number of packets
     *
     */
    void SetMaxPktsPerTraceFile(uint64_t maxPktsPerFile);

    /**
     * \brief Set mobility poll interval:WARNING: setting a low interval can
     * cause slowness
     *
     * \param t Time interval between fetching mobility/position information
     * Default: 0.25s
     *
     */
    void SetMobilityPollInterval(Time t);

    /**
     * \brief Set a callback function to listen to AnimationInterfaceSingleton write events
     *
     * \param cb Address of callback function
     *
     */
    void SetAnimWriteCallback(AnimWriteCallback cb);

    /**
     * \brief Reset the write callback function
     *
     */
    void ResetAnimWriteCallback();

    /**
     * \brief Helper function to set Constant Position for a given node
     * \param n Ptr to the node
     * \param x X coordinate of the node
     * \param y Y coordinate of the node
     * \param z Z coordinate of the node
     *
     */
    static void SetConstantPosition(Ptr<Node> n, double x, double y, double z = 0);

    /**
     * \brief Helper function to update the description for a given node
     * \param n Ptr to the node
     * \param descr A string to briefly describe the node
     *
     */
    void UpdateNodeDescription(Ptr<Node> n, std::string descr);

    /**
     * \brief Helper function to update the description for a given node
     * \param nodeId Id of the node
     * \param descr A string to briefly describe the node
     *
     */
    void UpdateNodeDescription(uint32_t nodeId, std::string descr);

    /**
     * \brief Helper function to update the image of a node
     * \param nodeId Id of the node
     * \param resourceId Id of the image resource that was previously added
     *
     */
    void UpdateNodeImage(uint32_t nodeId, uint32_t resourceId);

    /**
     * \brief Helper function to update the size of a node
     * \param n Ptr to the node
     * \param width Width of the node
     * \param height Height of the node
     *
     */
    void UpdateNodeSize(Ptr<Node> n, double width, double height);

    /**
     * \brief Helper function to update the size of a node
     * \param nodeId Id of the node
     * \param width Width of the node
     * \param height Height of the node
     *
     */
    void UpdateNodeSize(uint32_t nodeId, double width, double height);

    /**
     * \brief Helper function to update the node color
     * \param n Ptr to the node
     * \param r Red component value (0-255)
     * \param g Green component value (0-255)
     * \param b Blue component value (0-255)
     *
     */
    void UpdateNodeColor(Ptr<Node> n, uint8_t r, uint8_t g, uint8_t b);

    /**
     * \brief Helper function to update the node color
     * \param nodeId Id of the node
     * \param r Red component value (0-255)
     * \param g Green component value (0-255)
     * \param b Blue component value (0-255)
     *
     */
    void UpdateNodeColor(uint32_t nodeId, uint8_t r, uint8_t g, uint8_t b);

    /**
     * \brief Helper function to update a node's counter referenced by the nodeCounterId
     * \param nodeCounterId The counter Id obtained from AddNodeCounter
     * \param nodeId Node Id of the node
     * \param counter Current value of the counter
     *
     */
    void UpdateNodeCounter(uint32_t nodeCounterId, uint32_t nodeId, double counter);

    /**
     * \brief Helper function to set the background image
     * \param fileName File name of the background image
     * \param x X coordinate of the image
     * \param y Y coordinate of the image
     * \param scaleX X scale of the image
     * \param scaleY Y scale of the image
     * \param opacity Opacity of the background: A value between 0.0 and 1.0. 0.0 is transparent,
     *        1.0 is opaque
     *
     */
    void SetBackgroundImage(std::string fileName,
                            double x,
                            double y,
                            double scaleX,
                            double scaleY,
                            double opacity);

    /**
     * \brief Helper function to update the description for a link
     * \param fromNode Node Id of the "from Node" of the p2p link
     * \param toNode Node Id of the "to Node" of the p2p link
     * \param linkDescription Description of the link such as link bandwidth
     *
     */
    void UpdateLinkDescription(uint32_t fromNode, uint32_t toNode, std::string linkDescription);

    /**
     * \brief Helper function to update the description for a link
     * \param fromNode Ptr to the "from Node" of the p2p link
     * \param toNode Ptr to the "to Node" of the p2p link
     * \param linkDescription Description of the link such as link bandwidth
     *
     */
    void UpdateLinkDescription(Ptr<Node> fromNode, Ptr<Node> toNode, std::string linkDescription);

    /**
     * \brief Helper function to print the routing path from a source node to destination IP
     * \param fromNodeId The source node
     * \param destinationIpv4Address The destination Ipv4 Address
     *
     */
    void AddSourceDestination(uint32_t fromNodeId, std::string destinationIpv4Address);

    /**
     * \brief Is AnimationInterfaceSingleton started
     *
     * \returns true if AnimationInterfaceSingleton was started
     */
    bool IsStarted() const;

    /**
     * \brief Do not trace packets. This helps reduce the trace file size if
     * AnimationInterfaceSingleton is solely used for tracking mobility, routing paths and counters
     */
    void SkipPacketTracing();

    /**
     *
     * \brief Enable Packet metadata
     * \param enable if true enables writing the packet metadata to the XML trace file
     *        if false disables writing the packet metadata
     *
     */
    void EnablePacketMetadata(bool enable = true);

    /**
     *
     * \brief Get trace file packet count (This used only for testing)
     *
     * \returns Number of packets recorded in the current trace file
     */
    uint64_t GetTracePktCount() const;

    /**
     *
     * \brief Setup a node counter
     * \param counterName A string to identify the counter
     * \param counterType The type of the counter, such as uint32, double etc
     *
     * \returns The id of the counter to be used as a reference for future
     */
    uint32_t AddNodeCounter(std::string counterName, CounterType counterType);

    /**
     *
     * \brief Add a resource such as the path to an image file
     * \param resourcePath Absolute Path to an image/resource
     *
     * \returns a number identifying the resource
     */
    uint32_t AddResource(std::string resourcePath);

    /**
     *
     * \brief Get node's energy fraction (This used only for testing)
     * \param node
     *
     * \returns current node's remaining energy (between [0, 1])
     */
    double GetNodeEnergyFraction(Ptr<const Node> node) const;
    bool IsInTimeWindow();
    bool IsTracking();
    /**
     * Get net device from context
     * \param context the context string
     * \returns the device
     */
    Ptr<NetDevice> GetNetDeviceFromContext(std::string context);
    Vector UpdatePosition(Ptr<NetDevice> ndev);
    void IncrementAnimUid();
    uint64_t GetAnimUid();
    /**
     * Add byte tag function
     * \param animUid the UID
     * \param p the packet
     */
    void AddByteTag(uint64_t animUid, Ptr<const Packet> p);

    /// ProtocolType enumeration
    enum ProtocolType
    {
        UAN,
        // LTE,
        WIFI,
        WIMAX,
        CSMA,
        LRWPAN
    };

    /**
     * Add pending packet function
     * \param protocolType the protocol type
     * \param animUid the UID
     * \param pktInfo the packet info
     */
    void AddPendingPacket(ProtocolType protocolType, uint64_t animUid, AnimPacketInfo pktInfo);
    /**
     * Get anim UID from packet function
     * \param p the packet
     * \returns the UID
     */
    uint64_t GetAnimUidFromPacket(Ptr<const Packet> p);
    /**
     * Is packet pending function
     * \param animUid the UID
     * \param protocolType the protocol type
     * \returns true if a packet is pending
     */
    bool IsPacketPending(uint64_t animUid, AnimationInterface::ProtocolType protocolType);
    /**
     * Output CSMA packet function
     * \param p the packet
     * \param pktInfo the packet info
     */
    void OutputCsmaPacket(Ptr<const Packet> p, AnimPacketInfo& pktInfo);
    std::map<uint64_t, AnimPacketInfo>& GetPendingCsmaPacketsMap();
    /**
     * Get node from context
     * \param context the context string
     * \returns the node
     */
    Ptr<Node> GetNodeFromContext(const std::string& context) const;
    void AddNodeToNodeEnqueueMap(uint32_t nodeId);
    void AddNodeToNodeDequeueMap(uint32_t nodeId);
    void AddNodeToNodeDropMap(uint32_t nodeId);
    /// Check maximum packets per trace file function
    void CheckMaxPktsPerTraceFile();
    /**
     * Write XMLP function
     * \param pktType the packet type
     * \param fId the FID
     * \param fbTx the FB transmit
     * \param lbTx the LB transmit
     * \param tId the TID
     * \param fbRx the FB receive
     * \param lbRx the LB receive
     * \param metaInfo the meta info
     */
    void WriteXmlP(std::string pktType,
                   uint32_t fId,
                   double fbTx,
                   double lbTx,
                   uint32_t tId,
                   double fbRx,
                   double lbRx,
                   std::string metaInfo = "");

    bool IsEnablePacketMetadata();
    std::string GetPacketMetadata(Ptr<const Packet> p);
};

} // namespace ns3
#endif
