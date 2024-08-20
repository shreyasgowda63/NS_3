/*
 * Copyright (c)
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

#include "ns3/data-rate.h"
#include "ns3/dhcp6-header.h"
#include "ns3/dhcp6-helper.h"
#include "ns3/header-serialization-test.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv6-address-helper.h"
#include "ns3/log.h"
#include "ns3/ping-helper.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/simple-net-device.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/trace-helper.h"

using namespace ns3;

/**
 * \ingroup dhcp6
 * \defgroup dhcp6-test DHCPv6 module tests
 */

/**
 * \ingroup dhcp6-test
 * \ingroup tests
 *
 * \brief DHCPv6 header tests
 */
class Dhcp6TestCase : public TestCase
{
  public:
    Dhcp6TestCase();
    ~Dhcp6TestCase() override;

    /**
     * Triggered by an address lease on a client.
     * \param context The test name.
     * \param newAddress The leased address.
     */
    void LeaseObtained(std::string context, const Ipv6Address& newAddress);

  private:
    void DoRun() override;
    Ipv6Address m_leasedAddress[2]; //!< Address given to the nodes
};

Dhcp6TestCase::Dhcp6TestCase()
    : TestCase("Dhcp6 test case ")
{
}

Dhcp6TestCase::~Dhcp6TestCase()
{
}

void
Dhcp6TestCase::LeaseObtained(std::string context, const Ipv6Address& newAddress)
{
    uint8_t numericalContext = std::stoi(context, nullptr, 10);

    if (numericalContext >= 0 && numericalContext <= 2)
    {
        m_leasedAddress[numericalContext] = newAddress;
    }
}

void
Dhcp6TestCase::DoRun()
{
    /*Set up devices*/
    NodeContainer nodes;
    nodes.Create(3);

    NodeContainer net(nodes);

    SimpleNetDeviceHelper simpleNetDevice;
    simpleNetDevice.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    simpleNetDevice.SetDeviceAttribute("DataRate", DataRateValue(DataRate("5Mbps")));
    NetDeviceContainer devNet = simpleNetDevice.Install(net);

    InternetStackHelper internetv6;
    internetv6.SetIpv6StackInstall(true);
    internetv6.Install(nodes);

    Ipv6AddressHelper ipv6;
    Ipv6InterfaceContainer i = ipv6.Assign(devNet);

    Dhcp6Helper dhcpHelper;

    NetDeviceContainer serverNetDevices;
    serverNetDevices.Add(devNet.Get(0));
    ApplicationContainer dhcpServerApp = dhcpHelper.InstallDhcp6Server(serverNetDevices);

    Ptr<Dhcp6Server> server = dhcpHelper.GetDhcp6Server(devNet.Get(0));
    server->AddSubnet(Ipv6Address("2001:db8::"),
                      Ipv6Prefix(64),
                      Ipv6Address("2001:db8::1"),
                      Ipv6Address("2001:db8::ff"));

    // TODO: Add multiple address pool
    dhcpServerApp.Start(Seconds(0.0));
    dhcpServerApp.Stop(Seconds(20.0));

    NetDeviceContainer dhcpClientNetDevs;
    dhcpClientNetDevs.Add(devNet.Get(1));
    dhcpClientNetDevs.Add(devNet.Get(2));

    ApplicationContainer dhcpClientApps = dhcpHelper.InstallDhcp6Client(dhcpClientNetDevs);
    dhcpClientApps.Start(Seconds(1.0));
    dhcpClientApps.Stop(Seconds(20.0));

    dhcpClientApps.Get(0)->TraceConnect("NewLease",
                                        "0",
                                        MakeCallback(&Dhcp6TestCase::LeaseObtained, this));
    dhcpClientApps.Get(1)->TraceConnect("NewLease",
                                        "1",
                                        MakeCallback(&Dhcp6TestCase::LeaseObtained, this));

    Simulator::Stop(Seconds(21.0));
    Simulator::Run();

    // Client apps start at different times. How to check the address?
    // NS_TEST_ASSERT_MSG_EQ(m_leasedAddress[0],
    //                       Ipv6Address("2001:db8::1"),
    //                       m_leasedAddress[0] << " instead of "
    //                                          << "2001:db8::1");

    // NS_TEST_ASSERT_MSG_EQ(m_leasedAddress[1],
    //                       Ipv6Address("2001:db8::2"),
    //                       m_leasedAddress[1] << " instead of "
    //                                          << "2001:db8::2");

    Simulator::Destroy();
}

/**
 * \ingroup dhcp6-test
 * \ingroup tests
 *
 * \brief DHCPv6 TestSuite
 */
class Dhcp6TestSuite : public TestSuite
{
  public:
    Dhcp6TestSuite();
};

Dhcp6TestSuite::Dhcp6TestSuite()
    : TestSuite("dhcp6", Type::UNIT)
{
    AddTestCase(new Dhcp6TestCase, TestCase::Duration::QUICK);
}

static Dhcp6TestSuite dhcp6TestSuite; //!< Static variable for test initialization
