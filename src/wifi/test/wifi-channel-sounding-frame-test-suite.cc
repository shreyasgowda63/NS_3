/*
 * Copyright (c) 2023 Georgia Institute of Technology
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
 * Author: Jingyuan Zhang <jingyuan_z@gatech.edu>
 */

#include "ns3/ctrl-headers.h"
#include "ns3/header-serialization-test.h"
#include "ns3/log.h"
#include "ns3/mgt-action-headers.h"
#include "ns3/random-variable-stream.h"
#include "ns3/wifi-mac-header.h"

#include <cmath>
#include <sstream>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiChannelSoundingFrameTest");

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test channel sounding frame serialization and deserialization
 */
class NdpaTest : public HeaderSerializationTestCase
{
  public:
    /**
     * Constructor
     */
    NdpaTest();
    ~NdpaTest() override;

  private:
    void DoRun() override;
};

NdpaTest::NdpaTest()
    : HeaderSerializationTestCase("Check serialization and deserialization of NDPA packet")
{
}

NdpaTest::~NdpaTest()
{
}

void
NdpaTest::DoRun()
{
    CtrlNdpaHeader ndpa;
    ndpa.SetSoundingDialogToken(0x7f);

    // Adding one STA Info
    CtrlNdpaHeader::StaInfo staInfo;
    staInfo.m_aid11 = 0x07ff;
    staInfo.m_ruStart = 1;
    staInfo.m_ruEnd = 5;
    staInfo.m_feedbackTypeNg = 1;
    staInfo.m_disambiguation = 1;
    staInfo.m_codebookSize = 1;
    staInfo.m_nc = 1;
    ndpa.AddStaInfoField(staInfo);
    TestHeaderSerialization(ndpa);

    // Adding another STA Info
    staInfo.m_aid11 = 0x0257;
    staInfo.m_ruStart = 0;
    staInfo.m_ruEnd = 32;
    staInfo.m_feedbackTypeNg = 0;
    staInfo.m_disambiguation = 0;
    staInfo.m_codebookSize = 0;
    staInfo.m_nc = 3;
    ndpa.AddStaInfoField(staInfo);
    TestHeaderSerialization(ndpa);
}

class BfrpTriggerTest : public HeaderSerializationTestCase
{
  public:
    /**
     * Constructor
     */
    BfrpTriggerTest();
    ~BfrpTriggerTest() override;

  private:
    void DoRun() override;
};

BfrpTriggerTest::BfrpTriggerTest()
    : HeaderSerializationTestCase("Check serialization and deserialization of BFRP Trigger packet")
{
}

BfrpTriggerTest::~BfrpTriggerTest()
{
}

void
BfrpTriggerTest::DoRun()
{
    CtrlTriggerHeader bfrpTrigger;
    bfrpTrigger.SetType(TriggerFrameType::BFRP_TRIGGER);

    // Adding one user Info
    CtrlTriggerUserInfoField userInfo(TriggerFrameType::BFRP_TRIGGER, TriggerFrameVariant::HE);
    userInfo.SetBfrpTriggerDepUserInfo(1);
    bfrpTrigger.AddUserInfoField(userInfo);
    TestHeaderSerialization(bfrpTrigger);

    // Adding another User Info
    userInfo.SetBfrpTriggerDepUserInfo(0xff);
    bfrpTrigger.AddUserInfoField(userInfo);
    TestHeaderSerialization(bfrpTrigger);
}

class BfReportTest : public HeaderSerializationTestCase
{
  public:
    /**
     * Constructor
     */
    BfReportTest();
    ~BfReportTest() override;

    /**
     * Generate random channel information given HE MIMO Control field
     * \param heMimoControlHeader HE MIMO Control field
     * \return Channel information
     */
    HeCompressedBfReport::ChannelInfo GetBfCompressedReportInfo(
        HeMimoControlHeader heMimoControlHeader);

    std::vector<std::vector<uint8_t>> GetMuExclusiveReportInfo(
        HeMimoControlHeader heMimoControlHeader);

  private:
    void DoRun() override;
};

