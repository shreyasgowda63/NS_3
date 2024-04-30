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

#include "ns3/channel-sounding.h"
#include "ns3/config.h"
#include "ns3/ctrl-headers.h"
#include "ns3/he-configuration.h"
#include "ns3/he-frame-exchange-manager.h"
#include "ns3/he-phy.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/mgt-action-headers.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/packet.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/rr-multi-user-scheduler.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/uinteger.h"
#include "ns3/wifi-acknowledgment.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-psdu.h"

#include <functional>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiMacChannelSoundingTestSuite");

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test sequences of channel sounding frame sequence
 *
 *
 * In each test, at least two rounds of channel sounding frames will be checked. The recording stops
 * until a MU channel sounding is recorded or the simulation time is reached.
 */
class ChannelSoundingSequenceTest : public TestCase
{
  public:
    /**
     * Constructor
     * \param nStation the number of stations
     * \param width the channel bandwidth
     * \param channelSoundingInterval the channel sounding interval
     * \param txopLimit the TXOP limit in microseconds
     * \param numAntennas the number of antennas at each device
     * \param maxNc the maximum number of columns in compressed beamforming matrix
     * \param ng subcarrier grouping parameter Ng
     * \param fineCodebook whether a fine codebook should be use. If true, then codebook size of
     * (6,4) and (9,7) should be used for SU and MU channel sounding, respectively. If false,
     * codebook size of (4,2) and (7,5) should be used for SU and MU channel sounding, respectively.
     */
    ChannelSoundingSequenceTest(uint8_t nStation,
                                uint16_t width,
                                Time channelSoundingInterval,
                                uint16_t txopLimit,
                                uint8_t numAntennas,
                                uint8_t maxNc,
                                uint8_t ng,
                                bool fineCodebook);
    ~ChannelSoundingSequenceTest() override;

    /**
     * Function to trace packets received by the AP
     * \param context the context
     * \param p the packet
     */
    void ApReceive(std::string context, Ptr<const Packet> p);

    /**
     * Function to trace packets received by stations
     * \param context the context
     * \param p the packet
     */
    void StaReceive(std::string context, Ptr<const Packet> p);

    /**
     * Callback invoked when FrameExchangeManager passes PSDUs to the PHY
     * \param context the context
     * \param psduMap the PSDU map
     * \param txVector the TX vector
     * \param txPowerW the tx power in Watts
     */
    void Transmit(std::string context,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW);
    /**
     * Check correctness of frame exchange of channel sounding
     * \param idxTx the index of TxFrameInfo in m_txPsdus where the check should start
     * \param idxRx the index of RxPacketInfo in m_rxPackets where the check should start
     */
    void CheckFrameExchange(uint16_t& idxTx, uint16_t& idxRx);

    /**
     * Check correctness of transmitted channel sounding frames
     */
    void CheckResults();

  private:
    void DoRun() override;

    struct TxFrameInfo
    {
        Time startTx;             ///< start TX time
        Time endTx;               ///< end TX time
        WifiConstPsduMap psduMap; ///< transmitted PSDU map
        WifiTxVector txVector;    ///< Tx vectors
    };

    struct RxPacketInfo
    {
        Time rxTime;              ///< packet reception time
        Ptr<const Packet> packet; ///< Wifi Mac header of the received packet
    };

    NetDeviceContainer m_staDevices;          ///< Netdevices for all the stations
    Ptr<WifiNetDevice> m_apDevice;            ///< Netdevice for AP
    Ipv4InterfaceContainer staNodeInterfaces; ///< Ipv4 Interfaces for all the stations
    std::vector<TxFrameInfo> m_txPsdus;       ///< transmitted PSDUs
    std::vector<RxPacketInfo> m_rxPackets;    ///< received packets
    std::map<Mac48Address, ChannelSounding::ChannelInfo>
        m_staSuChannel; ///< channel information measured at stations in SU channel sounding: Mac
                        ///< address of the station, measured channel information at the station
    std::map<Mac48Address, ChannelSounding::ChannelInfo>
        m_staMuChannel; ///< channel information measured at stations in MU channel sounding: Mac
                        ///< address of the station, measured channel information at the station

