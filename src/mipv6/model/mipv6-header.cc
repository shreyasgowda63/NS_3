/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 Jadavpur University, India
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
 * Author: Manoj Kumar Rana <manoj24.rana@gmail.com>
 */

#include "ns3/assert.h"
#include "ns3/log.h"
#include "mipv6-header.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (Mipv6Header);

NS_LOG_COMPONENT_DEFINE ("Mipv6Header");

TypeId Mipv6Header::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::Mipv6Header")
    .SetParent<Header> ()
    .AddConstructor<Mipv6Header> ()
  ;
  return tid;
}

TypeId Mipv6Header::GetInstanceTypeId () const
{
  return GetTypeId ();
}

Mipv6Header::Mipv6Header ()
  : m_payload_proto (59),
  m_header_len (0),
  m_mh_type (0),
  m_reserved (0),
  m_checksum (0)
{
}

Mipv6Header::~Mipv6Header ()
{
}

uint8_t Mipv6Header::GetPayloadProto () const
{
  return m_payload_proto;
}

void Mipv6Header::SetPayloadProto (uint8_t payload_proto)
{
  m_payload_proto = payload_proto;
}

uint8_t Mipv6Header::GetHeaderLen () const
{
  return m_header_len;
}

void Mipv6Header::SetHeaderLen (uint8_t header_len)
{
  m_header_len = header_len;
}

uint8_t Mipv6Header::GetMhType () const
{
  return m_mh_type;
}

void Mipv6Header::SetMhType (uint8_t mh_type)
{
  m_mh_type = mh_type;
}

uint16_t Mipv6Header::GetChecksum () const
{
  return m_checksum;
}

void Mipv6Header::SetChecksum (uint16_t checksum)
{
  m_checksum = checksum;
}

void Mipv6Header::Print (std::ostream& os) const
{
  os << "( payload_proto = " << (uint32_t) GetPayloadProto () << " header_len = " << (uint32_t) GetHeaderLen () << " mh_type = " << (uint32_t) GetMhType () << " checksum = " << (uint32_t) GetChecksum () << ")";
}

uint32_t Mipv6Header::GetSerializedSize () const
{
  return 6;
}

uint32_t Mipv6Header::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  m_payload_proto = i.ReadU8 ();
  m_header_len = i.ReadU8 ();
  m_mh_type = i.ReadU8 ();
  i.ReadU8 ();
  m_checksum = i.ReadNtohU16 ();

  return GetSerializedSize ();
}

void Mipv6Header::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteU8 (m_payload_proto);
  i.WriteU8 (m_header_len);
  i.WriteU8 (m_mh_type);
  i.WriteU8 (m_reserved);
  i.WriteU16 (0);
}


Mipv6OptionField::Mipv6OptionField (uint32_t optionsOffset)
  : m_optionData (0),
  m_optionsOffset (optionsOffset)
{
}

Mipv6OptionField::~Mipv6OptionField ()
{
}

uint32_t Mipv6OptionField::GetSerializedSize () const
{
  return m_optionData.GetSize () + CalculatePad ((Mipv6OptionHeader::Alignment) {8,0});
}

void Mipv6OptionField::Serialize (Buffer::Iterator start) const
{
  start.Write (m_optionData.Begin (), m_optionData.End ());
  uint32_t fill = CalculatePad ((Mipv6OptionHeader::Alignment) {8,0});

  NS_LOG_LOGIC ("fill with " << fill << " bytes padding");
  switch (fill)
    {
    case 0:
      return;
    case 1:
      Ipv6MobilityOptionPad1Header ().Serialize (start);
      return;
    default:
      Ipv6MobilityOptionPadnHeader (fill).Serialize (start);
      return;
    }
}

uint32_t Mipv6OptionField::Deserialize (Buffer::Iterator start, uint32_t length)
{
  uint8_t buf[length];
  start.Read (buf, length);
  m_optionData = Buffer ();
  m_optionData.AddAtEnd (length);
  m_optionData.Begin ().Write (buf, length);
  return length;
}

void Mipv6OptionField::AddOption (Mipv6OptionHeader const& option)
{
  NS_LOG_FUNCTION (this << option);

  uint32_t pad = CalculatePad (option.GetAlignment ());

  NS_LOG_LOGIC ("need " << pad << " bytes padding");
  switch (pad)
    {
    case 0:
      break;       //no padding needed
    case 1:
      AddOption (Ipv6MobilityOptionPad1Header ());
      break;
    default:
      AddOption (Ipv6MobilityOptionPadnHeader (pad));
      break;
    }

  m_optionData.AddAtEnd (option.GetSerializedSize ());
  Buffer::Iterator it = m_optionData.End ();
  it.Prev (option.GetSerializedSize ());
  option.Serialize (it);
}

