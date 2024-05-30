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

#include "wifi-co-trace-helper.h"

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/pointer.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy-state-helper.h"
#include "ns3/wifi-phy.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiCoTraceHelper");

WifiCoTraceHelper::WifiCoTraceHelper()
{
    NS_LOG_FUNCTION(this);
}

WifiCoTraceHelper::WifiCoTraceHelper(Time startTime, Time stopTime)
{
    NS_LOG_FUNCTION(this << startTime.As(Time::S) << stopTime.As(Time::S));
    NS_ASSERT_MSG(startTime <= stopTime,
                  "Invalid Start: " << startTime << " and Stop: " << stopTime << " Time");

    m_startTime = startTime;
    m_stopTime = stopTime;
}

void
WifiCoTraceHelper::Start(Time start)
{
    NS_LOG_FUNCTION(this << start.As(Time::S));
    NS_ASSERT_MSG(start <= m_stopTime,
                  "Invalid Start: " << start << " and Stop: " << m_stopTime << " Time");

    m_startTime = start;
}

void
WifiCoTraceHelper::Stop(Time stop)
{
    NS_LOG_FUNCTION(this << stop.As(Time::S));
    NS_ASSERT_MSG(m_startTime <= stop,
                  "Invalid Start: " << m_startTime << " and Stop: " << stop << " Time");

    m_stopTime = stop;
}

void
WifiCoTraceHelper::Reset()
{
    NS_LOG_FUNCTION(this);

    for (auto& record : m_deviceRecords)
    {
        record.m_linkStateDurations.clear();
    }
}

void
WifiCoTraceHelper::Enable(NodeContainer nodes)
{
    NS_LOG_FUNCTION(this << nodes.GetN());
    NetDeviceContainer netDevices;
    for (uint32_t i = 0; i < nodes.GetN(); i++)
    {
        for (uint32_t j = 0; j < nodes.Get(i)->GetNDevices(); j++)
        {
            netDevices.Add(nodes.Get(i)->GetDevice(j));
        }
    }
    Enable(netDevices);
}

void
WifiCoTraceHelper::Enable(NetDeviceContainer devices)
{
    NS_LOG_FUNCTION(this << devices.GetN());

    for (uint32_t j = 0; j < devices.GetN(); j++)
    {
        auto device = DynamicCast<WifiNetDevice>(devices.Get(j));
        if (!device)
        {
            NS_LOG_INFO("Ignoring deviceId: " << devices.Get(j)->GetIfIndex() << " on nodeId: "
                                              << devices.Get(j)->GetNode()->GetId()
                                              << " because it is not of type WifiNetDevice");
            continue;
        }
        auto idx = m_numDevices++;
        m_deviceRecords.emplace_back(device);
        for (uint32_t k = 0; k < device->GetNPhys(); k++)
        {
            auto wifiPhyStateHelper = device->GetPhy(k)->GetState();
            auto linkCallback =
                MakeCallback(&WifiCoTraceHelper::NotifyWifiPhyState, this).Bind(idx, k);
            wifiPhyStateHelper->TraceConnectWithoutContext("State", linkCallback);
        }
    }
}

void
WifiCoTraceHelper::PrintStatistics(std::ostream& os, Time::Unit unit) const
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(m_deviceRecords.size() == m_numDevices, "m_deviceRecords size mismatch");

    for (size_t i = 0; i < m_numDevices; i++)
    {
        std::string nodeName = m_deviceRecords[i].m_nodeName;
        std::string deviceName = m_deviceRecords[i].m_deviceName;
        if (nodeName.empty())
        {
            nodeName = std::to_string(m_deviceRecords[i].m_nodeId);
        }
        if (deviceName.empty())
        {
            deviceName = std::to_string(m_deviceRecords[i].m_ifIndex);
        }
        auto numLinks = m_deviceRecords[i].m_linkStateDurations.size();
        if (numLinks == 1)
        {
            auto& statistics = m_deviceRecords[i].m_linkStateDurations.begin()->second;
            os << "\n"
               << "---- COT for " << nodeName << ":" << deviceName << " ----"
               << "\n";
            PrintLinkStates(os, statistics, unit);
        }
        else if (numLinks > 1)
        {
            os << "\nDevice \"" << nodeName << ":" << deviceName
               << "\" has statistics for multiple links: "
               << "\n";
            for (auto& linkStates : m_deviceRecords[i].m_linkStateDurations)
            {
                os << "\n"
                   << "---- COT for " << nodeName << ":" << deviceName << "#Link"
                   << std::to_string(linkStates.first) << " ---"
                   << "\n";
                PrintLinkStates(os, linkStates.second, unit);
            }
        }
        else
        {
            os << "\nDevice \"" << nodeName << ":" << deviceName << "\" has no statistics."
               << "\n";
        }
    }
    os << "\n";
}

std::ostream&
WifiCoTraceHelper::PrintLinkStates(std::ostream& os,
                                   const std::map<WifiPhyState, Time>& linkStates,
                                   Time::Unit unit) const
{
    NS_LOG_FUNCTION(this);
    os << "Showing duration by states: "
       << "\n";

    auto percents = ComputePercentage(linkStates);
    bool showPercents = !percents.empty();

    std::vector<std::string> stateColumn{};
    std::vector<std::string> durationColumn{};
    std::vector<std::string> percentColumn{};

    for (const auto& it : linkStates)
    {
        std::stringstream stateStream;
        stateStream << it.first << ": ";
        stateColumn.emplace_back(stateStream.str());

        std::stringstream durationStream;
        durationStream << std::showpoint << std::fixed << std::setprecision(2)
                       << it.second.As(unit);
        durationColumn.emplace_back(durationStream.str());

        if (showPercents)
        {
            std::stringstream percentStream;
            percentStream << std::showpoint << std::fixed << std::setprecision(2) << " ("
                          << percents.at(it.first) << "%)";
            percentColumn.emplace_back(percentStream.str());
        }
    }

    alignDecimal(durationColumn);
    if (showPercents)
    {
        alignDecimal(percentColumn);
    }
    alignWidth(stateColumn);
    alignWidth(durationColumn);

    for (size_t i = 0; i < stateColumn.size(); i++)
    {
        os << stateColumn.at(i) << durationColumn.at(i);
        if (showPercents)
        {
            os << percentColumn.at(i);
        }
        os << "\n";
    }

    return os;
}