    uint8_t m_nStations;            ///< total number of stations
    uint16_t m_channelWidth;        ///< PHY channel bandwidth in MHz
    uint16_t m_txopLimit;           ///< TXOP limit in microseconds
    Time m_channelSoundingInterval; ///< channel sounding interval
    uint8_t m_numAntennas;          ///< number of antennas per device
    uint8_t m_maxNc;                ///< max Nc used for beamforming report frame
    uint8_t m_ng;                   ///< subcarrier grouping Ng
    bool m_fineCodebook; ///< whether coarse codebook size ((4,2) for SU and (7,5) for MU) is used
                         ///< for channel sounding
    bool m_csStart;      ///< whether channel sounding has started
    uint8_t m_numCs;     ///< the number of channel sounding sequences that have been recorded
    uint8_t m_numTxBfReport; ///< the number of beamforming reports transmitted at stations in one
                             ///< round of channel sounding
    uint8_t m_numRxBfReport; ///< the number of beamforming reports received at the AP in one round
                             ///< of channel sounding
    uint8_t m_nCsStations;   ///< the number of stations scheduled in one round of channel sounding
};

ChannelSoundingSequenceTest::ChannelSoundingSequenceTest(uint8_t nStation,
                                                         uint16_t width,
                                                         Time channelSoundingInterval,
                                                         uint16_t txopLimit,
                                                         uint8_t numAntennas,
                                                         uint8_t maxNc,
                                                         uint8_t ng,
                                                         bool fineCodebook)
    : TestCase(
          "Check correct operation of channel sounding and downlink data transmission sequences"),
      m_nStations(nStation),
      m_channelWidth(width),
      m_txopLimit(txopLimit),
      m_channelSoundingInterval(channelSoundingInterval),
      m_numAntennas(numAntennas),
      m_maxNc(maxNc),
      m_ng(ng),
      m_fineCodebook(fineCodebook),
      m_csStart(false),
      m_numCs(0),
      m_numTxBfReport(0),
      m_numRxBfReport(0),
      m_nCsStations(0)
{
}

ChannelSoundingSequenceTest::~ChannelSoundingSequenceTest()
{
}

void
ChannelSoundingSequenceTest::ApReceive(std::string context, Ptr<const Packet> p)
{
    if (m_csStart)
    {
        WifiMacHeader hdr;
        p->PeekHeader(hdr);
        if (hdr.IsActionNoAck())
        {
            m_numRxBfReport++;
            m_rxPackets.push_back({Simulator::Now(), p});
        }
        if (m_numRxBfReport == m_nCsStations)
        {
            m_csStart = false;
            m_numTxBfReport = 0;
            m_numRxBfReport = 0;
            m_numCs++;
            if (m_numCs == 1)
            {
                for (uint8_t i = 0; i < m_nStations; i++)
                {
                    uint16_t port = 9;
                    UdpClientHelper client(staNodeInterfaces.GetAddress(i), port);
                    client.SetAttribute("MaxPackets", UintegerValue(2));
                    client.SetAttribute("Interval", TimeValue(Seconds(0.1)));
                    client.SetAttribute("PacketSize", UintegerValue(700));
                    ApplicationContainer clientApp = client.Install(m_apDevice->GetNode());

                    Time appStartTime = Seconds(0.51);
                    clientApp.Start(appStartTime);
                    clientApp.Stop(appStartTime + Seconds(0.5));
                }
            }
        }
    }
}

void
ChannelSoundingSequenceTest::StaReceive(std::string context, Ptr<const Packet> p)
{
    WifiMacHeader hdr;
    p->PeekHeader(hdr);
    if (hdr.IsQosData() && m_numCs == 2 && m_numRxBfReport == m_nCsStations)
    {
        Simulator::Stop();
    }
}

