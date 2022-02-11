/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Lawrence Livermore National Laboratory
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
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */
#include "ns3/test.h"

#include "ns3/simulator.h"
#include "ns3/simulator-adapter.h"
#include "ns3/global-value.h"

/**
 * \file
 * \ingroup core-tests
 * SimulatorAdapter test suite.
 */


namespace ns3 {

namespace tests {

/**
 * Base class for SimulatorAdapter test classes.
 * For the functions they specialize this base class
 * provides fallback implementations which log the forwarding
 * through each SimulatorAdapter test class.
 */
class SimulatorAdapterTestBase : public SimulatorAdapter
{
public:
  /**
   * Get the type Id.
   * \return the object TypeId
   */
  static TypeId GetTypeId ();

  /** Default constructor. */
  SimulatorAdapterTestBase () : m_name ("AdapterBase") { };
  /**
   * Construct with the name from the derived class.
   * \param [in] nm The derived class tag name.
   */
  SimulatorAdapterTestBase (std::string nm) : m_name (nm) { };
  
  // Inherited - these will be overridden by the derived classes
  virtual TypeId  GetInstanceTypeId () const;
  virtual EventId Schedule (Time const &delay, EventImpl *event);
  virtual EventId ScheduleNow (EventImpl *event);
  virtual void    ScheduleWithContext (uint32_t context, Time const &delay, EventImpl *event);

protected:
  /** The tag name. */
  std::string m_name;
};

NS_OBJECT_ENSURE_REGISTERED (SimulatorAdapterTestBase);

/* static */
TypeId
SimulatorAdapterTestBase::GetTypeId ()
{
  static TypeId tid = TypeId ("SimulatorAdapterTestBase")
    .SetParent<SimulatorAdapter> ()
    .SetGroupName ("Core")
    .AddConstructor<SimulatorAdapterTestBase> ()
    ;
  return tid;
}

TypeId
SimulatorAdapterTestBase::GetInstanceTypeId () const
{
  return GetTypeId ();
}

EventId
SimulatorAdapterTestBase::Schedule (Time const &delay, EventImpl *event)
{
  std::cout << m_name << "::Schedule(): pass" << std::endl;
  return m_simulator->Schedule (delay, event);
}

EventId
SimulatorAdapterTestBase::ScheduleNow (EventImpl * event)
{
  std::cout << m_name << "::ScheduleNow(): pass" << std::endl;
  return m_simulator->ScheduleNow (event);
}

void
SimulatorAdapterTestBase::ScheduleWithContext (uint32_t context, Time const &delay, EventImpl *event)
{
  std::cout << m_name << "::ScheduleWithContext(): pass" << std::endl;
  m_simulator->ScheduleWithContext (context, delay, event);
}

/** SimulatorAdapterTestBase with custom Schedule(). */
class SimulatorAdapterTestA : public SimulatorAdapterTestBase
{
public:
  /**
   * Get the type Id.
   * \return the object TypeId
   */
  static TypeId GetTypeId ();
  /** Default constructor. */
  SimulatorAdapterTestA () : SimulatorAdapterTestBase ("AdapterA") { };

  // Inherited
  virtual TypeId  GetInstanceTypeId () const;
  EventId Schedule (Time const & delay, EventImpl *event);
};

NS_OBJECT_ENSURE_REGISTERED (SimulatorAdapterTestA);

/* static */
TypeId
SimulatorAdapterTestA::GetTypeId ()
{
  static TypeId tid = TypeId ("SimulatorAdapterTestA")
    .SetParent<SimulatorAdapterTestBase> ()
    .SetGroupName ("Core")
    .AddConstructor<SimulatorAdapterTestA> ()
    ;
  return tid;
}

TypeId
SimulatorAdapterTestA::GetInstanceTypeId () const
{
  return GetTypeId ();
}

EventId
SimulatorAdapterTestA::Schedule (Time const &delay, EventImpl *event)
{
  std::cout << m_name << "::Schedule() for " << delay.As ()
            << " with event @" << std::hex << event
            << std::endl;
  return m_simulator->Schedule (delay, event);
}

/** SimulatorAdapterTestBase with custom ScheduleNow() */
class SimulatorAdapterTestB : public SimulatorAdapterTestBase
{
public:
  /**
   * Get the type Id.
   * \return the object TypeId
   */
  static TypeId GetTypeId ();
  /** Default constructor. */
  SimulatorAdapterTestB () : SimulatorAdapterTestBase ("AdapterB") { };

