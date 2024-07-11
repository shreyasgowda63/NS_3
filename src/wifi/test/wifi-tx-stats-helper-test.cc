/*
 * Copyright (c) 2024 Huazhong University of Science and Technology
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
 */

#include <ns3/test.h>
#include <ns3/log.h>
#include "ns3/ap-wifi-mac.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/mobility-helper.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/qos-txop.h"
#include "ns3/qos-utils.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/single-model-spectrum-channel.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-ppdu.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-tx-stats-helper.h"

using namespace ns3;

/**
 * \ingroup wifi-test
 * \brief Implements a test case to evaluate the transmission process of multiple WiFi
 * MAC Layer frames (MPDU). This testcase, unlike the other, uses .11a to test the
 * handling of non-Block ACKs.
 *
 * This class extends the TestCase class to simulate and analyze the transmission of MPDUs,
 * from a STA to a AP, using single link. It specifically tests the WifiTxStatsHelper's
 * capability to store per-packet info including source node ID, number
 * of failures, MAC layer enqueue time, PHY layer transmission start time, MAC layer ACK
 * reception time, MAC layer dequeue time. It also tests the correctness of final statistics
 * including successes, retransmitted packets, retransmissions, average failures,
 * failed packets, mean end-to-end delay. Both per-node and total results are examined.
 */
class WifiTxStatsHelperTestSingleLink : public TestCase {
public:
    WifiTxStatsHelperTestSingleLink();
    ~WifiTxStatsHelperTestSingleLink() override;

    /**
     * Callback invoked when PHY starts transmission of a PSDU, used to record TX start
     * time and TX duration.
     *
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
     * Check correctness of transmitted frames
     */
    void CheckResults();

private:
    void DoRun() override;
    WifiTxStatsHelper m_wifiTxStats;
    std::vector<Time> m_pktTxStartTimes;
    std::vector<Time> m_pktDurations;
    Time m_sifs{0};
    Time m_difs{0};
    Time m_slot{0};
    uint32_t m_cwMin{0};
};

WifiTxStatsHelperTestSingleLink::WifiTxStatsHelperTestSingleLink()
    : TestCase("Check single link case of tx stats")
{
}

WifiTxStatsHelperTestSingleLink::~WifiTxStatsHelperTestSingleLink()
{
}

void WifiTxStatsHelperTestSingleLink::Transmit(std::string context, WifiConstPsduMap psduMap, WifiTxVector txVector,
    double txPowerW)
{
    m_pktTxStartTimes.push_back(Simulator::Now());
    m_pktDurations.push_back(WifiPhy::CalculateTxDuration(psduMap, txVector, WIFI_PHY_BAND_5GHZ));
}

