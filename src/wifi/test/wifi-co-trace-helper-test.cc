/*
 * Copyright (c) 2009 University of Washington
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
 */

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/eht-configuration.h"
#include "ns3/enum.h"
#include "ns3/he-phy.h"
#include "ns3/integer.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/names.h"
#include "ns3/node-list.h"
#include "ns3/object.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"
#include "ns3/wifi-co-trace-helper.h"
#include "ns3/wifi-helper.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-mpdu.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy-state.h"
#include "ns3/wifi-psdu.h"
#include "ns3/yans-wifi-helper.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("WifiCoTraceHelperTest");

/**
 * \ingroup wifi-test
 * \brief It's a base class with some utility methods for other test cases in this file.
 */
class WifiCoTraceHelperBaseTestCase : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param testName Name of a test.
     */
    WifiCoTraceHelperBaseTestCase(std::string testName)
        : TestCase(testName)
    {
        m_wificohelper.Stop(m_simulationStop);
    }

    /**
     * Destructor
     */
    ~WifiCoTraceHelperBaseTestCase() override
    {
    }

  protected:
    /**
     * A helper function that sends a number of packets from a particular node.
     *
     * @param num Number of packets to send.
     * @param fromNodeId Id of sender node.
     * @param toNodeId Id of receiver node.
     *
     */
    void SendPackets(size_t num, size_t fromNodeId, size_t toNodeId);

    /**
     * It's a callback function that measures the time-duration a state occupies on a Phy. It should
     * be attached to the WifiPhyStateHelper's trace source to receive callbacks. It updates
     * sumOfDurations that's received by reference.
     *
     * @param forState A state for which duration should be updated. Ignore duration for all other
     * callback states.
     * @param sumOfDurations A variable passed by reference which is updated by this function.
     * @param callbackStart Instant at which the duration starts.
     * @param callbackDuration Magnitude of duration.
     * @param callbackState The state for which callback is made.
     */
    void MeasureExpectedDuration(WifiPhyState forState,
                                 Time& sumOfDurations,
                                 Time callbackStart,
                                 Time callbackDuration,
                                 WifiPhyState callbackState);

    /**
     * Get WifiPhyStateHelper attached to a node and its Phy.
     *
     * @param nodeId Id of a node.
     * @param phyId Id of a PHY.
     * @return WifiPhyStateHelper corresponding to nodeId, phyId.
     */
    Ptr<WifiPhyStateHelper> GetPhyStateHelper(size_t nodeId, size_t phyId);

    /**
     * It gets the channel occupancy of a link on a node measured by WifiCoTraceHelper.
     *
     * @param nodeId Id of a node.
     * @param linkId Id of link.
     * @return Statistics measured by WifiCoTraceHelper corresponding to nodeId,linkId.
     */
    const std::map<WifiPhyState, Time>& GetChannelOccupancy(size_t nodeId, size_t linkId);

    /**
     * It asserts that the two channel occupancy values match with each other.
     *
     * @param actual Channel occupancy measured by WifiCoTraceHelper
     * @param expected Channel occupancy measured by this test class from trace sources.
     */
    void CheckChannelOccupancy(const std::map<WifiPhyState, Time>& actual,
                               const std::map<WifiPhyState, Time>& expected);

    /**
     * A helper function that creates a PacketSocketClient.
     *
     * @param sockAddr A PacketSocketAddr
     * @param pktSize Packet size in bytes.
     * @param interval Time interval between generation of consecutive packets.
     * @param start Instant at which the application should start.
     *
     * @return Returns client application.
     */
    Ptr<PacketSocketClient> GetClientApplication(const PacketSocketAddress& sockAddr,
                                                 const std::size_t pktSize,
                                                 const Time& interval,
                                                 const Time& start);
    /**
     * We follow the convention that nodeId 0 is AP and rest are non-AP in infrastructure mode. This
     * method will install PacketSocketServer on AP and PacketSocketClient on non-AP.
     */
    void InstallPacketSocketServerAndClient();

    /**
     * A helper function that sets tid-to-link mapping.
     *
     * @param mapping A string that configure tid-to-link mapping.
     */
    void ConfigureTidToLinkMapping(std::string mapping);

    Time m_simulationStop{Seconds(5.0)}; ///< Instant at which simulation should stop.
    WifiCoTraceHelper m_wificohelper; ///< Instance of WifiCoTraceHelper tested by this test case.
    NodeContainer m_nodes;            ///< Container of all nodes instantiated in this test case.
    NetDeviceContainer m_devices;     ///< Container of all devices instantiated in this test case.
    std::vector<Ptr<PacketSocketClient>>
        m_clientApps; ///< vector of m_clientApps installed on non-AP nodes.
};

const std::map<WifiPhyState, Time>&
WifiCoTraceHelperBaseTestCase::GetChannelOccupancy(size_t nodeId, size_t linkId)
{
    auto& devRecords = m_wificohelper.GetDeviceRecords();
    auto senderRecord = std::find_if(devRecords.begin(), devRecords.end(), [nodeId](auto& x) {
        return x.m_nodeId == nodeId;
    });

    NS_ASSERT_MSG((senderRecord != devRecords.end()), "Expected statistics for nodeId: " << nodeId);

    auto stats = senderRecord->m_linkStateDurations.find(linkId);
    auto end_itr = senderRecord->m_linkStateDurations.end();
    NS_ASSERT_MSG((stats != end_itr),
                  "Expected statistics at nodeId: " << nodeId << ", linkId: " << linkId);

    return stats->second;
}