  // Inherited
  virtual TypeId  GetInstanceTypeId () const;
  virtual EventId ScheduleNow (EventImpl *event);
};

NS_OBJECT_ENSURE_REGISTERED (SimulatorAdapterTestB);

/* static */
TypeId
SimulatorAdapterTestB::GetTypeId ()
{
  static TypeId tid = TypeId ("SimulatorAdapterTestB")
    .SetParent<SimulatorAdapterTestBase> ()
    .SetGroupName ("Core")
    .AddConstructor<SimulatorAdapterTestB> ()
    ;
  return tid;
}

TypeId
SimulatorAdapterTestB::GetInstanceTypeId () const
{
  return GetTypeId ();
}

EventId
SimulatorAdapterTestB::ScheduleNow (EventImpl *event)
{
  std::cout << m_name << "::ScheduleNow()"
            << " with event @" << std::hex << event
            << std::endl;
  return m_simulator->ScheduleNow (event);
}

/** SimulatorAdapterTestBase with custom ScheduleWithContext() */
class SimulatorAdapterTestC : public SimulatorAdapterTestBase
{
public:
  /**
   * Get the type Id.
   * \return the object TypeId
   */
  static TypeId GetTypeId ();
  /** Default constructor. */
  SimulatorAdapterTestC () : SimulatorAdapterTestBase ("AdapterC") { };

  // Inherited
  virtual TypeId  GetInstanceTypeId () const;
  virtual void ScheduleWithContext (uint32_t context, Time const &delay, EventImpl *event);
};

NS_OBJECT_ENSURE_REGISTERED (SimulatorAdapterTestC);

/* static */
TypeId
SimulatorAdapterTestC::GetTypeId ()
{
  static TypeId tid = TypeId ("SimulatorAdapterTestC")
    .SetParent<SimulatorAdapterTestBase> ()
    .SetGroupName ("Core")
    .AddConstructor<SimulatorAdapterTestC> ()
    ;
  return tid;
}

TypeId
SimulatorAdapterTestC::GetInstanceTypeId () const
{
  return GetTypeId ();
}

void
SimulatorAdapterTestC::ScheduleWithContext (uint32_t context, Time const &delay, EventImpl *event)
{
  std::cout << m_name << "::ScheduleWithContext from " << context
            << " for " << delay.As ()
            << " with event @" << std::hex << event
            << std::endl;
  m_simulator->ScheduleWithContext (context, delay, event);
}


/** TestCase for SimulatorAdapter. */
class SimulatorAdapterTestCase : public TestCase
{
public:
  /** Constructor. */
  SimulatorAdapterTestCase ();

  /** 
   * The event function. This reports when it executes, 
   * and \p how it was scheduled.
   * \param [in] how Which Schedule variant was used to schedule this event.
   */
  static void EventFunc (std::string how);

private:
  // Inherited
  void DoSetup ();
  void DoRun ();
  void DoTeardown ();
};

SimulatorAdapterTestCase::SimulatorAdapterTestCase ()
  : TestCase ("Check chaining of SimulatorAdapters")
{
}

/* static */
void
SimulatorAdapterTestCase::EventFunc (std::string how)
{
  std::cout << "EventFunc at " << Now ().As ()
            << " by " << how << "()"
            << std::endl;

  if (how != "ScheduleNow")
    {
      Simulator::ScheduleNow (EventFunc, "ScheduleNow");
    }
}

void
SimulatorAdapterTestCase::DoSetup ()
{
  SimulatorAdapter::ConfigureSimulator ();
  SimulatorAdapter::AddAdapter ("SimulatorAdapterTestA");
  SimulatorAdapter::AddAdapter ("SimulatorAdapterTestB");
  SimulatorAdapter::AddAdapter ("SimulatorAdapterTestC");

  // Add some events, which will instantiate the chain
  ns3::Simulator::Schedule (Seconds (1), EventFunc, "Schedule");
  ns3::Simulator::ScheduleWithContext (10, Seconds (2), EventFunc, "ScheduleWithContext");
}

void
SimulatorAdapterTestCase::DoRun ()
{
  // Test that we can access a specific adapter
  auto sim = Simulator::GetImplementation();
  auto simB = sim->GetObject<SimulatorAdapterTestB> ();
  NS_TEST_ASSERT_MSG_NE (simB, 0,
                         "Unable to access a specific adapter");
  std::cout << "Successfully accessed SimulatorAdapterTestB" << std::endl;

  // And now let it run
  Simulator::Run ();
}

void
SimulatorAdapterTestCase::DoTeardown ()
{
  Simulator::Destroy ();
}

/** The SimulatorAdapter TestSuite. */
class SimulatorAdapterTestSuite : public TestSuite
{
public:
  /** Constructor. */
  SimulatorAdapterTestSuite ()
    : TestSuite ("simulator-adapter")
  {
    AddTestCase (new SimulatorAdapterTestCase (), TestCase::QUICK);
  }
} g_simulatorAdapterTestSuite;  /**< SimulatorAdapterTestSuite instance variable. */


}  // namespace tests

} // namespace ns3
