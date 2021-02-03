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

namespace ns3 {
class PositionAware;
/** @ingroup PositionAware
 * @brief Helper for creating/installing position aware objects
 */
class PositionAwareHelper
{
public:
  //Force the use of non-default values.
  PositionAwareHelper () = delete;
  /**
   * @brief Construct a new Position Aware Helper object with a given delta_position and timeout
   *
   * @param time desired timeout
   * @param deltaPosition desired delta_position (distance the object must move to trigger callback)
   * @note Using the helper overrides the values that exist in the attribute system
   */
  PositionAwareHelper (const Time& time,const double& deltaPosition);
  ~PositionAwareHelper ();
  /**
   * @brief Set the Timeout
   *
   * @param time The desired timeout duration
   */
  void SetTimeout (const Time& time);
  /**
   * @brief Get the Timeout
   *
   * @return Time value coresponding to the timeout
   */
  Time GetTimeout () const;
  /**
   * @brief Set the delta position (the distance the object can move from reference position)
   *
   * @param deltaPosition delta position (the distance the object can move from reference position)
   */
  void SetDeltaPosition (const double& deltaPosition);
  /**
   * @brief Get the delta position (the distance the object can move from reference position)
   *
   * @return delta position (the distance the object can move from reference position)
   */
  double GetDeltaPosition () const;
  /**
   * @brief Installs position aware on single node
   *
   * @param node node to install on
   */
  void Install (Ptr<Node> node) const;
  /**
   * @brief Installs position aware on a single node by name
   * @note Mobility must be installed first
   *
   * @param node_name
   */
  void Install (std::string& node_name) const;
  /**
   * @brief Installs position aware on all nodes in a node container
   * @note Mobility must be installed first
   *
   * @param container
   */
  void Install (NodeContainer container) const;
  /**
   * @brief Installs position aware on all nodes create so far
   * @note Mobility must be installed first
   *
   */
  void InstallAll (void);

private:
  /// Timeout to use while creating objects
  Time m_timeout;
  /// delta positions to use while creating objects
  double m_deltaPosition;
};

} //namespace ns3


#endif //POSITION_AWARE_HELPER_H

