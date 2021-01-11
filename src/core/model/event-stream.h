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

#ifndef EVENT_STREAM_H_
#define EVENT_STREAM_H_

#include "sim-event.h"
#include "object.h"
#include "random-variable-stream.h"

#include <deque>
#include <vector>

/**
 * \file
 * \ingroup events
 * ns3::EventStream definition
 */

namespace ns3 {

/**
 * \ingroup events
 * An abstract base class which defines the interface for an event stream
 */
class EventStream : public Object
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
  EventStream ()
  {}

  /**
   * Destructor
   */
  virtual ~EventStream ()
  {}

  /**
   * Check if the stream has more events
   *
   * \return \c true if the stream is empty, \c false otherwise
   */
  virtual bool IsEmpty () const = 0;

  /**
   * Check if the stream has space to insert more events
   *
   * \return \c true if the stream has space to hold another event, \c false
   * otherwise
   */
  virtual bool IsFull () const = 0;

  /**
   * Add an event to the stream.
   *
   * The position of the inserted event is left as an implementation detail
   * of the derived class.
   *
   * \pre IsFull () must be \c false
   *
   * \param ev Event to add to the stream
   *
   * \return \c true if the event was added to the stream, \c false otherwise
   */
  virtual bool Insert (SimEvent ev) = 0;

  /**
   * Look at the next event in the stream without removing it
   *
   * \pre IsEmpty () must be \c false
   *
   * \return A const reference to the next event in the stream
   */
  virtual const SimEvent& Peek () const = 0;

  /**
   * Remove the next event in the stream
   *
   * \pre IsEmpty () must be \c false
   *
   * \return The event that was removed
   */
  virtual SimEvent Next () = 0;

  /**
   * Remove an event with \p key from the stream
   *
   * Searches the stream for an event that matches \p key and removes it
   *
   * \param key Key of the event to remove
   *
   * \return \c true if the event was found in the stream, \c false otherwise
   */
  virtual bool Remove (const SimEventKey& key) = 0;
};  //  class EventStream

/**
 * An event stream implementation that returns events in the same
 * order they were inserted (first in, first out)
 */
class FifoEventStream : public EventStream
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
  FifoEventStream ();

  /**
   * Destructor
   */
  virtual ~FifoEventStream ();

  /**
   * Set the maximum number of events that the stream can hold
   *
   * \param newSize The maximum number of events the stream can hold
   */
  void SetStreamSize (uint32_t newSize);

  /**
   * Get the maximum number of events that the stream can hold
   */
  uint32_t GetStreamSize () const;

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
   * Maximum size of the stream
   */
  uint32_t m_streamSize;

  /**
   * Location of the next event in the stream
   */
  std::size_t m_head;

  /**
   * Location of the last event in the stream
   */
  std::size_t m_tail;

  /**
   * Number of events in the stream
   */
  uint32_t m_count;

  /**
   * Storage area for events
   */
  Buffer m_buffer;
};  // class FifoEventStream

/**
 * An event stream implementation that returns events in a random order
 */
class RandomEventStream : public EventStream
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
  RandomEventStream ();

  /**
   * Destructor
   */
  virtual ~RandomEventStream ();

  void SetRandomSource (Ptr<RandomVariableStream> rand);

  /**
   * Set the maximum number of events that the stream can hold
   *
   * \param newSize The maximum number of events the stream can hold
   */
  void SetStreamSize (uint32_t newSize);

  /**
   * Get the maximum number of events that the stream can hold
   */
  uint32_t GetStreamSize () const;

  //Inherited functions
  virtual bool IsEmpty () const;
  virtual bool IsFull () const;
  virtual bool Insert (SimEvent ev);
  virtual const SimEvent& Peek () const;
  virtual SimEvent Next ();
  virtual bool Remove (const SimEventKey& key);

private:
  using Buffer = std::deque<SimEvent>;

  uint32_t m_streamSize;
  Buffer m_buffer;
  Ptr<RandomVariableStream> m_random;
};  // class RandomEventStream

}   //  ns3 namespace

#endif
