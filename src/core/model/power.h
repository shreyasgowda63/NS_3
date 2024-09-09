/*
 * Copyright (c) 2008 INRIA
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
 */
#ifndef NS_POWER_H
#define NS_POWER_H

#include "attribute-helper.h"
#include "attribute.h"
#include "double.h"
#include "units.h"

/**
 * \file
 * \ingroup attribute_watt_t
 * attribute value declaration
 *
 * wraps units::power::watt_t
 */

namespace ns3
{

ATTRIBUTE_VALUE_DEFINE_WITH_NAME(units::power::dBm_t, Power);
ATTRIBUTE_ACCESSOR_DEFINE(Power);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(units::power::dBm_t, Power, Double);

} // namespace ns3

#endif /* POWER_H */
