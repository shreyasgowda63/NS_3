/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright(c) 2005 - 2014 Intel Corporation. All rights reserved.
 * Copyright(c) 2013 - 2015 Intel Mobile Communications GmbH
 * Copyright(c) 2016 - 2017 Intel Deutschland GmbH
 * Copyright(c) 2018 - 2019 Intel Corporation
 * Copyright(c) 2019 - Rémy Grünblatt <remy@grunblatt.org>
 * Copyright(c) 2021 - Alexander Krotov <krotov@iitp.ru>
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
 * Authors: Rémy Grünblatt <remy@grunblatt.org>
 *          Alexander Krotov <krotov@iitp.ru>
 */
#ifndef INTEL_RATE_WIFI_MANAGER_H
#define INTEL_RATE_WIFI_MANAGER_H

#include "ns3/traced-value.h"
#include "ns3/wifi-remote-station-manager.h"

namespace ns3 {

/**
 * \ingroup wifi
 * \brief Intel rate control from Linux.
 */
class IntelWifiManager : public WifiRemoteStationManager
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  IntelWifiManager ();
  virtual ~IntelWifiManager ();

private:
  //overridden from base class
  void DoInitialize (void) override;
  WifiRemoteStation* DoCreateStation (void) const override;
  void DoReportRxOk (WifiRemoteStation *station,
                     double rxSnr, WifiMode txMode) override;
  void DoReportRtsFailed (WifiRemoteStation *station) override;
  void DoReportDataFailed (WifiRemoteStation *station) override;
  void DoReportRtsOk (WifiRemoteStation *station,
                      double ctsSnr, WifiMode ctsMode, double rtsSnr) override;
  void DoReportDataOk (WifiRemoteStation *station, double ackSnr, WifiMode ackMode,
                       double dataSnr, uint16_t dataChannelWidth, uint8_t dataNss) override;
  void DoReportAmpduTxStatus (WifiRemoteStation *station, uint16_t nSuccessfulMpdus, uint16_t nFailedMpdus,
                              double rxSnr, double dataSnr, uint16_t dataChannelWidth, uint8_t dataNss) override;
  void DoReportFinalRtsFailed (WifiRemoteStation *station) override;
  void DoReportFinalDataFailed (WifiRemoteStation *station) override;
  WifiTxVector DoGetDataTxVector (WifiRemoteStation *station) override;
  WifiTxVector DoGetRtsTxVector (WifiRemoteStation *station) override;

  void CheckInit (WifiRemoteStation *station);

  /**
   * Check the validity of a combination of number of streams, chWidth and mode.
   *
   * \param phy pointer to the wifi phy
   * \param streams the number of streams
   * \param chWidth the channel width (MHz)
   * \param mode the wifi mode
   * \returns true if the combination is valid
   */
  bool IsValidMcs (Ptr<WifiPhy> phy, uint8_t streams, uint16_t chWidth, WifiMode mode);

  /**
   * Returns a list of only the VHT MCS supported by the device.
   * \returns the list of the VHT MCS supported
   */
  WifiModeList GetVhtDeviceMcsList (void) const;

  /**
   * Returns a list of only the HT MCS supported by the device.
   * \returns the list of the HT MCS supported
   */
  WifiModeList GetHtDeviceMcsList (void) const;

  int MAX_HT_GROUP_RATES = 8;     //!< Number of rates (or MCS) per HT group.
  int MAX_VHT_GROUP_RATES = 10;   //!< Number of rates (or MCS) per VHT group.

  WifiMode m_ctlMode;  //!< Wifi mode for RTS frames

  TracedValue<uint64_t> m_currentRate; //!< Trace rate changes
};

} //namespace ns3

#endif /* INTEL_RATE_WIFI_MANAGER_H */
