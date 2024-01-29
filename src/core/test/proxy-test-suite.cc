/*
 * Copyright (c) 2023 NITK Surathkal
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
 * Authors: Raghuram Kannan <raghuramkannan400@gmail.com>
 */

#include "ns3/proxy.h"
#include "ns3/test.h"

/**
 * \file
 * \ingroup core-tests
 * \ingroup proxy
 * \ingroup proxy-tests
 * Proxy object test suite.
 */

/**
 * \ingroup core-tests
 * \defgroup proxy-tests Proxy object test suite
 */

namespace ns3
{

/**
 * \ingroup proxy-tests
 * Simple test object which is to be proxied.
 */
class ProxyTestAggregatedObject : public Object
{
  public:
    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ProxyTestAggregatedObject")
                                .SetParent<Object>()
                                .SetGroupName("Core")
                                .AddConstructor<ProxyTestAggregatedObject>();
        return tid;
    }
};

NS_OBJECT_TEMPLATE_CLASS_DEFINE(Proxy, ProxyTestAggregatedObject);

namespace tests
{

/**
 * \ingroup proxy-tests
 * Simple test object with another object aggregated through a Proxy.
 */
class ProxyTestMainObject : public Object
{
  public:
    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ProxyTestMainObject")
                                .SetParent<Object>()
                                .SetGroupName("Core")
                                .AddConstructor<ProxyTestMainObject>();
        return tid;
    }
};

/**
 * \ingroup proxy-tests
 * Checks if the proxied object can be obtained.
 */
class BasicTestCase : public TestCase
{
  public:
    /** Constructor. */
    BasicTestCase();

  private:
    void DoRun() override;
};

BasicTestCase::BasicTestCase()
    : TestCase("Check if proxied object can be obtained")
{
}

void
BasicTestCase::DoRun()
{
    auto aggregated = CreateObject<ProxyTestAggregatedObject>();
    auto mainObjectA = CreateObject<ProxyTestMainObject>();
    auto mainObjectB = CreateObject<ProxyTestMainObject>();
    // The following would fail:
    // mainObjectA->AggregateObject(aggregated);
    // mainObjectB->AggregateObject(aggregated);

    // The following works:
    auto proxyA = CreateObject<Proxy<ProxyTestAggregatedObject>>(aggregated);
    mainObjectA->AggregateObject(proxyA);
    auto proxyB = CreateObject<Proxy<ProxyTestAggregatedObject>>(aggregated);
    mainObjectB->AggregateObject(proxyB);

    auto proxiedByA = mainObjectA->GetObject<Proxy<ProxyTestAggregatedObject>>()->GetPointer();
    auto proxiedByB = mainObjectB->GetObject<Proxy<ProxyTestAggregatedObject>>()->GetPointer();

    auto dunno = mainObjectA->GetObject<Proxy<ProxyTestAggregatedObject>>();
    std::cout << "dunno object is of type " << dunno->GetTypeId() << std::endl;

    NS_TEST_ASSERT_MSG_NE(proxiedByA, nullptr, "Unable to get proxied object");
    NS_TEST_ASSERT_MSG_NE(proxiedByB, nullptr, "Unable to get proxied object");
    NS_TEST_ASSERT_MSG_EQ(proxiedByA, proxiedByB, "Proxied objects are different");

    // the following should also work.
    auto aggregatedOneWay = CreateObject<ProxyTestAggregatedObject>();
    auto mainObjectOneWayA = CreateObject<ProxyTestMainObject>();
    auto mainObjectOneWayB = CreateObject<ProxyTestMainObject>();

    mainObjectOneWayA->AggregateObjectOneWay(aggregatedOneWay);
    mainObjectOneWayB->AggregateObjectOneWay(aggregatedOneWay);

    auto proxiedByANew = mainObjectOneWayA->GetObject<ProxyTestAggregatedObject>();
    auto proxiedByBNew = mainObjectOneWayB->GetObject<ProxyTestAggregatedObject>();

    std::cout << proxiedByANew << std::endl;
    std::cout << proxiedByBNew << std::endl;

    auto iter = mainObjectOneWayA->GetAggregateIterator();
    while (iter.HasNext())
    {
        std::cout << "aggregated " << iter.Next()->GetInstanceTypeId() << std::endl;
    }

    std::cout << proxiedByANew->GetTypeId() << std::endl;
    std::cout << proxiedByBNew->GetTypeId() << std::endl;
}

/**
 * \ingroup proxy-tests
 * Proxy Test Suite
 */
class ProxyTestSuite : public TestSuite
{
  public:
    ProxyTestSuite();
};

ProxyTestSuite::ProxyTestSuite()
    : TestSuite("proxy-test-suite")
{
    AddTestCase(new BasicTestCase);
}

/**
 * \ingroup proxy-tests
 * ProxyTestSuite instance variable.
 */
static ProxyTestSuite g_proxyTestSuite;

} // namespace tests

} // namespace ns3
