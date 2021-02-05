/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 Lawrence Livermore National Laboratory
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
 * Author:  Mathew Bielejeski <bielejeski1@llnl.gov>
 */

#include "ns3/core-module.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EventSetTestSuite");

/**
 * A function that does nothing.  Used for the event implementation
 */
void noop ()
{}

class EventSetTestCase : public TestCase
{
public:
  EventSetTestCase (const std::string& name, ObjectFactory setFactory)
    :   TestCase (name),
      m_uid (0),
      m_timestamp (0),
      m_factory (setFactory)
  {}

  virtual ~EventSetTestCase ()
  {}

protected:
  SimEvent MakeEvent ();
  SimEvent MakeEvent (uint64_t timestamp);
  Ptr<EventSet> MakeSet () const;

  virtual void DoSetup ();
  virtual void DoTeardown ();
  virtual void DoRun ();

private:
  void TestDefaultConstructedSetIsEmpty ();
  void TestSetIsNotEmptyAfterInsert ();
  void TestSetIsNotEmptyAfterPeek ();
  void TestSetIsEmptyAfterNext ();
  void TestRemoveReturnsTrueWhenMatchIsFound ();
  void TestRemoveReturnsFalseWhenMatchIsNotFound ();
  void TestSetIsEmptyAfterRemove ();

  uint32_t m_uid;
  uint64_t m_timestamp;
  ObjectFactory m_factory;
  std::unique_ptr<EventGarbageCollector> m_garbage;
};

SimEvent
EventSetTestCase::MakeEvent ()
{
  return MakeEvent (m_timestamp);
}

SimEvent
EventSetTestCase::MakeEvent (uint64_t timestamp)
{
  SimEvent ev;
  ev.key.m_ts = timestamp;
  ev.key.m_uid = m_uid++;
  ev.key.m_context = 0;

  ev.impl = ns3::MakeEvent (&noop);

  EventId id (ev.impl, ev.key.m_ts, ev.key.m_context, ev.key.m_uid);
  m_garbage->Track (id);

  return ev;
}

Ptr<EventSet>
EventSetTestCase::MakeSet () const
{
  return m_factory.Create<EventSet> ();
}

void
EventSetTestCase::TestDefaultConstructedSetIsEmpty ()
{
  auto eventSet = MakeSet ();

  NS_TEST_ASSERT_MSG_EQ (eventSet->IsEmpty (), true,
                         "Default constructed event set is not empty");
}

void
EventSetTestCase::TestSetIsNotEmptyAfterInsert ()
{
  auto eventSet = MakeSet ();

  eventSet->Insert (MakeEvent ());

  NS_TEST_ASSERT_MSG_EQ (eventSet->IsEmpty (), false,
                         "Set with events is empty");
}

void
EventSetTestCase::TestSetIsNotEmptyAfterPeek ()
{
  auto eventSet = MakeSet ();

  eventSet->Insert (MakeEvent ());

  NS_TEST_EXPECT_MSG_EQ (eventSet->IsEmpty (), false,
                         "Set is empty after inserting an event");

  eventSet->Peek ();

  NS_TEST_ASSERT_MSG_EQ (eventSet->IsEmpty (), false,
                         "Set is empty after calling Peek");
}

void
EventSetTestCase::TestSetIsEmptyAfterNext ()
{
  auto eventSet = MakeSet ();

  eventSet->Insert (MakeEvent ());

  NS_TEST_EXPECT_MSG_EQ (eventSet->IsEmpty (), false,
                         "Set is empty after inserting an event");

  eventSet->Next ();

  NS_TEST_ASSERT_MSG_EQ (eventSet->IsEmpty (), true,
                         "Set is not empty after calling Next");
}

void
EventSetTestCase::TestRemoveReturnsTrueWhenMatchIsFound ()
{
  auto eventSet = MakeSet ();

  auto event = MakeEvent ();
  eventSet->Insert (event);

  NS_TEST_EXPECT_MSG_EQ (eventSet->IsEmpty (), false,
                         "Set is empty after inserting an event");

  auto removed = eventSet->Remove (event.key);

  NS_TEST_ASSERT_MSG_EQ (removed, true,
                         "Event key was not found in event set");
}

void
EventSetTestCase::TestRemoveReturnsFalseWhenMatchIsNotFound ()
{
  auto eventSet = MakeSet ();

  auto event = MakeEvent ();
  eventSet->Insert (event);

  NS_TEST_EXPECT_MSG_EQ (eventSet->IsEmpty (), false,
                         "Set is empty after inserting an event");

  auto badKey = event.key;
  badKey.m_ts = 1;

  auto removed = eventSet->Remove (badKey);

  NS_TEST_ASSERT_MSG_EQ (removed, false,
                         "Bad event key was found in event set");
}

