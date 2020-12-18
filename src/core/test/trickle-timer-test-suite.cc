/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Universita' di Firenze, Italy
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
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "ns3/trickle-timer.h"
#include "ns3/test.h"

/**
 * \file
 * \ingroup core-tests
 * \ingroup timer
 * \ingroup timer-tests
 *
 * Trickle Timer test suite.
 */

namespace ns3 {

namespace tests {


/**
 * \ingroup timer-tests
 *  Watchdog test
 */
class TrickleTimerTestCase : public TestCase
{
public:
  /** Constructor. */
  TrickleTimerTestCase ();
  virtual void DoRun (void);
  /**
   * Function to invoke when Watchdog expires.
   * \param arg The argument passed.
   */
  void ExpireTimer (int arg);
  bool m_expired;         //!< Flag for expired Watchdog
  Time m_expiredTime;     //!< Time when Watchdog expired
  int  m_expiredArgument; //!< Argument supplied to expired Watchdog
};

TrickleTimerTestCase::TrickleTimerTestCase ()
  : TestCase ("Check the Trickle Timer algorithm")
{}

void
TrickleTimerTestCase::ExpireTimer (int arg)
{
  m_expired = true;
  m_expiredTime = Simulator::Now ();
  m_expiredArgument = arg;

  std::cout << m_expiredTime << std::endl;
}

void
TrickleTimerTestCase::DoRun (void)
{
  m_expired = false;
  m_expiredArgument = 0;
  m_expiredTime = Seconds (0);

  TrickleTimer trickle (Time (1), 4, 1);
  trickle.SetFunction (&TrickleTimerTestCase::ExpireTimer, this);
  trickle.SetArguments (1);
  trickle.Enable ();
  trickle.Reset ();
//  trickle.Ping (MicroSeconds (10));
//  Simulator::Schedule (MicroSeconds ( 5), &Watchdog::Ping, &watchdog, MicroSeconds (20));
//  Simulator::Schedule (MicroSeconds (20), &Watchdog::Ping, &watchdog, MicroSeconds ( 2));
//  Simulator::Schedule (MicroSeconds (23), &Watchdog::Ping, &watchdog, MicroSeconds (17));
  Simulator::Stop (Time (5000));

  Simulator::Run ();
  Simulator::Destroy ();

  std::cout << m_expired << " at " << m_expiredTime << " with arg " << m_expiredArgument << std::endl;
//  NS_TEST_ASSERT_MSG_EQ (m_expired, true, "The timer did not expire ??");
//  NS_TEST_ASSERT_MSG_EQ (m_expiredTime, MicroSeconds (40), "The timer did not expire at the expected time ?");
//  NS_TEST_ASSERT_MSG_EQ (m_expiredArgument, 1, "We did not get the right argument");
}


/**
 * \ingroup timer-tests
 *  Trickle Timer test suite
 */
class TrickleTimerTestSuite : public TestSuite
{
public:
  /** Constructor. */
  TrickleTimerTestSuite ()
    : TestSuite ("trickle-timer")
  {
    AddTestCase (new TrickleTimerTestCase ());
  }
};

/**
 * \ingroup timer-tests
 * TrickleTimerTestSuite instance variable.
 */
static TrickleTimerTestSuite g_trickleTimerTestSuite;


}    // namespace tests

}  // namespace ns3
