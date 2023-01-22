/*
 * Copyright (c) 2010 CTTC
 * Copyright (c) 2020 Lawrence Livermore National Laboratory
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 * Modified By: Mathew Bielejeski <bielejeski1@llnl.gov> (Refactoring)
 */
#include "waveform-generator-helper.h"

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

NS_LOG_COMPONENT_DEFINE("WaveformGeneratorHelper");

WaveformGeneratorHelper::WaveformGeneratorHelper()
    : m_periodSet(false),
      m_period(),
      m_dutyCycleSet(false),
      m_dutyCycle(0)
{
    m_phy.SetTypeId("ns3::WaveformGenerator");
    m_device.SetTypeId("ns3::NonCommunicatingNetDevice");
    m_antenna.SetTypeId("ns3::IsotropicAntennaModel");
}

WaveformGeneratorHelper::~WaveformGeneratorHelper()
{
}

void
WaveformGeneratorHelper::SetChannel(Ptr<SpectrumChannel> channel)
{
    m_channel = channel;
}

void
WaveformGeneratorHelper::SetChannel(std::string channelName)
{
    Ptr<SpectrumChannel> channel = Names::Find<SpectrumChannel>(channelName);
    m_channel = channel;
}

void
WaveformGeneratorHelper::SetPeriod(Time duration)
{
    NS_LOG_FUNCTION(this << duration);

    NS_ASSERT_MSG(duration.IsStrictlyPositive(), "Waveform period must be > 0");

    m_periodSet = true;
    m_period = duration;
}

void
WaveformGeneratorHelper::SetDutyCycle(double percentage)
{
    NS_LOG_FUNCTION(this << percentage);

    NS_ASSERT_MSG(percentage > 0, "Duty cycle must be greater than 0");
    NS_ASSERT_MSG(percentage <= 1.0, "Duty cycle must be less than or equal to 1");

    m_dutyCycleSet = true;
    m_dutyCycle = percentage;
}

void
WaveformGeneratorHelper::SetTxPowerSpectralDensity(Ptr<SpectrumValue> psd)
{
    m_txPsd = psd;

    NS_LOG_INFO("SpectrumValue: " << *m_txPsd);
}

void
WaveformGeneratorHelper::SetPhyAttribute(std::string name, const AttributeValue& v)
{
    if (name == "Period")
    {
        Ptr<const AttributeChecker> checker = MakeTimeChecker();

        Ptr<AttributeValue> attrVal = checker->CreateValidValue(v);
        TimeValue* tval = dynamic_cast<TimeValue*>(PeekPointer(attrVal));

        NS_ASSERT_MSG(tval,
                      "AttributeValue for attribute " << name << " is not a TimeValue instance");

        SetPeriod(tval->Get());
    }
    else if (name == "DutyCycle")
    {
        Ptr<const AttributeChecker> checker = MakeDoubleChecker<double>();

        Ptr<AttributeValue> attrVal = checker->CreateValidValue(v);
        DoubleValue* dval = dynamic_cast<DoubleValue*>(PeekPointer(attrVal));

        NS_ASSERT_MSG(dval,
                      "AttributeValue for attribute " << name << " is not a DoubleValue instance");

        SetDutyCycle(dval->Get());
    }
    else
    {
        m_phy.Set(name, v);
    }
}

NetDeviceContainer
WaveformGeneratorHelper::Install(NodeContainer c) const
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
WaveformGeneratorHelper::Install(Ptr<Node> node) const
{
    NS_ASSERT(node);

    Ptr<NonCommunicatingNetDevice> dev = m_device.Create()->GetObject<NonCommunicatingNetDevice>();

    NS_ASSERT(dev);

    Ptr<WaveformGenerator> phy = m_phy.Create()->GetObject<WaveformGenerator>();
    NS_ASSERT(phy);

    NS_ASSERT_MSG(m_periodSet, "Waveform period is not set");
    NS_ASSERT_MSG(m_dutyCycle, "Waveform duty cycle is not set");

    // The duty cycle is the percentage of the period where the waveform is "on"
    // This calculation matches the one used by the original waveform generator
    Time onTime(m_period.GetTimeStep() * m_dutyCycle);

    NS_LOG_INFO("Calculated waveform duration: " << onTime);

    phy->AddTimeSlot(onTime, m_txPsd);

    Time offTime = m_period - onTime;
    phy->SetFixedInterval(offTime);

    NS_LOG_INFO("Calculated waveform interval: " << offTime);

    dev->SetPhy(phy);

    phy->SetMobility(node->GetObject<MobilityModel>());

    phy->SetDevice(dev);

    NS_ASSERT_MSG(m_channel,
                  "missing call to "
                  "WaveformGeneratorHelper::SetChannel ()");

    phy->SetChannel(m_channel);
    dev->SetChannel(m_channel);

    Ptr<AntennaModel> antenna = m_antenna.Create()->GetObject<AntennaModel>();
    NS_ASSERT_MSG(antenna, "error in creating the AntennaModel object");
    phy->SetAntenna(antenna);

    node->AddDevice(dev);

    return dev;
}

Ptr<NetDevice>
WaveformGeneratorHelper::Install(std::string nodeName) const
{
    Ptr<Node> node = Names::Find<Node>(nodeName);
    return Install(node);
}

} // namespace ns3