void
ChannelSoundingSequenceTest::Transmit(std::string context,
                                      WifiConstPsduMap psduMap,
                                      WifiTxVector txVector,
                                      double txPowerW)
{
    for (const auto& [staId, psdu] : psduMap)
    {
        NS_LOG_INFO("Sending " << psdu->GetHeader(0).GetTypeString() << " #MPDUs "
                               << psdu->GetNMpdus() << " Sender address "
                               << psdu->GetHeader(0).GetAddr2() << " Receiver address "
                               << psdu->GetHeader(0).GetAddr1());
    }

    if (!m_csStart && psduMap.begin()->second->GetHeader(0).IsNdpa())
    {
        m_csStart = true;
        CtrlNdpaHeader ndpaHeader;
        psduMap.begin()->second->GetPayload(0)->PeekHeader(ndpaHeader);
        m_nCsStations = ndpaHeader.GetNumStaInfoFields();
    }

    if (m_csStart)
    {
        if (m_numTxBfReport < m_nCsStations &&
            (psduMap.begin()->second->GetHeader(0).IsNdpa() ||
             psduMap.begin()->second->GetHeader(0).IsNdp() ||
             psduMap.begin()->second->GetHeader(0).IsTrigger() ||
             psduMap.begin()->second->GetHeader(0).IsActionNoAck()))
        {
            Time txDuration = WifiPhy::CalculateTxDuration(psduMap, txVector, WIFI_PHY_BAND_5GHZ);
            m_txPsdus.push_back(
                {Simulator::Now(), Simulator::Now() + txDuration, psduMap, txVector});
        }

        if (psduMap.begin()->second->GetHeader(0).IsActionNoAck())
        {
            m_numTxBfReport++;

            // Store channel information measured at the station
            Mac48Address staAddress = psduMap.begin()->second->GetAddr2();
            for (uint8_t i = 0; i < m_staDevices.GetN(); i++)
            {
                auto dev = DynamicCast<WifiNetDevice>(m_staDevices.Get(i));
                if (dev->GetAddress() == staAddress)
                {
                    auto staMac = DynamicCast<StaWifiMac>(dev->GetMac());
                    auto heFem =
                        DynamicCast<HeFrameExchangeManager>(staMac->GetFrameExchangeManager());
                    if (m_nCsStations == 1)
                    {
                        m_staSuChannel.emplace(staAddress,
                                               heFem->GetCsBeamformee()->GetChannelInfo());
                    }
                    else if (m_nCsStations > 1)
                    {
                        m_staMuChannel.emplace(staAddress,
                                               heFem->GetCsBeamformee()->GetChannelInfo());
                    }
                }
            }
        }
    }
}

void
ChannelSoundingSequenceTest::CheckResults()
{
    uint16_t idxTx = 0;
    uint16_t idxRx = 0;
    // Check SU channel sounding
    CtrlNdpaHeader ndpaHeader;
    m_txPsdus[0].psduMap[SU_STA_ID]->GetPayload(0)->PeekHeader(ndpaHeader);
    NS_TEST_EXPECT_MSG_EQ(
        ndpaHeader.GetNumStaInfoFields(),
        1,
        "Expect that only one user scheduled in the first round of channel sounding.");
    CheckFrameExchange(idxTx, idxRx);

    // Check MU channel sounding
    m_txPsdus[3].psduMap[SU_STA_ID]->GetPayload(0)->PeekHeader(ndpaHeader);
    NS_TEST_EXPECT_MSG_GT(ndpaHeader.GetNumStaInfoFields(), 1, "Expected MU channel sounding.");
    NS_TEST_EXPECT_MSG_LT_OR_EQ(ndpaHeader.GetNumStaInfoFields(),
                                m_nStations,
                                "Expected MU channel sounding.");
    CheckFrameExchange(idxTx, idxRx);
}