Ptr<WifiPhyStateHelper>
WifiCoTraceHelperBaseTestCase::GetPhyStateHelper(size_t nodeId, size_t phyId)
{
    auto wifiDevice = DynamicCast<WifiNetDevice>(m_devices.Get(nodeId));
    auto wifiPhyStateHelper = wifiDevice->GetPhy(phyId)->GetState();
    return wifiPhyStateHelper;
}

void
WifiCoTraceHelperBaseTestCase::MeasureExpectedDuration(WifiPhyState forState,
                                                       Time& expected,
                                                       Time callbackStart,
                                                       Time callbackDuration,
                                                       WifiPhyState callbackState)
{
    if (forState == callbackState)
    {
        expected += callbackDuration;
    }
}

void
WifiCoTraceHelperBaseTestCase::SendPackets(size_t num, size_t fromNodeId, size_t toNodeId)
{
    auto dev = DynamicCast<WifiNetDevice>(m_devices.Get(fromNodeId));
    auto from = dev->GetMac()->GetAddress();
    auto to = DynamicCast<WifiNetDevice>(m_devices.Get(toNodeId))->GetMac()->GetAddress();

    size_t pktSizeInBytes = 1000;
    for (size_t i = 0; i < num; i++)
    {
        dev->GetMac()->Enqueue(Create<Packet>(pktSizeInBytes), to, from);
    }
}

void
WifiCoTraceHelperBaseTestCase::CheckChannelOccupancy(const std::map<WifiPhyState, Time>& actual,
                                                     const std::map<WifiPhyState, Time>& expected)
{
    for (const WifiPhyState s :
         {WifiPhyState::TX, WifiPhyState::RX, WifiPhyState::IDLE, WifiPhyState::CCA_BUSY})
    {
        if (expected.at(s) == Seconds(0))
        {
            NS_TEST_ASSERT_MSG_EQ((actual.find(s) == actual.end()),
                                  true,
                                  "State " << s << " shouldn't be measured");
        }
        else
        {
            auto it = actual.find(s);
            NS_TEST_ASSERT_MSG_EQ((it != actual.end()),
                                  true,
                                  "State " << s << " should be measured");
            NS_TEST_ASSERT_MSG_EQ(it->second, expected.at(s), "Measured duration should be same");
        }
    }
}

Ptr<PacketSocketClient>
WifiCoTraceHelperBaseTestCase::GetClientApplication(const PacketSocketAddress& sockAddr,
                                                    const std::size_t pktSize,
                                                    const Time& interval,
                                                    const Time& start)
{
    auto client = CreateObject<PacketSocketClient>();
    client->SetAttribute("PacketSize", UintegerValue(pktSize));
    client->SetAttribute("MaxPackets", UintegerValue(0));
    client->SetAttribute("Interval", TimeValue(interval));
    client->SetAttribute("Priority", UintegerValue(0));
    client->SetRemote(sockAddr);
    client->SetStartTime(start);
    return client;
}

void
WifiCoTraceHelperBaseTestCase::ConfigureTidToLinkMapping(std::string mapping)
{
    for (size_t i = 0; i < m_devices.GetN(); i++)
    {
        auto wifiDevice = DynamicCast<WifiNetDevice>(m_devices.Get(i));
        wifiDevice->GetMac()->GetEhtConfiguration()->SetAttribute(
            "TidToLinkMappingNegSupport",
            EnumValue(WifiTidToLinkMappingNegSupport::ANY_LINK_SET));

        wifiDevice->GetMac()->GetEhtConfiguration()->SetAttribute("TidToLinkMappingUl",
                                                                  StringValue(mapping));
    }
}

void
WifiCoTraceHelperBaseTestCase::InstallPacketSocketServerAndClient()
{
    // Install packet socket on all nodes
    PacketSocketHelper packetSocket;
    packetSocket.Install(m_nodes);

    // Install server on AP
    size_t apNodeId = 0;
    Ptr<WifiNetDevice> apDevice = DynamicCast<WifiNetDevice>(m_devices.Get(apNodeId));

    PacketSocketAddress srvAddr;
    srvAddr.SetSingleDevice(apDevice->GetIfIndex());
    srvAddr.SetProtocol(1);
    auto psServer = CreateObject<PacketSocketServer>();
    psServer->SetLocal(srvAddr);
    (m_nodes.Get(apNodeId))->AddApplication(psServer);
    psServer->SetStartTime(Seconds(0));
    psServer->SetStopTime(m_simulationStop);

    // Install client on non-AP
    for (size_t staNodeId = 1; staNodeId < m_devices.GetN(); ++staNodeId)
    {
        Ptr<WifiNetDevice> staDevice = DynamicCast<WifiNetDevice>(m_devices.Get(staNodeId));

        PacketSocketAddress sockAddr;
        sockAddr.SetSingleDevice(staDevice->GetIfIndex());
        sockAddr.SetPhysicalAddress(apDevice->GetAddress());
        sockAddr.SetProtocol(1);

        auto clientApp = GetClientApplication(sockAddr, 1000, MicroSeconds(100), Seconds(0.0));
        staDevice->GetNode()->AddApplication(clientApp);
        m_clientApps.push_back(clientApp);
    }
}

/**
 * \ingroup wifi-test
 * \brief Send one packet from one WifiNetDevice to other.
 *
 * This test case configures two ad-hoc Wi-Fi STAs. One STA sends a single
 * packet to the other at time instant 1 second.  It enables WifCoTraceHelper
 * on both STAs. It asserts the statistics measured by the helper equals statistic collected
 * independently from trace sources.
 *
 */
