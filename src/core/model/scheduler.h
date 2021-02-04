/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "object.h"
#include "sim-event.h"

#include <cstdint>
#include <deque>
#include <memory>

/**
 * \file
 * \ingroup scheduler
 * \ingroup events
 * ns3::Scheduler abstract base class, ns3::Scheduler::Event and
 * ns3::Scheduler::EventKey declarations.
 */

namespace ns3 {

class EventImpl;
class EventSet;

/**
 * \ingroup core
 * \defgroup scheduler Scheduler and Events
 * \brief Manage the event list by creating and scheduling events.
 */
/**
 * \ingroup scheduler
 * \defgroup events Events
 */
/**
 * \ingroup scheduler
 * \brief Maintain the event list
 *
 *
 * In ns-3 the Scheduler manages the future event list.  There are several
 * different Scheduler implementations with different time and space tradeoffs.
 * Which one is "best" depends in part on the characteristics
 * of the model being executed.  For optimized production work common
 * practice is to benchmark each Scheduler on the model of interest.
 * The utility program utils/bench-simulator.cc can do simple benchmarking
 * of each SchedulerImpl against an exponential or user-provided
 * event time distribution.
 *
 * The most important Scheduler functions for time performance are (usually)
 * Scheduler::Insert (for new events) and Scheduler::RemoveNext (for pulling
 * off the next event to execute).  Simulator::Cancel is usually
 * implemented by simply setting a bit on the Event, but leaving it in the
 * Scheduler; the Simulator just skips those events as they are encountered.
 *
 * For models which need a large event list the Scheduler overhead
 * and per-event memory cost could also be important.  Some models
 * rely heavily on Scheduler::Cancel, however, and these might benefit
 * from using Scheduler::Remove instead, to reduce the size of the event
 * list, at the time cost of actually removing events from the list.
 *
 * A summary of the main characteristics
 * of each SchedulerImpl is provided below.  See the individual
 * Scheduler pages for details on the complexity of the other API calls.
 * (Memory overheads assume pointers and `std::size_t` are both 8 bytes.)
 *
 * <table class="markdownTable">
 * <tr class="markdownTableHead">
 *      <th class="markdownTableHeadCenter" colspan="2"> %Scheduler Type </th>
 *      <th class="markdownTableHeadCenter" colspan="4">Complexity</th>
 * </tr>
 * <tr class="markdownTableHead">
 *      <th class="markdownTableHeadLeft" rowspan="2"> %SchedulerImpl </th>
 *      <th class="markdownTableHeadLeft" rowspan="2"> Method </th>
 *      <th class="markdownTableHeadCenter" colspan="2"> %Time </th>
 *      <th class="markdownTableHeadCenter" colspan="2"> Space</th>
 * </tr>
 * <tr class="markdownTableHead">
 *      <th class="markdownTableHeadLeft"> %Insert()</th>
 *      <th class="markdownTableHeadLeft"> %RemoveNext()</th>
 *      <th class="markdownTableHeadLeft"> Overhead</th>
 *      <th class="markdownTableHeadLeft"> Per %Event</th>
 * </tr>
 * <tr class="markdownTableBody">
 *      <td class="markdownTableBodyLeft"> CalendarScheduler </td>
 *      <td class="markdownTableBodyLeft"> `<std::list> []` </td>
 *      <td class="markdownTableBodyLeft"> Constant </td>
 *      <td class="markdownTableBodyLeft"> Constant </td>
 *      <td class="markdownTableBodyLeft"> 24 bytes </td>
 *      <td class="markdownTableBodyLeft"> 16 bytes </td>
 * </tr>
 * <tr class="markdownTableBody">
 *      <td class="markdownTableBodyLeft"> HeapScheduler </td>
 *      <td class="markdownTableBodyLeft"> Heap on `std::vector` </td>
 *      <td class="markdownTableBodyLeft"> Logarithmic  </td>
 *      <td class="markdownTableBodyLeft"> Logarithmic </td>
 *      <td class="markdownTableBodyLeft"> 24 bytes </td>
 *      <td class="markdownTableBodyLeft"> 0 </td>
 * </tr>
 * <tr class="markdownTableBody">
 *      <td class="markdownTableBodyLeft"> ListScheduler </td>
 *      <td class="markdownTableBodyLeft"> `std::list` </td>
 *      <td class="markdownTableBodyLeft"> Linear </td>
 *      <td class="markdownTableBodyLeft"> Constant </td>
 *      <td class="markdownTableBodyLeft"> 24 bytes </td>
 *      <td class="markdownTableBodyLeft"> 16 bytes </td>
 * </tr>
 * <tr class="markdownTableBody">
 *      <td class="markdownTableBodyLeft"> MapScheduler </td>
 *      <td class="markdownTableBodyLeft"> `std::map` </td>
 *      <td class="markdownTableBodyLeft"> Logarithmic </td>
 *      <td class="markdownTableBodyLeft"> Constant </td>
 *      <td class="markdownTableBodyLeft"> 40 bytes </td>
 *      <td class="markdownTableBodyLeft"> 32 bytes </td>
 * </tr>
 * <tr class="markdownTableBody">
 *      <td class="markdownTableBodyLeft"> PriorityQueueScheduler </td>
 *      <td class="markdownTableBodyLeft"> `std::priority_queue<,std::vector>` </td>
 *      <td class="markdownTableBodyLeft"> Logarithmic  </td>
 *      <td class="markdownTableBodyLeft"> Logarithmic </td>
 *      <td class="markdownTableBodyLeft"> 24 bytes </td>
 *      <td class="markdownTableBodyLeft"> 0 </td>
 * </tr>
 * </table>
 *
 * It is possible to change the Scheduler choice during a simulation,
 * via Simulator::SetScheduler.
 *
 * The Scheduler base class specifies the interface used to maintain the
 * event list. If you want to provide a new event list scheduler,
 * you need to create a subclass of this base class and implement
 * all the pure virtual methods defined here.
 *
 * The only tricky aspect of this API is the memory management of
 * the EventImpl pointer which is a member of the Event data structure.
 * The lifetime of this pointer is assumed to always be longer than
 * the lifetime of the Scheduler class which means that the caller
 * is responsible for ensuring that this invariant holds through
 * calling EventId::Ref and SimpleRefCount::Unref at the right time.
 * Typically, EventId::Ref is called before Insert and SimpleRefCount::Unref is called
 * after a call to one of the Remove methods.
 *
 * ## Event Order ##
 *
 * The order that events are returned by the Scheduler is dependent on two factors:
 *
 * 1. The specific SchedulerImpl used
 * 2. The EventSet implementation used by the Scheduler
 *
 * The SchedulerImpl controls how scheduled events are stored and is responsible
 * for deciding how events with the same timestamp are ordered.
 *
 * ### Event Set ###
 *
 * An EventSet is a class which holds a collection of events that have the
 * same timestamp.  The purpose of the EventSet is to provide finer grained
 * control over how events with the same timestamp are ordered.
 *
 * Currently there are three implementations of the EventSet, each of which
 * orders events in a different way.
 *
 * * FifoEventSet
 * * LifoEventSet
 * * RandomEventSet
 *
 * Custom implementations are possible by deriving a class from the EventSet
 * base class.
 *
 * #### FifoEventSet ####
 *
 * The FifoEventSet does not make any changes to the event order.  It returns
 * events in the same order that they are pulled from the event list. This
 * class is the default implementation used by the scheduler.
 *
 * #### LifoEventSet ####
 *
 * The LifoEventSet returns events in the reverse of the insertion order.  The
 * last event inserted is the first one removed.
 *
 * #### RandomEventSet ####
 *
 * The RandomEventSet shuffles the events in an event set and returns them in
 * a random order.
 *
 * This implementation is useful for testing a model with different event
 * orderings to ensure that the model does not depend on a specific
 * ordering of events.
 *
 */
class Scheduler : public Object
{
public:
  /**
   *  Register this type.
   *  \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  using EventKey = SimEventKey;
  using Event = SimEvent;

  /**
   * Default Constructor
   */
  Scheduler ();