void
ChannelSoundingSequenceTest::CheckFrameExchange(uint16_t& idxTx, uint16_t& idxRx)
{
    Time sifs = m_apDevice->GetMac()->GetWifiPhy()->GetSifs();

    // Check NDPA transmission
    NS_TEST_EXPECT_MSG_EQ((m_txPsdus[idxTx].psduMap.size() == 1 &&
                           m_txPsdus[idxTx].psduMap[SU_STA_ID]->GetHeader(0).IsNdpa()),
                          true,
                          "Expect that an NDPA frame is sent.");
    CtrlNdpaHeader ndpaHeader;
    m_txPsdus[idxTx].psduMap[SU_STA_ID]->GetPayload(0)->PeekHeader(ndpaHeader);

    // Check whether NDPA information indicates correct simulation parameters
    uint8_t nCsStations = ndpaHeader.GetNumStaInfoFields();
    NS_TEST_EXPECT_MSG_EQ(ndpaHeader.begin()->m_nc + 1, m_maxNc, "Expected correct max Nc");
    bool codebook = ndpaHeader.begin()->m_codebookSize;

    uint8_t feedbackType = ndpaHeader.begin()->m_feedbackTypeNg;
    NS_TEST_EXPECT_MSG_LT(feedbackType, 4, "Expected proper Feedback Type and Ng subfield in NDPA");
    switch (feedbackType)
    {
    case 0:
        NS_TEST_EXPECT_MSG_EQ(nCsStations, 1, "Expected SU channel sounding");
        NS_TEST_EXPECT_MSG_EQ(m_ng, 4, "Expect that Ng is 4");
        NS_TEST_EXPECT_MSG_EQ(codebook, m_fineCodebook, "Expected correct codebook size");
        break;
    case 1:
        NS_TEST_EXPECT_MSG_EQ(nCsStations, 1, "Expected SU channel sounding");
        NS_TEST_EXPECT_MSG_EQ(m_ng, 16, "Expect that Ng is 16");
        NS_TEST_EXPECT_MSG_EQ(codebook, m_fineCodebook, "Expected correct codebook size");
        break;
    case 2:
        NS_TEST_EXPECT_MSG_GT(nCsStations, 1, "Expected MU channel sounding");
        NS_TEST_EXPECT_MSG_EQ(m_ng, 4, "Expect that Ng is 4");
        NS_TEST_EXPECT_MSG_EQ(codebook, m_fineCodebook, "Expected correct codebook size");
        break;
    case 3:
        NS_TEST_EXPECT_MSG_GT(nCsStations, 1, "Expected MU channel sounding");
        NS_TEST_EXPECT_MSG_EQ(m_ng, 16, "Expect that Ng is 16");
        NS_TEST_EXPECT_MSG_EQ(codebook, 1, "Expected correct codebook size");
        break;
    default:
        break;
    }

    // Check NDP transmission
    NS_TEST_EXPECT_MSG_EQ((m_txPsdus[idxTx + 1].psduMap.size() == 1 &&
                           m_txPsdus[idxTx + 1].psduMap[SU_STA_ID]->GetHeader(0).IsNdp()),
                          true,
                          "Expected an NDP frame");
    NS_TEST_EXPECT_MSG_EQ(m_txPsdus[idxTx].endTx + sifs,
                          m_txPsdus[idxTx + 1].startTx,
                          "NDP frame sent sent at proper time.");

    uint8_t frameIdx = 2;
    if (nCsStations > 1)
    {
        // Check BFRP trigger transmission
        NS_TEST_EXPECT_MSG_EQ(
            (m_txPsdus[idxTx + 2].psduMap.size() == 1 &&
             m_txPsdus[idxTx + 2].psduMap[SU_STA_ID]->GetHeader(0).IsTrigger() &&
             m_txPsdus[idxTx + 2].psduMap[SU_STA_ID]->GetHeader(0).GetAddr1().IsBroadcast()),
            true,
            "Expected a trigger frame");
        CtrlTriggerHeader triggerHeader;
        m_txPsdus[idxTx + 2].psduMap[SU_STA_ID]->GetPayload(0)->PeekHeader(triggerHeader);
        NS_TEST_EXPECT_MSG_EQ(triggerHeader.IsBfrp(), true, "Expected a BFRP Trigger Frame");
        NS_TEST_EXPECT_MSG_EQ(m_txPsdus[idxTx + 1].endTx + sifs,
                              m_txPsdus[idxTx + 2].startTx,
                              "BFRP Trigger frame sent at proper time.");
        frameIdx++;
    }

    // Check beamforming report transmission
    for (uint8_t i = 0; i < nCsStations; i++)
    {
        if (nCsStations > 1)
        {
            NS_TEST_EXPECT_MSG_EQ(m_txPsdus[idxTx + frameIdx + i].txVector.GetPreambleType(),
                                  WIFI_PREAMBLE_HE_TB,
                                  "Expected trigger-based beamforming report feedback");
        }
        NS_TEST_EXPECT_MSG_EQ(
            (m_txPsdus[idxTx + frameIdx + i].psduMap.size() == 1 &&
             m_txPsdus[idxTx + frameIdx + i].psduMap.begin()->second->GetHeader(0).IsActionNoAck()),
            true,
            "Expected a beamforming report frame");
        NS_TEST_EXPECT_MSG_EQ(m_txPsdus[idxTx + frameIdx - 1].endTx + sifs,
                              m_txPsdus[idxTx + frameIdx + i].startTx,
                              "Beamforming report frame sent at proper time");
    }

    // Check beamforming report frame reception
    for (uint8_t i = 0; i < nCsStations; i++)
    {
        WifiMacHeader hdr;
        Ptr<Packet> bfPacket = m_rxPackets[idxRx + i].packet->Copy();
        bfPacket->RemoveHeader(hdr);
        NS_TEST_EXPECT_MSG_EQ(hdr.IsActionNoAck(),
                              true,
                              "Expect that a beamforming report is received.");

        // Check correctness of channel information reception
        std::map<Mac48Address, ChannelSounding::ChannelInfo>::iterator channelIt;
        if (nCsStations == 1)
        {
            channelIt = m_staSuChannel.find(hdr.GetAddr2());
        }
        else if (nCsStations > 1)
        {
            channelIt = m_staMuChannel.find(hdr.GetAddr2());
        }

        WifiActionHeader actionHdr;
        bfPacket->RemoveHeader(actionHdr);

        HeMimoControlHeader heMimoControlHeader;
        bfPacket->RemoveHeader(heMimoControlHeader);

        HeCompressedBfReport heCompressedBfReport(heMimoControlHeader);
        bfPacket->RemoveHeader(heCompressedBfReport);

        NS_TEST_EXPECT_MSG_EQ((channelIt->second.m_stStreamSnr ==
                               heCompressedBfReport.GetChannelInfo().m_stStreamSnr),
                              true,
                              "Expected correct space-time stream SNR");

        NS_TEST_EXPECT_MSG_EQ(
            (channelIt->second.m_phi == heCompressedBfReport.GetChannelInfo().m_phi),
            true,
            "Expected correct Phi angles");
        NS_TEST_EXPECT_MSG_EQ(
            (channelIt->second.m_psi == heCompressedBfReport.GetChannelInfo().m_psi),
            true,
            "Expected correct Psi angles");

        if (heMimoControlHeader.GetFeedbackType() == HeMimoControlHeader::MU)
        {
            HeMuExclusiveBfReport heMuExclusiveBfReport(heMimoControlHeader);
            bfPacket->RemoveHeader(heMuExclusiveBfReport);
            NS_TEST_EXPECT_MSG_EQ(
                (channelIt->second.m_deltaSnr == heMuExclusiveBfReport.GetDeltaSnr()),
                true,
                "Expected correct Delta SNR");
        }
    }
    idxTx = idxTx + frameIdx + nCsStations;
    idxRx = idxRx + nCsStations;
}