class SendOnePacketTestCase : public WifiCoTraceHelperBaseTestCase
{
  public:
    /** Constructor. */
    SendOnePacketTestCase();
    /** Destructor. */
    ~SendOnePacketTestCase() override;

  private:
    /** Executes test case and assertions. */
    void DoRun() override;
    /** Setup test case's scenario. */
    void DoSetup() override;
    /** Clean resources after DoRun */
    void DoTeardown() override;
};

SendOnePacketTestCase::SendOnePacketTestCase()
    : WifiCoTraceHelperBaseTestCase(
          "SendOnePacketTestCase: Send one packet from one WifiNetDevice to other.")
{
}

/**
 * This destructor does nothing but we include it as a reminder that
 * the test case should clean up after itself
 */
SendOnePacketTestCase::~SendOnePacketTestCase()
{
}

void
SendOnePacketTestCase::DoRun()
{
    // The network is setup such that there are only two nodes. Each node is a single-link device
    // (SLD). One node transmits a packet to another.
    const size_t numDevices = 2;
    const size_t numPhys = 1;

    std::map<WifiPhyState, Time> expectedDurations[numDevices][numPhys];

    /**
     * Calculate expected durations through trace callback.
     */
    for (size_t i = 0; i < numDevices; i++)
    {
        for (size_t j = 0; j < numPhys; j++)
        {
            for (const WifiPhyState s :
                 {WifiPhyState::TX, WifiPhyState::RX, WifiPhyState::IDLE, WifiPhyState::CCA_BUSY})
            {
                auto phyHelper = GetPhyStateHelper(i, j);

                auto callback = MakeCallback(&SendOnePacketTestCase::MeasureExpectedDuration, this)
                                    .Bind(s, std::ref(expectedDurations[i][j][s]));
                phyHelper->TraceConnectWithoutContext("State", callback);
            }
        }
    }

    Simulator::Schedule(Seconds(1.0),
                        &SendOnePacketTestCase::SendPackets,
                        this,
                        1,
                        0 /*from*/,
                        1 /*to*/);

    Simulator::Stop(m_simulationStop);

    // Assert that Start and Stop Times of WifiCoHelper function correctly by defining three
    // helpers: before, during and after the packet transmission.
    WifiCoTraceHelper traceBeforeTx{Seconds(0), Seconds(1)};
    traceBeforeTx.Enable(m_nodes);

    WifiCoTraceHelper traceDuringTx{Seconds(1), Seconds(1.5)};
    traceDuringTx.Enable(m_nodes);

    WifiCoTraceHelper traceAfterTx;
    traceAfterTx.Start(Seconds(1.5));
    traceAfterTx.Stop(Seconds(2));
    traceAfterTx.Enable(m_nodes);

    Simulator::Run();
    Simulator::Destroy();

    std::cout << "## SendOnePacketTestCase ##\n";
    m_wificohelper.PrintStatistics(std::cout);

    /* Assert that measured durations are correct. */
    for (size_t d = 0; d < numDevices; d++)
    {
        for (size_t p = 0; p < numPhys; p++)
        {
            // Match the map returned by WifiCoTraceHelper with Expected values.
            auto& actual = GetChannelOccupancy(d, p);
            auto& expected = expectedDurations[d][p];
            CheckChannelOccupancy(actual, expected);
        }
    }

    std::cout << "## SendOnePacketTestCase: BeforeTX ##\n";
    traceBeforeTx.PrintStatistics(std::cout);
    std::cout << "## SendOnePacketTestCase: DuringTX ##\n";
    traceDuringTx.PrintStatistics(std::cout);
    std::cout << "## SendOnePacketTestCase: AfterTX ##\n";
    traceAfterTx.PrintStatistics(std::cout);

    /* Assert that durations are measured between start and stop times only */
    // Assert duration during packet transmission. Remove 1 sec from IDLE time because the helper
    // starts after 1 second.
    std::map<WifiPhyState, Time> expectedDuringTx{expectedDurations[0][0]};
    expectedDuringTx[WifiPhyState::IDLE] -= Seconds(1);
    CheckChannelOccupancy(traceDuringTx.GetDeviceRecords().at(0).m_linkStateDurations.at(0),
                          expectedDuringTx);

    // Only IDLE duration should be measured before transmission.
    std::map<WifiPhyState, Time> expectedBeforeTx;
    expectedBeforeTx[WifiPhyState::IDLE] = Seconds(1);
    NS_TEST_ASSERT_MSG_EQ(
        (traceBeforeTx.GetDeviceRecords().at(0).m_linkStateDurations.at(0) == expectedBeforeTx),
        true,
        "Only IDLE duration should be measured before transmission");
    NS_TEST_ASSERT_MSG_EQ(
        (traceBeforeTx.GetDeviceRecords().at(1).m_linkStateDurations.at(0) == expectedBeforeTx),
        true,
        "Only IDLE duration should be measured before transmission");

    // Nothing should be measured after transmission due to lack of simulation events.
    NS_TEST_ASSERT_MSG_EQ(traceAfterTx.GetDeviceRecords().at(0).m_linkStateDurations.empty(),
                          true,
                          "Durations shouldn't be measured after TX");
    NS_TEST_ASSERT_MSG_EQ(traceAfterTx.GetDeviceRecords().at(1).m_linkStateDurations.empty(),
                          true,
                          "Durations shouldn't be measured after TX");
}

