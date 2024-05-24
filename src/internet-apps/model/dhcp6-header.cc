/*
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
 */

#include "dhcp6-header.h"

#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Dhcp6Header");

Dhcp6Header::Dhcp6Header(uint8_t msgType, uint32_t transactId)
{
    m_msgType = msgType;
    m_transactId = transactId;
}

uint8_t
Dhcp6Header::GetMessageType()
{
    NS_LOG_FUNCTION(this);
    return m_msgType;
}

void
Dhcp6Header::SetMessageType(uint8_t msgType)
{
    NS_LOG_FUNCTION(this << msgType);
    m_msgType = msgType;
}

uint32_t
Dhcp6Header::GetTransactId()
{
    NS_LOG_FUNCTION(this);
    return m_transactId;
}

void
Dhcp6Header::SetTransactId(uint32_t transactId)
{
    NS_LOG_FUNCTION(this << transactId);
    m_transactId = transactId;
}

void
Dhcp6Header::ResetOptions()
{
    int i;
    for (i = 0; i < 65536; i++)
    {
        m_options[i] = false;
    }
}

} // namespace ns3
