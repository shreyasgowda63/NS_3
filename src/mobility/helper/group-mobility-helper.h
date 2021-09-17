/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 University of Padova
 * Copyright (c) 2020 Institute for the Wireless Internet of Things, Northeastern University, Boston, MA
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
 * Author: Michele Polese <michele.polese@gmail.com>
 */

#ifndef GROUP_MOBILITY_HELPER_H
#define GROUP_MOBILITY_HELPER_H

#include <vector>
#include "ns3/object-factory.h"
#include "ns3/attribute.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/position-allocator.h"
#include "node-container.h"
#include "ns3/mobility-helper.h"

namespace ns3 {

class PositionAllocator;
class MobilityModel;

/**
 * \ingroup mobility
 * \brief Helper used to install primary and secondary mobility models
 *
 * GroupMobilityHelper::InstallGroupMobility is the most important method here.
 */
class GroupMobilityHelper : public Object
{
public:
  /**
   * Construct a GroupMobilityHelper
   */
  GroupMobilityHelper ();

  /**
   * Destroy a GroupMobilityHelper
   */
  ~GroupMobilityHelper ();

  static TypeId GetTypeId (void);

  /**
   * Get a pointer to the associated MobilityHelper
   * \return a pointer to the MobilityHelper
   */
  MobilityHelper* GetMobilityHelper ();

  /**
   * Set a pointer to the associated MobilityHelper
   * \param a pointer to the MobilityHelper
   */
  void SetMobilityHelper (MobilityHelper* helper);

  /**
   * Install the secondary mobility model in the nodes passed as parameter
   * \param nodes a NodeContainer with the secondary nodes
   * \return a NodeContainer with the primary and the secondary
   */
  NodeContainer InstallGroupMobility (NodeContainer nodes);

protected:
  MobilityHelper* m_mobilityHelper; //!< the MobilityHelper used to configure the primary
  MobilityHelper m_privateMobilityHelper; //!< the MobilityHelper used to configure the slave
  std::string m_randomVarString; //!< a string which describes the type of RandomVariableStream for the variance of the slaves
  std::string m_secondaryMobilityModel; //!< a string which describes the type of secondary MobilityModel

};

} // namespace ns3

#endif /* GROUP_MOBILITY_HELPER_H */