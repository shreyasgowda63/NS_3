/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Mathew Bielejeski <bielejeski1@gmail.com>
 */

#include "ns3/core-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/test.h"

#include <sstream>
#include <string>
#include <vector>

/**
 * This test verifies that the WaveformConfigLoader is able to parse
 * configuration files and generate correct Waveforms from those
 * configurations.
 */
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WaveformConfigLoaderTest");

/**
 * Class which encapsulates the logic for generating one or
 * more complex waveform generator objects from a configuration file
 * and validating that the generators were created using the
 * correct data.
 */
class WaveformConfigLoaderTest : public TestCase
{
  private:
    /**
     * Holds data equivalent to BandInfo
     */
    struct BandTest
    {
        double lowFrequency;    //!< lower bound of a frequency band
        double centerFrequency; //!< center frequency of a band
        double highFrequency;   //!< upper bound of a frequency band
    };

    /**
     * Holds expected parameters for a time slot.
     */
    struct TimeSlotTest
    {
        Time duration;    //!< expected duration of a time slot
        size_t numValues; //!< expected number of values in the time slot
    };

    /**
     * Holds all of the expected data for a particular test instance.
     */
    struct GeneratorTest
    {
        size_t numBands; //!< expected number of bands
        size_t numSlots; //!< expected number of time slots

        /**
         * Container of band information that will be compared against
         * the bands stored in a complex waveform generator.
         */
        std::vector<BandTest> bandTests;

        /**
         * Container of time slot information that will be compared against
         * the time slots stored in a complex waveform generator.
         */
        std::vector<TimeSlotTest> slotTests;
    };

  public:
    /**
     * Constructor
     *
     * \param name name of the unit test
     */
    WaveformConfigLoaderTest(const std::string& name)
        : TestCase(name),
          m_input()
    {
    }

    /**
     * Sets the complex waveform config data that will be used
     * to generate the complex waveform generators during the test.
     *
     * \param input string containing waveform configuration data that
     * can be loaded using the WaveformConfigLoader.
     */
    void SetInput(std::string input)
    {
        m_input = std::move(input);
    }

    /**
     * Adds a new waveform generator test to the list of tests.
     * Use AddBandTest and AddTimeSlotTest to add band and time slot
     * test data to the generator test created by this function.
     *
     * \param numBands Number of bands the generator is expected to have.
     * \param numSlots Number of time slots the generator is expected to have.
     */
    void CreateGeneratorTest(size_t numBands, size_t numSlots)
    {
        GeneratorTest test;
        test.numBands = numBands;
        test.numSlots = numSlots;

        m_dataPoints.push_back(test);
    }

    /**
     * Adds band test data to the last generator test created by calling
     * CreateGeneratorTest.
     *
     * \param lowFreq Lower bound of the frequency band
     * \param centerFreq Center frequency of the band
     * \param highFreq Upper bound of the frequency band
     */
    void AddBandTest(double lowFreq, double centerFreq, double highFreq)
    {
        BandTest test;
        test.lowFrequency = lowFreq;
        test.centerFrequency = centerFreq;
        test.highFrequency = highFreq;

        GeneratorTest& genTest = m_dataPoints.back();
        genTest.bandTests.push_back(test);
    }

    /**
     * Adds time slot test data to the last generator test created by calling
     * CreateGeneratorTest.
     *
     * \param duration Expected duration of the time slot
     * \param numValues Expected number of values in the time slot
     */
    void AddTimeSlotTest(Time duration, size_t numValues)
    {
        TimeSlotTest test;
        test.duration = duration;
        test.numValues = numValues;

        GeneratorTest& genTest = m_dataPoints.back();
        genTest.slotTests.push_back(test);
    }

