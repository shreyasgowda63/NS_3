/*
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

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/wifi-helper.h"
#include "ns3/yans-wifi-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Dhcp6Example");

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);

    bool verbose = false;
    bool tracing = false;
    cmd.AddValue("verbose", "Turn on the logs", verbose);
    cmd.AddValue("tracing", "Turn on tracing", tracing);

    cmd.Parse(argc, argv);
    GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));

    if (verbose)
    {
        LogComponentEnable("DhcpServer", LOG_LEVEL_ALL);
        LogComponentEnable("DhcpClient", LOG_LEVEL_ALL);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    }

    Time stopTime = Seconds(25.0);

    NS_LOG_INFO("Create nodes.");
    NodeContainer nodes;
    NodeContainer router;
    nodes.Create(4);
    // router.Create(2);

    NodeContainer net(nodes);

    NS_LOG_INFO("Create channels.");
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("5Mbps"));
    csma.SetChannelAttribute("Delay", StringValue("2ms"));
    csma.SetDeviceAttribute("Mtu", UintegerValue(1500));
    NetDeviceContainer devNet = csma.Install(net);

    InternetStackHelper internetv6;
    internetv6.Install(nodes);

    Ipv6AddressHelper ipv6;
    Ipv6InterfaceContainer i = ipv6.AssignWithoutAddress(devNet);

    NS_LOG_INFO("Assign static IP address to the third node.");
    Ptr<Ipv6> ipv6proto = devNet.Get(3)->GetNode()->GetObject<Ipv6>();
    int32_t ifIndex = 0;
    ifIndex = ipv6proto->GetInterfaceForDevice(devNet.Get(3));
    Ipv6InterfaceAddress ipv6Addr =
        Ipv6InterfaceAddress(Ipv6Address("2001:db8::1"), Ipv6Prefix(128));
    ipv6proto->AddAddress(ifIndex, ipv6Addr);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");

    WifiHelper wifi;

    NetDeviceContainer staDevices;
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    staDevices = wifi.Install(phy, mac, nodes);

    NetDeviceContainer apDevices;
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    apDevices = wifi.Install(phy, mac, nodes.Get(0));

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

    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds",
                              RectangleValue(Rectangle(-50, 50, -50, 50)));
    mobility.Install(nodes);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes.Get(0));

    NS_LOG_INFO("Create DHCP applications.");
    Dhcp6Helper dhcp6Helper;

    // DHCP clients
    NetDeviceContainer dhcpClientNetDevs;
    dhcpClientNetDevs.Add(devNet.Get(0));
    dhcpClientNetDevs.Add(devNet.Get(1));

    dhcpClientNetDevs.Add(staDevices.Get(0));
    dhcpClientNetDevs.Add(staDevices.Get(1));

    ApplicationContainer dhcpClients = dhcp6Helper.InstallDhcp6Client(dhcpClientNetDevs);
    dhcpClients.Start(Seconds(1.0));
    dhcpClients.Stop(stopTime);

    // DHCP server
    std::vector<Ptr<NetDevice>> serverNetDevices;
    serverNetDevices.push_back(devNet.Get(2));
    serverNetDevices.push_back(staDevices.Get(2));
    ApplicationContainer dhcpServerApp = dhcp6Helper.InstallDhcp6Server(serverNetDevices);

    Ptr<Dhcp6Server> server = dhcp6Helper.GetDhcp6Server(devNet.Get(2));
    server->AddSubnet(Ipv6Address("2001:db8::"),
                      Ipv6Prefix(64),
                      Ipv6Address("2001:db8::1"),
                      Ipv6Address("2001:db8::ff"));

    dhcpServerApp.Start(Seconds(0.0));
    dhcpServerApp.Stop(stopTime);

    Simulator::Stop(stopTime + Seconds(2.0));

    if (tracing)
    {
        csma.EnablePcapAll("dhcp6-csma");
        phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        phy.EnablePcap("dhcp6-wifi", staDevices.Get(0));
    }

    NS_LOG_INFO("Run Simulation.");
    Simulator::Run();

    NS_LOG_INFO("Done.");
    Simulator::Destroy();
    return 0;
}
