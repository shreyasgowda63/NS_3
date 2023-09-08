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
namespace tests
{

/**
 * \ingroup proxy-tests
 * Simple test object which is aggregated with the proxy.
 */
class MainObject : public Object
{
  public:
    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("MainObject")
                                .SetParent<Object>()
                                .SetGroupName("Core")
                                .HideFromDocumentation()
                                .AddConstructor<MainObject>();
        return tid;
    }
};

/**
 * \ingroup proxy-tests
 * Simple test object which is to be proxied.
 */
class AggregatedObject : public Object
{
  public:
    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("AggregatedObject")
                                .SetParent<Object>()
                                .SetGroupName("Core")
                                .HideFromDocumentation()
                                .AddConstructor<AggregatedObject>();
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
    auto aggregated = CreateObject<AggregatedObject>();
    auto mainObjectA = CreateObject<MainObject>();
    auto mainObjectB = CreateObject<MainObject>();
    // The following would fail:
    // mainObjectA->AggregateObject(aggregated);
    // mainObjectB->AggregateObject(aggregated);

    // The following works:
    auto proxyA = CreateObject<Proxy<AggregatedObject>>(aggregated);
    mainObjectA->AggregateObject(proxyA);
    auto proxyB = CreateObject<Proxy<AggregatedObject>>(aggregated);
    mainObjectB->AggregateObject(proxyB);

    auto proxiedByA = mainObjectA->GetObject<Proxy<AggregatedObject>>()->PeekPointer();
    auto proxiedByB = mainObjectB->GetObject<Proxy<AggregatedObject>>()->PeekPointer();
    NS_TEST_ASSERT_MSG_NE(proxiedByA, nullptr, "Unable to get proxied object");
    NS_TEST_ASSERT_MSG_NE(proxiedByB, nullptr, "Unable to get proxied object");
    NS_TEST_ASSERT_MSG_EQ(proxiedByA, proxiedByB, "Proxied objects are different");
}

/**
 * \ingroup proxy-tests
 * Proxy Test Suite
 */
class ProxyTestSuite : public TestSuite
{
  public:
    /** Constructor. */
    ProxyTestSuite();
};

ProxyTestSuite::ProxyTestSuite()
    : TestSuite("proxy-test-suite")
{
    AddTestCase(new BasicTestCase);
}

/**
 * \ingroup proxy-tests
 *  ProxyTestSuite instance variable.
 */
static ProxyTestSuite g_proxyTestSuite;

} // namespace tests
} // namespace ns3
