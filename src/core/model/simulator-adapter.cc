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
 */

#include "simulator-adapter.h"

#include "config.h"
#include "global-value.h"
#include "log.h"
#include "string.h"

/**
 * \file
 * \ingroup simulator
 * Class ns3::SimulatorAdapter implementation.
 */

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimulatorAdapter");

NS_OBJECT_ENSURE_REGISTERED (SimulatorAdapter);


/* static */
std::vector<std::string> SimulatorAdapter::m_adapters {};


TypeId
SimulatorAdapter::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimulatorAdapter")
    .SetParent<SimulatorImpl> ()
    .SetGroupName ("Core")
    .AddConstructor<SimulatorAdapter> ()
    .AddAttribute ("SimulatorImplementationType",
                   "Underlying simulator implementation type.",
                   StringValue ("ns3::DefaultSimulatorImpl"),
                   MakeStringAccessor (&SimulatorAdapter::m_simulatorImplType),
                   MakeStringChecker ())
  ;
  return tid;
}


/* static */
void
SimulatorAdapter::ConfigureSimulator (std::string simImplType /* = "" */)
{
  NS_LOG_FUNCTION (simImplType);
  std::string simImpl {simImplType};
  if (simImpl.size () == 0)
    {
      StringValue siv;
      GlobalValue::GetValueByName ("SimulatorImplementationType", siv);
      simImpl = siv.Get();
      NS_LOG_LOGIC ("using GlobalValue " << simImpl);
    }
  else
    {
      NS_LOG_LOGIC ("using supplied type " << simImplType);
    }
  Config::SetDefault ("ns3::SimulatorAdapter::SimulatorImplementationType",
                      StringValue (simImpl));
}


/* static */
void
SimulatorAdapter::AddAdapter (std::string adapterType)
{
  NS_LOG_FUNCTION (adapterType);
  // Error check:  is adapterType a SimulatorAdapter?
  m_adapters.push_back (adapterType);

  GlobalValue::Bind ("SimulatorImplementationType", StringValue (adapterType));
}


void
SimulatorAdapter::NotifyConstructionCompleted ()
{
  static bool completionInProgress {false};

  if (completionInProgress)
    {
      // We're being called reentrantly from the adapters
      // we're creating so just forward up from this instance
      NS_LOG_LOGIC (GetInstanceTypeId ().GetName () << 
                    ": notifying up (reentrant)");
      SimulatorImpl::NotifyConstructionCompleted ();
      return;
    }

  // Build the chain of adapters
  completionInProgress = true;
  NS_LOG_LOGIC ("instantiating chain");

  // There should be at least one adapter
  NS_ASSERT_MSG (m_adapters.size (), "Need to SimulatorAdapter::AddAdapter()");

  // We should be the last adapter added
  std::string me = GetInstanceTypeId ().GetName ();
  NS_ASSERT_MSG (m_adapters.back () == me,
                 "SimulatorAdapter instance is not the last one added.");

  // Instantiate the base simulator
  ObjectFactory factory;
  factory.SetTypeId (m_simulatorImplType);
  Ptr<SimulatorImpl> last = factory.Create<SimulatorImpl> ();
  NS_LOG_LOGIC ("created base simulator " << m_simulatorImplType
                << " @" << std::hex << last);

  // Construct the adapter chain
  for (auto adapter : m_adapters)
    {
      if (adapter == me)
        {
          break;  // I'm already instantiated
        }
      NS_LOG_LOGIC ("adding adapter " << adapter);
      factory.SetTypeId (adapter);
      Ptr<SimulatorAdapter> next = factory.Create<SimulatorAdapter> ();
      NS_ASSERT_MSG (next, "failed creating adapter " << adapter);
      NS_LOG_LOGIC ("added adapter " << adapter << 
                    " @" << std::hex << PeekPointer (next) );
      next->m_simulator = last;
      AggregateObject (next);
      last = next;
    }
  NS_LOG_LOGIC ("setting final instance (" << me << ") to call adapter"
                " @" << std::hex << last);
  m_simulator = last;

  NS_LOG_LOGIC ("created all adapters, notifying up");
  // Finally, notify base class
  SimulatorImpl::NotifyConstructionCompleted ();
  NS_LOG_LOGIC ("done");
}


SimulatorAdapter::SimulatorAdapter ()
  : SimulatorImpl ()
{
  NS_LOG_FUNCTION (this);
}


SimulatorAdapter::~SimulatorAdapter ()
{
}


/******** Forwarding implementations of inherited functions ********/

void
SimulatorAdapter::Destroy ()
{
  m_simulator->Destroy ();
}

bool 
SimulatorAdapter::IsFinished (void) const
{
  return m_simulator->IsFinished ();
}

void 
SimulatorAdapter::Stop (void)
{
  m_simulator->Stop ();
}

void 
SimulatorAdapter::Stop (Time const &delay)
{
  m_simulator->Stop (delay);
}

EventId
SimulatorAdapter::Schedule (Time const &delay, EventImpl *event)
{
  return m_simulator->Schedule (delay, event);
}

void
SimulatorAdapter::ScheduleWithContext (uint32_t context, Time const &delay, 
                                           EventImpl *event)
{
  m_simulator->ScheduleWithContext (context, delay, event);
}

EventId
SimulatorAdapter::ScheduleNow (EventImpl *event)
{
  return m_simulator->ScheduleNow (event);
}

EventId
SimulatorAdapter::ScheduleDestroy (EventImpl *event)
{
  return m_simulator->ScheduleDestroy (event);
}

void
SimulatorAdapter::Remove (const EventId &id)
{
  m_simulator->Remove (id);
}

void
SimulatorAdapter::Cancel (const EventId &id)
{
  m_simulator->Cancel (id);
}

bool
SimulatorAdapter::IsExpired (const EventId &id) const
{
  return m_simulator->IsExpired (id);
}

void
SimulatorAdapter::Run (void)
{
  m_simulator->Run ();
}

Time
SimulatorAdapter::Now (void) const
{
  return m_simulator->Now ();
}

Time 
SimulatorAdapter::GetDelayLeft (const EventId &id) const
{
  return m_simulator->GetDelayLeft (id);
}

Time 
SimulatorAdapter::GetMaximumSimulationTime (void) const
{
  return m_simulator->GetMaximumSimulationTime ();
}

void
SimulatorAdapter::SetScheduler (ObjectFactory schedulerFactory)
{
  m_simulator->SetScheduler (schedulerFactory);
}

uint32_t 
SimulatorAdapter::GetSystemId (void) const
{
  return m_simulator->GetSystemId ();
}

uint32_t
SimulatorAdapter::GetContext (void) const
{
  return m_simulator->GetContext ();
}

uint64_t
SimulatorAdapter::GetEventCount (void) const
{
  return m_simulator->GetEventCount ();
}

void
SimulatorAdapter::DoDispose (void)
{
  if (m_simulator)
    {
      m_simulator->Dispose ();
      m_simulator = NULL;
    }
  SimulatorImpl::DoDispose ();
}


} // namespace ns3
