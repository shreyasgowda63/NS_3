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
#ifndef GROUP_SECONDARY_OUTDOOR_MOBILITY_MODEL_H
#define GROUP_SECONDARY_OUTDOOR_MOBILITY_MODEL_H

#include "ns3/group-secondary-mobility-model.h"

namespace ns3 {

/**
 * \ingroup buildings
 *
 * \brief MobilityModel which inherits GroupSecondaryMobilityModel but also checks if the position of the secondary is outdoor
 */
class GroupSecondaryOutdoorMobilityModel : public GroupSecondaryMobilityModel
{
public:
  static TypeId GetTypeId (void);

protected:
  virtual bool CheckForSpecialConditions (Vector position);

private:
  /**
   * Check if a position is indoor or not
   * \param position a Vector with the position
   * \return true if outdoor
   */
  bool IsOutdoor (Vector position);
};

} // namespace ns3

#endif /* GROUP_SECONDARY_OUTDOOR_MOBILITY_MODEL_H */
