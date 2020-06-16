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
#include "ns3/position-filter.h"

#include "ns3/command-line.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/vector.h"
#include "ns3/gnuplot.h"

#include <fstream>
#include <iostream>
#include <iomanip>

/**
 * \file
 * \ingroup mobility-examples
 * \ingroup mobility
 * Example program illustrating use of ns3::HexagonalPositionAllocator
 */

using namespace ns3;

void
RandomPositionTable (Ptr<PositionAllocator> r, std::size_t npoints,
                     Ptr<HexagonalPositionAllocator> h)
{
  std::cout << "\n"
            << "Random Position           "
            << "Nearest Node                    "
            << "Delta                   "
            << "Length "
            << "Range   "
            << "Inside"
            << "\n"
            << "       X       Y       Z  Index "
            << "       X       Y       Z"
            << "       X       Y       Z "
            << "\n"
            << " ------- ------- -------  ------"
            << " ------- ------- -------"
            << "  ------ ------- -------"
            << "  ------ ------- ------- "
            << "\n"
            << std::fixed << std::setprecision (1)
            << std::endl;
  
  double maxRange = h->GetSpacing () / std::sqrt (3.0);
  std::size_t w = 8;
  
  for (std::size_t i = 0; i < npoints; ++i)
    {
      Vector3D v = r->GetNext();
      Vector3D e = h->GetNearestGridPoint (v);
      auto index = h->GetIndex (v);
      Vector3D delta = v - e;
      bool inRange = (delta.GetLength () < maxRange);
      bool inLayout = h->IsInside (v);

      std::cout << std::setw (w) << v.x << std::setw (w) << v.y << std::setw (w) << v.z
                << std::setw (w) << index
                << std::setw (w) << e.x << std::setw (w) << e.y << std::setw (w) << e.z
                << std::setw (w) << delta.x << std::setw (w) << delta.y << std::setw (w) << delta.z
                << std::setw (w) << delta.GetLength ()
                << (inRange  ? "      ok" : "     FAR")
                << (inLayout ? "      ok" : "     OUT")
                << std::endl;

    }  // for npoints

}  // RandomPostionTable


// There is a patch to provide this natively.
// Remove this function when that patch is merged.
std::string
GetDefaultOrientation (void)
{
  using Hex = HexagonalPositionAllocator;
  
  struct TypeId::AttributeInformation info;
  Hex::GetTypeId ().LookupAttributeByName ("Orientation", &info);
  std::string orientation
    = DynamicCast<const EnumValue> (info.initialValue)
    ->SerializeToString (MakeEnumChecker (Hex::FlatTop, "FlatTop", Hex::PointyTop, "PointyTop"));
  return orientation;
}  


int
main (int argc, char** argv)
{

  double spacing = 1000;
  std::size_t rings = 3;
  std::size_t npoints = 100;
  bool invert = false;
  bool gnuplot = false;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("spacing", "Distance between nodes, in meters", spacing);
  cmd.AddValue ("rings",   "Number of rings", rings);
  cmd.AddValue ("npoints",
                "Number of uniformly random points to sample within the layout",
                npoints);
  cmd.AddValue ("orientation",
                "ns3::HexagonalPositionAllocator::Orientation");
  cmd.AddValue ("invert", "Invert the meaning of \"inside\" when filtering",
                invert);
  cmd.AddValue ("gnuplot", "generate Gnuplot script", gnuplot);
  cmd.Parse (argc, argv);

  auto h = CreateObject<HexagonalPositionAllocator> ();
  h->SetSpacing (spacing);
  h->SetRings (rings);

  std::string orientation = GetDefaultOrientation ();

  std::cout << "HexagonalPositionAllocator:"
            << "\n    Orientation:      " << orientation
	    << "\n    Spacing:          " << h->GetSpacing () << " m"
	    << "\n    Rings:            " << h->GetRings ()
	    << "\n    Total nodes:      " << h->GetN ()
	    << "\n    Overall radius:   " << h->GetRadius () << " m"
            << "\n\nParameters for random sampling and filtering: "
            << "\n    Number of points: " << npoints
            << "\n    Filtering:        " << (invert ? "Inverse" : "Normal")
	    << "\n" << std::endl;

  std::cout << "Node   Position\n"
            << "Index           X           Y           Z\n"
            << "-----  ----------  ----------  ----------\n"
            << std::fixed << std::setprecision (1)
            << std::endl;
  for (std::size_t i = 0; i < h->GetN (); ++i)
    {
      auto p = h->GetNext ();
      std::cout << std::setw ( 5) << i
                << std::setw (12) << p.x
                << std::setw (12) << p.y
                << std::setw (12) << p.z
                << std::endl;
    }

  // Get some samples to test GetNearestGridPoint (), without filtering
  auto rnd = CreateObject<UniformDiscPositionAllocator> ();
  rnd->SetRho (h->GetRadius ());
  double maxRange = h->GetSpacing () / std::sqrt (3.0);


  std::cout << "\nRandom positions, no filtering:\n"
            << "Max disc radius:   " << h->GetRadius () << "m\n"
            << "Max allowed range: " << maxRange << "m\n"
            << "NOTE:  some points will be outside the layout since "
            << "the random disc covers more area than the layout hexagon.\n"
            << "All points should be within range of the nearest grid point, however.\n"
            << "Random                   Nearest Node            "
            << "Delta                  Distance  In Range   Inside"
            << std::fixed << std::setprecision (1)
            << std::endl;
  RandomPositionTable (rnd, npoints, h);


  // Demonstrate filtering
  auto hpf = MakePositionAllocatorFilter (h);
  auto fpa = CreateObject<FilteredPositionAllocator> ();
  fpa->SetPositionAllocator (rnd);
  fpa->SetPositionFilter (hpf);
  fpa->SetInvert (invert);

  std::cout << "\nRandom positions, WITH filtering:\n"
            << "Max disc radius:   " << h->GetRadius () << "\n"
            << "Max allowed range: " << maxRange << "\n"
            << "Invert or normal filtering: " << (invert ? "invert" : "normal") << "\n"
            << "NOTE:  In "
            << (invert ?
                "Inverse filtering ALL points should be " :
                "Normal filtering there should be NO points ")
            << "outside the layout.\n"
            << "Random                   Nearest Node            "
            << "Delta                  Distance  In Range   Inside"
            << std::fixed << std::setprecision (1)
            << std::endl;
  RandomPositionTable (fpa, npoints, h);
  
  if (gnuplot)
    {
      std::string plotName ("hex-allocator");
      Gnuplot2dDataset d;
      d.SetStyle (Gnuplot2dDataset::POINTS);
      d.SetExtra ("linestyle 1");

      // HexPositionAllocator repeats the point set
      for (std::size_t i = 0; i < h->GetN (); ++i)
        {
          auto p = h->GetNext ();
          d.Add (p.x, p.y);
        }

      std::string plotFile = plotName + ".png";
      Gnuplot g (plotFile, "Hex Position Allocator Example");
      g.SetTerminal ("png");
      g.SetExtra ("set size square");
      // Get a reasonable range
      double r = 1.05 * h->GetRadius ();
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