void
EventSetTestCase::TestSetIsEmptyAfterRemove ()
{
  auto eventSet = MakeSet ();

  auto event = MakeEvent ();

  eventSet->Insert (event);

  NS_TEST_EXPECT_MSG_EQ (eventSet->IsEmpty (), false,
                         "Set is empty after inserting an event");

  auto removed = eventSet->Remove (event.key);

  NS_TEST_EXPECT_MSG_EQ (removed, true,
                         "Event key was not found in event set");

  NS_TEST_ASSERT_MSG_EQ (event.impl->IsCancelled (), true,
                         "Event was found but not cancelled");
}

void
EventSetTestCase::DoSetup ()
{
  m_timestamp = 1e9;
  m_uid = 1;

  m_garbage.reset (new EventGarbageCollector ());
}

void
EventSetTestCase::DoTeardown ()
{
  //deleting the eventgarbagecollector will clean up all of the events
  m_garbage.reset ();
}

void
EventSetTestCase::DoRun ()
{
  TestDefaultConstructedSetIsEmpty ();
  TestSetIsNotEmptyAfterInsert ();
  TestSetIsNotEmptyAfterPeek ();
  TestRemoveReturnsTrueWhenMatchIsFound ();
  TestRemoveReturnsFalseWhenMatchIsNotFound ();
  TestSetIsEmptyAfterRemove ();
}

class FifoEventSetTestCase : public EventSetTestCase
{
public:
  FifoEventSetTestCase ()
    :   EventSetTestCase ("fifo-event-set",
                             ObjectFactory ("ns3::FifoEventSet"))
  {}

  virtual ~FifoEventSetTestCase ()
  {}

protected:
  void TestEventsRemovedInSameOrderAsInsertion ();
  void TestSetIsFullAfterAddingTooManyEvents ();
  void TestSetIsNotFullAfterRemovingEvents ();

  virtual void DoRun ();
};

void
FifoEventSetTestCase::TestEventsRemovedInSameOrderAsInsertion ()
{
  const uint32_t eventCount = 10;

  auto eventSet = MakeSet ();

  std::vector<SimEvent> events;

  for (uint32_t i = 0; i < eventCount; ++i)
    {
      events.push_back (MakeEvent ());

      eventSet->Insert (events.back ());
    }

  for (uint32_t i = 0; i < eventCount; ++i)
    {
      auto event = eventSet->Next ();

      NS_TEST_ASSERT_MSG_EQ (event.key, events[i].key,
                             "Event not removed in the same order as insertion");
    }

  NS_TEST_ASSERT_MSG_EQ (eventSet->IsEmpty (), true,
                         "Set is not empty after removing all events");
}

void
FifoEventSetTestCase::TestSetIsFullAfterAddingTooManyEvents ()
{
  const uint32_t eventSetSize = 10;

  auto eventSet = MakeSet ();

  Ptr<FifoEventSet> fifoSet = eventSet->GetObject <FifoEventSet> ();
  fifoSet->SetMaxSize (eventSetSize);

  NS_TEST_EXPECT_MSG_EQ (eventSet->IsFull (), false,
                         "Set is full before adding any events");

  for (uint32_t i = 0; i < eventSetSize; ++i)
    {
      eventSet->Insert (MakeEvent ());
    }

  NS_TEST_ASSERT_MSG_EQ (eventSet->IsFull (), true,
                         "Set is not full after adding events");
}

void
FifoEventSetTestCase::TestSetIsNotFullAfterRemovingEvents ()
{
  const uint32_t eventSetSize = 10;

  auto eventSet = MakeSet ();

  Ptr<FifoEventSet> fifoSet = eventSet->GetObject <FifoEventSet> ();
  fifoSet->SetMaxSize (eventSetSize);

  NS_TEST_EXPECT_MSG_EQ (eventSet->IsFull (), false,
                         "Set is full before adding any events");

  for (uint32_t i = 0; i < eventSetSize; ++i)
    {
      eventSet->Insert (MakeEvent ());
    }

  NS_TEST_EXPECT_MSG_EQ (eventSet->IsFull (), true,
                         "Set is not full after adding events");

  eventSet->Next ();

  NS_TEST_ASSERT_MSG_EQ (eventSet->IsFull (), false,
                         "Set is still full after removing an event");
}