uint32_t Mipv6OptionField::CalculatePad (Mipv6OptionHeader::Alignment alignment) const
{
  return (alignment.offset - (m_optionData.GetSize () + m_optionsOffset)) % alignment.factor;
}

uint32_t Mipv6OptionField::GetOptionsOffset ()
{
  return m_optionsOffset;
}

Buffer Mipv6OptionField::GetOptionBuffer ()
{
  return m_optionData;
}


NS_OBJECT_ENSURE_REGISTERED (Ipv6MobilityBindingUpdateHeader);

TypeId Ipv6MobilityBindingUpdateHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::Ipv6MobilityBindingUpdateHeader")
    .SetParent<Mipv6Header> ()
    .AddConstructor<Ipv6MobilityBindingUpdateHeader> ()
  ;
  return tid;
}

TypeId Ipv6MobilityBindingUpdateHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

Ipv6MobilityBindingUpdateHeader::Ipv6MobilityBindingUpdateHeader ()
  : Mipv6OptionField (12),
    m_reserved2 (0)
{
  SetHeaderLen (0);
  SetMhType (IPV6_MOBILITY_BINDING_UPDATE);
  SetChecksum (0);

  SetSequence (0);
  SetFlagA (0);
  SetFlagH (0);
  SetFlagL (0);
  SetFlagK (0);
  SetLifetime (0);
}

Ipv6MobilityBindingUpdateHeader::~Ipv6MobilityBindingUpdateHeader ()
{
}

uint16_t Ipv6MobilityBindingUpdateHeader::GetSequence () const
{
  return m_sequence;
}

void Ipv6MobilityBindingUpdateHeader::SetSequence (uint16_t sequence)
{
  m_sequence = sequence;
}

bool Ipv6MobilityBindingUpdateHeader::GetFlagA () const
{
  return m_flagA;
}

void Ipv6MobilityBindingUpdateHeader::SetFlagA (bool a)
{
  m_flagA = a;
}

bool Ipv6MobilityBindingUpdateHeader::GetFlagH () const
{
  return m_flagH;
}

void Ipv6MobilityBindingUpdateHeader::SetFlagH (bool h)
{
  m_flagH = h;
}

bool Ipv6MobilityBindingUpdateHeader::GetFlagL () const
{
  return m_flagL;
}

void Ipv6MobilityBindingUpdateHeader::SetFlagL (bool l)
{
  m_flagL = l;
}

bool Ipv6MobilityBindingUpdateHeader::GetFlagK () const
{
  return m_flagK;
}

void Ipv6MobilityBindingUpdateHeader::SetFlagK (bool k)
{
  m_flagK = k;
}

uint16_t Ipv6MobilityBindingUpdateHeader::GetLifetime () const
{
  return m_lifetime;
}

void Ipv6MobilityBindingUpdateHeader::SetLifetime (uint16_t lifetime)
{
  m_lifetime = lifetime;
}

void Ipv6MobilityBindingUpdateHeader::Print (std::ostream& os) const
{
  os << "( payload_proto = " << (uint32_t) GetPayloadProto () << " header_len = " << (uint32_t) GetHeaderLen () << " mh_type = " << (uint32_t) GetMhType () << " checksum = " << (uint32_t) GetChecksum ();
  os << " sequence = " << (uint32_t) GetSequence () << ")";
}

uint32_t Ipv6MobilityBindingUpdateHeader::GetSerializedSize () const
{
  return 12;
}

void Ipv6MobilityBindingUpdateHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  uint32_t reserved2 = m_reserved2;

  i.WriteU8 (GetPayloadProto ());

  i.WriteU8 ((uint8_t) (( GetSerializedSize () >> 3) - 1));
  i.WriteU8 (GetMhType ());
  i.WriteU8 (0);

  i.WriteU16 (0);

  i.WriteHtonU16 (m_sequence);

  if (m_flagA)
    {
      reserved2 |= (uint16_t)(1 << 15);
    }

  if (m_flagH)
    {
      reserved2 |= (uint16_t)(1 << 14);
    }

  if (m_flagL)
    {
      reserved2 |= (uint16_t)(1 << 13);
    }

  if (m_flagK)
    {
      reserved2 |= (uint16_t)(1 << 12);
    }

  i.WriteHtonU16 (reserved2);
  i.WriteHtonU16 (m_lifetime);

}