BfReportTest::BfReportTest()
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of beamforming report packet")
{
}

BfReportTest::~BfReportTest()
{
}

HeCompressedBfReport::ChannelInfo
BfReportTest::GetBfCompressedReportInfo(HeMimoControlHeader heMimoControlHeader)
{
    HeCompressedBfReport::ChannelInfo channelInfo;
    uint8_t nc = heMimoControlHeader.GetNc() + 1;
    uint8_t nr = heMimoControlHeader.GetNr() + 1;
    uint8_t na = HeCompressedBfReport::CalculateNa(nc, nr);
    uint16_t ns = HeCompressedBfReport::GetNSubcarriers(heMimoControlHeader.GetRuStart(),
                                                        heMimoControlHeader.GetRuEnd(),
                                                        heMimoControlHeader.GetNg());

    uint16_t bits1, bits2;
    switch (heMimoControlHeader.GetFeedbackType())
    {
    case HeMimoControlHeader::SU:
        switch (heMimoControlHeader.GetCodebookInfo())
        {
        case 1:
            bits1 = 6;
            bits2 = 4;
            break;
        case 0:
            bits1 = 4;
            bits2 = 2;
            break;
        default:
            NS_FATAL_ERROR("Wrong codebook size.");
            break;
        }
        break;
    case HeMimoControlHeader::MU:
        switch (heMimoControlHeader.GetNg())
        {
        case 4:
            switch (heMimoControlHeader.GetCodebookInfo())
            {
            case 0:
                bits1 = 7;
                bits2 = 5;
                break;
            case 1:
                bits1 = 9;
                bits2 = 7;
                break;
            default:
                NS_FATAL_ERROR("Unsupported codebook size for MU case");
                break;
            }
            break;
        case 16:
            bits1 = 9;
            bits2 = 7;
            break;
        default:
            NS_FATAL_ERROR("Unsupported subcarrier grouping parameter Ng for MU case");
            break;
        }
        break;
    default:
        NS_FATAL_ERROR("Feedback type of channel sounding is not supported.");
        break;
    }

    Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable>();

    for (uint8_t i = 0; i < nc; i++)
    {
        channelInfo.m_stStreamSnr.push_back(x->GetInteger(0, pow(2, 8) - 1));
    }

    for (uint16_t i = 0; i < ns; i++)
    {
        std::vector<uint16_t> phi;
        std::vector<uint16_t> psi;

        for (uint8_t j = 0; j < na / 2; j++)
        {
            phi.push_back(x->GetInteger(0, pow(2, bits1) - 1));
            psi.push_back(x->GetInteger(0, pow(2, bits2) - 1));
        }

        channelInfo.m_phi.push_back(phi);
        channelInfo.m_psi.push_back(psi);
    }

    return channelInfo;
}

std::vector<std::vector<uint8_t>>
BfReportTest::GetMuExclusiveReportInfo(HeMimoControlHeader heMimoControlHeader)
{
    std::vector<std::vector<uint8_t>> deltaSnr;

    uint8_t nc = heMimoControlHeader.GetNc() + 1;
    uint16_t ns = HeCompressedBfReport::GetNSubcarriers(heMimoControlHeader.GetRuStart(),
                                                        heMimoControlHeader.GetRuEnd(),
                                                        heMimoControlHeader.GetNg());

    Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable>();
    for (uint16_t i = 0; i < ns; i++)
    {
        std::vector<uint8_t> deltaSnrPerSubcarrier;
        for (uint8_t j = 0; j < nc; j++)
        {
            deltaSnrPerSubcarrier.push_back(x->GetInteger(0, pow(2, 4) - 1));
        }
        deltaSnr.push_back(deltaSnrPerSubcarrier);
    }
    return deltaSnr;
}