void
FifoEventSetTestCase::DoRun ()
{
  //first run the parent test cases
  EventSetTestCase::DoRun ();

  //now run test cases specific to this type of event event set 
  TestEventsRemovedInSameOrderAsInsertion ();
  TestSetIsFullAfterAddingTooManyEvents ();
  TestSetIsNotFullAfterRemovingEvents ();
}

class RandomEventSetTestCase : public EventSetTestCase
{
public:
  RandomEventSetTestCase ()
    :   EventSetTestCase ("random-event-set",
                             ObjectFactory ("ns3::RandomEventSet"))
  {}

  virtual ~RandomEventSetTestCase ()
  {}

protected:
  void TestEventsRemovedInRandomOrder ();

  virtual void DoRun ();

};

void
RandomEventSetTestCase::TestEventsRemovedInRandomOrder ()
{
  const uint32_t eventCount = 100;
  const uint64_t timestamp = 1000;

  auto eventSet = MakeSet ();

  std::map<SimEventKey, uint32_t> insertOrder;

  for (uint32_t i = 0; i < eventCount; ++i)
    {
      auto event = MakeEvent (timestamp);
      insertOrder[event.key] = i;

      eventSet->Insert (event);

      NS_LOG_DEBUG ("Insertion: key=(" << event.key << "), position=" << i);
    }

  std::map<SimEventKey, uint32_t> removalOrder;

  for (uint32_t i = 0; i < eventCount; ++i)
    {
      auto event = eventSet->Next ();

      auto iter = insertOrder.find (event.key);

      NS_TEST_ASSERT_MSG_EQ ( (iter != insertOrder.end ()), true,
                              "Event was not found in list of inserted events");

      removalOrder[event.key] = i;

      NS_LOG_DEBUG ("Removal: key=(" << iter->first << "), insert position="
                                     << iter->second << ", removal position=" << i);
    }


  NS_TEST_ASSERT_MSG_EQ (eventSet->IsEmpty (), true,
                         "Set is not empty after removing all events");



  NS_TEST_ASSERT_MSG_EQ ( (removalOrder != insertOrder), true,
                          "Events were removed in same order as inserted");
}

void
RandomEventSetTestCase::DoRun ()
{
  //first run the parent test cases
  EventSetTestCase::DoRun ();

  //now run test cases specific to this type of event set 
  TestEventsRemovedInRandomOrder ();
}

//====================================
//
// LifoEventSetTestCase
//
//====================================
class LifoEventSetTestCase : public EventSetTestCase
{
public:
  LifoEventSetTestCase ()
    :   EventSetTestCase ("lifo-event-set",
                             ObjectFactory ("ns3::LifoEventSet"))
  {}

  virtual ~LifoEventSetTestCase ()
  {}

protected:
  void TestEventsRemovedInLifoOrder ();

  virtual void DoRun ();

};

void
LifoEventSetTestCase::TestEventsRemovedInLifoOrder ()
{
  const uint32_t eventCount = 100;
  const uint64_t timestamp = 1000;

  auto eventSet = MakeSet ();

  std::vector<SimEventKey> insertOrder;

  for (uint32_t i = 0; i < eventCount; ++i)
    {
      auto event = MakeEvent (timestamp);
      insertOrder.push_back (event.key);

      eventSet->Insert (event);
    }

  auto iter = insertOrder.rbegin ();
  for (uint32_t i = 0; i < eventCount; ++i)
    {
      auto event = eventSet->Next ();

      NS_TEST_ASSERT_MSG_EQ (event.key, *iter,
                             "LifoEventSet did not return an event in LIFO order");

      ++iter;
    }


  NS_TEST_ASSERT_MSG_EQ (eventSet->IsEmpty (), true,
                         "LifoEventSet is not empty after removing all events");
}

void
LifoEventSetTestCase::DoRun ()
{
  //first run the parent test cases
  EventSetTestCase::DoRun ();

  //now run test cases specific to this type of event set
  TestEventsRemovedInLifoOrder ();
}

class EventSetTestSuite : public TestSuite
{
public:
  EventSetTestSuite ()
    :   TestSuite ("event-set")
  {
    RegisterTests ();
  }

private:
  void RegisterTests ();
};

void
EventSetTestSuite::RegisterTests ()
{
  AddTestCase (new FifoEventSetTestCase (), TestCase::QUICK);
  AddTestCase (new LifoEventSetTestCase (), TestCase::QUICK);
  AddTestCase (new RandomEventSetTestCase (), TestCase::QUICK);
}

EventSetTestSuite gEventSetTests;

}   //  namespace ns3