void
ChannelSoundingSequenceTest::DoRun()
{
    uint32_t previousSeed = RngSeedManager::GetSeed();
    uint64_t previousRun = RngSeedManager::GetRun();
    Config::SetGlobal("RngSeed", UintegerValue(1));
    Config::SetGlobal("RngRun", UintegerValue(1));

    Time simulationTime = Seconds(3);

    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(m_nStations);

    Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();

    SpectrumWifiPhyHelper phy;
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.SetChannel(spectrumChannel);
    phy.Set("Antennas", UintegerValue(m_numAntennas));
    phy.Set("MaxSupportedTxSpatialStreams", UintegerValue(m_numAntennas));
    phy.Set("MaxSupportedRxSpatialStreams", UintegerValue(m_numAntennas));

    std::string channelStr("{0, " + std::to_string(m_channelWidth) + ", BAND_5GHZ, 0}");
    phy.Set("ChannelSettings", StringValue(channelStr));

    int mcs = 8;
    std::ostringstream ossDataMode;
    ossDataMode << "HeMcs" << mcs;

    StringValue ctrlRate;
    auto nonHtRefRateMbps = HePhy::GetNonHtReferenceRate(mcs) / 1e6;
    std::ostringstream ossControlMode;
    ossControlMode << "OfdmRate" << nonHtRefRateMbps << "Mbps";
    ctrlRate = StringValue(ossControlMode.str());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ax);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue(ossDataMode.str()),
                                 "ControlMode",
                                 ctrlRate);
    wifi.ConfigHeOptions("NgSu",
                         UintegerValue(m_ng),
                         "NgMu",
                         UintegerValue(m_ng),
                         "CodebookSizeSu",
                         StringValue(m_fineCodebook ? "(6,4)" : "(4,2)"),
                         "CodebookSizeMu",
                         StringValue(m_fineCodebook ? "(9,7)" : "(7,5)"),
                         "MaxNc",
                         UintegerValue(m_maxNc - 1));

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns3");
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));

    m_staDevices = wifi.Install(phy, mac, wifiStaNodes);

    mac.SetType("ns3::ApWifiMac",
                "Ssid",
                SsidValue(ssid),
                "EnableBeaconJitter",
                BooleanValue(false));
    mac.SetMultiUserScheduler("ns3::RrMultiUserScheduler",
                              "AccessReqInterval",
                              TimeValue(MilliSeconds(1000)),
                              "ChannelSoundingInterval",
                              TimeValue(m_channelSoundingInterval),
                              "EnableMuMimo",
                              BooleanValue(true),
                              "UseCentral26TonesRus",
                              BooleanValue(false));

    m_apDevice = DynamicCast<WifiNetDevice>(wifi.Install(phy, mac, wifiApNode).Get(0));

    int64_t streamNumber = 10;
    streamNumber += wifi.AssignStreams(NetDeviceContainer(m_apDevice), streamNumber);
    streamNumber += wifi.AssignStreams(m_staDevices, streamNumber);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    positionAlloc->Add(Vector(0.0, 1.0, 0.0));
    positionAlloc->Add(Vector(-1.0, 0.0, 0.0));
    positionAlloc->Add(Vector(0.0, -1.0, 0.0));
    positionAlloc->Add(Vector(0.707, 0.707, 0.0));
    positionAlloc->Add(Vector(0.707, -0.707, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);

    NetDeviceContainer allDevices(NetDeviceContainer(m_apDevice), m_staDevices);
    for (uint32_t i = 0; i < allDevices.GetN(); i++)
    {
        auto dev = DynamicCast<WifiNetDevice>(allDevices.Get(i));
        dev->GetMac()->GetQosTxop(AC_BE)->SetTxopLimit(MicroSeconds(m_txopLimit));
        dev->GetMac()->GetQosTxop(AC_BK)->SetTxopLimit(MicroSeconds(m_txopLimit));
        dev->GetMac()->GetQosTxop(AC_VI)->SetTxopLimit(MicroSeconds(m_txopLimit));
        dev->GetMac()->GetQosTxop(AC_VO)->SetTxopLimit(MicroSeconds(m_txopLimit));
    }

    InternetStackHelper stack;
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);

    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");

    Ipv4InterfaceContainer apNodeInterface;

    staNodeInterfaces = address.Assign(m_staDevices);
    apNodeInterface = address.Assign(NetDeviceContainer(m_apDevice));

    uint16_t port = 9;
    UdpServerHelper server(port);
    ApplicationContainer serverApp = server.Install(wifiStaNodes);
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(simulationTime);

    // Send packets to the first station to trigger SU channel sounding
    UdpClientHelper client(staNodeInterfaces.GetAddress(0), port);
    client.SetAttribute("MaxPackets", UintegerValue(2));
    client.SetAttribute("Interval", TimeValue(Seconds(0.1)));
    client.SetAttribute("PacketSize", UintegerValue(700));
    ApplicationContainer clientApp = client.Install(wifiApNode.Get(0));

    clientApp.Start(Seconds(1));
    clientApp.Stop(Seconds(1.5));

    Config::Connect("/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxEnd",
                    MakeCallback(&ChannelSoundingSequenceTest::ApReceive, this));

    for (uint8_t i = 0; i < m_nStations; i++)
    {
        Config::Connect("/NodeList/" + std::to_string(i + 1) +
                            "/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxEnd",
                        MakeCallback(&ChannelSoundingSequenceTest::StaReceive, this));
    }

    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxPsduBegin",
                    MakeCallback(&ChannelSoundingSequenceTest::Transmit, this));

    Simulator::Stop(simulationTime);
    Simulator::Run();

    CheckResults();

    Simulator::Destroy();

    Config::SetGlobal("RngSeed", UintegerValue(previousSeed));
    Config::SetGlobal("RngRun", UintegerValue(previousRun));
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief wifi MAC Channel Sounding Test Suite
 */
