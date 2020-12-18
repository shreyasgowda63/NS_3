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
#include <vector>
#include <numeric>
#include <algorithm>

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
 *  TrickleTimer test
 */
class TrickleTimerTestCase : public TestCase
{
public:
  /** Constructor. */
  TrickleTimerTestCase ();
  virtual void DoRun (void);
  /**
   * Function to invoke when TrickleTimer expires.
   * \param arg The argument passed.
   */
  void ExpireTimer (int arg);
  bool m_expired;         //!< Flag for expired TrickleTimer
  std::vector<Time> m_expiredTimes;     //!< Time when TrickleTimer expired
  int  m_expiredArgument; //!< Argument supplied to expired TrickleTimer

  /**
   * Function to signal that the transient is over
   */
  void TransientOver (void);

  bool m_enableDataCollection;     //!< Collect data if true
};

TrickleTimerTestCase::TrickleTimerTestCase ()
  : TestCase ("Check the Trickle Timer algorithm")
{}

void
TrickleTimerTestCase::ExpireTimer (int arg)
{
  if (m_enableDataCollection==false)
    {
      return;
    }

  m_expired = true;
  m_expiredTimes.push_back (Simulator::Now ());
  m_expiredArgument = arg;

  // std::cout << Simulator::Now ().GetSeconds () << std::endl;
}

void
TrickleTimerTestCase::TransientOver (void)
{
  std::cout << "Transient is over" << std::endl;
  m_enableDataCollection = true;
}

void
TrickleTimerTestCase::DoRun (void)
{
  m_expired = false;
  m_expiredArgument = 0;
//  m_expiredTime = Seconds (0);
  m_enableDataCollection = false;
//  Time unit = Time (1);
  Time unit = Seconds (1);

  TrickleTimer trickle (unit, 4, 1);
  trickle.SetFunction (&TrickleTimerTestCase::ExpireTimer, this);
  trickle.SetArguments (1);
  trickle.Enable ();
  trickle.Reset ();

  // The transient is over at (exp2(doublings +1) -1) * MinInterval (worst case).
  Simulator::Schedule (unit*31, &TrickleTimerTestCase::TransientOver, this);
//  trickle.Ping (MicroSeconds (10));
//  Simulator::Schedule (MicroSeconds ( 5), &Watchdog::Ping, &watchdog, MicroSeconds (20));
//  Simulator::Schedule (MicroSeconds (20), &Watchdog::Ping, &watchdog, MicroSeconds ( 2));
//  Simulator::Schedule (MicroSeconds (23), &Watchdog::Ping, &watchdog, MicroSeconds (17));
  Simulator::Stop (unit * 50000);

  Simulator::Run ();
  Simulator::Destroy ();

  std::vector<Time> expirationFrequency;

  std::cout << "got " << m_expiredTimes.size () << " elements" << std::endl;

  expirationFrequency.resize (m_expiredTimes.size ());
  std::adjacent_difference (m_expiredTimes.begin (), m_expiredTimes.end (), expirationFrequency.begin ());
  expirationFrequency.erase (expirationFrequency.begin ());

  Time min = *std::min_element (expirationFrequency.begin (), expirationFrequency.end ());
  Time max = *std::max_element (expirationFrequency.begin (), expirationFrequency.end ());

  std::cout << "min is " << min/unit << " - max is " << max/unit << std::endl;

//  std::cout << m_expired << " at " << m_expiredTime << " with arg " << m_expiredArgument << std::endl;
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
