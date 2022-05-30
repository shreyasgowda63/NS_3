/*
 * Copyright (c) 2023 Tokushima University, Japan
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
 * Author:
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "zigbee-stack-container.h"

#include <ns3/names.h>

namespace ns3
{

ZigbeeStackContainer::ZigbeeStackContainer()
{
}

ZigbeeStackContainer::ZigbeeStackContainer(Ptr<ZigbeeStack> stack)
{
    m_stacks.emplace_back(stack);
}

ZigbeeStackContainer::ZigbeeStackContainer(std::string stackName)
{
    Ptr<ZigbeeStack> stack = Names::Find<ZigbeeStack>(stackName);
    m_stacks.emplace_back(stack);
}

ZigbeeStackContainer::Iterator
ZigbeeStackContainer::Begin() const
{
    return m_stacks.begin();
}

ZigbeeStackContainer::Iterator
ZigbeeStackContainer::End() const
{
    return m_stacks.end();
}

uint32_t
ZigbeeStackContainer::GetN() const
{
    return m_stacks.size();
}

Ptr<ZigbeeStack>
ZigbeeStackContainer::Get(uint32_t i) const
{
    return m_stacks[i];
}

void
ZigbeeStackContainer::Add(ZigbeeStackContainer other)
{
    for (auto i = other.Begin(); i != other.End(); i++)
    {
        m_stacks.emplace_back(*i);
    }
}

void
ZigbeeStackContainer::Add(Ptr<ZigbeeStack> stack)
{
    m_stacks.emplace_back(stack);
}

void
ZigbeeStackContainer::Add(std::string stackName)
{
    Ptr<ZigbeeStack> stack = Names::Find<ZigbeeStack>(stackName);
    m_stacks.emplace_back(stack);
}

} // namespace ns3
