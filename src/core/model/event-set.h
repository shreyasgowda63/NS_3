/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019   Lawrence Livermore National Laboratory
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
 * Author: Mathew Bielejeski <bielejeski1@gmail.com>
 */

#ifndef EVENT_SET_H_
#define EVENT_SET_H_

#include "sim-event.h"
#include "object.h"
#include "random-variable-stream.h"

#include <deque>
#include <vector>

/**
 * \file
 * \ingroup events
 * ns3::EventSet definition
 */

namespace ns3 {

/**
 * \ingroup events
 * An abstract base class which defines the interface for an event set
 */
class EventSet : public Object
{
public:

  /**
   * Get the TypeId of this Object
   *
   * \return The TypeId of this class
   */
  static TypeId GetTypeId ();

  /**
   * Default Constructor
   */
  EventSet ()
  {}

  /**
   * Destructor
   */
  virtual ~EventSet ()
  {}

  /**
   * Check if the set has more events
   *
   * \return \c true if the set is empty, \c false otherwise
   */
  virtual bool IsEmpty () const = 0;

  /**
   * Check if the set has space to insert more events
   *
   * \return \c true if the set cannot hold any more events, \c false
   * otherwise
   */
  virtual bool IsFull () const = 0;

  /**
   * Add an event to the set.
   *
   * The position of the inserted event is left as an implementation detail
   * of the derived class.
   *
   * \pre IsFull () must be \c false
   *
   * \param ev Event to add to the set
   *
   * \return \c true if the event was added to the set, \c false otherwise
   */
  virtual bool Insert (SimEvent ev) = 0;

  /**
   * Look at the next event in the set without removing it
   *
   * \pre IsEmpty () must be \c false
   *
   * \return A const reference to the next event in the set
   */
  virtual const SimEvent& Peek () const = 0;

  /**
   * Remove the next event in the set
   *
   * \pre IsEmpty () must be \c false
   *
   * \return The event that was removed
   */
  virtual SimEvent Next () = 0;

  /**
   * Remove an event with \p key from the set
   *
   * Searches the set for an event that matches \p key and removes it
   *
   * \param key Key of the event to remove
   *
   * \return \c true if the event was found in the set, \c false otherwise
   */
  virtual bool Remove (const SimEventKey& key) = 0;
};  //  class EventSet

/**
 * An event set implementation that returns events in the same
 * order they were inserted (first in, first out)
 */
class FifoEventSet : public EventSet
{
public:
  /**
   * Get the TypeId of this Object
   *
   * \return The TypeId of this class
   */
  static TypeId GetTypeId ();

  /**
   * Default Constructor
   */
  FifoEventSet ();

  /**
   * Destructor
   */
  virtual ~FifoEventSet ();

  /**
   * Set the maximum number of events that can be stored in the set
   *
   * \pre IsEmpty must be true
   *
   * \param newSize The maximum number of events that can be stored in the set
   */
  void SetMaxSize (uint32_t newSize);

  /**
   * Get the maximum number of events that the set can hold
   */
  uint32_t GetMaxSize () const;

  virtual bool IsEmpty () const;
  virtual bool IsFull () const;
  virtual bool Insert (SimEvent ev);
  virtual const SimEvent& Peek () const;
  virtual SimEvent Next ();
  virtual bool Remove (const SimEventKey& key);

private:
  // A collection of simulation events
  using Buffer = std::vector<SimEvent>;

  /**
   * Maximum size of the set
   */
  uint32_t m_maxSize;

  /**
   * Location of the next event in the set
   */
  std::size_t m_head;

  /**
   * Location of the last event in the set
   */
  std::size_t m_tail;

  /**
   * Number of events in the set
   */
  uint32_t m_count;

  /**
   * Storage area for events
   */
  Buffer m_buffer;
};  // class FifoEventSet

/**
 * An event set implementation that returns events in the reverse of their
 * insertions order (last in, first out)
 */
class LifoEventSet : public EventSet
{
public:
  /**
   * Get the TypeId of this Object
   *
   * \return The TypeId of this class
   */
  static TypeId GetTypeId ();

  /**
   * Default Constructor
   */
  LifoEventSet ();

  /**
   * Destructor
   */
  virtual ~LifoEventSet ();

  /**
   * Set the maximum number of events that can be stored in the set
   *
   * \pre IsEmpty() must be true
   *
   * \param newSize The maximum number of events that can be stored in the set
   */
  void SetMaxSize (uint32_t newSize);

  /**
   * Get the maximum number of events that the set can hold
   */
  uint32_t GetMaxSize () const;

  virtual bool IsEmpty () const;
  virtual bool IsFull () const;
  virtual bool Insert (SimEvent ev);
  virtual const SimEvent& Peek () const;
  virtual SimEvent Next ();
  virtual bool Remove (const SimEventKey& key);

private:
  /**
   * A collection of simulation events
   */
  using Buffer = std::vector<SimEvent>;

  /**
   * Maximum size of the set
   */
  uint32_t m_maxSize;

  /**
   * Storage area for events
   */
  Buffer m_buffer;
};  // class LifoEventSet

/**
 * An event set implementation that returns events in a random order
 *
 * This implementation uses a RandomVariableStream to randomize the order
 * of the events inserted into the set.
 *
 * When an event is inserted into the set, the position of an existing event
 * in the buffer is selected at random using a RandomVariableStream.  The
 * event currently occupying that selected position is moved to the end of
 * buffer and the new event is inserted into the vacated spot.
 *
 * The default RandomVariableStream used by this class is UniformRandomVariable
 * with the maximum value set to the maximum set size.
 *
 * RandomEventSet::SetRandomSource can be used to change the random
 * variable implementation used to shuffle events.
 */
class RandomEventSet : public EventSet
{
public:

  /**
   * Get the TypeId of this Object
   *
   * \return The TypeId of this class
   */
  static TypeId GetTypeId ();

  /**
   * Default Constructor
   */
  RandomEventSet ();

  /**
   * Destructor
   */
  virtual ~RandomEventSet ();

  /**
   * Set the random number generator to use for shuffling events
   *
   * The maximum integer returned by \p rand should be greater than or
   * equal to the value returned by GetMaxSize.
   *
   * \param rand The RandomVariableStream implementation to use
   */
  void SetRandomSource (Ptr<RandomVariableStream> rand);

  /**
   * Set the maximum number of events that the set can hold
   *
   * \param newSize The maximum number of events the set can hold
   */
  void SetMaxSize (uint32_t newSize);

  /**
   * Get the maximum number of events that the set can hold
   */
  uint32_t GetMaxSize () const;

  //Inherited functions
  virtual bool IsEmpty () const;
  virtual bool IsFull () const;
  virtual bool Insert (SimEvent ev);
  virtual const SimEvent& Peek () const;
  virtual SimEvent Next ();
  virtual bool Remove (const SimEventKey& key);

private:
  /**
   * A collection of simulation events
   */
  using Buffer = std::vector<SimEvent>;

  /**
   * The maximum number of events that can be held in the set before it is full
   */
  uint32_t m_maxSize;

  /**
   * Buffer that holds the events in a random order
   */
  Buffer m_buffer;

  /**
   * Source of randomness used to shuffle the events
   */
  Ptr<RandomVariableStream> m_random;
};  // class RandomEventSet

}   //  ns3 namespace

#endif