  private:
    /**
     * Generates one or more WaveformGenerator objects using the
     * data supplied to SetInput.
     * Compares the configuration of the generator objects to the data
     * stored in the generator tests.
     */
    void DoRun() override
    {
        NS_TEST_ASSERT_MSG_EQ(m_input.empty(), false, "No input supplied");

        std::stringstream stream(m_input);
        WaveformConfigLoader loader;

        SpectrumChannelHelper channelHelper = SpectrumChannelHelper::Default();
        channelHelper.SetChannel("ns3::SingleModelSpectrumChannel");
        Ptr<SpectrumChannel> channel = channelHelper.Create();

        NodeContainer nodes;
        nodes.Create(m_dataPoints.size());

        NetDeviceContainer devices = loader.Load(stream, channel, nodes);

        NS_TEST_ASSERT_MSG_EQ(devices.GetN(),
                              nodes.GetN(),
                              "Number of devices created does not match number of nodes");

        auto testIter = m_dataPoints.begin();

        for (uint32_t nodeIndex = 0; nodeIndex < nodes.GetN(); ++nodeIndex, ++testIter)
        {
            Ptr<Node> node = nodes.Get(nodeIndex);

            Ptr<WaveformGenerator> generator = node->GetDevice(0)
                                                   ->GetObject<NonCommunicatingNetDevice>()
                                                   ->GetPhy()
                                                   ->GetObject<WaveformGenerator>();

            NS_TEST_ASSERT_MSG_NE(generator,
                                  nullptr,
                                  "Node " << nodeIndex
                                          << " does not have a net device with a "
                                             "complex waveform generator");

            size_t numSlots = generator->TimeSlotCount();
            NS_TEST_EXPECT_MSG_EQ(numSlots,
                                  testIter->numSlots,
                                  "Time slot mismatch for node " << nodeIndex);

            for (size_t i = 0; i < numSlots; ++i)
            {
                Ptr<const SpectrumModel> model = generator->GetTimeSlotSpectrumModel(i);

                NS_TEST_ASSERT_MSG_NE(model,
                                      nullptr,
                                      "Generator does not have a SpectrumModel at time slot " << i);

                NS_TEST_EXPECT_MSG_EQ(model->GetNumBands(),
                                      testIter->numBands,
                                      "Number of bands does not match at time slot "
                                          << i << " for node " << nodeIndex);

                if (!testIter->bandTests.empty())
                {
                    for (size_t bandIndex = 0; bandIndex < testIter->numBands; ++bandIndex)
                    {
                        const BandInfo& band = *(model->Begin() + bandIndex);
                        BandTest& bandTest = testIter->bandTests[bandIndex];

                        NS_TEST_EXPECT_MSG_EQ(band.fl,
                                              bandTest.lowFrequency,
                                              "Low frequency of band "
                                                  << bandIndex << " does not match for node "
                                                  << nodeIndex);

                        NS_TEST_EXPECT_MSG_EQ(band.fc,
                                              bandTest.centerFrequency,
                                              "Center frequency of band "
                                                  << bandIndex << " does not match for node "
                                                  << nodeIndex);

                        NS_TEST_EXPECT_MSG_EQ(band.fh,
                                              bandTest.highFrequency,
                                              "High frequency of band "
                                                  << bandIndex << " does not match for node "
                                                  << nodeIndex);
                    }
                }

                if (!testIter->slotTests.empty())
                {
                    TimeSlotTest& slotTest = testIter->slotTests[i];

                    NS_TEST_EXPECT_MSG_EQ(generator->GetTimeSlotDuration(i),
                                          slotTest.duration,
                                          "Time slot duration mismatch for time slot "
                                              << i << " on node " << nodeIndex);

                    Ptr<const SpectrumValue> spectrumValue = generator->GetTimeSlotSpectrumValue(i);

                    NS_TEST_ASSERT_MSG_NE(spectrumValue,
                                          nullptr,
                                          "Generator does not have a SpectrumValue "
                                          "at time slot "
                                              << i);

                    size_t valueCount =
                        spectrumValue->ConstValuesEnd() - spectrumValue->ConstValuesBegin();

                    NS_TEST_EXPECT_MSG_EQ(valueCount,
                                          slotTest.numValues,
                                          "Value count mismatch for time slot " << i << " on node "
                                                                                << nodeIndex);
                }
            }
        }
    }

  private:
    std::string m_input;                     //!< String containing waveform config data
    std::vector<GeneratorTest> m_dataPoints; //!< Container of expected outputs
};

/**
 * This class is responsible for creating all of the unit tests used by
 * this test suite.
 */
class WaveformConfigLoaderTestSuite : public TestSuite
{
  public:
    /**
     * Constructor
     */
    WaveformConfigLoaderTestSuite();

  private:
    /**
     * Generates all of the unit tests for the test suite.
     */
    void Run();
};

WaveformConfigLoaderTestSuite::WaveformConfigLoaderTestSuite()
    : TestSuite("waveform-config-loader", UNIT)
{
    Run();
}

