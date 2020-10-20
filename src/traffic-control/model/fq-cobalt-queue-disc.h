/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 NITK Surathkal
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
 * Authors: Bhaskar Kataria <bhaskar.k7920@gmail.com>
 *          Tom Henderson <tomhend@u.washington.edu> 
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 *          Vivek Jain <jain.vivek.anand@gmail.com>
 *          Ankit Deepak <adadeepak8@gmail.com>
 */

#ifndef FQ_COBALT_QUEUE_DISC
#define FQ_COBALT_QUEUE_DISC

#include "ns3/fq-queue-disc.h"
namespace ns3 {

/**
 * \ingroup traffic-control
 *
 * \brief A FqCobalt packet queue disc
 */

class FqCobaltQueueDisc : public FqQueueDisc {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief FqCobaltQueueDisc constructor
   */
  FqCobaltQueueDisc ();

  virtual ~FqCobaltQueueDisc ();

private:
  virtual void InitializeParams (void);
  std::string m_interval;    //!< CoDel interval attribute
  std::string m_target;      //!< CoDel target attribute
  double m_increment;        //!< increment value for marking probability
  double m_decrement;        //!< decrement value for marking probability
  double m_Pdrop;            //!< Drop Probability
  Time m_blueThreshold;      //!< Threshold to enable blue enhancement

};

} // namespace ns3

#endif /* FQ_COBALT_QUEUE_DISC */