void
WifiCoTraceHelper::alignDecimal(std::vector<std::string>& column) const
{
    size_t maxPos = 0;
    char decimal = '.';

    for (auto& s : column)
    {
        size_t pos = s.find_first_of(decimal);
        if (pos > maxPos)
        {
            maxPos = pos;
        }
    }

    for (auto& s : column)
    {
        auto padding = std::string(maxPos - s.find_first_of(decimal), ' ');
        s = padding + s;
    }
}

void
WifiCoTraceHelper::alignWidth(std::vector<std::string>& column) const
{
    size_t maxWidth = 0;

    for (auto& s : column)
    {
        size_t width = s.length();
        if (width > maxWidth)
        {
            maxWidth = width;
        }
    }

    for (auto& s : column)
    {
        auto padding = std::string(maxWidth - s.length(), ' ');
        s = s + padding;
    }
}

std::map<WifiPhyState, double>
WifiCoTraceHelper::ComputePercentage(const std::map<WifiPhyState, Time>& linkStates) const
{
    NS_LOG_FUNCTION(this);
    Time total;
    for (const auto& it : linkStates)
    {
        total += it.second;
    }

    if (total.IsZero())
    {
        std::map<WifiPhyState, double> empty;
        return empty;
    }

    std::map<WifiPhyState, double> percents;
    for (const auto& it : linkStates)
    {
        percents[it.first] = it.second.GetDouble() * 100.0 / total.GetDouble();
    }

    return percents;
}

const std::vector<WifiCoTraceHelper::DeviceRecord>&
WifiCoTraceHelper::GetDeviceRecords() const
{
    return m_deviceRecords;
}

void
WifiCoTraceHelper::NotifyWifiPhyState(std::size_t idx,
                                      std::size_t phyId,
                                      Time start,
                                      Time duration,
                                      WifiPhyState state)
{
    NS_LOG_FUNCTION(this << idx << phyId << start.As(Time::S) << duration.As(Time::US) << state);

    // Compute duration that overlaps with [m_startTime, m_stopTime]
    Time overlappingDuration;
    if (duration.IsNegative())
    {
        // Overlapping duration should be negative because duration is negative.
        overlappingDuration =
            -1 * computeOverlappingDuration(m_startTime, m_stopTime, start + duration, start);
    }
    else
    {
        overlappingDuration =
            computeOverlappingDuration(m_startTime, m_stopTime, start, start + duration);
    }

    if (!overlappingDuration.IsZero())
    {
        auto nodeId = m_deviceRecords[idx].m_nodeId;
        auto deviceId = m_deviceRecords[idx].m_ifIndex;
        auto device = NodeList::GetNode(nodeId)->GetDevice(deviceId);
        auto wifiDevice = DynamicCast<WifiNetDevice>(device);
        NS_ASSERT_MSG(wifiDevice, "Error, Device type is not WifiNetDevice.");

        std::optional<uint8_t> linkId = wifiDevice->GetMac()->GetLinkForPhy(phyId);

        if (linkId.has_value())
        {
            NS_LOG_INFO("Add device node "
                        << m_deviceRecords[idx].m_nodeId << " index "
                        << m_deviceRecords[idx].m_ifIndex << " linkId " << *linkId << " duration "
                        << overlappingDuration.As(Time::US) << " state " << state);
            m_deviceRecords[idx].AddLinkMeasurement(*linkId, start, overlappingDuration, state);
        }
        else
        {
            NS_LOG_DEBUG("LinkId not found for phyId:" << phyId);
        }
    }
}

Time
WifiCoTraceHelper::computeOverlappingDuration(Time beg1, Time end1, Time beg2, Time end2)
{
    const Time Zero{Seconds(0)};

    NS_ASSERT_MSG(beg1 >= Zero && end1 >= Zero && beg1 <= end1,
                  "Interval: [" << beg1 << "," << end1 << "] is invalid.");
    NS_ASSERT_MSG(beg2 >= Zero && end2 >= Zero && beg2 <= end2,
                  "Interval: [" << beg2 << "," << end2 << "] is invalid.");

    Time beg = Max(beg1, beg2);
    Time end = Min(end1, end2);
    Time duration = end - beg;

    return duration > Zero ? duration : Zero;
}

WifiCoTraceHelper::DeviceRecord::DeviceRecord(Ptr<WifiNetDevice> device)
    : m_nodeId(device->GetNode()->GetId()),
      m_ifIndex(device->GetIfIndex())
{
    NS_LOG_FUNCTION(this << device);
    if (!Names::FindName(device->GetNode()).empty())
    {
        m_nodeName = Names::FindName(device->GetNode());
    }
    if (!Names::FindName(device).empty())
    {
        m_deviceName = Names::FindName(device);
    }
}

void
WifiCoTraceHelper::DeviceRecord::AddLinkMeasurement(size_t linkId,
                                                    Time start,
                                                    Time duration,
                                                    WifiPhyState state)
{
    NS_LOG_FUNCTION(this << linkId << start.As(Time::S) << duration.As(Time::S) << state);
    auto& stateDurations = m_linkStateDurations[linkId];
    stateDurations[state] += duration;
}

} // namespace ns3
