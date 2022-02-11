/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Gustavo Carneiro
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
 * Author: Gustavo Carneiro  <gjcarneiro@gmail.com>
 */

#ifndef VISUAL_SIMULATOR_IMPL_H
#define VISUAL_SIMULATOR_IMPL_H

#include "ns3/simulator-adapter.h"

namespace ns3 {


/**
 * \defgroup  visualizer Visualizer
 *
 */

/**
 * \ingroup visualizer
 * \ingroup simulator
 *
 * \brief A replacement simulator that starts the visualizer
 *
 * To use this class, run any ns-3 simulation with the command-line
 * argument --SimulatorImplementationType=ns3::VisualSimulatorImpl.
 * This causes the visualizer (PyViz) to start automatically.
 **/
class VisualSimulatorImpl : public SimulatorAdapter
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  VisualSimulatorImpl ();
  ~VisualSimulatorImpl ();

  /// calls Run() in the wrapped simulator
  void RunRealSimulator (void);

  // Inherited
  virtual void Run (void);

protected:
  void DoDispose ();
  void NotifyConstructionCompleted (void);

};

} // namespace ns3

#endif /* VISUAL_SIMULATOR_IMPL_H */