void WifiTxStatsHelperTestSingleLink::DoRun()
{
    std::string dataMode = "OfdmRate12Mbps";
    std::string ackMode = "OfdmRate6Mbps";

    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(40);
    int64_t streamNumber = 100;

    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(1);

    Ptr<SingleModelSpectrumChannel> spectrumChannel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel>();
    spectrumChannel->AddPropagationLossModel(lossModel);
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
            CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);

    SpectrumWifiPhyHelper phy;
    phy.SetChannel(spectrumChannel);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue(dataMode),
                                 "ControlMode",
                                 StringValue(ackMode),
                                 "MaxSsrc",
                                 UintegerValue(7));

    WifiMacHelper mac;
    mac.SetType("ns3::StaWifiMac",
                "QosSupported",
                BooleanValue(false),
                "Ssid",
                SsidValue(Ssid("test-ssid")));
    auto staDevices = wifi.Install(phy, mac, wifiStaNodes);

    mac.SetType("ns3::ApWifiMac",
                "QosSupported",
                BooleanValue(false),
                "Ssid",
                SsidValue(Ssid("test-ssid")),
                "BeaconInterval",
                TimeValue(MicroSeconds(102400)),
                "EnableBeaconJitter",
                BooleanValue(false));
    auto apDevices = wifi.Install(phy, mac, wifiApNode);

    wifi.AssignStreams(apDevices, streamNumber);
    wifi.AssignStreams(staDevices, streamNumber);

    m_sifs = DynamicCast<WifiNetDevice>(apDevices.Get(0))->GetPhy()->GetSifs();
    m_slot = DynamicCast<WifiNetDevice>(apDevices.Get(0))->GetPhy()->GetSlot();
    m_difs = m_sifs + 2 * m_slot;
    m_cwMin = DynamicCast<WifiNetDevice>(apDevices.Get(0))->GetMac()->GetTxop()->GetMinCw();

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);

    PacketSocketHelper packetSocket;
    packetSocket.Install(wifiApNode);
    packetSocket.Install(wifiStaNodes);

    // UL traffic (TX statistics will be installed at STA side)
    PacketSocketAddress socket;
    socket.SetSingleDevice(staDevices.Get(0)->GetIfIndex());
    socket.SetPhysicalAddress(apDevices.Get(0)->GetAddress());
    socket.SetProtocol(1);

    Ptr<PacketSocketClient> client = CreateObject<PacketSocketClient>();
    client->SetAttribute("PacketSize", UintegerValue(1500));
    client->SetAttribute("MaxPackets", UintegerValue(3));
    client->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
    client->SetRemote(socket);
    wifiStaNodes.Get(0)->AddApplication(client);
    client->SetStartTime(MicroSeconds(210000));
    client->SetStopTime(Seconds(1.0));

    Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer>();
    server->SetLocal(socket);
    wifiApNode.Get(0)->AddApplication(server);
    server->SetStartTime(Seconds(0.0));
    server->SetStopTime(Seconds(1.0));

    Ptr<ReceiveListErrorModel> apPem = CreateObject<ReceiveListErrorModel>();

    // We corrupt AP side reception so that:
    // 1) the 2nd data frame is retransmitted and succeeds (1 failure, 1 success)
    // 2) the 3rd data frame is transmitted 7 times (=Ssrc) and finally fails (7 failures, 0 success)
    //
    // No. of pkt       |   0   |   1   |   2   |   3   |   4   |   5   |   6   |   7   |   8   |
    // No. recvd by AP  |       |       |   0   |       |       |   1   |       |   2   |       |
    // AP's pkts        |  Bea  |  Bea  |       |  Ack  | AsRes |       |  Bea  |       | Ack1  |
    // STA's pkts       |       |       | AsReq |       |       |  Ack  |       | Data1 |       |
    //
    // No. of pkt       |   9   |  10   |  11   |  12   |  13   |  ...  |  18   |  19   |  ...
    // No. recvd by AP  | 3 (x) |   4   |       | 5 (x) | 6 (x) |  ...  |11 (x) |       |  ...
    // AP's pkts        |       |       | Ack2  |       |       |  ...  |       |  Bea  |  ...
    // STA's pkts       | Data2 | Data2 |       | Data3 | Data3 |  ...  | Data3 |       |  ...
    //
    // Legend:
    // Bea = Beacon, AsReq = Association Request, AsRes = Association Response
    // AP side corruption is indicated with (x)

    apPem->SetList({3, 5, 6, 7, 8, 9, 10, 11});
    DynamicCast<WifiNetDevice>(apDevices.Get(0))->GetMac()->GetWifiPhy()->SetPostReceptionErrorModel(apPem);

    NetDeviceContainer allNetDev;
    allNetDev.Add(apDevices);
    allNetDev.Add(staDevices);
    m_wifiTxStats.Enable(allNetDev);
    m_wifiTxStats.Start(Seconds(0));
    m_wifiTxStats.Stop(Seconds(1));

    // Trace PSDU TX to get start time and duration
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxPsduBegin",
                    MakeCallback(&WifiTxStatsHelperTestSingleLink::Transmit, this));

    Simulator::Stop(Seconds(1));
    Simulator::Run();
    CheckResults();
    Simulator::Destroy();
}

