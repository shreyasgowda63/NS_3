/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Lawrence Livermore National Laboratory
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
 * Author: Mathew Bielejeski <bielejeski1@llnl.gov>
 */

#include "advanced-waveform-generator-helper.h"

#include "ns3/antenna-model.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/names.h"
#include "ns3/non-communicating-net-device.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-channel.h"
#include "ns3/spectrum-model.h"
#include "ns3/spectrum-propagation-loss-model.h"
#include "ns3/waveform-generator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("AdvancedWaveformGeneratorHelper");

AdvancedWaveformGeneratorHelper::AdvancedWaveformGeneratorHelper()
    : m_interval(Seconds(1))
{
    m_phy.SetTypeId("ns3::WaveformGenerator");
    m_device.SetTypeId("ns3::NonCommunicatingNetDevice");
    m_antenna.SetTypeId("ns3::IsotropicAntennaModel");
}

AdvancedWaveformGeneratorHelper::~AdvancedWaveformGeneratorHelper()
{
}

void
AdvancedWaveformGeneratorHelper::SetChannel(Ptr<SpectrumChannel> channel)
{
    m_channel = channel;
}

void
AdvancedWaveformGeneratorHelper::SetChannel(std::string channelName)
{
    Ptr<SpectrumChannel> channel = Names::Find<SpectrumChannel>(channelName);
    m_channel = channel;
}

void
AdvancedWaveformGeneratorHelper::SetBands(const Bands& bands)
{
    m_model = Create<SpectrumModel>(bands);
}

void
AdvancedWaveformGeneratorHelper::SetBands(Bands::const_iterator begin, Bands::const_iterator end)
{
    Bands bands(begin, end);
    SetBands(bands);
}

void
AdvancedWaveformGeneratorHelper::SetModel(Ptr<const SpectrumModel> model)
{
    m_model = model;
}

void
AdvancedWaveformGeneratorHelper::AddTxPowerSpectralDensity(Time duration,
                                                           const std::vector<double>& psd)
{
    TransmitSlice slice;
    slice.duration = duration;
    slice.psd = psd;

    m_slices.emplace_back(std::move(slice));
}

void
AdvancedWaveformGeneratorHelper::AddTxPowerSpectralDensity(Time duration,
                                                           Ptr<const SpectrumValue> value)
{
    if (!m_model)
    {
        SetModel(value->GetSpectrumModel());
    }

    NS_ASSERT_MSG(m_model == value->GetSpectrumModel(),
                  "SpectrumValue has a different model than the one passed to"
                  " SetModel()");

    TransmitSlice slice;
    slice.duration = duration;
    slice.psd.assign(value->ConstValuesBegin(), value->ConstValuesEnd());

    m_slices.emplace_back(std::move(slice));
}

void
AdvancedWaveformGeneratorHelper::SetInterval(Time interval)
{
    NS_ASSERT_MSG(interval.IsPositive(), "Interval between waveforms must be >= 0");

    m_interval = interval;
}

NetDeviceContainer
AdvancedWaveformGeneratorHelper::Install(NodeContainer c) const
{
    NetDeviceContainer devices;
    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        Ptr<Node> node = *i;

        Ptr<NetDevice> device = Install(node);

        devices.Add(device);
    }

    return devices;
}

Ptr<NetDevice>
AdvancedWaveformGeneratorHelper::Install(Ptr<Node> node) const
{
    NS_ASSERT(node);

    NS_ASSERT_MSG(m_model,
                  "missing call to either "
                  "AdvancedWaveformGeneratorHelper::SetBands () "
                  "or AdvancedWaveformGeneratorHelper::SetModel ()");

    Ptr<NonCommunicatingNetDevice> dev = m_device.Create()->GetObject<NonCommunicatingNetDevice>();

    NS_ASSERT(dev);

    Ptr<WaveformGenerator> phy = m_phy.Create()->GetObject<WaveformGenerator>();
    NS_ASSERT(phy);

    dev->SetPhy(phy);

    phy->SetMobility(node->GetObject<MobilityModel>());

    phy->SetDevice(dev);

    for (const auto& slice : m_slices)
    {
        NS_ASSERT_MSG(slice.psd.size() == m_model->GetNumBands(),
                      "number of power spectral density values in transmit slice "
                      "does not equal the number of bands in the spectrum model");

        Ptr<SpectrumValue> spectrumValue = Create<SpectrumValue>(m_model);

        for (size_t i = 0; i < slice.psd.size(); ++i)
        {
            (*spectrumValue)[i] = slice.psd[i];
        }

        phy->AddTimeSlot(slice.duration, spectrumValue);
    }

    phy->SetFixedInterval(m_interval);

    NS_ASSERT_MSG(m_channel,
                  "missing call to "
                  "AdvancedWaveformGeneratorHelper::SetChannel ()");

    phy->SetChannel(m_channel);
    dev->SetChannel(m_channel);

    Ptr<AntennaModel> antenna = m_antenna.Create()->GetObject<AntennaModel>();
    NS_ASSERT_MSG(antenna, "error in creating the AntennaModel object");
    phy->SetAntenna(antenna);

    node->AddDevice(dev);

    return dev;
}

Ptr<NetDevice>
AdvancedWaveformGeneratorHelper::Install(std::string nodeName) const
{
    Ptr<Node> node = Names::Find<Node>(nodeName);
    return Install(node);
}

} // namespace ns3
