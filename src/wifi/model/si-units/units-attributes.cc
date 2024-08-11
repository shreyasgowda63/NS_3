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

#include "units-attributes.h"

namespace ns3
{

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(dB, dB);
ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(dB, dB);

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(dBm, dBm);
ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(dBm, dBm);

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(mWatt, mWatt);
ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(mWatt, mWatt);

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(dBm_per_Hz, dBm_per_Hz);
ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(dBm_per_Hz, dBm_per_Hz);

ATTRIBUTE_HELPER_CPP(Hz);
ATTRIBUTE_HELPER_CPP(degree);
ATTRIBUTE_HELPER_CPP(radian);

} // namespace ns3