void
WaveformConfigLoaderTestSuite::Run()
{
    std::string input;
    WaveformConfigLoaderTest* t = nullptr;

    input = "#Empty file with just a comment";

    t = new WaveformConfigLoaderTest("empty file");
    t->SetInput(input);
    AddTestCase(t, TestCase::QUICK);

    input = R"___(
        interval constant 1000
        band 2.412e9 22
        txslot 20 -70
        dbm 2.412e9 -30
        end waveform
        )___";

    t = new WaveformConfigLoaderTest("missing begin waveform");
    t->SetInput(input);
    AddTestCase(t, TestCase::QUICK);

    input = R"___(
        begin waveform
        interval constant 1000
        band 2.412e9 22
        txslot 20 -70
        dbm 2.412e9 -30
        end waveform
        )___";

    t = new WaveformConfigLoaderTest("missing node");
    t->SetInput(input);
    AddTestCase(t, TestCase::QUICK);

    input = R"___(
        begin waveform
        node 1
        band 2.412e9 22
        txslot 20 -70
        dbm 2.412e9 -30
        end waveform
        )___";

    t = new WaveformConfigLoaderTest("missing interval");
    t->SetInput(input);
    AddTestCase(t, TestCase::QUICK);

    input = R"___(
        begin waveform
        node 1
        interval constant 1000
        txslot 20 -70
        dbm 2.412e9 -30
        end waveform
        )___";

    t = new WaveformConfigLoaderTest("missing band");
    t->SetInput(input);
    AddTestCase(t, TestCase::QUICK);

    input = R"___(
        begin waveform
        node 1
        interval constant 1000
        band 2.412e9 22
        dbm 2.412e9 -30
        end waveform
        )___";

    t = new WaveformConfigLoaderTest("missing txslot");
    t->SetInput(input);
    AddTestCase(t, TestCase::QUICK);

    input = R"___(
        begin waveform
        node 1
        interval constant 1000
        band 2.412e9 22
        txslot 20 -70
        dbm 2.412e9 -30
        )___";

    t = new WaveformConfigLoaderTest("missing end waveform");
    t->SetInput(input);
    AddTestCase(t, TestCase::QUICK);

    input = R"___(
        begin waveform
        node 1
        interval uniform 200 500
        band 2.412e9 22
        txslot 20 -70
        dbm 2.412e9 -30
        end waveform
        )___";

    t = new WaveformConfigLoaderTest("invalid interval args");
    t->SetInput(input);
    AddTestCase(t, TestCase::QUICK);

    input = R"___(
        begin waveform
        node 1
        interval constant 1000
        band 2.412e9 22
        dbm 2.412e9 -30
        txslot 20 -70
        end waveform
        )___";

    t = new WaveformConfigLoaderTest("dbm before txslot");
    t->SetInput(input);
    AddTestCase(t, TestCase::QUICK);

    input = R"___(
        begin waveform
        node 0
        interval constant 1000
        band 2.412e9 2.2e7
        txslot 20 -70
        dbm 2.412e9 -30
        end waveform
        )___";

    t = new WaveformConfigLoaderTest("basic waveform");
    t->SetInput(input);
    t->CreateGeneratorTest(1, 1);
    t->AddBandTest(2.401e9, 2.412e9, 2.423e9);
    t->AddTimeSlotTest(MilliSeconds(20), 1);
    AddTestCase(t, TestCase::QUICK);

    input = R"___(
        begin waveform
        node 0
        interval random 50 100
        band 2.412e9 2.2e7
        txslot 20 -70
        dbm 2.412e9 -30
        end waveform
        )___";

    t = new WaveformConfigLoaderTest("random interval");
    t->SetInput(input);
    t->CreateGeneratorTest(1, 1);
    t->AddBandTest(2.401e9, 2.412e9, 2.423e9);
    t->AddTimeSlotTest(MilliSeconds(20), 1);
    AddTestCase(t, TestCase::QUICK);

    input = R"___(
        begin waveform
        node 0
        interval custom "ns3::SequentialRandomVariable[Min=5|Max=1000]"
        band 2.412e9 2.2e7
        txslot 20 -70
        dbm 2.412e9 -30
        end waveform
        )___";

    t = new WaveformConfigLoaderTest("custom interval");
    t->SetInput(input);
    t->CreateGeneratorTest(1, 1);
    t->AddBandTest(2.401e9, 2.412e9, 2.423e9);
    t->AddTimeSlotTest(MilliSeconds(20), 1);
    AddTestCase(t, TestCase::QUICK);

    input = R"___(
        begin waveform
        node 0
        interval constant 100
        band 2.412e9 2.2e7
        band 2.437e9 2.2e7
        band 2.462e9 2.2e7
        txslot 20 -70
        end waveform
        )___";

    t = new WaveformConfigLoaderTest("multiple bands");
    t->SetInput(input);
    t->CreateGeneratorTest(3, 1);
    t->AddBandTest(2.401e9, 2.412e9, 2.423e9);
    t->AddBandTest(2.426e9, 2.437e9, 2.448e9);
    t->AddBandTest(2.451e9, 2.462e9, 2.473e9);
    t->AddTimeSlotTest(MilliSeconds(20), 3);
    AddTestCase(t, TestCase::QUICK);

    input = R"___(
        begin waveform
        node 0
        interval constant 100
        band 2.412e9 2.2e7
        txslot 20 -70
        dbm 2.412e9 -30
        txslot 20 -70
        dbm 2.412e9 -40
        txslot 50 -70
        dbm 2.412e9 -30
        end waveform
        )___";

    t = new WaveformConfigLoaderTest("multiple time slots");
    t->SetInput(input);
    t->CreateGeneratorTest(1, 3);
    t->AddBandTest(2.401e9, 2.412e9, 2.423e9);
    t->AddTimeSlotTest(MilliSeconds(20), 1);
    t->AddTimeSlotTest(MilliSeconds(20), 1);
    t->AddTimeSlotTest(MilliSeconds(50), 1);
    AddTestCase(t, TestCase::QUICK);

    input = R"___(
        begin waveform
        node 0
        interval constant 100
        band 2.412e9 2.2e7
        band 2.437e9 2.2e7
        band 2.462e9 2.2e7
        txslot 20 -70
        dbm 2.412e9 -30
        txslot 10 -70
        txslot 30 -70
        dbm 2.412e9 -30
        dbm 2.437e9 -40
        dbm 2.462e9 -35
        txslot 10 -70
        end waveform
        )___";

    t = new WaveformConfigLoaderTest("multiple bands multiple time slots");
    t->SetInput(input);
    t->CreateGeneratorTest(3, 4);
    t->AddBandTest(2.401e9, 2.412e9, 2.423e9);
    t->AddBandTest(2.426e9, 2.437e9, 2.448e9);
    t->AddBandTest(2.451e9, 2.462e9, 2.473e9);
    t->AddTimeSlotTest(MilliSeconds(20), 3);
    t->AddTimeSlotTest(MilliSeconds(10), 3);
    t->AddTimeSlotTest(MilliSeconds(30), 3);
    t->AddTimeSlotTest(MilliSeconds(10), 3);
    AddTestCase(t, TestCase::QUICK);

    input = R"___(
        begin waveform
        node 0
        interval constant 100
        band 2.412e9 2.2e7
        txslot 20 -70
        dbm 2.412e9 -30
        end waveform

        begin waveform
        node 1
        interval constant 100
        band 2.437e9 2.2e7
        txslot 35 -70
        dbm 2.437e9 -30
        end waveform

        begin waveform
        node 2
        interval constant 100
        band 2.462e9 2.2e7
        txslot 50 -70
        dbm 2.462e9 -30
        end waveform
        )___";

    t = new WaveformConfigLoaderTest("multiple waveforms");
    t->SetInput(input);
    t->CreateGeneratorTest(1, 1);
    t->AddBandTest(2.401e9, 2.412e9, 2.423e9);
    t->AddTimeSlotTest(MilliSeconds(20), 1);
    t->CreateGeneratorTest(1, 1);
    t->AddBandTest(2.426e9, 2.437e9, 2.448e9);
    t->AddTimeSlotTest(MilliSeconds(35), 1);
    t->CreateGeneratorTest(1, 1);
    t->AddBandTest(2.451e9, 2.462e9, 2.473e9);
    t->AddTimeSlotTest(MilliSeconds(50), 1);
    AddTestCase(t, TestCase::QUICK);
}

static WaveformConfigLoaderTestSuite g_waveformConfigLoaderTestSuite;
