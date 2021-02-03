/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2020 National Technology & Engineering Solutions of Sandia,
 * LLC (NTESS). Under the terms of Contract DE-NA0003525 with NTESS,
 * the U.S. Government retains certain rights in this software.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 *
 */

#ifndef BRUTEFORCE_HPP
#define BRUTEFORCE_HPP

#include "spatial-index.h"

#include <map>

namespace ns3 {


  /**
   * \ingroup spatial-index
   *
   * \brief Spatial index implementation using a brute force.
   */
class BruteForceSpatialIndexing: public ns3::SpatialIndexing {
 public:
  virtual void doAdd(Ptr<const Node> _node, const Vector& _position)                                    override;
  virtual void remove(Ptr<const Node> _node)                                 override;
  virtual void update(Ptr<const Node> _node, const Vector& _position)                                 override;
  virtual std::vector<Ptr<const Node> > getNodesInRange(double _range, const Vector& _position, const Ptr<const Node> _sourceNode) override; //Ptr<const MobilityModel> _node, double _range)  override;

  virtual void HandlePositionChange(Ptr<const PositionAware> _position_aware) override;

private:
  /// Maps nodes to positions
  std::map<Ptr<const Node>, Vector> m_map;
};

}//namespace ns3
#endif//BRUTEFORCE_HPP

