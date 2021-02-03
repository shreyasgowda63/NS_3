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

#ifndef POSITION_AWARE_HELPER_H
#define POSITION_AWARE_HELPER_H

#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/object-factory.h"

namespace ns3{
class PositionAware;

class PositionAwareHelper
{
 public:
  PositionAwareHelper();
  PositionAwareHelper(const Time& _t,const double& _d);
  ~PositionAwareHelper();
  void SetTimeout(const Time& _t);
  Time GetTimeout() const;
  void SetDistance(const double& _d);
  double GetDistance() const;
  void Install(Ptr<Node> _node) const;
  void Install(std::string& _node_name) const;
  void Install(NodeContainer _container) const;
  void InstallAll(void);
 private:
//  ObjectFactory position_aware_factory_;
  Time timeout_;
  double distance_;
};

}//namespace ns3


#endif//POSITION_AWARE_HELPER_H