void
SendOnePacketTestCase::DoSetup()
{
    // LogComponentEnable("WifiCoTraceHelper", LOG_LEVEL_DEBUG);

    uint32_t nWifi = 2;
    m_nodes.Create(nWifi);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);

    uint8_t linkId = 0;
    wifi.SetRemoteStationManager(linkId,
                                 "ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("OfdmRate12Mbps"),
                                 "ControlMode",
                                 StringValue("OfdmRate12Mbps"));

    mac.SetType("ns3::AdhocWifiMac");
    m_devices = wifi.Install(phy, mac, m_nodes);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    auto distance = 0.1;
    positionAlloc->Add(Vector(distance, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(m_nodes);

    // Adding nodes to wificohelper
    m_wificohelper.Enable(m_nodes);
}

void
SendOnePacketTestCase::DoTeardown()
{
    for (size_t i = 0; i < m_nodes.GetN(); i++)
    {
        m_nodes.Get(i)->Dispose();
    }
}

/**
 * \ingroup wifi-test
 * \brief Trace channel occupany on each link of MLDs
 *
 * This test case configures one AP and one non-AP MLMR with three links. It generates symmetric
 * uplink traffic on link#1 and link#2 only. It asserts that the traced durations are similar on
 * link#1 and link#2 and dissimilar on link#0.
 *
 */
class MLOTestCase : public WifiCoTraceHelperBaseTestCase
{
  public:
    /** Constructor. */
    MLOTestCase();
    /** Destructor. */
    ~MLOTestCase() override;

  private:
    void DoRun() override;
    void DoSetup() override;
    void DoTeardown() override;
};

MLOTestCase::MLOTestCase()
    : WifiCoTraceHelperBaseTestCase(
          "MLOTestCase: Track channel occupancy on multiple links of a multi-link device (MLD).")
{
}

/**
 * This destructor does nothing but we include it as a reminder that
 * the test case should clean up after itself
 */
MLOTestCase::~MLOTestCase()
{
}

void
MLOTestCase::DoRun()
{
    // The network is setup such that there is a AP and an uplink STA. Each node is a multi-link
    // device (MLD) with three links.
    const size_t numDevices = 2;
    const size_t numPhys = 3;

    std::map<WifiPhyState, Time> expectedDurations[numDevices][numPhys];

    for (size_t i = 0; i < numDevices; i++)
    {
        for (size_t j = 0; j < numPhys; j++)
        {
            for (const WifiPhyState s :
                 {WifiPhyState::TX, WifiPhyState::RX, WifiPhyState::IDLE, WifiPhyState::CCA_BUSY})
            {
                auto phyHelper = GetPhyStateHelper(i, j);

                auto callback = MakeCallback(&MLOTestCase::MeasureExpectedDuration, this)
                                    .Bind(s, std::ref(expectedDurations[i][j][s]));
                phyHelper->TraceConnectWithoutContext("State", callback);
            }
        }
    }

    m_clientApps.at(0)->SetStartTime(Seconds(1.0));
    /*
     * Set TID to 1 so that packets are transmitted on linkId 1.
     */
    Simulator::Schedule(Seconds(1.0),
                        &PacketSocketClient::SetAttribute,
                        m_clientApps.at(0),
                        "Priority",
                        UintegerValue(1));
    /*
     * Change TID from 1 to 2 so that packets are transmitted on linkId 2.
     */
    Simulator::Schedule(Seconds(2.0),
                        &PacketSocketClient::SetAttribute,
                        m_clientApps.at(0),
                        "Priority",
                        UintegerValue(2));

    Simulator::Stop(m_simulationStop);

    Simulator::Run();
    Simulator::Destroy();

    std::cout << "## MLOTestCase ##\n";
    m_wificohelper.PrintStatistics(std::cout);

    for (size_t d = 0; d < numDevices; d++)
    {
        for (size_t p = 0; p < numPhys; p++)
        {
            // Match the map returned by WifiCoTraceHelper with Expected values.
            auto& actual = GetChannelOccupancy(d, p);
            auto& expected = expectedDurations[d][p];
            CheckChannelOccupancy(actual, expected);
        }
    }

    size_t clientNodeId = 1;
    auto durationOnLink0 = GetChannelOccupancy(clientNodeId, 0);
    auto durationOnLink1 = GetChannelOccupancy(clientNodeId, 1);
    auto durationOnLink2 = GetChannelOccupancy(clientNodeId, 2);

    /* Assert that TX durations on Link1 and Link2 should be similar due to symmetry.*/
    NS_TEST_ASSERT_MSG_EQ_TOL(durationOnLink1[WifiPhyState::TX],
                              durationOnLink2[WifiPhyState::TX],
                              MilliSeconds(1.0),
                              "TX durations should be similar");
    /* Assert that TX duration on Link1 should be substantially more than Link0 because traffic
     * isn't transmitted on Link0 */
    NS_TEST_ASSERT_MSG_EQ(
        (durationOnLink1[WifiPhyState::TX] - durationOnLink0[WifiPhyState::TX] > MilliSeconds(1.0)),
        true,
        "TX durations shouldn't be similar");

    // Assert that statistics after reset should be cleared.
    m_wificohelper.Reset();
    NS_TEST_ASSERT_MSG_EQ((m_wificohelper.GetDeviceRecords().size()),
                          numDevices,
                          "Placeholder for device records shouldn't be cleared");

    std::cout << "## MLOTestCase:Reset ##\n";
    m_wificohelper.PrintStatistics(std::cout);

    for (size_t d = 0; d < numDevices; d++)
    {
        for (size_t p = 0; p < numPhys; p++)
        {
            // Match the map returned by WifiCoTraceHelper with Expected values.
            auto& statistics = m_wificohelper.GetDeviceRecords().at(d).m_linkStateDurations;
            NS_TEST_ASSERT_MSG_EQ((statistics.empty()), true, "Statistics should be cleared");
        }
    }
}

void
MLOTestCase::DoSetup()
{
    // LogComponentEnable("WifiCoTraceHelper", LOG_LEVEL_INFO);
    RngSeedManager::SetSeed(1);
    m_simulationStop = (Seconds(3.0));

    NodeContainer ap;
    ap.Create(1);

    uint32_t nWifi = 1;
    NodeContainer sta;
    sta.Create(nWifi);

    m_nodes.Add(ap);
    m_nodes.Add(sta);

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211be);

    // Create multiple spectrum channels
    Ptr<MultiModelSpectrumChannel> spectrumChannel2_4Ghz =
        CreateObject<MultiModelSpectrumChannel>();
    Ptr<MultiModelSpectrumChannel> spectrumChannel5Ghz = CreateObject<MultiModelSpectrumChannel>();
    Ptr<MultiModelSpectrumChannel> spectrumChannel6Ghz = CreateObject<MultiModelSpectrumChannel>();

    // SpectrumWifiPhyHelper (3 links)
    SpectrumWifiPhyHelper phy(3);
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.AddChannel(spectrumChannel2_4Ghz, WIFI_SPECTRUM_2_4_GHZ);
    phy.AddChannel(spectrumChannel5Ghz, WIFI_SPECTRUM_5_GHZ);
    phy.AddChannel(spectrumChannel6Ghz, WIFI_SPECTRUM_6_GHZ);

    // configure operating channel for each link
    phy.Set(0, "ChannelSettings", StringValue("{0, 20, BAND_2_4GHZ, 0}"));
    phy.Set(1, "ChannelSettings", StringValue("{0, 20, BAND_5GHZ, 0}"));
    phy.Set(2, "ChannelSettings", StringValue("{0, 20, BAND_6GHZ, 0}"));

    // configure rate manager for each link
    wifi.SetRemoteStationManager(1,
                                 "ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("EhtMcs9"),
                                 "ControlMode",
                                 StringValue("EhtMcs9"));
    wifi.SetRemoteStationManager(2,
                                 "ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("EhtMcs9"),
                                 "ControlMode",
                                 StringValue("EhtMcs9"));

    uint8_t linkId = 0;
    wifi.SetRemoteStationManager(linkId,
                                 "ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("EhtMcs9"),
                                 "ControlMode",
                                 StringValue("EhtMcs9"));

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    m_devices.Add(wifi.Install(phy, mac, ap));
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    m_devices.Add(wifi.Install(phy, mac, sta));

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    auto distance = 0.1;
    positionAlloc->Add(Vector(distance, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(m_nodes);

    ConfigureTidToLinkMapping("0 0;1 1; 2,3,4,5,6,7 2");
    InstallPacketSocketServerAndClient();
    m_clientApps.at(0)->SetAttribute("Interval", TimeValue(MilliSeconds(100.0)));

    m_wificohelper.Stop(m_simulationStop);
    m_wificohelper.Enable(m_nodes);
}

void
MLOTestCase::DoTeardown()
{
    for (size_t i = 0; i < m_nodes.GetN(); i++)
    {
        m_nodes.Get(i)->Dispose();
    }
}

/**
 * \ingroup wifi-test
 * \brief One AP and one uplink STA in infrastructure mode.
 *
 * This test case configures one AP and one STA on a single link. It configures the STA to send
 * traffic to AP at a saturated offered load. It configures WifiCoTraceHelper on both AP and STA.
 */
class SaturatedOfferedLoadTestCase : public WifiCoTraceHelperBaseTestCase
{
  public:
    /** Constructor. */
    SaturatedOfferedLoadTestCase();
    /** Destructor. */
    ~SaturatedOfferedLoadTestCase() override;

  private:
    void DoRun() override;
    void DoSetup() override;
    void DoTeardown() override;
};

SaturatedOfferedLoadTestCase::SaturatedOfferedLoadTestCase()
    : WifiCoTraceHelperBaseTestCase(
          "SaturatedOfferedLoadTestCase: A saturated wifi network with one AP and an uplink STA")
{
}

/**
 * This destructor does nothing but we include it as a reminder that
 * the test case should clean up after itself
 */
SaturatedOfferedLoadTestCase::~SaturatedOfferedLoadTestCase()
{
}

void
SaturatedOfferedLoadTestCase::DoRun()
{
    // The network is setup such that there is one uplink STA (NodeId 1) and one AP (NodeId 0).
    // Each node is a single-link device (SLD). Application installed on STA generates a saturating
    // workload.
    const size_t numDevices = 2;
    const size_t numPhys = 1;

    std::map<WifiPhyState, Time> expectedDurations[numDevices][numPhys];

    for (size_t i = 0; i < numDevices; i++)
    {
        for (size_t j = 0; j < numPhys; j++)
        {
            for (const WifiPhyState s :
                 {WifiPhyState::TX, WifiPhyState::RX, WifiPhyState::IDLE, WifiPhyState::CCA_BUSY})
            {
                auto phyHelper = GetPhyStateHelper(i, j);

                auto callback =
                    MakeCallback(&SaturatedOfferedLoadTestCase::MeasureExpectedDuration, this)
                        .Bind(s, std::ref(expectedDurations[i][j][s]));
                phyHelper->TraceConnectWithoutContext("State", callback);
            }
        }
    }

    Simulator::Stop(m_simulationStop);
    Simulator::Run();
    Simulator::Destroy();

    std::cout << "## SaturatedOfferedLoadTestCase ##\n";
    m_wificohelper.PrintStatistics(std::cout);

    for (size_t d = 0; d < numDevices; d++)
    {
        for (size_t p = 0; p < numPhys; p++)
        {
            // Match the map returned by WifiCoTraceHelper with Expected values.
            auto& actual = GetChannelOccupancy(d, p);
            auto& expected = expectedDurations[d][p];
            CheckChannelOccupancy(actual, expected);
        }
    }
}

void
SaturatedOfferedLoadTestCase::DoSetup()
{
    // LogComponentEnable("WifiCoTraceHelper", LOG_LEVEL_INFO);
    m_simulationStop = Seconds(1.0);

    uint32_t nWifi = 1;

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nWifi);
    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");

    WifiHelper wifi;

    NetDeviceContainer staDevices;
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    staDevices = wifi.Install(phy, mac, wifiStaNodes);

    NetDeviceContainer apDevices;
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    apDevices = wifi.Install(phy, mac, wifiApNode);

    MobilityHelper mobility;

    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(5.0),
                                  "DeltaY",
                                  DoubleValue(10.0),
                                  "GridWidth",
                                  UintegerValue(3),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiStaNodes);
    mobility.Install(wifiApNode);

    m_nodes.Add(wifiStaNodes);
    m_nodes.Add(wifiApNode);

    m_devices.Add(staDevices);
    m_devices.Add(apDevices);

    InstallPacketSocketServerAndClient();
    m_clientApps.at(0)->SetAttribute(
        "Interval",
        TimeValue(MicroSeconds(20))); // Overriding to generate a Saturated load.
    m_wificohelper.Enable(m_nodes);
}

