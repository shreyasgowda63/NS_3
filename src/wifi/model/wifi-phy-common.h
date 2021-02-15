/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006,2007 INRIA
 * Copyright (c) 2020 Orange Labs
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Rediet <getachew.redieteab@orange.com>
 */

#ifndef WIFI_PHY_COMMON_H
#define WIFI_PHY_COMMON_H

#include "ns3/fatal-error.h"

/**
 * \file
 * \ingroup wifi
 * Declaration of the following enums:
 * - ns3::WifiPreamble
 * - ns3::WifiModulationClass
 * - ns3::WifiPpduField
 */

namespace ns3 {

/**
 * \ingroup wifi
 * The type of preamble to be used by an IEEE 802.11 transmission
 */
enum WifiPreamble
{
  WIFI_PREAMBLE_LONG,
  WIFI_PREAMBLE_SHORT,
  WIFI_PREAMBLE_HT_MF,
  WIFI_PREAMBLE_HT_GF,
  WIFI_PREAMBLE_VHT_SU,
  WIFI_PREAMBLE_VHT_MU,
  WIFI_PREAMBLE_HE_SU,
  WIFI_PREAMBLE_HE_ER_SU,
  WIFI_PREAMBLE_HE_MU,
  WIFI_PREAMBLE_HE_TB
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param preamble the preamble
 * \returns a reference to the stream
 */
inline std::ostream& operator<< (std::ostream &os, const WifiPreamble &preamble)
{
  switch (preamble)
    {
      case WIFI_PREAMBLE_LONG:
        return (os << "LONG");
      case WIFI_PREAMBLE_SHORT:
        return (os << "SHORT");
      case WIFI_PREAMBLE_HT_MF:
        return (os << "HT_MF");
      case WIFI_PREAMBLE_HT_GF:
        return (os << "HT_GF");
      case WIFI_PREAMBLE_VHT_SU:
        return (os << "VHT_SU");
      case WIFI_PREAMBLE_VHT_MU:
        return (os << "VHT_MU");
      case WIFI_PREAMBLE_HE_SU:
        return (os << "HE_SU");
      case WIFI_PREAMBLE_HE_ER_SU:
        return (os << "HE_ER_SU");
      case WIFI_PREAMBLE_HE_MU:
        return (os << "HE_MU");
      case WIFI_PREAMBLE_HE_TB:
        return (os << "HE_TB");
      default:
        NS_FATAL_ERROR ("Invalid preamble");
        return (os << "INVALID");
    }
}

/**
 * \ingroup wifi
 * This enumeration defines the modulation classes per
 * (Table 10-6 "Modulation classes"; IEEE 802.11-2016, with
 * updated in 802.11ax/D6.0 as Table 10-9).
 */
enum WifiModulationClass
{
  /** Modulation class unknown or unspecified. A WifiMode with this
  WifiModulationClass has not been properly initialized. */
  WIFI_MOD_CLASS_UNKNOWN = 0,
  WIFI_MOD_CLASS_DSSS,     //!< DSSS (Clause 15)
  WIFI_MOD_CLASS_HR_DSSS,  //!< HR/DSSS (Clause 16)
  WIFI_MOD_CLASS_ERP_OFDM, //:< ERP-OFDM (18.4)
  WIFI_MOD_CLASS_OFDM,     //!< OFDM (Clause 17)
  WIFI_MOD_CLASS_HT,       //!< HT (Clause 19)
  WIFI_MOD_CLASS_VHT,      //!< VHT (Clause 21)
  WIFI_MOD_CLASS_HE        //!< HE (Clause 27)
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param modulation the WifiModulationClass
 * \returns a reference to the stream
 */
inline std::ostream& operator<< (std::ostream &os, const WifiModulationClass &modulation)
{
  switch (modulation)
    {
      case WIFI_MOD_CLASS_DSSS:
        return (os << "DSSS");
      case WIFI_MOD_CLASS_HR_DSSS:
        return (os << "HR/DSSS");
      case WIFI_MOD_CLASS_ERP_OFDM:
        return (os << "ERP-OFDM");
      case WIFI_MOD_CLASS_OFDM:
        return (os << "OFDM");
      case WIFI_MOD_CLASS_HT:
        return (os << "HT");
      case WIFI_MOD_CLASS_VHT:
        return (os << "VHT");
      case WIFI_MOD_CLASS_HE:
        return (os << "HE");
      default:
        NS_FATAL_ERROR ("Unknown modulation");
        return (os << "unknown");
    }
}

/**
 * \ingroup wifi
 * The type of PPDU field (grouped for convenience)
 */
enum WifiPpduField
{
  /**
   * SYNC + SFD fields for DSSS or ERP,
   * shortSYNC + shortSFD fields for HR/DSSS or ERP,
   * HT-GF-STF + HT-GF-LTF1 fields for HT-GF,
   * L-STF + L-LTF fields otherwise.
   */
  WIFI_PPDU_FIELD_PREAMBLE = 0,
  /**
   * PHY header field for DSSS or ERP,
   * short PHY header field for HR/DSSS or ERP,
   * field not present for HT-GF,
   * L-SIG field or L-SIG + RL-SIG fields otherwise.
   */
  WIFI_PPDU_FIELD_NON_HT_HEADER,
  WIFI_PPDU_FIELD_HT_SIG,   //!< HT-SIG field
  WIFI_PPDU_FIELD_TRAINING, //!< STF + LTF fields (excluding those in preamble for HT-GF)
  WIFI_PPDU_FIELD_SIG_A,    //!< SIG-A field
  WIFI_PPDU_FIELD_SIG_B,    //!< SIG-B field
  WIFI_PPDU_FIELD_DATA      //!< data field
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param field the PPDU field
 * \returns a reference to the stream
 */
inline std::ostream& operator<< (std::ostream &os, const WifiPpduField &field)
{
  switch (field)
    {
      case WIFI_PPDU_FIELD_PREAMBLE:
        return (os << "preamble");
      case WIFI_PPDU_FIELD_NON_HT_HEADER:
        return (os << "non-HT header");
      case WIFI_PPDU_FIELD_HT_SIG:
        return (os << "HT-SIG");
      case WIFI_PPDU_FIELD_TRAINING:
        return (os << "training");
      case WIFI_PPDU_FIELD_SIG_A:
        return (os << "SIG-A");
      case WIFI_PPDU_FIELD_SIG_B:
        return (os << "SIG-B");
      case WIFI_PPDU_FIELD_DATA:
        return (os << "data");
      default:
        NS_FATAL_ERROR ("Unknown field");
        return (os << "unknown");
    }
}

} //namespace ns3

#endif /* WIFI_PHY_COMMON_H */
