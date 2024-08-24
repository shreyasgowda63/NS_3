/*
 * Copyright (c) 2024 NITK Surathkal
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
 * Author: Kavya Bhat <kavyabhat@gmail.com>
 *
 */

/*
 * Network layout:
 *
 * S0 is a DHCPv6 server. It has two interfaces, a CSMA interface and a Wifi
 * interface.
 * N0 and N1 are DHCPv6 clients. They have two interfaces - one CSMA and one
 * Wifi interface. Each interface obtains an address lease from the server.
 * R0 is a router with one CSMA interface, and is also set up as the Wifi access
 * point.
 *                ┌-------------------------------------------------┐
 *                | DHCPv6 Clients                                  |
 *                |                                                 |
 *                |                                Static address   |
 *                |                                 2001:db8::1     |
 *                |   ┌──────┐       ┌──────┐        ┌──────┐       |
 *                |   │  N0  │       │  N1  │        │  N2  │       |
 *                |   └──────┘       └──────┘        └──────┘       |
 *                |       │              │               │          |
 *                └-------│--------------│---------------│----------┘
 *  DHCPv6 Server         │              │               │
 *        ┌──────┐        │              │               │      ┌──────┐Router,
 *        │  S0  │────────┴──────────────┴───────────────┴──────│  R0  │AP node
 *        └──────┘                                              └──────┘
 * Notes:
 * 1. The DHCPv6 server is not assigned any static address as it operates only
 *    in the link-local domain.
 * 2. N2 has a statically assigned address to demonstrate the operation of the
 *    DHCPv6 Decline message.
 * 3. The server is usually on the router in practice, but we demonstrate in
 *    this example a standalone server.
 * 4. Linux uses fairly large values for address lifetimes (in thousands of
 *    seconds). In this example, we have set shorter lifetimes for the purpose
 *    of observing the Renew messages within a shorter simulation run.
 * 5. The nodes use two interfaces each for the purpose of demonstrating
 *    DHCPv6 operation when multiple interfaces are present on the client or
 *    server nodes.
 *
 * This example demonstrates how to set up a simple DHCPv6 server and two DHCPv6
 * clients. The clients begin to request an address lease using a Solicit
 * message only after receiving a Router Advertisement containing the 'M' bit
 * from the router, R0.
 *
 * The server responds with an Advertise message with all available address
 * offers, and the client sends a Request message to the server for these
 * addresses. The server then sends a Reply message to the client, which
 * performs Duplicate Address Detection to check if any other node on the link
 * already uses this address.
 * If the address is in use by any other node, the client sends a Decline
 * message to the server. If the address is not in use, the client begins using
 * this address.
 * At the end of the address lease lifetime, the client sends a Renew message
 * to the server, which renews the lease and allows the client to continue using
 * the same address.
 *
 * The user may enable packet traces in this example to observe the following
 * message exchanges:
 * 1. Solicit - Advertise - Request - Reply
 * 2. Solicit - Advertise - Request - Reply - Decline
 * 3. Renew - Reply
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
using namespace ns3::internet_apps;

NS_LOG_COMPONENT_DEFINE("Dhcp6Example");

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);

    bool verbose = false;
    bool enablePcap = false;
    cmd.AddValue("verbose", "Turn on the logs", verbose);
    cmd.AddValue("enablePcap", "Enable/Disable pcap file generation", enablePcap);

    cmd.Parse(argc, argv);
    GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));

    if (verbose)
    {
        LogComponentEnable("Dhcp6Server", LOG_LEVEL_INFO);
        LogComponentEnable("Dhcp6Client", LOG_LEVEL_INFO);
    }

    Time stopTime = Seconds(25.0);

    NS_LOG_INFO("Create nodes.");
    NodeContainer nonRouterNodes;
    nonRouterNodes.Create(4);
    Ptr<Node> router = CreateObject<Node>();
    NodeContainer all(nonRouterNodes, router);

    NS_LOG_INFO("Create channels.");
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("5Mbps"));
    csma.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer devices = csma.Install(all); // all nodes

    InternetStackHelper internetv6;
    internetv6.Install(all);

    NS_LOG_INFO("Create networks and assign IPv6 Addresses.");

    Ipv6AddressHelper ipv6;
    ipv6.SetBase(Ipv6Address("2001:db8::"), Ipv6Prefix(64));
    NetDeviceContainer nonRouterDevices;
    nonRouterDevices.Add(devices.Get(0)); // The server node, S0.
    nonRouterDevices.Add(devices.Get(1)); // The first client node, N0.
    nonRouterDevices.Add(devices.Get(2)); // The second client node, N1.
    nonRouterDevices.Add(devices.Get(3)); // The third client node, N2.
    Ipv6InterfaceContainer i = ipv6.AssignWithoutAddress(nonRouterDevices);

    NS_LOG_INFO("Assign static IP address to the third node.");
    Ptr<Ipv6> ipv6proto = nonRouterNodes.Get(3)->GetObject<Ipv6>();
    int32_t ifIndex = ipv6proto->GetInterfaceForDevice(devices.Get(3));
    Ipv6InterfaceAddress ipv6Addr =
        Ipv6InterfaceAddress(Ipv6Address("2001:db8::1"), Ipv6Prefix(128));
    ipv6proto->AddAddress(ifIndex, ipv6Addr);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");

    WifiHelper wifi;

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevices = wifi.Install(phy, mac, nonRouterNodes);

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevices = wifi.Install(phy, mac, router);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nonRouterNodes); // Clients and server
    mobility.Install(router);         // Router node

    NS_LOG_INFO("Assign static IP address to the router node.");
    NetDeviceContainer routerDevice;
    routerDevice.Add(devices.Get(4));   // CSMA interface of the node R0.
    routerDevice.Add(apDevices.Get(0)); // Wifi interface of the node R0.
    Ipv6InterfaceContainer r1 = ipv6.Assign(routerDevice);
    r1.SetForwarding(0, true);

    NS_LOG_INFO("Create Radvd applications.");
    RadvdHelper radvdHelper;

    /* Set up unsolicited RAs */
    radvdHelper.AddAnnouncedPrefix(r1.GetInterfaceIndex(0), Ipv6Address("2001:db8::0"), 64);
    radvdHelper.GetRadvdInterface(r1.GetInterfaceIndex(0))->SetManagedFlag(true);

    radvdHelper.AddAnnouncedPrefix(r1.GetInterfaceIndex(1), Ipv6Address("2001:cafe::0"), 64);
    radvdHelper.GetRadvdInterface(r1.GetInterfaceIndex(1))->SetManagedFlag(true);

    NS_LOG_INFO("Create DHCP applications.");
    Dhcp6Helper dhcp6Helper;

    NS_LOG_INFO("Set timers to desired values.");
    dhcp6Helper.SetServerAttribute("RenewTime", StringValue("10s"));
    dhcp6Helper.SetServerAttribute("RebindTime", StringValue("16s"));
    dhcp6Helper.SetServerAttribute("PreferredLifetime", StringValue("18s"));
    dhcp6Helper.SetServerAttribute("ValidLifetime", StringValue("20s"));

    // DHCP clients
    NetDeviceContainer dhcpClientNetDevs;
    dhcpClientNetDevs.Add(nonRouterDevices.Get(1));
    dhcpClientNetDevs.Add(nonRouterDevices.Get(2));

    dhcpClientNetDevs.Add(staDevices.Get(1));
    dhcpClientNetDevs.Add(staDevices.Get(2));

    ApplicationContainer dhcpClients = dhcp6Helper.InstallDhcp6Client(dhcpClientNetDevs);
    dhcpClients.Start(Seconds(1.0));
    dhcpClients.Stop(stopTime);

    // DHCP server
    NetDeviceContainer serverNetDevices;
    serverNetDevices.Add(nonRouterDevices.Get(0));
    serverNetDevices.Add(staDevices.Get(0));
    ApplicationContainer dhcpServerApp = dhcp6Helper.InstallDhcp6Server(serverNetDevices);

    Ptr<Dhcp6Server> server = DynamicCast<Dhcp6Server>(dhcpServerApp.Get(0));
    server->AddSubnet(Ipv6Address("2001:db8::"),
                      Ipv6Prefix(64),
                      Ipv6Address("2001:db8::1"),
                      Ipv6Address("2001:db8::ff"));

    dhcpServerApp.Start(Seconds(0.0));
    dhcpServerApp.Stop(stopTime);

    ApplicationContainer radvdApps = radvdHelper.Install(router);
    radvdApps.Start(Seconds(1.0));
    radvdApps.Stop(stopTime);

    Simulator::Stop(stopTime + Seconds(2.0));

    if (enablePcap)
    {
        csma.EnablePcapAll("dhcp6-csma");
        phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        phy.EnablePcap("dhcp6-wifi", staDevices);
    }

    NS_LOG_INFO("Run Simulation.");
    Simulator::Run();

    NS_LOG_INFO("Done.");
    Simulator::Destroy();
    return 0;
}