  /** Destructor. */
  virtual ~Scheduler () = 0;

  /**
   * Insert a new Event in the schedule.
   *
   * \param [in] ev Event to store in the event list
   */
  virtual void Insert (const Event &ev) = 0;

  /**
   * Test if the scheduler is empty.
   *
   * \returns \c true if there are no more events to process and \c false otherwise.
   */
  bool IsEmpty (void) const;

  /**
   * Get the next event without removing it.
   *
   * \warning This method cannot be invoked if the list is empty.
   *
   * \returns A copy of the next event.
   */
  const Event PeekNext (void) const;
  /**
   * Remove the earliest event from the event list.
   *
   * \warning This method cannot be invoked if the list is empty.
   *
   * \warning In most cases the event returned by RemoveNext will match the one
   * returned by PeekNext, i.e. PeekNext() == RemoveNext().  In some simulator
   * implementations, it is possible for events to be added between the call to
   * PeekNext and the call to RemoveNext.  In those situations, the event returned
   * by RemoveNext may be different than the one previously returned by PeekNext.
   *
   * \return The Event.
   */
  Event RemoveNext (void);
  /**
   * Remove a specific event from the event list.
   *
   * \param [in] ev The event to remove
   */
  void Remove (const Event &ev);

  /**
   * Change the EventSet implementation to use
   *
   * The EventSet is used by the Scheduler as a staging space for a set
   * of events that have the same timestamp.  The EventSet implementation is
   * free to modify the collection of events in any way it desires, from changing
   * the order of events to adding or deleting events.  Calling PeekNext
   * and RemoveNext pulls events from the EventSet.  When the set is
   * empty, the scheduler will fill it with the next set of events.
   *
   * \param eventSet The new EventSet implementation
   */
  void SetEventSet (Ptr<EventSet> eventSet);

private:
  /**
   * Test if the scheduler is empty.
   *
   * \returns \c true if there are no more events to process and \c false otherwise.
   */
  virtual bool DoIsEmpty (void) const = 0;

  /**
   * Get a pointer to the next event.
   *
   * \warning This method cannot be invoked if the list is empty.
   *
   * \returns A copy of the next event.
   */
  virtual Event DoPeekNext (void) const = 0;
  /**
   * Remove the earliest event from the event list.
   *
   * \warning This method cannot be invoked if the list is empty.
   *
   * \return The Event.
   */
  virtual Event DoRemoveNext (void) = 0;
  /**
   * Remove a specific event from the event list.
   *
   * \param [in] ev The event to remove
   */
  virtual void DoRemove (const Event &ev) = 0;

  /**
   * Fill the event set with events from the underlying implementation
   */
  void FillEventSet ();

  uint64_t m_currentTimestamp;   //!< Timestamp of events in m_eventSet
  Ptr<EventSet> m_eventSet;   //!< Next set of events
};

} // namespace ns3


#endif /* SCHEDULER_H */