void
BfReportTest::DoRun()
{
    // Test HE MIMO Control header
    HeMimoControlHeader heMimoControlHeader;
    heMimoControlHeader.SetNc(7);
    heMimoControlHeader.SetNr(7);
    heMimoControlHeader.SetGrouping(16);
    heMimoControlHeader.SetCodebookInfo(1);
    heMimoControlHeader.SetFeedbackType(HeMimoControlHeader::MU);
    heMimoControlHeader.SetRemainingFeedback(7);
    heMimoControlHeader.SetFirstFeedback(1);
    heMimoControlHeader.SetBw(160);
    heMimoControlHeader.SetRuStart(72);
    heMimoControlHeader.SetRuEnd(73);
    heMimoControlHeader.SetSoundingDialogToken(63);
    heMimoControlHeader.SetDisallowedSubchannelBitmapPresent(0);
    TestHeaderSerialization(heMimoControlHeader);

    heMimoControlHeader.SetDisallowedSubchannelBitmapPresent(1);
    heMimoControlHeader.SetDisallowedSubchannelBitmap(255);
    TestHeaderSerialization(heMimoControlHeader);

    // Test HE Compressed Beamforming Report header and MU Exclusive Beamforming Report header
    uint16_t ruEnd;
    HeCompressedBfReport::ChannelInfo compressedBfInfo;
    std::vector<std::vector<uint8_t>> muExclusiveBfInfo;
    for (uint16_t width = 20; width <= 160; width = 2 * width)
    {
        switch (width)
        {
        case 20:
            ruEnd = 8;
            break;
        case 40:
            ruEnd = 17;
            break;
        case 80:
            ruEnd = 36;
            break;
        case 160:
            ruEnd = 73;
            break;
        default:
            break;
        }
        heMimoControlHeader.SetBw(width);
        heMimoControlHeader.SetRuStart(0);
        heMimoControlHeader.SetRuEnd(ruEnd);
        for (uint8_t nc = 1; nc <= 4; nc++)
        {
            heMimoControlHeader.SetNc(nc - 1);
            // Generate random channel information for MU Exclusive Beamforming Report header
            muExclusiveBfInfo = GetMuExclusiveReportInfo(heMimoControlHeader);

            // Test MU Exclusive Beamforming Report header
            HeMuExclusiveBfReport heMuExclusiveBfReport(heMimoControlHeader);
            heMuExclusiveBfReport.SetDeltaSnr(muExclusiveBfInfo);
            TestHeaderSerialization(heMuExclusiveBfReport, heMimoControlHeader);

            for (uint8_t ng = 4; ng <= 16; ng = 4 * ng)
            {
                heMimoControlHeader.SetGrouping(ng);

                for (uint8_t nr = nc; nr <= 4; nr++)
                {
                    if (nr == 1)
                    {
                        continue;
                    }

                    heMimoControlHeader.SetNr(nr - 1);
                    for (uint8_t feedback = 0; feedback <= 1; feedback++)
                    {
                        heMimoControlHeader.SetFeedbackType(HeMimoControlHeader::CsType(feedback));
                        for (uint8_t codebook = 0; codebook <= 1; codebook++)
                        {
                            heMimoControlHeader.SetCodebookInfo(codebook);
                            HeCompressedBfReport heCompressedBfReport(heMimoControlHeader);

                            // Generate random channel information for HE Compressed Beamforming
                            // Report header
                            compressedBfInfo = GetBfCompressedReportInfo(heMimoControlHeader);
                            heCompressedBfReport.SetChannelInfo(compressedBfInfo);
                            // Test HE Compressed Beamforming Report header
                            TestHeaderSerialization(heCompressedBfReport, heMimoControlHeader);
                        }
                    }
                }
            }
        }
    }
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief wifi channel sounding frames Test Suite
 */
class WifiChannelSoundingFrameTestSuite : public TestSuite
{
  public:
    WifiChannelSoundingFrameTestSuite();
};

WifiChannelSoundingFrameTestSuite::WifiChannelSoundingFrameTestSuite()
    : TestSuite("wifi-channel-sounding-frame", UNIT)
{
    AddTestCase(new NdpaTest(), TestCase::QUICK);
    AddTestCase(new BfrpTriggerTest(), TestCase::QUICK);
    AddTestCase(new BfReportTest(), TestCase::QUICK);
}

static WifiChannelSoundingFrameTestSuite g_wifiChannelSoundingFrameTestSuite; ///< the test suite
