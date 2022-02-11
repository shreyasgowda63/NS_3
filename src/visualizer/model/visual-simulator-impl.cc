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
 * Author: Gustavo Carneiro <gjcarneiro@gmail.com> <gjc@inescporto.pt>
 */
#include <Python.h>
#undef HAVE_PTHREAD_H
#undef HAVE_SYS_STAT_H
#include "visual-simulator-impl.h"
#include "ns3/default-simulator-impl.h"
#include "ns3/log.h"
#include "ns3/packet-metadata.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VisualSimulatorImpl");

NS_OBJECT_ENSURE_REGISTERED (VisualSimulatorImpl);


TypeId
VisualSimulatorImpl::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VisualSimulatorImpl")
    .SetParent<SimulatorAdapter> ()
    .SetGroupName ("Visualizer")
    .AddConstructor<VisualSimulatorImpl> ()
  ;
  return tid;
}


VisualSimulatorImpl::VisualSimulatorImpl ()
  : SimulatorAdapter ()
{
  PacketMetadata::Enable ();
}

VisualSimulatorImpl::~VisualSimulatorImpl ()
{
}

void
VisualSimulatorImpl::DoDispose (void)
{
  SimulatorAdapter::DoDispose ();
}

void
VisualSimulatorImpl::Run (void)
{
  if (!Py_IsInitialized ())
    {
      #if PY_MAJOR_VERSION >= 3
        const wchar_t *argv[] = { L"python", NULL};
        Py_Initialize ();
        PySys_SetArgv (1, (wchar_t**) argv);
      #else
        const char *argv[] = { "python", NULL};
        Py_Initialize ();
        PySys_SetArgv (1, (char**) argv);
      #endif
      PyRun_SimpleString (
                          "import visualizer\n"
                          "visualizer.start();\n"
                          );
    }
  else
    {
      PyGILState_STATE __py_gil_state = PyGILState_Ensure ();
    
      PyRun_SimpleString (
                          "import visualizer\n"
                          "visualizer.start();\n"
                          );

      PyGILState_Release (__py_gil_state);
    }
}

void
VisualSimulatorImpl::RunRealSimulator (void)
{
  m_simulator->Run ();
}


} // namespace ns3