uint32_t Ipv6MobilityBindingUpdateHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  SetPayloadProto (i.ReadU8 ());
  SetHeaderLen (i.ReadU8 ());
  SetMhType (i.ReadU8 ());
  i.ReadU8 ();

  SetChecksum (i.ReadU16 ());

  m_sequence = i.ReadNtohU16 ();

  m_reserved2 = i.ReadNtohU16 ();

  m_flagA = false;
  m_flagH = false;
  m_flagL = false;
  m_flagK = false;

  if (m_reserved2 & (1 << 15))
    {
      m_flagA = true;
    }

  if (m_reserved2 & (1 << 14))
    {
      m_flagH = true;
    }

  if (m_reserved2 & (1 << 13))
    {
      m_flagL = true;
    }

  if (m_reserved2 & (1 << 12))
    {
      m_flagK = true;
    }

  m_lifetime = i.ReadNtohU16 ();

  return GetSerializedSize ();
}

NS_OBJECT_ENSURE_REGISTERED (Ipv6MobilityBindingAckHeader);

TypeId Ipv6MobilityBindingAckHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::Ipv6MobilityBindingAckHeader")
    .SetParent<Mipv6Header> ()
    .AddConstructor<Ipv6MobilityBindingAckHeader> ()
  ;
  return tid;
}

TypeId Ipv6MobilityBindingAckHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

Ipv6MobilityBindingAckHeader::Ipv6MobilityBindingAckHeader ()
  : Mipv6OptionField (12),
    m_reserved2 (0)
{
  SetHeaderLen (0);
  SetMhType (IPV6_MOBILITY_BINDING_ACKNOWLEDGEMENT);
  SetChecksum (0);

  SetStatus (0);
  SetFlagK (0);
  SetSequence (0);
  SetLifetime (0);
}

Ipv6MobilityBindingAckHeader::~Ipv6MobilityBindingAckHeader ()
{
}

uint8_t Ipv6MobilityBindingAckHeader::GetStatus () const
{
  return m_status;
}

void Ipv6MobilityBindingAckHeader::SetStatus (uint8_t status)
{
  m_status = status;
}

bool Ipv6MobilityBindingAckHeader::GetFlagK () const
{
  return m_flagK;
}

void Ipv6MobilityBindingAckHeader::SetFlagK (bool k)
{
  m_flagK = k;
}

uint16_t Ipv6MobilityBindingAckHeader::GetSequence () const
{
  return m_sequence;
}

void Ipv6MobilityBindingAckHeader::SetSequence (uint16_t sequence)
{
  m_sequence = sequence;
}

uint16_t Ipv6MobilityBindingAckHeader::GetLifetime () const
{
  return m_lifetime;
}

void Ipv6MobilityBindingAckHeader::SetLifetime (uint16_t lifetime)
{
  m_lifetime = lifetime;
}

void Ipv6MobilityBindingAckHeader::Print (std::ostream& os) const
{
  os << "( payload_proto = " << (uint32_t) GetPayloadProto () << " header_len = " << (uint32_t) GetHeaderLen () << " mh_type = " << (uint32_t) GetMhType () << " checksum = " << (uint32_t) GetChecksum ();
  os << " status = " << (uint32_t) GetStatus () << " sequence = " << (uint32_t) GetSequence () << ")";
}

uint32_t Ipv6MobilityBindingAckHeader::GetSerializedSize () const
{
  return 12;
}

void Ipv6MobilityBindingAckHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  uint32_t reserved2 = m_reserved2;

  i.WriteU8 (GetPayloadProto ());

  i.WriteU8 ( (uint8_t) (( GetSerializedSize () >> 3) - 1) );
  i.WriteU8 (GetMhType ());
  i.WriteU8 (0);
  i.WriteU16 (0);

  i.WriteU8 (m_status);

  if (m_flagK)
    {
      reserved2 |= (uint8_t)(1 << 7);
    }

  i.WriteU8 (reserved2);
  i.WriteHtonU16 (m_sequence);
  i.WriteHtonU16 (m_lifetime);

}

uint32_t Ipv6MobilityBindingAckHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  SetPayloadProto (i.ReadU8 ());
  SetHeaderLen (i.ReadU8 ());
  SetMhType (i.ReadU8 ());
  i.ReadU8 ();

  SetChecksum (i.ReadU16 ());

  m_status = i.ReadU8 ();

  m_reserved2 = i.ReadU8 ();


  if (m_reserved2 & (1 << 7))
    {
      m_flagK = true;
    }

  m_sequence = i.ReadNtohU16 ();
  m_lifetime = i.ReadNtohU16 ();
  return GetSerializedSize ();
}

NS_OBJECT_ENSURE_REGISTERED (Ipv6BindingErrorHeader);