void
SaturatedOfferedLoadTestCase::DoTeardown()
{
    for (size_t i = 0; i < m_nodes.GetN(); i++)
    {
        m_nodes.Get(i)->Dispose();
    }
}

/**
 * \ingroup wifi-test
 * \brief LinkId of non-AP MLD changes after multilink setup.
 *
 * This test case configures one AP MLD with three links And one non-AP MLD with two links. non-AP
 * MLD renames its link after multilink setup. It assets that WifiCoTraceHelper should capture
 * statistics of the renamed link.
 */
class LinkRenameTestCase : public WifiCoTraceHelperBaseTestCase
{
  public:
    /** Constructor. */
    LinkRenameTestCase();
    /** Destructor. */
    ~LinkRenameTestCase() override;

  private:
    void DoRun() override;
    void DoSetup() override;
    void DoTeardown() override;
};

LinkRenameTestCase::LinkRenameTestCase()
    : WifiCoTraceHelperBaseTestCase(
          "LinkRenameTestCase: WifiCoTraceHelper should record statistics under new LinkId.")
{
}

/**
 * This destructor does nothing but we include it as a reminder that
 * the test case should clean up after itself
 */
LinkRenameTestCase::~LinkRenameTestCase()
{
}

void
LinkRenameTestCase::DoRun()
{
    auto staNodeId = 1;

    Simulator::Stop(m_simulationStop);

    Simulator::Run();
    Simulator::Destroy();

    std::cout << "## LinkRenameTestCase ##\n";
    m_wificohelper.PrintStatistics(std::cout);

    auto staStatistics = m_wificohelper.GetDeviceRecords().at(staNodeId).m_linkStateDurations;

    // Note that sta has only two phys. So, a linkId of '2' is created by renaming one of the
    // existing links.
    auto renamedLinkId = 2;
    auto it = staStatistics.find(renamedLinkId);
    NS_TEST_ASSERT_MSG_EQ((it != staStatistics.end()),
                          true,
                          "Link: " << renamedLinkId << " isn't present at nonAP MLD");
}

