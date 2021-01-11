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

    NS_LOG_COMPONENT_DEFINE ("EventStreamTestSuite");

/**
 * A function that does nothing.  Used for the event implementation 
 */
void noop()
{}

class EventStreamTestCase : public TestCase
{
public:
    EventStreamTestCase (const std::string& name, ObjectFactory streamFactory)
        :   TestCase (name),
            m_uid (0),
            m_timestamp (0),
            m_factory (streamFactory)
    {}

    virtual ~EventStreamTestCase ()
    {}

protected:
    SimEvent MakeEvent ();
    SimEvent MakeEvent (uint64_t timestamp);
    Ptr<EventStream> MakeStream () const;

    virtual void DoSetup ();
    virtual void DoTeardown ();
    virtual void DoRun ();


private:
    void TestDefaultConstructedStreamIsEmpty ();
    void TestStreamIsNotEmptyAfterInsert ();
    void TestStreamIsNotEmptyAfterPeek ();
    void TestStreamIsEmptyAfterNext ();
    void TestRemoveReturnsTrueWhenMatchIsFound ();
    void TestRemoveReturnsFalseWhenMatchIsNotFound ();
    void TestStreamIsEmptyAfterRemove ();

    uint32_t m_uid;
    uint64_t m_timestamp;
    ObjectFactory m_factory;
    std::unique_ptr<EventGarbageCollector> m_garbage;
};

SimEvent
EventStreamTestCase::MakeEvent () 
{
    return MakeEvent(m_timestamp);
}

SimEvent
EventStreamTestCase::MakeEvent (uint64_t timestamp) 
{
    SimEvent ev;
    ev.key.m_ts = timestamp;
    ev.key.m_uid = m_uid++;
    ev.key.m_context = 0;

    ev.impl = ns3::MakeEvent (&noop);

    EventId id(ev.impl, ev.key.m_ts, ev.key.m_context, ev.key.m_uid);
    m_garbage->Track (id);

    return ev;
}

Ptr<EventStream>
EventStreamTestCase::MakeStream () const
{
    return m_factory.Create<EventStream> ();
}

void
EventStreamTestCase::TestDefaultConstructedStreamIsEmpty ()
{
    auto stream = MakeStream ();

    NS_TEST_ASSERT_MSG_EQ (stream->IsEmpty (), true,
                            "Default constructed stream is not empty");
}

void
EventStreamTestCase::TestStreamIsNotEmptyAfterInsert ()
{
    auto stream = MakeStream ();

    stream->Insert (MakeEvent ());

    NS_TEST_ASSERT_MSG_EQ (stream->IsEmpty (), false,
                            "Stream with events is empty");
}

void
EventStreamTestCase::TestStreamIsNotEmptyAfterPeek ()
{
    auto stream = MakeStream ();

    stream->Insert (MakeEvent ());

    NS_TEST_EXPECT_MSG_EQ (stream->IsEmpty (), false,
                            "Stream is empty after inserting an event");

    stream->Peek ();

    NS_TEST_ASSERT_MSG_EQ (stream->IsEmpty (), false,
                            "Stream is empty after calling Peek");
}

void
EventStreamTestCase::TestStreamIsEmptyAfterNext ()
{
    auto stream = MakeStream ();

    stream->Insert (MakeEvent ());

    NS_TEST_EXPECT_MSG_EQ (stream->IsEmpty (), false,
                            "Stream is empty after inserting an event");

    stream->Next ();

    NS_TEST_ASSERT_MSG_EQ (stream->IsEmpty (), true,
                            "Stream is not empty after calling Next");
}

void 
EventStreamTestCase::TestRemoveReturnsTrueWhenMatchIsFound ()
{
    auto stream = MakeStream ();

    auto event = MakeEvent ();
    stream->Insert (event);

    NS_TEST_EXPECT_MSG_EQ (stream->IsEmpty (), false,
                            "Stream is empty after inserting an event");

    auto removed = stream->Remove (event.key);

    NS_TEST_ASSERT_MSG_EQ (removed, true,
                            "Event key was not found in stream");
}

