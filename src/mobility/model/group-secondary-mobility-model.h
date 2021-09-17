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

#ifndef GROUP_SECONDARY_MOBILITY_MODEL_H
#define GROUP_SECONDARY_MOBILITY_MODEL_H

#include "mobility-model.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {

/**
 * \ingroup mobility
 *
 * \brief MobilityModel which follows a primary MobilityModel with a certain deviation
 */
class GroupSecondaryMobilityModel : public MobilityModel
{
public:
  /**
   * Register this type with the TypeId system.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Create a GroupSecondaryMobilityModel object
   */
  GroupSecondaryMobilityModel ();
  
  /**
   * Destroy a GroupSecondaryMobilityModel object
   */
  virtual ~GroupSecondaryMobilityModel ();

  /**
   * Get the associated primary MobilityModel
   * \return a smart pointer to the associated primary MobilityModel
   */
  Ptr<MobilityModel> GetPrimaryMobilityModel ();

  /**
   * Set the associated primary MobilityModel
   * \param a smart pointer to the associated primary MobilityModel
   */
  void SetPrimaryMobilityModel (Ptr<MobilityModel> model);

  /**
   * Method to be called when the primary trigger the NotifyCourseChanged callback
   * \param a smart pointer to the associated primary MobilityModel
   */
  void PrimaryCourseChanged (Ptr<MobilityModel const> primary);

protected:
  /**
   * check for conditions that can lead to the rejection of the selected position
   * \param postion a Vector with a position
   * \return a bool, which is true if the special condition applies
   */
  virtual bool CheckForSpecialConditions (Vector position);

private:
  Vector DoGetPosition (void) const;
  Vector DoGetVelocity (void) const;
  void DoSetPosition (const Vector &position);

protected:
  Ptr<MobilityModel> m_primary; //!< the primary mobility model
  Ptr<RandomVariableStream> m_randomVar; //<! the random variable that is used to randomize the position
  Vector m_lastPosition; //!< the last position computed after a primary course change
};

} // namespace ns3

#endif /* GROUP_SECONDARY_MOBILITY_MODEL_H */