class WifiMacChannelSoundingTestSuite : public TestSuite
{
  public:
    WifiMacChannelSoundingTestSuite();
};

WifiMacChannelSoundingTestSuite::WifiMacChannelSoundingTestSuite()
    : TestSuite("wifi-mac-channel-sounding", UNIT)
{
    uint16_t txopLimit = 5440;
    uint8_t nStations = 6;
    Time csInterval = Seconds(0.6);

    for (uint16_t width = 20; width <= 160;)
    {
        for (uint8_t numAntennas = 2; numAntennas <= 4; numAntennas++)
        {
            for (uint8_t maxNc = 1; maxNc <= numAntennas; maxNc++)
            {
                // Ng = 16, codebook size (6,4) for SU and (9,7) for MU
                AddTestCase(new ChannelSoundingSequenceTest(nStations,
                                                            width,
                                                            csInterval,
                                                            txopLimit,
                                                            numAntennas,
                                                            maxNc,
                                                            16,
                                                            true),
                            TestCase::QUICK);

                // Ng = 16, codebook size (4,2) for SU and (7,5) for MU
                AddTestCase(new ChannelSoundingSequenceTest(nStations,
                                                            width,
                                                            csInterval,
                                                            txopLimit,
                                                            numAntennas,
                                                            maxNc,
                                                            16,
                                                            false),
                            TestCase::QUICK);

                // Ng = 4, codebook size (6,4) for SU and (9,7) for MU
                AddTestCase(new ChannelSoundingSequenceTest(nStations,
                                                            width,
                                                            csInterval,
                                                            txopLimit,
                                                            numAntennas,
                                                            maxNc,
                                                            4,
                                                            true),
                            TestCase::QUICK);

                // Ng = 4, codebook size (4,2) for SU and (7,5) for MU
                AddTestCase(new ChannelSoundingSequenceTest(nStations,
                                                            width,
                                                            csInterval,
                                                            txopLimit,
                                                            numAntennas,
                                                            maxNc,
                                                            4,
                                                            false),
                            TestCase::QUICK);
            }
        }
        width *= 2;
    }
}

static WifiMacChannelSoundingTestSuite g_wifiMacChannelSoundingTestSuite; ///< the test suite
