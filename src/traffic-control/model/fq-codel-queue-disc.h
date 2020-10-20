/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Universita' degli Studi di Napoli Federico II
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
 * Authors: Pasquale Imputato <p.imputato@gmail.com>
 *          Stefano Avallone <stefano.avallone@unina.it>
 */

#ifndef FQ_CODEL_QUEUE_DISC
#define FQ_CODEL_QUEUE_DISC

#include "ns3/fq-queue-disc.h"

namespace ns3 {

/**
 * \ingroup traffic-control
 *
 * \brief A flow queue used by the FqCoDel queue disc
 */

class FqCoDelQueueDisc : public FqQueueDisc{
public:
  /**
   * \brief FqCoDelQueueDisc constructor
   */
  FqCoDelQueueDisc ();

  virtual ~FqCoDelQueueDisc ();
  static TypeId GetTypeId (void);
private:
   void InitializeParams (void);
  std::string m_interval;    //!< CoDel interval attribute
  std::string m_target;      //!< CoDel target attribute
};
} // namespace ns3

#endif /* FQ_CODEL_QUEUE_DISC */