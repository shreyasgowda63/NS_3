/*
 * Copyright (c) 2018 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Viyom Mittal <viyommittal@gmail.com>
 *         Vivek Jain <jain.vivek.anand@gmail.com>
 *         Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 *
 */

#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-recovery-ops.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpClassicRecoveryTestSuite");

/**
 * \brief Classic Recovery algorithm test
 */
class ClassicRecoveryTest : public TestCase
{
  public:
    /**
     * \brief Constructor.
     * \param cWnd Congestion window.
     * \param segmentSize Segment size.
     * \param ssThresh Slow Start Threshold.
     * \param dupAckCount Duplicate acknowledgement Threshold.
     * \param name Test description.
     */
    ClassicRecoveryTest(uint32_t cWnd,
                        uint32_t segmentSize,
                        uint32_t ssThresh,
                        uint32_t dupAckCount,
                        const std::string& name);

  private:
    void DoRun() override;

    uint32_t m_cWnd;        //!< Congestion window.
    uint32_t m_segmentSize; //!< Segment size.
    uint32_t m_ssThresh;    //!< Slow Start Threshold.
    uint32_t m_dupAckCount; //!< Duplicate acknowledgement Threshold.

    Ptr<TcpSocketState> m_state; //!< TCP socket state.
};

ClassicRecoveryTest::ClassicRecoveryTest(uint32_t cWnd,
                                         uint32_t segmentSize,
                                         uint32_t ssThresh,
                                         uint32_t dupAckCount,
                                         const std::string& name)
    : TestCase(name),
      m_cWnd(cWnd),
      m_segmentSize(segmentSize),
      m_ssThresh(ssThresh),
      m_dupAckCount(dupAckCount)
{
}

void
ClassicRecoveryTest::DoRun()
{
    m_state = CreateObject<TcpSocketState>();

    m_state->m_cWnd = m_cWnd;
    m_state->m_segmentSize = m_segmentSize;
    m_state->m_ssThresh = m_ssThresh;

    Ptr<TcpClassicRecovery> recovery = CreateObject<TcpClassicRecovery>();

    NS_TEST_ASSERT_MSG_EQ(recovery->GetName(),
                          "TcpClassicRecovery",
                          "The name of recovery used should be TcpClassicRecovery");

    recovery->EnterRecovery(m_state, m_dupAckCount, 1000, 0);
    NS_TEST_ASSERT_MSG_EQ(m_state->m_cWnd,
                          m_state->m_ssThresh,
                          "cWnd should be set to ssThresh on entering recovery");
    NS_TEST_ASSERT_MSG_EQ(
        m_state->m_cWndInfl,
        (m_state->m_ssThresh + (m_dupAckCount * m_state->m_segmentSize)),
        "cWndInfl should be set to (ssThresh + dupAckCount * segmentSize) on entering recovery");

    uint32_t cWndInflPrevious = m_state->m_cWndInfl;
    uint32_t cWndPrevious = m_state->m_cWnd;
    recovery->DoRecovery(m_state, 500);
    NS_TEST_ASSERT_MSG_EQ(
        m_state->m_cWndInfl,
        (cWndInflPrevious + m_state->m_segmentSize),
        "m_cWndInfl should be increased by one segmentSize on calling DoRecovery");
    NS_TEST_ASSERT_MSG_EQ(m_state->m_cWnd, cWndPrevious, "cWnd should not change in recovery");

    recovery->ExitRecovery(m_state);
    NS_TEST_ASSERT_MSG_EQ(m_state->m_cWndInfl,
                          m_state->m_ssThresh,
                          "cWndInfl should be set to ssThresh on exiting recovery");
    NS_TEST_ASSERT_MSG_EQ(m_state->m_cWnd,
                          m_state->m_ssThresh,
                          "cWnd should be set to ssThresh on exiting recovery");
}

/**
 * \ingroup internet-test
 *
 * \brief Classic Recovery TestSuite
 */
class ClassicRecoveryTestSuite : public TestSuite
{
  public:
    ClassicRecoveryTestSuite()
        : TestSuite("tcp-classic-recovery-test", Type::UNIT)
    {
        AddTestCase(new ClassicRecoveryTest(3000,
                                            500,
                                            2500,
                                            3,
                                            "Classic recovery test with 500 bytes segmentSize"),
                    TestCase::Duration::QUICK);
        AddTestCase(new ClassicRecoveryTest(3000,
                                            1000,
                                            2500,
                                            3,
                                            "Classic recovery test with 1000 bytes segmentSize"),
                    TestCase::Duration::QUICK);
        AddTestCase(new ClassicRecoveryTest(3000,
                                            500,
                                            2500,
                                            4,
                                            "Classic recovery test with 4 DupAck threshold"),
                    TestCase::Duration::QUICK);
        AddTestCase(new ClassicRecoveryTest(3000,
                                            500,
                                            1000,
                                            3,
                                            "Classic recovery test with 1000 bytes ssThresh"),
                    TestCase::Duration::QUICK);
        AddTestCase(new ClassicRecoveryTest(2500,
                                            500,
                                            2500,
                                            3,
                                            "Classic recovery test with same cWnd and ssThresh"),
                    TestCase::Duration::QUICK);
        AddTestCase(new ClassicRecoveryTest(1000,
                                            500,
                                            2500,
                                            3,
                                            "Classic recovery test with cWnd lesser than ssThresh"),
                    TestCase::Duration::QUICK);
    }
};

static ClassicRecoveryTestSuite
    g_TcpClassicRecoveryTest; //!< Static variable for test initialization
