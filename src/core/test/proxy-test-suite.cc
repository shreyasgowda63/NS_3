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
 * Simple test object to be proxied.
 */
class TestObject : public Object
{
  public:
    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("TestObject")
                                .SetParent<Object>()
                                .SetGroupName("Core")
                                .HideFromDocumentation()
                                .AddConstructor<TestObject>();
        return tid;
    }

    /** Constructor. */
    TestObject()
    {
    }
};

/**
 * \ingroup proxy-tests
 * Simple Alternate test object which is aggregated with the proxy of the Test Object.
 */
class AlternateTestObject : public Object
{
  public:
    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("TestObjectA")
                                .SetParent<Object>()
                                .SetGroupName("Core")
                                .HideFromDocumentation()
                                .AddConstructor<AlternateTestObject>();
        return tid;
    }

    /** Constructor. */
    AlternateTestObject()
    {
    }
};

/**
 * \ingroup proxy-tests
 * Simple Common test object which is aggregated with Test Object and Alternate Test Object.
 */
class CommonTestObject : public Object
{
  public:
    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("TestObjectA")
                                .SetParent<Object>()
                                .SetGroupName("Core")
                                .HideFromDocumentation()
                                .AddConstructor<CommonTestObject>();
        return tid;
    }

    /** Constructor. */
    CommonTestObject()
    {
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
    /** Destructor. */
    ~BasicTestCase() override;

  private:
    void DoRun() override;
    void DoTeardown() override;
};

BasicTestCase::BasicTestCase()
    : TestCase("Check if proxied object can be obtained")
{
}

BasicTestCase::~BasicTestCase()
{
}

void
BasicTestCase::DoTeardown()
{
}

void
BasicTestCase::DoRun()
{
    auto m_testObject = CreateObject<TestObject>();
    auto m_alternateTestObject = CreateObject<AlternateTestObject>();
    auto m_commonTestObject = CreateObject<CommonTestObject>();
    m_testObject->AggregateObject(m_commonTestObject);
    m_alternateTestObject->AggregateObject(m_commonTestObject);
    auto proxy = CreateObject<Proxy<TestObject>>(m_testObject);
    m_alternateTestObject->AggregateObject(proxy);
    bool flag = (m_alternateTestObject->GetObject<Proxy<TestObject>>() != nullptr);
    NS_TEST_ASSERT_MSG_EQ(flag, true, "Unable to get proxied object");
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