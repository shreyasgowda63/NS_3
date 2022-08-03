/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2022 TU Wien, Vienna
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
 * Author: Daniel Lukitsch <daniel.lukitsch96@gmail.com>
 */

#ifndef LRWPAN_PHY_LISTENER_H
#define LRWPAN_PHY_LISTENER_H

namespace ns3 {

class Time;

/**
 * \brief receive notifications about PHY events.
 */
class LrWpanPhyListener
{
public:
  // This destructor is needed.
  virtual ~LrWpanPhyListener () {}

  /**
   * Register the start of a reception.
   *
   * \param duration TODO
   */
  virtual void NotifyRxStart (Time duration) = 0;

  /**
   * Register the successful end of a reception.
   *
   */
  virtual void NotifyRxEndOk (void) = 0;

  /**
   * Register the end of an unsuccessful reception.
   *
   */
  virtual void NotifyRxEndError (void) = 0;

  /**
   * Register the start of a transmission.
   *
   * \param duration
   * \param txPowerDbm transmit power in dBm
   */
  virtual void NotifyTxStart (Time duration, double txPowerDbm) = 0;

  /**
   * Register RX and TX switchoff.
   *
   */
  virtual void NotifyTxOffRxOff (void) = 0;
  virtual void NotifyTxOffRxOffByForce (void) = 0;

  /**
   * Register receiver on.
   *
   */
  virtual void NotifyRxOn (void) = 0;

  /**
   * Register transmit on.
   *
   */
  virtual void NotifyTxOn (void) = 0;
};

} //namespace ns3

#endif /* LRWPAN_PHY_LISTENER_H */