void WifiTxStatsHelperTestSingleLink::CheckResults()
{
    Time tolerance = NanoSeconds(50); // due to propagation delay
    auto finalResults = m_wifiTxStats.GetFinalStatistics();
    auto successInfo = m_wifiTxStats.GetSuccessInfoMap();
    auto failureInfo = m_wifiTxStats.GetFailureInfoMap();

    NS_TEST_ASSERT_MSG_EQ(finalResults.m_numSuccessPerNodeLink[1][0], 2, "Number of success packets should be 2");
    NS_TEST_ASSERT_MSG_EQ(finalResults.m_numSuccessTotal, 2, "Number of success packets should be 2");
    NS_TEST_ASSERT_MSG_EQ(finalResults.m_numRetransmittedPktsPerNode[1], 1, "Number of retransmitted successful packets should be 1");
    NS_TEST_ASSERT_MSG_EQ(finalResults.m_numRetransmittedTotal, 1, "Number of retransmitted successful packets should be 1");
    NS_TEST_ASSERT_MSG_EQ(finalResults.m_numRetransmissionPerNode[1], 1, "Number of retransmission should be 1");
    NS_TEST_ASSERT_MSG_EQ(finalResults.m_avgFailuresPerNode[1], 0.5, "Avg retransmission needed should be 0.5");
    NS_TEST_ASSERT_MSG_EQ(finalResults.m_avgFailuresTotal, 0.5, "Avg retransmission needed should be 0.5");
    NS_TEST_ASSERT_MSG_EQ(finalResults.m_numFinalFailedPerNode[1], 1, "Number of final failed packets should be 1");
    NS_TEST_ASSERT_MSG_EQ(finalResults.m_numFinalFailedTotal, 1, "Number of final failed packets should be 1");

    NS_TEST_ASSERT_MSG_EQ(successInfo[1][0][0].m_srcNodeId, 1, "Source node ID of the 1st data packet should be 1");
    NS_TEST_ASSERT_MSG_EQ(successInfo[1][0][1].m_srcNodeId, 1, "Source node ID of the 2nd data packet should be 1");
    NS_TEST_ASSERT_MSG_EQ(failureInfo[1][0].m_srcNodeId, 1, "Source node ID of the 3rd data packet should be 1");

    NS_TEST_ASSERT_MSG_EQ(successInfo[1][0][0].m_failures, 0, "The 1st data packet should have no failures");
    NS_TEST_ASSERT_MSG_EQ(successInfo[1][0][1].m_failures, 1, "The 2nd data packet should have 1 failure");
    NS_TEST_ASSERT_MSG_EQ(failureInfo[1][0].m_failures, 7, "The 3rd data packet should have 7 failures");

    NS_TEST_ASSERT_MSG_EQ(successInfo[1][0][0].m_txStarted, true, "The 1st data packet should have been TXed");
    NS_TEST_ASSERT_MSG_EQ(successInfo[1][0][1].m_txStarted, true, "The 2nd data packet should have been TXed");
    NS_TEST_ASSERT_MSG_EQ(failureInfo[1][0].m_txStarted, true, "The 3rd data packet should have been TXed");

    NS_TEST_ASSERT_MSG_EQ(successInfo[1][0][0].m_acked, true, "The 1st data packet should have been acked");
    NS_TEST_ASSERT_MSG_EQ(successInfo[1][0][1].m_acked, true, "The 2nd data packet should have been acked");
    NS_TEST_ASSERT_MSG_EQ(failureInfo[1][0].m_acked, false, "The 3rd data packet should not have been acked");

    NS_TEST_ASSERT_MSG_EQ(successInfo[1][0][0].m_dequeued, true, "The 1st data packet should have been dequeued");
    NS_TEST_ASSERT_MSG_EQ(successInfo[1][0][1].m_dequeued, true, "The 2nd data packet should have been dequeued");
    NS_TEST_ASSERT_MSG_EQ(failureInfo[1][0].m_dequeued, true, "The 3rd data packet should have been dequeued");

    NS_TEST_ASSERT_MSG_EQ(successInfo[1][0][0].m_enqueueTime, successInfo[1][0][1].m_enqueueTime, "Packets should be enqueued at the same time");
    NS_TEST_ASSERT_MSG_EQ(successInfo[1][0][0].m_enqueueTime, failureInfo[1][0].m_enqueueTime, "Packets should be enqueued at the same time");

    NS_TEST_ASSERT_MSG_GT_OR_EQ(successInfo[1][0][0].m_txStartTime,
        successInfo[1][0][0].m_enqueueTime,
        "Packets should be TXed after enqueued");
    NS_TEST_ASSERT_MSG_LT_OR_EQ(successInfo[1][0][0].m_txStartTime,
        successInfo[1][0][0].m_enqueueTime + tolerance + m_cwMin * m_slot,
        "Packet backoff slots should not exceed cwMin");
    NS_TEST_ASSERT_MSG_EQ(successInfo[1][0][0].m_txStartTime,
        m_pktTxStartTimes[7],
        "Wrong TX start time");
    NS_TEST_ASSERT_MSG_GT_OR_EQ(successInfo[1][0][0].m_ackTime,
        m_pktTxStartTimes[7] + m_pktDurations[7] + m_sifs + m_pktDurations[8],
        "Wrong Ack reception time");
    NS_TEST_ASSERT_MSG_LT_OR_EQ(successInfo[1][0][0].m_ackTime,
        m_pktTxStartTimes[7] + m_pktDurations[7] + m_sifs + m_pktDurations[8] + 2 * tolerance,
        "Wrong Ack reception time");
    NS_TEST_ASSERT_MSG_EQ(successInfo[1][0][0].m_dequeueTime,
        successInfo[1][0][0].m_ackTime,
        "Dequeue and Ack should be at the same time");

    NS_TEST_ASSERT_MSG_GT_OR_EQ(successInfo[1][0][1].m_txStartTime,
        m_pktTxStartTimes[8] + m_pktDurations[8] + m_difs,
        "Packets should be TXed after enqueued");
    NS_TEST_ASSERT_MSG_LT_OR_EQ(successInfo[1][0][1].m_txStartTime,
        m_pktTxStartTimes[8] + m_pktDurations[8] + m_difs + tolerance + m_cwMin * m_slot,
        "Packet backoff slots should not exceed cwMin");
    NS_TEST_ASSERT_MSG_EQ(successInfo[1][0][1].m_txStartTime,
        m_pktTxStartTimes[9],
        "Wrong TX start time");
    NS_TEST_ASSERT_MSG_GT_OR_EQ(successInfo[1][0][1].m_ackTime,
        m_pktTxStartTimes[10] + m_pktDurations[10] + m_sifs + m_pktDurations[11],
        "Wrong Ack reception time");
    NS_TEST_ASSERT_MSG_LT_OR_EQ(successInfo[1][0][1].m_ackTime,
        m_pktTxStartTimes[10] + m_pktDurations[10] + m_sifs + m_pktDurations[11] + ((m_cwMin + 1) * 2 - 1) * m_slot + 2 * tolerance,
        "Wrong Ack reception time");
    NS_TEST_ASSERT_MSG_EQ(successInfo[1][0][1].m_dequeueTime,
        successInfo[1][0][1].m_ackTime,
        "Dequeue and Ack should be at the same time");

    NS_TEST_ASSERT_MSG_GT_OR_EQ(failureInfo[1][0].m_txStartTime,
        m_pktTxStartTimes[11] + m_pktDurations[11] + m_difs,
        "Packets should be TXed after enqueued");
    NS_TEST_ASSERT_MSG_LT_OR_EQ(failureInfo[1][0].m_txStartTime,
        m_pktTxStartTimes[11] + m_pktDurations[11] + m_difs + tolerance + m_cwMin * m_slot,
        "Packet backoff slots should not exceed cwMin");
    NS_TEST_ASSERT_MSG_EQ(failureInfo[1][0].m_txStartTime,
        m_pktTxStartTimes[12],
        "Wrong TX start time");
    NS_TEST_ASSERT_MSG_GT_OR_EQ(failureInfo[1][0].m_dequeueTime,
        m_pktTxStartTimes[18] + m_pktDurations[18],
        "Wrong Dequeue time for failed packet");
    NS_TEST_ASSERT_MSG_LT_OR_EQ(failureInfo[1][0].m_dequeueTime,
        m_pktTxStartTimes[18] + m_pktDurations[18] + m_sifs + m_slot + m_pktDurations[11],
        "Wrong Dequeue time for failed packet");
}