void
LinkRenameTestCase::DoSetup()
{
    // LogComponentEnable("WifiCoTraceHelper", LOG_LEVEL_DEBUG);

    m_simulationStop = Seconds(3);

    NodeContainer ap;
    ap.Create(1);

    uint32_t nWifi = 1;
    NodeContainer sta;
    sta.Create(nWifi);

    m_nodes.Add(ap);
    m_nodes.Add(sta);

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");

    // Create multiple spectrum channels
    Ptr<MultiModelSpectrumChannel> spectrumChannel2_4Ghz =
        CreateObject<MultiModelSpectrumChannel>();
    Ptr<MultiModelSpectrumChannel> spectrumChannel5Ghz = CreateObject<MultiModelSpectrumChannel>();

    // SpectrumWifiPhyHelper (2 links)
    SpectrumWifiPhyHelper nonApPhyHelper(2);
    nonApPhyHelper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    nonApPhyHelper.AddChannel(spectrumChannel5Ghz, WIFI_SPECTRUM_5_GHZ);
    nonApPhyHelper.AddChannel(spectrumChannel5Ghz, WIFI_SPECTRUM_5_GHZ);

    // configure operating channel for each link
    nonApPhyHelper.Set(0, "ChannelSettings", StringValue("{42, 80, BAND_5GHZ, 0}"));
    nonApPhyHelper.Set(1, "ChannelSettings", StringValue("{0, 80, BAND_5GHZ, 0}"));

    nonApPhyHelper.Set("FixedPhyBand", BooleanValue(true));

    WifiHelper nonApWifiHelper;
    nonApWifiHelper.SetStandard(WIFI_STANDARD_80211be);

    // configure rate manager for each link
    uint8_t firstLinkId = 0;
    nonApWifiHelper.SetRemoteStationManager(firstLinkId,
                                            "ns3::ConstantRateWifiManager",
                                            "DataMode",
                                            StringValue("EhtMcs9"),
                                            "ControlMode",
                                            StringValue("EhtMcs9"));
    nonApWifiHelper.SetRemoteStationManager(1,
                                            "ns3::ConstantRateWifiManager",
                                            "DataMode",
                                            StringValue("EhtMcs9"),
                                            "ControlMode",
                                            StringValue("EhtMcs9"));

    SpectrumWifiPhyHelper apPhyHelper(3);
    apPhyHelper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    apPhyHelper.AddChannel(spectrumChannel2_4Ghz, WIFI_SPECTRUM_2_4_GHZ);
    apPhyHelper.AddChannel(spectrumChannel5Ghz, WIFI_SPECTRUM_5_GHZ);
    apPhyHelper.AddChannel(spectrumChannel5Ghz, WIFI_SPECTRUM_5_GHZ);

    // configure operating channel for each link
    apPhyHelper.Set(0, "ChannelSettings", StringValue("{6, 40, BAND_2_4GHZ, 0}"));
    apPhyHelper.Set(1, "ChannelSettings", StringValue("{42, 80, BAND_5GHZ, 0}"));
    apPhyHelper.Set(2, "ChannelSettings", StringValue("{0, 0, BAND_5GHZ, 0}"));

    apPhyHelper.Set("FixedPhyBand", BooleanValue(true));

    WifiHelper apWifiHelper;
    apWifiHelper.SetStandard(WIFI_STANDARD_80211be);

    apWifiHelper.SetRemoteStationManager(firstLinkId,
                                         "ns3::ConstantRateWifiManager",
                                         "DataMode",
                                         StringValue("EhtMcs9"),
                                         "ControlMode",
                                         StringValue("EhtMcs9"));
    apWifiHelper.SetRemoteStationManager(1,
                                         "ns3::ConstantRateWifiManager",
                                         "DataMode",
                                         StringValue("EhtMcs9"),
                                         "ControlMode",
                                         StringValue("EhtMcs9"));
    apWifiHelper.SetRemoteStationManager(2,
                                         "ns3::ConstantRateWifiManager",
                                         "DataMode",
                                         StringValue("EhtMcs9"),
                                         "ControlMode",
                                         StringValue("EhtMcs9"));

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid), "BeaconGeneration", BooleanValue(true));
    m_devices.Add(apWifiHelper.Install(apPhyHelper, mac, ap));

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(true));
    m_devices.Add(nonApWifiHelper.Install(nonApPhyHelper, mac, sta));

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    auto distance = 0.1;
    positionAlloc->Add(Vector(distance, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(m_nodes);

    InstallPacketSocketServerAndClient();
    m_clientApps.at(0)->SetAttribute("Interval", TimeValue(Seconds(0.25)));
    m_wificohelper.Enable(m_nodes);
}

void
LinkRenameTestCase::DoTeardown()
{
    for (size_t i = 0; i < m_nodes.GetN(); i++)
    {
        m_nodes.Get(i)->Dispose();
    }
}

/**
 * \ingroup wifi-test
 * \brief Main Phy switches between links for a non-AP EMLSR.
 *
 * This test case configures one AP MLD with two links and one EMLSR non-AP MLD with two links.
 * Phy#1 is the main Phy. Uplink traffic is distributed symmetrically on the two links.
 * WifiCoTraceHelper should capture similar TX statistics on both Link#0 and Link#1 even though the
 * traffic is transmitted only on main phy#1.
 *
 */
class EMLSRTestCase : public WifiCoTraceHelperBaseTestCase
{
  public:
    /** Constructor. */
    EMLSRTestCase();
    /** Destructor. */
    ~EMLSRTestCase() override;

  private:
    void DoRun() override;
    void DoSetup() override;
    void DoTeardown() override;
};

EMLSRTestCase::EMLSRTestCase()
    : WifiCoTraceHelperBaseTestCase("EMLSRTestCase: WifiCoTraceHelper should record statistics by "
                                    "LinkId instead of PhyId of a non-AP EMLSR.")
{
}

/**
 * This destructor does nothing but we include it as a reminder that
 * the test case should clean up after itself
 */
EMLSRTestCase::~EMLSRTestCase()
{
}

void
EMLSRTestCase::DoRun()
{
    size_t clientNodeId = 1;

    Simulator::Stop(m_simulationStop); /* It's value is 3 Seconds */
    m_wificohelper.Start(Seconds(0.0));
    m_wificohelper.Stop(Seconds(3.0));

    Simulator::Run();
    Simulator::Destroy();

    std::cout << "## EMLSRTestCase ##\n";
    m_wificohelper.PrintStatistics(std::cout);

    /* Main Phy is 1 and there are two links on the client generating uplink traffic.
       Assert that TX duration is recorded for both Link#0 and Link#1.
     */
    auto staStatistics = m_wificohelper.GetDeviceRecords().at(clientNodeId).m_linkStateDurations;
    auto durationOnLink0 = GetChannelOccupancy(clientNodeId, 0);
    auto durationOnLink1 = GetChannelOccupancy(clientNodeId, 1);

    /* Assert that TX durations on Link0 and Link1 should be similar.*/
    NS_TEST_ASSERT_MSG_EQ((durationOnLink0[WifiPhyState::TX] > MilliSeconds(10.0)),
                          true,
                          "TX duration on Link#0 isn't recorded as expected.");
    NS_TEST_ASSERT_MSG_EQ((durationOnLink1[WifiPhyState::TX] > MilliSeconds(10.0)),
                          true,
                          "TX duration on Link#1 isn't recorded as expected.");

    /* Assert that total duration on all links is close enough to simulation duration */
    Time totalSimulationDuration = Seconds(3.0);
    Time sumOnLink0;
    Time sumOnLink1;
    for (auto& it : durationOnLink0)
    {
        sumOnLink0 += it.second;
    }
    for (auto& it : durationOnLink1)
    {
        sumOnLink1 += it.second;
    }
    NS_TEST_ASSERT_MSG_EQ_TOL(
        sumOnLink0,
        totalSimulationDuration,
        MilliSeconds(75),
        "Sum of states' durations on Link#0 isn't close to simulation duration.");
    NS_TEST_ASSERT_MSG_EQ_TOL(
        sumOnLink1,
        totalSimulationDuration,
        MilliSeconds(75),
        "Sum of states' durations on Link#1 isn't close to simulation duration.");
}

void
EMLSRTestCase::DoSetup()
{
    // LogComponentEnable("WifiCoTraceHelper", LOG_LEVEL_INFO);
    RngSeedManager::SetSeed(2);
    m_simulationStop = Seconds(3.0);

    NodeContainer ap;
    ap.Create(1);

    uint32_t nWifi = 1;
    NodeContainer sta;
    sta.Create(nWifi);

    m_nodes.Add(ap);
    m_nodes.Add(sta);

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211be);
    wifi.ConfigEhtOptions("EmlsrActivated", BooleanValue(true));
    wifi.ConfigEhtOptions("TransitionTimeout", TimeValue(MicroSeconds(1024)));
    wifi.ConfigEhtOptions("MediumSyncDuration", TimeValue(MicroSeconds(3200)));
    wifi.ConfigEhtOptions("MsdOfdmEdThreshold", IntegerValue(-72));
    wifi.ConfigEhtOptions("MsdMaxNTxops", UintegerValue(0));

    // Create multiple spectrum channels
    Ptr<MultiModelSpectrumChannel> spectrumChannel2_4Ghz =
        CreateObject<MultiModelSpectrumChannel>();
    Ptr<MultiModelSpectrumChannel> spectrumChannel5Ghz = CreateObject<MultiModelSpectrumChannel>();

    // SpectrumWifiPhyHelper (2 links)
    SpectrumWifiPhyHelper phy(2);
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.AddChannel(spectrumChannel2_4Ghz, WIFI_SPECTRUM_2_4_GHZ);
    phy.AddChannel(spectrumChannel5Ghz, WIFI_SPECTRUM_5_GHZ);
    phy.Set("ChannelSwitchDelay", TimeValue(MicroSeconds(100)));

    // configure operating channel for each link
    phy.Set(0, "ChannelSettings", StringValue("{0, 20, BAND_2_4GHZ, 0}"));
    phy.Set(1, "ChannelSettings", StringValue("{0, 20, BAND_5GHZ, 0}"));

    // configure rate manager for each link
    uint8_t linkId = 0;
    wifi.SetRemoteStationManager(linkId,
                                 "ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("EhtMcs9"),
                                 "ControlMode",
                                 StringValue("EhtMcs9"));
    wifi.SetRemoteStationManager(1,
                                 "ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("EhtMcs9"),
                                 "ControlMode",
                                 StringValue("EhtMcs9"));

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    m_devices.Add(wifi.Install(phy, mac, ap));

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    mac.SetEmlsrManager("ns3::DefaultEmlsrManager",
                        "EmlsrLinkSet",
                        StringValue("0,1"), // enable EMLSR on all links
                        "MainPhyId",
                        UintegerValue(1),
                        "EmlsrPaddingDelay",
                        TimeValue(MicroSeconds(32)),
                        "EmlsrTransitionDelay",
                        TimeValue(MicroSeconds(128)),
                        "SwitchAuxPhy",
                        BooleanValue(true),
                        "AuxPhyChannelWidth",
                        UintegerValue(20));
    m_devices.Add(wifi.Install(phy, mac, sta));

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    auto distance = 0.1;
    positionAlloc->Add(Vector(distance, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(m_nodes);

    InstallPacketSocketServerAndClient();
    m_clientApps.at(0)->SetAttribute("Interval", TimeValue(MilliSeconds(25.0)));

    m_wificohelper.Enable(m_nodes);
}

void
EMLSRTestCase::DoTeardown()
{
    for (size_t i = 0; i < m_nodes.GetN(); i++)
    {
        m_nodes.Get(i)->Dispose();
    }
}

/**
 * \ingroup wifi-test
 * \brief Wifi Channel Occupancy Helper Test Suite
 */
class WifiCoHelperTestSuite : public TestSuite
{
  public:
    /** Constructor. */
    WifiCoHelperTestSuite();
};

WifiCoHelperTestSuite::WifiCoHelperTestSuite()
    : TestSuite("wifi-co-trace-helper", Type::UNIT)
{
    AddTestCase(new SendOnePacketTestCase, TestCase::Duration::QUICK);
    AddTestCase(new MLOTestCase, TestCase::Duration::QUICK);
    AddTestCase(new LinkRenameTestCase, TestCase::Duration::QUICK);
    AddTestCase(new EMLSRTestCase, TestCase::Duration::QUICK);
    AddTestCase(new SaturatedOfferedLoadTestCase, TestCase::Duration::QUICK);
}

/**
 * \ingroup wifi-test
 * WifiCoHelperTestSuite instance variable.
 */
static WifiCoHelperTestSuite g_WifiCoHelperTestSuite;
