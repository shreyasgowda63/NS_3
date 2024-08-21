/*
 * Copyright (c) 2024
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
 * Authors: Jiwoong Lee <porce@berkeley.edu>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef WIFI_UNITS_H
#define WIFI_UNITS_H

#include <ns3/si-units.h>
#include <ns3/units-attributes.h>

/**
 * \file
 * \ingroup wifi
 * Declaration of the SI units (as weak types aliases) used across wifi module
 */

namespace ns3
{

using mWatt_t = double;       //!< mWatt weak type
using Watt_t = double;        //!< Watt weak type
using dBw_t = double;         //!< dBw weak type
using dBm_t = double;         //!< dBm weak type
using dB_t = double;          //!< dB weak type
using dBr_t = dB_t;           //!< dBr weak type
using Hz_t = int64_t;         //!< Hz weak type
using KHz_t = int64_t;        //!< KHz weak type
using MHz_t = int64_t;        //!< MHz weak type
using GHz_t = int64_t;        //!< GHz weak type
using meter_t = double;       //!< meter weak type
using ampere_t = double;      //!< ampere weak type
using volt_t = double;        //!< volt weak type
using degree_t = double;      //!< degree weak type
using joule_t = double;       //!< joule weak type
using dBm_per_Hz_t = double;  //!< dBm/Hz weak type
using dBm_per_MHz_t = double; //!< dBm/MHz weak type

} // namespace ns3

#endif /* WIFI_UNITS_H */