/**
 * \ingroup wifi-test
 * \brief Implements a test case to evaluate the transmission process of multiple WiFi
 * MAC Layer frames (MPDU). This testcase, unlike the other, uses .11be network to test the
 * handling of Block ACKs and Multi-Link Operation.
 *
 * This class extends the TestCase class to simulate and analyze the transmission of MPDUs,
 * from a STA to a AP, using single link. It specifically tests the WifiTxStatsHelper's
 * capability to store per-packet info including source node ID, Traffic ID, successful Link
 * ID, number of failures, MAC layer enqueue time, PHY layer transmission start time, MAC
 * layer ACK reception time, MAC layer dequeue time. It also tests the correctness of final
 * statistics including successes, retransmitted packets, retransmissions, average failures,
 * failed packets, mean end-to-end delay. Both per-node and total results are examined.
 */
// class WifiTxStatsHelperTestMultiLink : public TestCase {
//
// };

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief WifiTxStatsHelper Test Suite
 */
class WifiTxStatsHelperTestSuite : public TestSuite
{
public:
    WifiTxStatsHelperTestSuite();
};

WifiTxStatsHelperTestSuite::WifiTxStatsHelperTestSuite()
    : TestSuite("wifi-tx-stats-helper", Type::UNIT)
{
    AddTestCase(new WifiTxStatsHelperTestSingleLink(), TestCase::Duration::QUICK);
    // AddTestCase(new WifiTxStatsHelperTestMultiLink(), TestCase::Duration::QUICK);
}

static WifiTxStatsHelperTestSuite g_wifiTxStatsHelperTestSuite; ///< the test suite