void 
EventStreamTestCase::TestRemoveReturnsFalseWhenMatchIsNotFound ()
{
    auto stream = MakeStream ();

    auto event = MakeEvent ();
    stream->Insert (event);

    NS_TEST_EXPECT_MSG_EQ (stream->IsEmpty (), false,
                            "Stream is empty after inserting an event");

    auto badKey = event.key;
    badKey.m_ts = 1;

    auto removed = stream->Remove (badKey);

    NS_TEST_ASSERT_MSG_EQ (removed, false,
                            "Bad event key was found in stream");
}

void 
EventStreamTestCase::TestStreamIsEmptyAfterRemove ()
{
    auto stream = MakeStream ();

    auto event = MakeEvent ();

    stream->Insert (event);

    NS_TEST_EXPECT_MSG_EQ (stream->IsEmpty (), false,
                            "Stream is empty after inserting an event");

    auto removed = stream->Remove (event.key);

    NS_TEST_EXPECT_MSG_EQ (removed, true,
                            "Event key was not found in stream");

    NS_TEST_ASSERT_MSG_EQ (event.impl->IsCancelled (), true,
                            "Event was found but not cancelled");
}

void
EventStreamTestCase::DoSetup ()
{
    m_timestamp = 1e9;
    m_uid = 1;

    m_garbage.reset (new EventGarbageCollector ());
}

void
EventStreamTestCase::DoTeardown ()
{
    //deleting the eventgarbagecollector will clean up all of the events
    m_garbage.reset ();
}

void
EventStreamTestCase::DoRun ()
{
    TestDefaultConstructedStreamIsEmpty ();
    TestStreamIsNotEmptyAfterInsert ();
    TestStreamIsNotEmptyAfterPeek ();
    TestRemoveReturnsTrueWhenMatchIsFound ();
    TestRemoveReturnsFalseWhenMatchIsNotFound ();
    TestStreamIsEmptyAfterRemove ();
}

class FifoEventStreamTestCase : public EventStreamTestCase
{
public:
    FifoEventStreamTestCase()
        :   EventStreamTestCase ("fifo-event-stream",
                                 ObjectFactory ("ns3::FifoEventStream"))
    {}

    virtual ~FifoEventStreamTestCase ()
    {}

protected:
    void TestEventsRemovedInSameOrderAsInsertion ();
    void TestStreamIsFullAfterAddingTooManyEvents ();
    void TestStreamIsNotFullAfterRemovingEvents ();

    virtual void DoRun ();
};

void
FifoEventStreamTestCase::TestEventsRemovedInSameOrderAsInsertion ()
{
    const uint32_t eventCount = 10;

    auto stream = MakeStream ();

    std::vector<SimEvent> events;

    for (uint32_t i = 0; i < eventCount; ++i)
    {
        events.push_back(MakeEvent ());

        stream->Insert (events.back ());
    }

    for (uint32_t i = 0; i < eventCount; ++i)
    {
        auto event = stream->Next ();

        NS_TEST_ASSERT_MSG_EQ (event.key, events[i].key,
                                "Event not removed in the same order as insertion");
    }

    NS_TEST_ASSERT_MSG_EQ (stream->IsEmpty (), true,
                            "Stream is not empty after removing all events");
}

void 
FifoEventStreamTestCase::TestStreamIsFullAfterAddingTooManyEvents ()
{
    const uint32_t streamSize = 10;

    auto stream = MakeStream ();

    Ptr<FifoEventStream> fifoStream = stream->GetObject <FifoEventStream> ();
    fifoStream->SetStreamSize (streamSize);

    NS_TEST_EXPECT_MSG_EQ (stream->IsFull (), false,
                            "Stream is full before adding any events");

    for (uint32_t i = 0; i < streamSize; ++i)
    {
        stream->Insert (MakeEvent ());
    }

    NS_TEST_ASSERT_MSG_EQ (stream->IsFull (), true,
                            "Stream is not full after adding events"); 
}

