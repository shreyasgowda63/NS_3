/*
 * Copyright (c) 2009 CTTC
 * Copyright (c) 2017 Lawrence Livermore National Laboratory
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
 * Modified By: Mathew Bielejeski <bielejeski1@llnl.gov> (Multiple time slots)
 */

#include "waveform-generator.h"

#include <ns3/antenna-model.h>
#include <ns3/double.h>
#include <ns3/log.h>
#include <ns3/object-factory.h>
#include <ns3/packet-burst.h>
#include <ns3/pointer.h>
#include <ns3/random-variable-stream.h>
#include <ns3/simulator.h>
#include <ns3/string.h>

#include <cstdint>
#include <limits>
#include <sstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WaveformGenerator");

NS_OBJECT_ENSURE_REGISTERED(WaveformGenerator);

WaveformGenerator::WaveformGenerator()
    : m_mobility(nullptr),
      m_netDevice(nullptr),
      m_channel(nullptr),
      m_timeSlots(),
      m_nextSlot(0),
      m_startTime(Seconds(0))
{
    NS_LOG_FUNCTION(this);
}

WaveformGenerator::~WaveformGenerator()
{
}

void
WaveformGenerator::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_channel = nullptr;
    m_netDevice = nullptr;
    m_mobility = nullptr;
    if (m_nextEvent.IsRunning())
    {
        m_nextEvent.Cancel();
    }

    m_nextSlot = 0;
    m_timeSlots.clear();
}

