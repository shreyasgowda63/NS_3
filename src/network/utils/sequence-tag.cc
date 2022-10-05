/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2022 IITP RAS
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
 * Author: Alexander Krotov <krotov@iitp.ru>
 */
#include "sequence-tag.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SequenceTag");

NS_OBJECT_ENSURE_REGISTERED (SequenceTag);

TypeId
SequenceTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SequenceTag")
    .SetParent<Tag> ()
    .SetGroupName("Network")
    .AddConstructor<SequenceTag> ()
  ;
  return tid;
}
TypeId
SequenceTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t
SequenceTag::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 4;
}
void
SequenceTag::Serialize (TagBuffer buf) const
{
  NS_LOG_FUNCTION (this << &buf);
  buf.WriteU32 (m_sequence);
}
void
SequenceTag::Deserialize (TagBuffer buf)
{
  NS_LOG_FUNCTION (this << &buf);
  m_sequence = buf.ReadU32 ();
}
void
SequenceTag::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "Sequence=" << m_sequence;
}
SequenceTag::SequenceTag ()
  : Tag ()
{
  NS_LOG_FUNCTION (this);
}

SequenceTag::SequenceTag (uint32_t id)
  : Tag (),
    m_sequence (id)
{
  NS_LOG_FUNCTION (this << id);
}

void
SequenceTag::SetSequence (uint32_t id)
{
  NS_LOG_FUNCTION (this << id);
  m_sequence = id;
}

uint32_t
SequenceTag::GetSequence (void) const
{
  NS_LOG_FUNCTION (this);
  return m_sequence;
}

} // namespace ns3

