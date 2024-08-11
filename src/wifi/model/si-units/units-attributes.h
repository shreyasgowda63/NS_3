/*
 * Copyright (c) 2024 Jiwoong Lee
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
 */

#ifndef UNITS_ATTRIBUTES_H
#define UNITS_ATTRIBUTES_H

#include "units-angle.h"
#include "units-energy.h"

#include <ns3/attribute-helper.h>
#include <ns3/attribute.h>
#include <ns3/double.h>

namespace ns3
{

// See wifi-si-units-test.cc for usages

ATTRIBUTE_VALUE_DEFINE_WITH_NAME(dB, dB);
ATTRIBUTE_ACCESSOR_DEFINE(dB);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(dB, dB, Double);

ATTRIBUTE_VALUE_DEFINE_WITH_NAME(dBm, dBm);
ATTRIBUTE_ACCESSOR_DEFINE(dBm);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(dBm, dBm, Double);

ATTRIBUTE_VALUE_DEFINE_WITH_NAME(mWatt, mWatt);
ATTRIBUTE_ACCESSOR_DEFINE(mWatt);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(mWatt, mWatt, Double);

ATTRIBUTE_VALUE_DEFINE_WITH_NAME(dBm_per_Hz, dBm_per_Hz);
ATTRIBUTE_ACCESSOR_DEFINE(dBm_per_Hz);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(dBm_per_Hz, dBm_per_Hz, Double);

ATTRIBUTE_HELPER_HEADER(Hz);     // HzValue
ATTRIBUTE_HELPER_HEADER(degree); // degreeValue
ATTRIBUTE_HELPER_HEADER(radian); // radianValue

} // namespace ns3

#endif // UNITS_ATTRIBUTES_H
