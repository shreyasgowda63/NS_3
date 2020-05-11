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
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "ns3/hex-position-allocator.h"
#include "ns3/gnuplot.h"
#include "ns3/command-line.h"

#include <cstdlib>    // system()
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

/**
 * \file
 * \ingroup mobility-examples
 * \ingroup mobility
 * Example program illustrating use of ns3::HexagonalPositionAllocator
 */

using namespace ns3;



int
main (int argc, char** argv)
{

  double spacing = 1000;
  int rings = 2;
  bool gnuplot = false;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("isd",     "inter-site distance, in meters", spacing);
  cmd.AddValue ("rings",   "number of rings", rings);
  cmd.AddValue ("gnuplot", "generate Gnuplot script", gnuplot);
  cmd.Parse (argc, argv);

  HexagonalPositionAllocator h;
  h.SetSpacing (spacing);
  h.SetRings (rings);

  std::cout << "HexagonalPositionAllocator:"
            << "\n    Spacing:        " << h.GetSpacing ()
            << "\n    Rings:          " << h.GetRings ()
            << "\n    Total nodes:    " << h.GetN ()
            << "\n    Overall radius: " << h.GetRadius ()
            << "\n" << std::endl;

  std::cout << "Index           X           Y           Z"
            << std::fixed << std::setprecision (1)
            << std::endl;
  for (std::size_t i = 0; i < h.GetN (); ++i)
    {
      auto p = h.GetNext ();
      std::cout << std::setw ( 5) << i
                << std::setw (12) << p.x
                << std::setw (12) << p.y
                << std::setw (12) << p.z
                << std::endl;
    }


  if (gnuplot)
    {
      std::string plotName ("main-hex-allocator");
      Gnuplot2dDataset d;
      d.SetStyle (Gnuplot2dDataset::POINTS);
      d.SetExtra ("linestyle 1");

      // HexPositionAllocator repeats the point set
      for (std::size_t i = 0; i < h.GetN (); ++i)
        {
          auto p = h.GetNext ();
          d.Add (p.x, p.y);
        }

      std::string plotFile = plotName + ".png";
      Gnuplot g (plotFile, "Hex Position Allocator Example");
      g.SetTerminal ("png");
      g.SetExtra ("set size square");
      // Get a reasonable range
      double r = 1.05 * h.GetRadius ();
      std::stringstream ss;
      ss << "set xrange [-" << r << ":" << r << "]";
      g.AppendExtra (ss.str ());
      g.AppendExtra ("set key off");
      g.AppendExtra ("set style line 1 pointtype 7 pointsize 2");
      g.AddDataset (d);


      std::string gnuFile = plotName + ".plt";
      std::cout << "\nWriting Gnuplot file: " << gnuFile << std::endl;
      std::ofstream gnuStream (gnuFile);
      g.GenerateOutput (gnuStream);
      gnuStream.close ();

      std::cout << "Generating " << plotFile;
      std::string shellcmd = "gnuplot " + gnuFile;
      std::system (shellcmd.c_str ());
    }
  return 0;
}