TypeId Ipv6BindingErrorHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::Ipv6BindingErrorHeader")
    .SetParent<Mipv6Header> ()
    .AddConstructor<Ipv6BindingErrorHeader> ()
  ;
  return tid;
}
TypeId Ipv6BindingErrorHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}
Ipv6BindingErrorHeader::Ipv6BindingErrorHeader ()
  : m_reserved2 (0)
{
  SetHeaderLen (0);
  SetMhType (IPv6_BINDING_ERROR);
  SetChecksum (0);

  SetStatus (0);
  SetHomeAddress (Ipv6Address::GetAny ());

}
Ipv6BindingErrorHeader::~Ipv6BindingErrorHeader ()
{
}

void Ipv6BindingErrorHeader::SetStatus (uint8_t stat)
{
  m_status = stat;
}

uint8_t Ipv6BindingErrorHeader::GetStatus () const
{
  return m_status;
}

Ipv6Address Ipv6BindingErrorHeader::GetHomeAddress () const
{
  return m_hoa;
}

void Ipv6BindingErrorHeader::SetHomeAddress (Ipv6Address hoa)
{
  m_hoa = hoa;
}

uint32_t Ipv6BindingErrorHeader::GetSerializedSize () const
{
  return 25;
}

void Ipv6BindingErrorHeader::Print (std::ostream& os) const
{
  os << "( payload_proto = " << (uint32_t) GetPayloadProto () << " header_len = " << (uint32_t) GetHeaderLen () << " mh_type = " << (uint32_t) GetMhType () << " checksum = " << (uint32_t) GetChecksum ();
  os << " status = " << (uint8_t) GetStatus () << "Home_Address" << (Ipv6Address) GetHomeAddress () << ")";
}

void Ipv6BindingErrorHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (GetPayloadProto ());

  i.WriteU8 ((uint8_t) (( GetSerializedSize () >> 3) - 1));
  i.WriteU8 (GetMhType ());
  i.WriteU8 (0);
  i.WriteU16 (0);

  i.WriteU8 (GetStatus ());

  i.WriteU8 (m_reserved2);

  uint8_t buf[16];
  Ipv6Address addr;
  addr = GetHomeAddress ();
  addr.Serialize (buf);
  i.Write (buf,16);
}

uint32_t Ipv6BindingErrorHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  SetPayloadProto (i.ReadU8 ());
  SetHeaderLen (i.ReadU8 ());
  SetMhType (i.ReadU8 ());
  SetChecksum (i.ReadU16 ());


  SetStatus (i.ReadU8 ());
  i.ReadU8 ();
  uint8_t buf[16];
  i.Read (buf,16);
  SetHomeAddress (Ipv6Address::Deserialize (buf));
  return GetSerializedSize ();

}

NS_OBJECT_ENSURE_REGISTERED (Ipv6BindingRefreshRequestHeader);

TypeId Ipv6BindingRefreshRequestHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::Ipv6BindingRefreshRequestHeader")
    .SetParent<Mipv6Header> ()
    .AddConstructor<Ipv6BindingRefreshRequestHeader> ()
  ;
  return tid;
}
TypeId Ipv6BindingRefreshRequestHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}
Ipv6BindingRefreshRequestHeader::Ipv6BindingRefreshRequestHeader ()
  : m_reserved2 (0)
{
  SetHeaderLen (0);
  SetMhType (IPv6_BINDING_REFRESH_REQUEST);
  SetChecksum (0);
}
Ipv6BindingRefreshRequestHeader::~Ipv6BindingRefreshRequestHeader ()
{
}

uint32_t Ipv6BindingRefreshRequestHeader::GetSerializedSize () const
{
  return 8;
}

void Ipv6BindingRefreshRequestHeader::Print (std::ostream& os) const
{
  os << "( payload_proto = " << (uint32_t) GetPayloadProto () << " header_len = " << (uint32_t) GetHeaderLen () << " mh_type = " << (uint32_t) GetMhType () << " checksum = " << (uint32_t) GetChecksum ();

}

void Ipv6BindingRefreshRequestHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (GetPayloadProto ());

  i.WriteU8 ((uint8_t) (( GetSerializedSize () >> 3) - 1));
  i.WriteU8 (GetMhType ());
  i.WriteU8 (0);
  i.WriteU16 (0);


  i.WriteU16 (m_reserved2);
}

uint32_t Ipv6BindingRefreshRequestHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  SetPayloadProto (i.ReadU8 ());
  SetHeaderLen (i.ReadU8 ());
  SetMhType (i.ReadU8 ());
  SetChecksum (i.ReadU16 ());

  i.ReadU8 ();
  return GetSerializedSize ();

}




} /* namespace ns3 */
