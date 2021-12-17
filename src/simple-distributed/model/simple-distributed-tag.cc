/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *  Copyright 2020. Lawrence Livermore National Security, LLC.
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
 * Author: Steven Smith <smith84@llnl.gov>
 */

#include "simple-distributed-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimpleDistributedTag");

NS_OBJECT_ENSURE_REGISTERED (SimpleDistributedTag);

TypeId
SimpleDistributedTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimpleDistributedTag")
    .SetParent<Tag> ()
    .SetGroupName ("SimpleDistributed")
    .AddConstructor<SimpleDistributedTag> ()
  ;
  return tid;
}
TypeId
SimpleDistributedTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
SimpleDistributedTag::GetSerializedSize (void) const
{
  return 6 + 6 + 4 + 8 + 8 + 8 + 2;
}
void
SimpleDistributedTag::Serialize (TagBuffer i) const
{
  NS_LOG_FUNCTION (this);
  uint8_t mac[6];
  m_src.CopyTo (mac);
  i.Write (mac, 6);
  m_dst.CopyTo (mac);
  i.Write (mac, 6);
  i.WriteU32(m_srcId);
  i.WriteDouble(m_srcPosition.x);
  i.WriteDouble(m_srcPosition.y);
  i.WriteDouble(m_srcPosition.z);
  i.WriteU16 (m_protocolNumber);
}
void
SimpleDistributedTag::Deserialize (TagBuffer i)
{
  NS_LOG_FUNCTION (this);
  uint8_t mac[6];
  i.Read (mac, 6);
  m_src.CopyFrom (mac);
  i.Read (mac, 6);
  m_dst.CopyFrom (mac);
  m_srcId = i.ReadU32 ();
  m_srcPosition.x = i.ReadDouble();
  m_srcPosition.y = i.ReadDouble();
  m_srcPosition.z = i.ReadDouble();
  m_protocolNumber = i.ReadU16 ();
}

Mac48Address
SimpleDistributedTag::GetSrc (void) const
{
  return m_src;
}

uint32_t
SimpleDistributedTag::GetSrcId (void) const
{
  return m_srcId;
}

Vector
SimpleDistributedTag::GetSrcPosition (void) const
{
  return m_srcPosition;
}

Mac48Address
SimpleDistributedTag::GetDst (void) const
{
  return m_dst;
}

uint16_t
SimpleDistributedTag::GetProto (void) const
{
  return m_protocolNumber;
}

void
SimpleDistributedTag::Print (std::ostream &os) const
{
  os << "src=" << m_src << " dst=" << m_dst << " proto=" << m_protocolNumber;
}

} // namespace ns3  
