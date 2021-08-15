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
/** @ingroup PositionAware 
 * @brief Helper for creating/installing position aware objects
 */
class PositionAwareHelper
{
 public:
  PositionAwareHelper();
  /**
   * @brief Construct a new Position Aware Helper object with a given radius and timeout
   * 
   * @param _t desired timeout
   * @param _d desired radius
   */
  PositionAwareHelper(const Time& _t,const double& _d);
  ~PositionAwareHelper();
  /**
   * @brief Set the Timeout
   * 
   * @param _t timeout
   */
  void SetTimeout(const Time& _t);
  /**
   * @brief Get the Timeout
   * 
   * @return Time 
   */
  Time GetTimeout() const;
  /**
   * @brief Set the Distance
   * 
   * @param _d distance
   */
  void SetDistance(const double& _d);
  /**
   * @brief Get the Distance
   * 
   * @return double 
   */
  double GetDistance() const;
  /**
   * @brief Installs position aware on single node
   * 
   * @param _node node to install on
   */
  void Install(Ptr<Node> _node) const;
  /**
   * @brief Installs position aware on a single node by name
   * @note Mobility must be installed first
   * 
   * @param _node_name 
   */
  void Install(std::string& _node_name) const;
  /**
   * @brief Installs position aware on all nodes in a node container
   * @note Mobility must be installed first
   * 
   * @param _container 
   */
  void Install(NodeContainer _container) const;
  /**
   * @brief Installs position aware on all nodes create so far
   * @note Mobility must be installed first
   * 
   */
  void InstallAll(void);
 private:
  /// Timeout to use while creating objects
  Time m_timeout;
  /// Distance to use while creating objects
  double m_distance;
};

}//namespace ns3


#endif//POSITION_AWARE_HELPER_H

