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
#include "ns3/smart-event.h"
#include "ns3/test.h"

/**
 * \file
 * \ingroup core-tests
 * \ingroup timer
 * \ingroup timer-tests
 * SmartEvent test suite.
 */

namespace ns3 {

  namespace tests {
    

/**
 * \ingroup timer-tests
 *  SmartEvent test
 */
class SmartEventTestCase : public TestCase
{
public:
  /** Constructor. */
  SmartEventTestCase ();
  virtual void DoRun (void);
  /**
   * Function to invoke when SmartEvent expires.
   * \param index The SmartEvent index
   * \param value The argument passed.
   */
  void Expire (int index, int value);
  bool m_expired[3];         //!< Flag for expired SmartEvent
  Time m_expiredTime[3];     //!< Time when SmartEvent expired
  int m_expiredArgument[3];  //!< Argument supplied to expired SmartEvent
};

SmartEventTestCase::SmartEventTestCase()
  : TestCase ("Check that we can change appropriately a SmartEvent")
{
}

void
SmartEventTestCase::Expire (int index, int value)
{
  m_expired[index] = true;
  m_expiredTime[index] = Simulator::Now ();
  m_expiredArgument[index] = value;
}

void
SmartEventTestCase::DoRun (void)
{
  m_expired[0] = false;
  m_expired[1] = false;
  m_expired[2] = false;
  m_expiredArgument[0] = 0;
  m_expiredArgument[1] = 0;
  m_expiredArgument[2] = 0;
  m_expiredTime[0] = Seconds (0);
  m_expiredTime[1] = Seconds (0);
  m_expiredTime[2] = Seconds (0);


  SmartEvent normal;
  normal.SetFunction (&SmartEventTestCase::Expire, this);
  normal.SetArguments (0, 10);
  normal.SetNewExpiration (Seconds (10));

  SmartEvent delayed;
  delayed.SetFunction (&SmartEventTestCase::Expire, this);
  delayed.SetArguments (1, 20);
  delayed.SetNewExpiration (Seconds (10));
  Simulator::Schedule (Seconds ( 5), &SmartEvent::SetNewExpiration, &delayed, Seconds (15));

  SmartEvent advanced;
  advanced.SetFunction (&SmartEventTestCase::Expire, this);
  advanced.SetArguments (2, 30);
  advanced.SetNewExpiration (Seconds (10));
  Simulator::Schedule (Seconds ( 2), &SmartEvent::SetNewExpiration, &advanced, Seconds (3));

  Simulator::Run ();
  Simulator::Destroy ();

  NS_TEST_ASSERT_MSG_EQ (m_expired[0], true, "The normal timer did not expire ??");
  NS_TEST_ASSERT_MSG_EQ (m_expiredTime[0], Seconds (10), "The normal timer did not expire at the expected time ?");
  NS_TEST_ASSERT_MSG_EQ (m_expiredArgument[0], 10, "We did not get the right argument for the normal timer");

  NS_TEST_ASSERT_MSG_EQ (m_expired[1], true, "The delayed timer did not expire ??");
  NS_TEST_ASSERT_MSG_EQ (m_expiredTime[1], Seconds (20), "The delayed timer did not expire at the expected time ?");
  NS_TEST_ASSERT_MSG_EQ (m_expiredArgument[1], 20, "We did not get the right argument for the delayed timer");

  NS_TEST_ASSERT_MSG_EQ (m_expired[2], true, "The shrunken timer did not expire ??");
  NS_TEST_ASSERT_MSG_EQ (m_expiredTime[2], Seconds (5), "The shrunken timer did not expire at the expected time ?");
  NS_TEST_ASSERT_MSG_EQ (m_expiredArgument[2], 30, "We did not get the right argument for the shrunken timer");
}


/**
 * \ingroup timer-tests
 *  SmartEvent test suite
 */
class SmartEventTestSuite : public TestSuite
{
public:
  /** Constructor. */
  SmartEventTestSuite()
    : TestSuite ("smart-event")
  {
    AddTestCase (new SmartEventTestCase ());
  }
};

/**
 * \ingroup timer-tests
 * SmartEventTestSuite instance variable.
 */
static SmartEventTestSuite g_smartEventTestSuite;


  }  // namespace tests

}  // namespace ns3
