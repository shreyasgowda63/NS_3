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

#ifndef FQ_PIE_QUEUE_DISC
#define FQ_PIE_QUEUE_DISC

#include "ns3/fq-queue-disc.h"
namespace ns3 {

/**
 * \ingroup traffic-control
 *
 * \brief A FqPie packet queue disc
 */

class FqPieQueueDisc : public FqQueueDisc {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief FqPieQueueDisc constructor
   */
  FqPieQueueDisc ();

  virtual ~FqPieQueueDisc ();

private:
  virtual void InitializeParams (void);

  // PIE queue disc parameter
  double m_markEcnTh;        //!< ECN marking threshold (default 10% as suggested in RFC 8033)
  Time m_sUpdate;            //!< Start time of the update timer
  Time m_tUpdate;            //!< Time period after which CalculateP () is called
  Time m_qDelayRef;          //!< Desired queue delay
  uint32_t m_meanPktSize;    //!< Average packet size in bytes
  Time m_maxBurst;           //!< Maximum burst allowed before random early dropping kicks in
  double m_a;                //!< Parameter to pie controller
  double m_b;                //!< Parameter to pie controller
  uint32_t m_dqThreshold;    //!< Minimum queue size in bytes before dequeue rate is measured
  bool m_useDqRateEstimator; //!< Enable/Disable usage of dequeue rate estimator for queue delay calculation
  bool  m_isCapDropAdjustment;//!< Enable/Disable Cap Drop Adjustment feature mentioned in RFC 8033
  bool m_useDerandomization; //!< Enable Derandomization feature mentioned in RFC 8033
  Time m_activeThreshold;    //!< Threshold for activating PIE (disabled by default)
};

} // namespace ns3

#endif /* FQ_PIE_QUEUE_DISC */