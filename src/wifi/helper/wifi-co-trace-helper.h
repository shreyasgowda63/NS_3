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
 */

#ifndef WIFI_CO_HELPER_H
#define WIFI_CO_HELPER_H

#include "ns3/callback.h"
#include "ns3/config.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/simulator.h"
#include "ns3/wifi-phy-state.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace ns3
{

class WifiNetDevice;
class NodeContainer;
class NetDeviceContainer;

/**
 * @class WifiCoTraceHelper
 * @brief Track channel occupancy durations for WifiNetDevice
 *
 * The WifiCoTraceHelper class tracks the duration that a particular WifiNetDevice is in
 * different states.  The states are defined by the ns-3 WifiPhyStateHelper and include states such
 * as IDLE, CCA_BUSY, TX, and RX.  The helper tracks these durations between a user-configured start
 * and end time.  At the end of a simulation, this helper can print out statistics on channel
 * occupancy, and permits the export of an internal data structure to allow for custom printing or
 * statistics handling.
 *
 * This helper supports both single-link devices and multi-link devices (MLD).
 */
class WifiCoTraceHelper
{
  public:
    /**
     * @struct DeviceRecord
     * @brief Keeps track of channel occupancy statistics observed at a WifiNetDevice.
     *
     * Data structure to track durations of each WifiPhy state. Elements in m_linkStateDurations are
     * indexed by "linkId".
     */
    struct DeviceRecord
    {
        uint32_t m_nodeId;        ///< Id of Node on which the WifiNetDevice is installed.
        std::string m_nodeName;   ///< Name of Node on which the WifiNetDevice is installed. It's
                                  ///< empty if the name isn't configured.
        uint32_t m_ifIndex;       ///< Device Id of WifiNetDevice.
        std::string m_deviceName; ///< Device name. It's empty if the name isn't configured.

        /**
         * Duration statistics by link and state. LinkId is the key in map.
         */
        std::map<uint8_t, std::map<WifiPhyState, Time>> m_linkStateDurations;

        /**
         * Constructor.
         *
         * @param device The WifiNetDevice whose links will be monitored to collect statistics.
         */
        DeviceRecord(Ptr<WifiNetDevice> device);

      private:
        /**
         * A callback to update m_linkStateDurations.
         *
         * @param linkId Id of Link whose statistics is updated.
         * @param start Instant at which link switched its WifiPhyState to state.
         * @param duration Duration of time the link stays in this state.
         * @param state The state of the link from start to (start + duration).
         */
        void AddLinkMeasurement(size_t linkId, Time start, Time duration, WifiPhyState state);

        friend class WifiCoTraceHelper;
    };

    /**
     * Default Constructor. StartTime is Seconds(0) and stopTime is Time::Max()
     */
    WifiCoTraceHelper();

    /**
     * Construct a helper object measuring between two simulation time points [startTime, stopTime]
     *
     * @param startTime The measurement start time
     * @param stopTime The measurement stop time
     */
    WifiCoTraceHelper(Time startTime, Time stopTime);

    /**
     * Enables trace collection for all nodes and WifiNetDevices in the specified NodeContainer.
     * @param nodes The NodeContainer containing WifiNetDevices to which traces are to be connected.
     */
    void Enable(NodeContainer nodes);
    /**
     * Enables trace collection for all nodes corresponding to the devices in the specified
     * NetDeviceContainer.
     * @param devices The NetDeviceContainer to which traces are to be connected.
     */
    void Enable(NetDeviceContainer devices);

    /**
     * Starts the collection of statistics from a specified start time.
     * @param startTime The time to start collecting statistics.
     */
    void Start(Time startTime);

    /**
     * Stops the collection of statistics at a specified time.
     * @param stopTime The time to stop collecting statistics.
     */
    void Stop(Time stopTime);

    /**
     * Resets the current statistics, clearing all links and their durations. It does not disconnect
     * traced callbacks. It does not clear DeviceRecords. Only the statistics collected prior to
     * invoking this method is cleared.
     */
    void Reset();

    /**
     * Print measurement results on an output stream
     *
     * @param os The output stream to print to
     * @param unit The unit of time in which the durations should be printed. Defaults to AUTO.
     */
    void PrintStatistics(std::ostream& os, Time::Unit unit = Time::Unit::AUTO) const;

    /**
     * Returns measurement results on each installed device.
     *
     * @return Statistics of each device traced by this helper.
     */
    const std::vector<DeviceRecord>& GetDeviceRecords() const;

  private:
    uint32_t m_numDevices{0};     ///< Count the number of devices traced by this helper.
    Time m_startTime;             ///< Instant at which statistics collection should start.
    Time m_stopTime{Time::Max()}; ///< Instant at which statistics collection should stop.

    std::vector<DeviceRecord> m_deviceRecords; ///< Stores the collected statistics.

    /**
     * A callback used to update statistics.
     *
     * @param idx Index of the device in m_deviceRecords vector.
     * @param phyId Id of Phy whose state is switched.
     * @param start Instant at which link switched its WifiPhyState to state.
     * @param duration Duration of time the link stays in this state.
     * @param state The state of the link from start to (start + duration).
     */
    void NotifyWifiPhyState(std::size_t idx,
                            std::size_t phyId,
                            Time start,
                            Time duration,
                            WifiPhyState state);

    /**
     * Compute overlapping time-duration between two intervals. It is a helper function used by
     * NotifyWifiPhyState function.
     *
     * @param beg1 Beginning of first interval
     * @param end1 End of first interval
     * @param beg2 Beginning of second interval
     * @param end2 End of second interval
     * @return Return the time duration common between the two intervals.
     */
    Time computeOverlappingDuration(Time beg1, Time end1, Time beg2, Time end2);
    /**
     * A helper function used by PrintStatistics method. It converts absolute statistics to
     * percentages.
     *
     * @param linkStates Statistics of a link.
     * @return Statistics in percentages.
     */
    std::map<WifiPhyState, double> ComputePercentage(
        const std::map<WifiPhyState, Time>& linkStates) const;

    /**
     * A helper function used by PrintStatistics method. It prints statistics of a link.
     *
     * @param os The output stream to print to
     * @param linkStates Statistics of a link.
     * @param unit The unit of time in which the durations should be printed.
     * @return The output stream after IO operations.
     */
    std::ostream& PrintLinkStates(std::ostream& os,
                                  const std::map<WifiPhyState, Time>& linkStates,
                                  Time::Unit unit) const;

    /**
     * A helper function used by PrintLinkStates method to format the output. It pads each string at
     * left with space characters such that the decimal points are at the same position.
     *
     * @param column A column of strings. Each string must contain a decimal point.
     */
    void alignDecimal(std::vector<std::string>& column) const;

    /**
     * A helper function used by PrintLinkStates method to format the output. It pads each string at
     * right with space characters such that all strings have same width.
     *
     * @param column A column of strings.
     */
    void alignWidth(std::vector<std::string>& column) const;
};

} // namespace ns3

#endif