void 
FifoEventStreamTestCase::TestStreamIsNotFullAfterRemovingEvents ()
{
    const uint32_t streamSize = 10;

    auto stream = MakeStream ();

    Ptr<FifoEventStream> fifoStream = stream->GetObject <FifoEventStream> ();
    fifoStream->SetStreamSize (streamSize);

    NS_TEST_EXPECT_MSG_EQ (stream->IsFull (), false,
                            "Stream is full before adding any events");

    for (uint32_t i = 0; i < streamSize; ++i)
    {
        stream->Insert (MakeEvent ());
    }

    NS_TEST_EXPECT_MSG_EQ (stream->IsFull (), true,
                            "Stream is not full after adding events"); 

    stream->Next ();

    NS_TEST_ASSERT_MSG_EQ (stream->IsFull (), false,
                            "Stream is still full after removing an event"); 
}

void
FifoEventStreamTestCase::DoRun ()
{
    //first run the parent test cases
    EventStreamTestCase::DoRun ();

    //now run test cases specific to this type of event stream
    TestEventsRemovedInSameOrderAsInsertion ();
    TestStreamIsFullAfterAddingTooManyEvents ();
    TestStreamIsNotFullAfterRemovingEvents ();
}

class RandomEventStreamTestCase : public EventStreamTestCase
{
public:
    RandomEventStreamTestCase()
        :   EventStreamTestCase ("random-event-stream",
                                 ObjectFactory ("ns3::RandomEventStream"))
    {}

    virtual ~RandomEventStreamTestCase ()
    {}

protected:
    void TestEventsRemovedInRandomOrder ();

    virtual void DoRun ();

};

void
RandomEventStreamTestCase::TestEventsRemovedInRandomOrder ()
{
    const uint32_t eventCount = 10;

    auto stream = MakeStream ();

    std::map<SimEventKey, uint32_t> insertOrder;

    for (uint32_t i = 0; i < eventCount; ++i)
    {
        auto event = MakeEvent ();
        insertOrder[event.key] = i;

        stream->Insert (event);

        NS_LOG_DEBUG ("Insertion: key=(" << event.key << "), position=" << i);
    }

    std::map<SimEventKey, uint32_t> removalOrder;

    for (uint32_t i = 0; i < eventCount; ++i)
    {
        auto event = stream->Next ();

        auto iter = insertOrder.find(event.key);

        NS_TEST_ASSERT_MSG_EQ ( (iter != insertOrder.end()), true,
                                "Event was not found in list of inserted events");

        removalOrder[event.key] = i;

        NS_LOG_DEBUG ("Removal: key=(" << iter->first << "), insert position=" 
                      << iter->second << ", removal position=" << i);
    }


    NS_TEST_ASSERT_MSG_EQ (stream->IsEmpty (), true,
                            "Stream is not empty after removing all events");



    NS_TEST_ASSERT_MSG_EQ ( (removalOrder != insertOrder), true,
                            "Events were removed in same order as inserted");
}

void
RandomEventStreamTestCase::DoRun ()
{
    //first run the parent test cases
    EventStreamTestCase::DoRun ();

    //now run test cases specific to this type of event stream
    TestEventsRemovedInRandomOrder ();
}


class EventStreamTestSuite : public TestSuite
{
public:
    EventStreamTestSuite ()
        :   TestSuite("event-stream")
    {
        RegisterTests ();
    }

private:
    void RegisterTests ();
};

void
EventStreamTestSuite::RegisterTests ()
{
    AddTestCase (new FifoEventStreamTestCase (), TestCase::QUICK);
    AddTestCase (new RandomEventStreamTestCase (), TestCase::QUICK);
}

EventStreamTestSuite gEventStreamTests;

}   //  namespace ns3
