/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Lawrence Livermore National Laboratory
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
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>,
 *  based on visual-simulator-impl.h by Gustavo Carneiro  <gjcarneiro@gmail.com>
 */

#ifndef SIMULATOR_ADAPTER_H
#define SIMULATOR_ADAPTER_H

#include "simulator-impl.h"

#include <vector>

/**
 * \file
 * \ingroup simulator
 * Class ns3::SimulatorAdapter declaration.
 */

namespace ns3 {

/**
 * \ingroup simulator
 *
 * An adapter class for SimulatorImpl derivatives which 
 * just need to modify a few behaviors of an underlying 
 * SimulatorImpl engine.
 *
 * To use this class, derive from it and override any 
 * functions you need to customize.
 *
 * For the discussion below we will use `CustomSimulator` to 
 * represent a particular SimulatorAdapter derivative.
 *
 * For users there are two choices to be made:  what derivative of SimulatorImpl
 * should be the real simulator engine underneath any adapters, and
 * the list of adapters to be applied.
 *
 * The choices for the base SimulatorImpl engine include DefaultSimulatorImpl,
 * DistributedSimulatorImpl, and NullMessageSimulatorImpl. This choice
 * can be made by setting the "SimulatorImplementationType" GlobalValue;
 * see GlobalValue for the ways to set this value apart from using a
 * SimulatorAdapter.
 *
 * Using a SimulatorAdapter one can configure the base engine 
 * using the ConfigureSimulator() method.  If the argument is empty
 * the existing value of the GlobalValue will be used.
 *
 * To configure the adapters use the AddAdapter() method:
 * \code
 *   SimulatorAdapter::AddAdapter ("ns3::CustomSimulator");
 *   SimulatorAdapter::AddAdapter ("ns3::AnotherCustomSimulator");
 * \endcode
 *
 * The adapter chain will be called in LIFO order:  AnotherCustomSimulator will
 * forward to CustomSimulator, which will forward to the real engine.
 */
class SimulatorAdapter : public SimulatorImpl
{
public:
  /**
   * Get the type Id.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Configure the underlying SimulatorImpl to use.
   * If no value is given, the value given by the "SimulatorImplementationType"
   * GlobalValue will be used.
   *
   * This also sets the SimulatorImplementationType to "ns3::SimulatorAdapter".
   *
   * \param [in] simImplType The base SimulatorImpl to use.  If blank
   * the current value of the "SimulatorImplementationType" GlobalValue
   * will be used.
   */
  static void ConfigureSimulator (std::string simImplType = "");

  /**
   * Add an adapter to the chain.
   * \param [in] adapterType Type name of the SimulatorAdapter.  Usually
   * this begins with "ns3::...".
   */
  static void AddAdapter (std::string adapterType);


  /** Constructor. */
  SimulatorAdapter ();
  /** Destructor. */
  virtual ~SimulatorAdapter ();

  // Inherited from SimulatorImpl.  These are the customization points.
  virtual void Destroy ();
  virtual bool IsFinished (void) const;
  virtual void Stop (void);
  virtual void Stop (Time const &delay);
  virtual EventId Schedule (Time const &delay, EventImpl *event);
  virtual void ScheduleWithContext (uint32_t context, Time const &delay, EventImpl *event);
  virtual EventId ScheduleNow (EventImpl *event);
  virtual EventId ScheduleDestroy (EventImpl *event);
  virtual void Remove (const EventId &id);
  virtual void Cancel (const EventId &id);
  virtual bool IsExpired (const EventId &id) const;
  virtual void Run (void);
  virtual Time Now (void) const;
  virtual Time GetDelayLeft (const EventId &id) const;
  virtual Time GetMaximumSimulationTime (void) const;
  virtual void SetScheduler (ObjectFactory schedulerFactory);
  virtual uint32_t GetSystemId (void) const;
  virtual uint32_t GetContext (void) const;
  virtual uint64_t GetEventCount (void) const;
  virtual void PreEventHook (const EventId & id) {};

protected:

  // Inherited from ObjectBase
  virtual void NotifyConstructionCompleted (void);

  // Inherited from Object
  virtual void DoDispose (void);

  /** 
   * The real SimulatorImpl type to use. 
   * This defaults to DefaultSimulatorImpl, but can be changed
   * through the SimulatorImplType Attribute.
   */
  std::string m_simulatorImplType;

  /** 
   * The list of adapters to chain together. 
   * Add to the chain with AddAdapter().
   */
  // Is this really needed?  We're an Object, after all.
  static std::vector<std::string> m_adapters;
  /** 
   * The next SimulatorAdapter (or the real SimulatorImpl) in the chain.
   */
  Ptr<SimulatorImpl> m_simulator; ///< the simulator implementation

};

} // namespace ns3

#endif /* SIMULATOR_ADAPTER_H */