TypeId
WaveformGenerator::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::WaveformGenerator")
            .SetParent<SpectrumPhy>()
            .SetGroupName("Spectrum")
            .AddConstructor<WaveformGenerator>()
            .AddAttribute(
                "Interval",
                "A RandomVariableStream used to control the amount of time (in seconds) between"
                " the end of one transmission and the start of the next transmission",
                StringValue("ns3::ConstantRandomVariable[Constant=0]"),
                MakePointerAccessor(&WaveformGenerator::SetInterval,
                                    &WaveformGenerator::GetInterval),
                MakePointerChecker<RandomVariableStream>())
            .AddTraceSource("TxStart",
                            "Trace fired when a new transmission is started",
                            MakeTraceSourceAccessor(&WaveformGenerator::m_phyTxStartTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("TxEnd",
                            "Trace fired when a previously started transmission is finished",
                            MakeTraceSourceAccessor(&WaveformGenerator::m_phyTxEndTrace),
                            "ns3::Packet::TracedCallback");
    return tid;
}

Ptr<NetDevice>
WaveformGenerator::GetDevice() const
{
    NS_LOG_FUNCTION(this);

    return m_netDevice;
}

Ptr<MobilityModel>
WaveformGenerator::GetMobility() const
{
    NS_LOG_FUNCTION(this);

    return m_mobility;
}

Ptr<const SpectrumModel>
WaveformGenerator::GetRxSpectrumModel() const
{
    NS_LOG_FUNCTION(this);

    // this device is not interested in RX
    return nullptr;
}

void
WaveformGenerator::SetDevice(Ptr<NetDevice> d)
{
    NS_LOG_FUNCTION(this << d);

    m_netDevice = d;
}

void
WaveformGenerator::SetMobility(Ptr<MobilityModel> m)
{
    NS_LOG_FUNCTION(this << m);

    m_mobility = m;
}

void
WaveformGenerator::SetChannel(Ptr<SpectrumChannel> c)
{
    NS_LOG_FUNCTION(this << c);
    m_channel = c;
}

void
WaveformGenerator::StartRx(Ptr<SpectrumSignalParameters> params)
{
    NS_LOG_FUNCTION(this << params);
}

void
WaveformGenerator::AddTimeSlot(Time duration, Ptr<SpectrumValue> txs)
{
    NS_LOG_FUNCTION(this << duration << *txs);

    m_timeSlots.push_back(std::make_pair(duration, txs));

    NS_LOG_LOGIC("Number of time slots: " << m_timeSlots.size());
}

void
WaveformGenerator::ClearTimeSlots()
{
    NS_LOG_FUNCTION(this);

    m_timeSlots.clear();
}

std::size_t
WaveformGenerator::TimeSlotCount() const
{
    NS_LOG_FUNCTION(this);

    return m_timeSlots.size();
}

Time
WaveformGenerator::GetTimeSlotDuration(std::size_t index) const
{
    NS_LOG_FUNCTION(this << index);

    Time duration;
    if (index < m_timeSlots.size())
    {
        duration = m_timeSlots[index].first;
    }

    return duration;
}

Ptr<const SpectrumModel>
WaveformGenerator::GetTimeSlotSpectrumModel(std::size_t index) const
{
    NS_LOG_FUNCTION(this << index);

    Ptr<const SpectrumModel> model;

    if (index < m_timeSlots.size())
    {
        model = m_timeSlots[index].second->GetSpectrumModel();
    }

    return model;
}

Ptr<const SpectrumValue>
WaveformGenerator::GetTimeSlotSpectrumValue(std::size_t index) const
{
    NS_LOG_FUNCTION(this << index);

    Ptr<const SpectrumValue> value;

    if (index < m_timeSlots.size())
    {
        value = m_timeSlots[index].second;
    }

    return value;
}

Ptr<Object>
WaveformGenerator::GetAntenna() const
{
    NS_LOG_FUNCTION(this);

    return m_antenna;
}

void
WaveformGenerator::SetAntenna(Ptr<AntennaModel> a)
{
    NS_LOG_FUNCTION(this << a);

    m_antenna = a;
}

void
WaveformGenerator::GenerateWaveform()
{
    NS_LOG_FUNCTION(this);

    NS_LOG_LOGIC("Starting transmission of complex waveform");

    // start transmission from the beginning
    m_nextSlot = 0;

    // trigger the start callback
    m_phyTxStartTrace(nullptr);

    TransmitSlot(m_nextSlot);
}

void
WaveformGenerator::TransmitSlot(std::size_t slotIndex)
{
    NS_LOG_FUNCTION(this << slotIndex);

    if (slotIndex < m_timeSlots.size())
    {
        const auto& slot = m_timeSlots[slotIndex];

        Time duration = slot.first;
        Ptr<SpectrumValue> psd = slot.second;

        Ptr<SpectrumSignalParameters> txParams = Create<SpectrumSignalParameters>();
        txParams->duration = duration;
        txParams->psd = psd;
        txParams->txPhy = GetObject<SpectrumPhy>();
        txParams->txAntenna = m_antenna;

        NS_LOG_LOGIC("generating waveform : " << *psd);

        m_channel->StartTx(txParams);

        NS_LOG_LOGIC("scheduling next step of waveform");
        m_nextEvent =
            Simulator::Schedule(duration, &WaveformGenerator::TransmitSlot, this, ++slotIndex);
    }
    else
    {
        // no more slots, schedule the start of the next waveform
        NS_LOG_LOGIC("Finished waveform");

        // trigger the end callback
        m_phyTxEndTrace(nullptr);

        Time sleepTime = Seconds(m_interval->GetValue());
        m_nextEvent = Simulator::Schedule(sleepTime, &WaveformGenerator::GenerateWaveform, this);

        NS_LOG_LOGIC("Scheduled start of next complex waveform in " << sleepTime);
    }
}

void
WaveformGenerator::SetFixedInterval(Time duration)
{
    NS_LOG_FUNCTION(this << duration);

    double seconds = duration.GetSeconds();

    Ptr<RandomVariableStream> rand =
        CreateObjectWithAttributes<ConstantRandomVariable>("Constant", DoubleValue(seconds));

    SetInterval(rand);
}

void
WaveformGenerator::SetInterval(Ptr<RandomVariableStream> rand)
{
    NS_LOG_FUNCTION(this << rand);

    NS_ASSERT_MSG(rand, "WaveformGenerator interval is a null pointer");

    m_interval = rand;
}

Ptr<RandomVariableStream>
WaveformGenerator::GetInterval() const
{
    return m_interval;
}

void
WaveformGenerator::Start()
{
    NS_LOG_FUNCTION(this);

    if (!m_nextEvent.IsRunning())
    {
        NS_LOG_LOGIC("generator was not active, now starting");
        m_startTime = Now();
        m_nextEvent = Simulator::ScheduleNow(&WaveformGenerator::GenerateWaveform, this);
    }
}

void
WaveformGenerator::Stop()
{
    NS_LOG_FUNCTION(this);

    if (m_nextEvent.IsRunning())
    {
        m_nextEvent.Cancel();
        m_nextSlot = m_timeSlots.size();
    }
}

} // namespace ns3
