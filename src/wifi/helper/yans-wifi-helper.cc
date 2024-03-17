/*
 * Copyright (c) 2008 INRIA
 *
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "yans-wifi-helper.h"

#include "ns3/error-rate-model.h"
#include "ns3/frame-capture-model.h"
#include "ns3/interference-helper.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/preamble-detection-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/wifi-net-device.h"
#include "ns3/yans-wifi-phy.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("YansWifiHelper");

YansWifiChannelHelper::YansWifiChannelHelper()
{
}

YansWifiChannelHelper
YansWifiChannelHelper::Default()
{
    YansWifiChannelHelper helper;
    helper.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    helper.AddPropagationLoss("ns3::LogDistancePropagationLossModel");
    return helper;
}

Ptr<wifi::YansWifiChannel>
YansWifiChannelHelper::Create() const
{
    auto channel = CreateObject<wifi::YansWifiChannel>();
    Ptr<PropagationLossModel> prev = nullptr;
    for (auto i = m_propagationLoss.begin(); i != m_propagationLoss.end(); ++i)
    {
        Ptr<PropagationLossModel> cur = (*i).Create<PropagationLossModel>();
        if (prev)
        {
            prev->SetNext(cur);
        }
        if (m_propagationLoss.begin() == i)
        {
            channel->SetPropagationLossModel(cur);
        }
        prev = cur;
    }
    Ptr<PropagationDelayModel> delay = m_propagationDelay.Create<PropagationDelayModel>();
    channel->SetPropagationDelayModel(delay);
    return channel;
}

int64_t
YansWifiChannelHelper::AssignStreams(Ptr<wifi::YansWifiChannel> c, int64_t stream)
{
    return c->AssignStreams(stream);
}

YansWifiPhyHelper::YansWifiPhyHelper()
    : WifiPhyHelper(1), // YANS phy is not used for 11be devices
      m_channel(nullptr)
{
    m_phys.front().SetTypeId("ns3::YansWifiPhy");
    SetInterferenceHelper("ns3::InterferenceHelper");
    SetErrorRateModel("ns3::TableBasedErrorRateModel");
}

void
YansWifiPhyHelper::SetChannel(Ptr<wifi::YansWifiChannel> channel)
{
    m_channel = channel;
}

void
YansWifiPhyHelper::SetChannel(std::string channelName)
{
    auto channel = Names::Find<wifi::YansWifiChannel>(channelName);
    m_channel = channel;
}

std::vector<Ptr<wifi::WifiPhy>>
YansWifiPhyHelper::Create(Ptr<Node> node, Ptr<wifi::WifiNetDevice> device) const
{
    auto phy = m_phys.front().Create<wifi::YansWifiPhy>();
    auto interference = m_interferenceHelper.Create<wifi::InterferenceHelper>();
    phy->SetInterferenceHelper(interference);
    auto error = m_errorRateModel.front().Create<wifi::ErrorRateModel>();
    phy->SetErrorRateModel(error);
    if (m_frameCaptureModel.front().IsTypeIdSet())
    {
        auto frameCapture = m_frameCaptureModel.front().Create<wifi::FrameCaptureModel>();
        phy->SetFrameCaptureModel(frameCapture);
    }
    if (m_preambleDetectionModel.front().IsTypeIdSet())
    {
        auto preambleDetection =
            m_preambleDetectionModel.front().Create<wifi::PreambleDetectionModel>();
        phy->SetPreambleDetectionModel(preambleDetection);
    }
    phy->SetChannel(m_channel);
    phy->SetDevice(device);
    return std::vector<Ptr<wifi::WifiPhy>>({phy});
}

} // namespace ns3
