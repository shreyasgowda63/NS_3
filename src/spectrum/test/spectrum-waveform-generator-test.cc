/*
 * Copyright (c) 2011 CTTC
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
 * Author: Luis Pacheco <luisbelem@gmail.com>
 */
#include <ns3/core-module.h>
#include <ns3/spectrum-module.h>
#include <ns3/test.h>

NS_LOG_COMPONENT_DEFINE("WaveformGeneratorTest");

using namespace ns3;

/**
 * \ingroup spectrum-tests
 *
 * \brief Waveform generator Test
 */
class WaveformGeneratorTestCase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * \param offInterval Length of time between end of one transmission and start of the next
     * \param txTime Length of transmission time
     * \param stop Time when generator should stop transmitting
     */
    WaveformGeneratorTestCase(Time offInterval, Time txTime, Time stop);
    ~WaveformGeneratorTestCase() override;

  private:
    void DoRun() override;

    /**
     * Trace if the waveform is active
     * \param newPkt unused.
     */
    void TraceWave(Ptr<const Packet> newPkt);
    Time m_offInterval; //!< waveform period
    Time m_txInterval;  //!< waveform duty cycle
    Time m_stop;        //!< stop time
    int m_fails;        //!< failure check
};

void
WaveformGeneratorTestCase::TraceWave(Ptr<const Packet> newPkt)
{
    if (Now() > m_stop)
    {
        m_fails++;
    }
}

WaveformGeneratorTestCase::WaveformGeneratorTestCase(Time offInterval, Time txInterval, Time stop)
    : TestCase("Check stop method"),
      m_offInterval(offInterval),
      m_txInterval(txInterval),
      m_stop(stop),
      m_fails(0)
{
}

WaveformGeneratorTestCase::~WaveformGeneratorTestCase()
{
}

void
WaveformGeneratorTestCase::DoRun()
{
    Ptr<SpectrumValue> txPsd = MicrowaveOvenSpectrumValueHelper::CreatePowerSpectralDensityMwo1();

    SpectrumChannelHelper channelHelper = SpectrumChannelHelper::Default();
    channelHelper.SetChannel("ns3::SingleModelSpectrumChannel");
    Ptr<SpectrumChannel> channel = channelHelper.Create();

    Ptr<Node> n = CreateObject<Node>();

    AdvancedWaveformGeneratorHelper waveformGeneratorHelper;
    waveformGeneratorHelper.SetChannel(channel);
    waveformGeneratorHelper.SetInterval(m_offInterval);
    waveformGeneratorHelper.AddTxPowerSpectralDensity(m_txInterval, txPsd);
    NetDeviceContainer waveformGeneratorDevices = waveformGeneratorHelper.Install(n);

    Ptr<WaveformGenerator> wave = waveformGeneratorDevices.Get(0)
                                      ->GetObject<NonCommunicatingNetDevice>()
                                      ->GetPhy()
                                      ->GetObject<WaveformGenerator>();

    wave->TraceConnectWithoutContext("TxStart",
                                     MakeCallback(&WaveformGeneratorTestCase::TraceWave, this));

    Simulator::Schedule(Seconds(1.0), &WaveformGenerator::Start, wave);
    Simulator::Schedule(m_stop, &WaveformGenerator::Stop, wave);

    Simulator::Stop(Seconds(5.0));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(m_fails, 0, "Wave started after the stop method was called");

    Simulator::Destroy();
}

/**
 * \ingroup spectrum-tests
 *
 * \brief Waveform generator TestSuite
 */
class WaveformGeneratorTestSuite : public TestSuite
{
  public:
    WaveformGeneratorTestSuite();
};

WaveformGeneratorTestSuite::WaveformGeneratorTestSuite()
    : TestSuite("waveform-generator", SYSTEM)
{
    NS_LOG_INFO("creating WaveformGeneratorTestSuite");

    Time offInterval = Seconds(1.0);
    Time txTime = Seconds(0.5);

    // Stop while wave is active
    AddTestCase(new WaveformGeneratorTestCase(offInterval, txTime, Seconds(1.2)), TestCase::QUICK);
    // Stop after wave
    AddTestCase(new WaveformGeneratorTestCase(offInterval, txTime, Seconds(1.7)), TestCase::QUICK);
}

/// Static variable for test initialization
static WaveformGeneratorTestSuite g_waveformGeneratorTestSuite;
