/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Università di Firenze, Italy
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
 * Author: Alessio Bonadio <alessio.bonadio@gmail.com>
 */

#include "ns3/assert.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"

#include "sixlowpan-nd-header.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("SixLowPanNdHeader");

NS_OBJECT_ENSURE_REGISTERED (Icmpv6DuplicateAddress);

Icmpv6DuplicateAddress::Icmpv6DuplicateAddress ()
{
  NS_LOG_FUNCTION (this);
  SetType (Icmpv6Header::ICMPV6_ND_DUPLICATE_ADDRESS_REQUEST);
  SetCode (0);
  SetChecksum (0);
  m_status = 0;
  m_regTime = 0;
  m_eui64 = Mac64Address ("00:00:00:00:00:00:00:00");
  m_regAddress = Ipv6Address ("::");
}

Icmpv6DuplicateAddress::Icmpv6DuplicateAddress (bool request)
{
  NS_LOG_FUNCTION (this << request);
  SetType (request ? Icmpv6Header::ICMPV6_ND_DUPLICATE_ADDRESS_REQUEST : Icmpv6Header::ICMPV6_ND_DUPLICATE_ADDRESS_CONFIRM);
  SetCode (0);
  SetChecksum (0);
  m_status = 0;
  m_regTime = 0;
  m_eui64 = Mac64Address ("00:00:00:00:00:00:00:00");
  m_regAddress = Ipv6Address ("::");
}

Icmpv6DuplicateAddress::Icmpv6DuplicateAddress (uint16_t time, Mac64Address eui, Ipv6Address address)
{
  NS_LOG_FUNCTION (this);
  SetType (Icmpv6Header::ICMPV6_ND_DUPLICATE_ADDRESS_REQUEST);
  SetCode (0);
  SetChecksum (0);
  m_status = 0;
  m_regTime = time;
  m_eui64 = eui;
  m_regAddress = address;
}

Icmpv6DuplicateAddress::Icmpv6DuplicateAddress (uint8_t status, uint16_t time, Mac64Address eui, Ipv6Address address)
{
  NS_LOG_FUNCTION (this);
  SetType (Icmpv6Header::ICMPV6_ND_DUPLICATE_ADDRESS_CONFIRM);
  SetCode (0);
  SetChecksum (0);
  m_status = status;
  m_regTime = time;
  m_eui64 = eui;
  m_regAddress = address;
}

Icmpv6DuplicateAddress::~Icmpv6DuplicateAddress ()
{
  NS_LOG_FUNCTION (this);
}

TypeId Icmpv6DuplicateAddress::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::Icmpv6DuplicateAddress")
        .SetParent<Icmpv6Header> ()
        .SetGroupName ("Internet")
        .AddConstructor<Icmpv6DuplicateAddress> ()
        ;
  return tid;
}

TypeId Icmpv6DuplicateAddress::GetInstanceTypeId () const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

uint8_t Icmpv6DuplicateAddress::GetStatus () const
{
  NS_LOG_FUNCTION (this);
  return m_status;
}

void Icmpv6DuplicateAddress::SetStatus (uint8_t status)
{
  NS_LOG_FUNCTION (this << static_cast<uint32_t> (status));
  m_status = status;
}

uint16_t Icmpv6DuplicateAddress::GetRegTime () const
{
  NS_LOG_FUNCTION (this);
  return m_regTime;
}

void Icmpv6DuplicateAddress::SetRegTime (uint16_t time)
{
  NS_LOG_FUNCTION (this << time);
  m_regTime = time;
}

Mac64Address Icmpv6DuplicateAddress::GetEui64 () const
{
  NS_LOG_FUNCTION (this);
  return m_eui64;
}

void Icmpv6DuplicateAddress::SetEui64 (Mac64Address eui)
{
  NS_LOG_FUNCTION (this << eui);
  m_eui64 = eui;
}

Ipv6Address Icmpv6DuplicateAddress::GetRegAddress () const
{
  NS_LOG_FUNCTION (this);
  return m_regAddress;
}

void Icmpv6DuplicateAddress::SetRegAddress (Ipv6Address registered)
{
  NS_LOG_FUNCTION (this << registered);
  m_regAddress = registered;
}

void Icmpv6DuplicateAddress::Print (std::ostream& os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "( type = " << (uint32_t)GetType () << " code = " << (uint32_t)GetCode () << " status = " << (uint32_t)m_status << " lifetime = " << m_regTime << " EUI-64 = " << m_eui64 <<  " registered = " << m_regAddress << " checksum = " << (uint32_t)GetChecksum ()  << ")";
}

uint32_t Icmpv6DuplicateAddress::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 32;
}

void Icmpv6DuplicateAddress::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  uint8_t buf1[8];
  uint8_t buf2[16];
  uint16_t checksum = 0;
  Buffer::Iterator i = start;

  memset (buf1, 0x00, sizeof (buf1));
  memset (buf2, 0x0000, sizeof (buf2));

  i.WriteU8 (GetType ());
  i.WriteU8 (GetCode ());
  i.WriteU16 (checksum);
  i.WriteU8 (m_status);
  i.WriteU8 (0);
  i.WriteU16 (m_regTime);

  m_eui64.CopyTo (buf1);
  i.Write (buf1, 8);

  m_regAddress.Serialize (buf2);
  i.Write (buf2, 16);

  i = start;
  checksum = i.CalculateIpChecksum (i.GetSize (), GetChecksum ());

  i = start;
  i.Next (2);
  i.WriteU16 (checksum);
}

uint32_t Icmpv6DuplicateAddress::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  uint8_t buf1[8];
  uint8_t buf2[16];
  Buffer::Iterator i = start;

  memset (buf1, 0x00, sizeof (buf1));
  memset (buf2, 0x0000, sizeof (buf2));

  SetType (i.ReadU8 ());
  SetCode (i.ReadU8 ());
  SetChecksum (i.ReadU16 ());
  m_status = i.ReadU8 ();
  i.Next ();
  m_regTime = i.ReadU16 ();

  i.Read (buf1, 8);
  m_eui64.CopyFrom (buf1);

  i.Read (buf2, 16);
  m_regAddress = Ipv6Address::Deserialize (buf2);

  return GetSerializedSize ();
}


NS_OBJECT_ENSURE_REGISTERED (Icmpv6OptionExtendedAddressRegistration);

Icmpv6OptionExtendedAddressRegistration::Icmpv6OptionExtendedAddressRegistration ()
{
  NS_LOG_FUNCTION (this);
  SetType (Icmpv6Header::ICMPV6_OPT_ADDRESS_REGISTRATION);
  SetLength (1);
  m_status = 0;
  m_regTime = 0;
  m_rovrLength = 0;
}

Icmpv6OptionExtendedAddressRegistration::Icmpv6OptionExtendedAddressRegistration (uint16_t time, uint8_t* rovr, uint8_t rovrLength)
{
  NS_LOG_FUNCTION (this);
  SetType (Icmpv6Header::ICMPV6_OPT_ADDRESS_REGISTRATION);
  SetLength (1+rovrLength/8);
  m_status = 0;
  m_regTime = time;
  SetRovr (rovr, rovrLength);
}

Icmpv6OptionExtendedAddressRegistration::Icmpv6OptionExtendedAddressRegistration (uint8_t status, uint16_t time, uint8_t* rovr, uint8_t rovrLength)
{
  NS_LOG_FUNCTION (this);
  SetType (Icmpv6Header::ICMPV6_OPT_ADDRESS_REGISTRATION);
  SetLength (1+rovrLength/8);
  m_status = status;
  m_regTime = time;
  SetRovr (rovr, rovrLength);
}

Icmpv6OptionExtendedAddressRegistration::~Icmpv6OptionExtendedAddressRegistration ()
{
  NS_LOG_FUNCTION (this);
}

TypeId Icmpv6OptionExtendedAddressRegistration::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::Icmpv6OptionAddressRegistration")
        .SetParent<Icmpv6OptionHeader> ()
        .SetGroupName ("Internet")
        .AddConstructor<Icmpv6OptionExtendedAddressRegistration> ()
        ;
  return tid;
}

TypeId Icmpv6OptionExtendedAddressRegistration::GetInstanceTypeId () const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

uint8_t Icmpv6OptionExtendedAddressRegistration::GetStatus () const
{
  NS_LOG_FUNCTION (this);
  return m_status;
}

void Icmpv6OptionExtendedAddressRegistration::SetStatus (uint8_t status)
{
  NS_LOG_FUNCTION (this << static_cast<uint32_t> (status));
  m_status = status;
}

uint16_t Icmpv6OptionExtendedAddressRegistration::GetRegTime () const
{
  NS_LOG_FUNCTION (this);
  return m_regTime;
}

void Icmpv6OptionExtendedAddressRegistration::SetRegTime (uint16_t time)
{
  NS_LOG_FUNCTION (this << time);
  m_regTime = time;
}

uint8_t Icmpv6OptionExtendedAddressRegistration::GetRovr (uint8_t* rovr) const
{
  NS_LOG_FUNCTION (this);
  memcpy (rovr, m_rovr, m_rovrLength);
  return m_rovrLength;
}

void Icmpv6OptionExtendedAddressRegistration::SetRovr (uint8_t* rovr, uint8_t rovrLength)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG ((rovrLength == 8) || (rovrLength == 16) ||
                 (rovrLength == 24) || (rovrLength == 32), "ROVR length must be 64, 128, 192, or 256 bits");
  m_rovrLength = rovrLength;
  memcpy (m_rovr, rovr, rovrLength);
}

void Icmpv6OptionExtendedAddressRegistration::Print (std::ostream& os) const
{
  NS_LOG_FUNCTION (this << &os);

  std::ios oldState(nullptr);
  oldState.copyfmt(os);

  os << "( type = " << (uint32_t)GetType () << " length = " << (uint32_t)GetLength ()
     << " status " << (uint32_t)m_status << " lifetime " << m_regTime
     << " ROVR len " << m_rovrLength;
  for (uint8_t index = 0; index < m_rovrLength; index++)
    {
      os << std::hex << m_rovr[index];
    }
  os << ")";

  os.copyfmt(oldState);
}

uint32_t Icmpv6OptionExtendedAddressRegistration::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 8 + m_rovrLength;
}

void Icmpv6OptionExtendedAddressRegistration::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  i.WriteU8 (GetType ());
  i.WriteU8 (GetLength ());
  i.WriteU8 (m_status);
  i.WriteU8 (0);
  i.WriteU16 (0);
  i.WriteU16 (m_regTime);

  i.Write (m_rovr, m_rovrLength);
}

uint32_t Icmpv6OptionExtendedAddressRegistration::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;

  SetType (i.ReadU8 ());
  SetLength (i.ReadU8 ());
  m_rovrLength = (GetLength ()-1) * 8;

  m_status = i.ReadU8 ();
  i.Next (3);
  m_regTime = i.ReadU16 ();

  i.Read (m_rovr, m_rovrLength);

  return GetSerializedSize ();
}


NS_OBJECT_ENSURE_REGISTERED (Icmpv6OptionSixLowPanContext);

Icmpv6OptionSixLowPanContext::Icmpv6OptionSixLowPanContext ()
{
  NS_LOG_FUNCTION (this);
  SetType (Icmpv6Header::ICMPV6_OPT_SIXLOWPAN_CONTEXT);
  SetLength (0);
  m_contextLen = 0;
  m_c = false;
  m_cid = 0;
  m_validTime = 0;
  m_prefix = Ipv6Prefix ("::");
}

Icmpv6OptionSixLowPanContext::Icmpv6OptionSixLowPanContext (bool c, uint8_t cid, uint16_t time, Ipv6Prefix prefix)
{
  NS_LOG_FUNCTION (this);
  SetType (Icmpv6Header::ICMPV6_OPT_SIXLOWPAN_CONTEXT);

  if (prefix.GetPrefixLength () > 64)
    {
      SetLength (3);
    }
  else
    {
      SetLength (2);
    }

  m_contextLen = prefix.GetPrefixLength ();
  m_c = c;
  m_cid = cid;
  m_validTime = time;
  m_prefix = prefix;
}

Icmpv6OptionSixLowPanContext::~Icmpv6OptionSixLowPanContext ()
{
  NS_LOG_FUNCTION (this);
}

TypeId Icmpv6OptionSixLowPanContext::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::Icmpv6OptionSixLowPanContext")
            .SetParent<Icmpv6OptionHeader> ()
            .SetGroupName ("Internet")
            .AddConstructor<Icmpv6OptionSixLowPanContext> ()
            ;
  return tid;
}

TypeId Icmpv6OptionSixLowPanContext::GetInstanceTypeId () const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

uint8_t Icmpv6OptionSixLowPanContext::GetContextLen () const
{
  NS_LOG_FUNCTION (this);
  return m_contextLen;
}

bool Icmpv6OptionSixLowPanContext::IsFlagC () const
{
  NS_LOG_FUNCTION (this);
  return m_c;
}

void Icmpv6OptionSixLowPanContext::SetFlagC (bool c)
{
  NS_LOG_FUNCTION (this << c);
  m_c = c;
}

uint8_t Icmpv6OptionSixLowPanContext::GetCid () const
{
  NS_LOG_FUNCTION (this);
  return m_cid;
}

void Icmpv6OptionSixLowPanContext::SetCid (uint8_t cid)
{
  NS_LOG_FUNCTION (this << static_cast<uint32_t> (cid));
  NS_ASSERT (cid <= 15);
  m_cid = cid;
}

uint16_t Icmpv6OptionSixLowPanContext::GetValidTime () const
{
  NS_LOG_FUNCTION (this);
  return m_validTime;
}

void Icmpv6OptionSixLowPanContext::SetValidTime (uint16_t time)
{
  NS_LOG_FUNCTION (this << time);
  m_validTime = time;
}

Ipv6Prefix Icmpv6OptionSixLowPanContext::GetContextPrefix () const
{
  NS_LOG_FUNCTION (this);
  return m_prefix;
}

void Icmpv6OptionSixLowPanContext::SetContextPrefix (Ipv6Prefix prefix)
{
  NS_LOG_FUNCTION (this << prefix);
  m_prefix = prefix;
  m_contextLen = prefix.GetPrefixLength ();

  if (prefix.GetPrefixLength () > 64)
    {
      SetLength (3);
    }
  else
    {
      SetLength (2);
    }
}

void Icmpv6OptionSixLowPanContext::Print (std::ostream& os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "( type = " << (uint32_t)GetType () << " length = "
      << (uint32_t)GetLength () << " context length = "
      << (uint32_t)m_contextLen << " flag C = " << m_c << " CID = "
      << (uint32_t)m_cid << " lifetime = " << m_validTime
      << " context prefix = " << m_prefix.ConvertToIpv6Address () << "/" << +m_prefix.GetPrefixLength () << ")";
}

uint32_t Icmpv6OptionSixLowPanContext::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  uint8_t nb = GetLength () * 8;
  return nb;
}

void Icmpv6OptionSixLowPanContext::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  uint8_t buf[16];
  uint8_t bitfield = 0;

  memset (buf, 0x0000, sizeof (buf));

  i.WriteU8 (GetType ());
  i.WriteU8 (GetLength ());
  i.WriteU8 (m_contextLen);

  bitfield |= m_cid;
  bitfield |= (uint8_t)(m_c << 4);

  i.WriteU8 (bitfield);
  i.WriteU16 (0);
  i.WriteU16 (m_validTime);

  m_prefix.GetBytes (buf);
  if (m_contextLen <= 64)
    {
      i.Write (buf, 8);
    }
  else
    {
      i.Write (buf, 16);
    }
}

uint32_t Icmpv6OptionSixLowPanContext::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  uint8_t buf[16];
  uint8_t bitfield;

  memset (buf, 0x0000, sizeof (buf));

  SetType (i.ReadU8 ());
  SetLength (i.ReadU8 ());
  m_contextLen = i.ReadU8 ();

  bitfield = i.ReadU8 ();
  m_c = false;
  if (bitfield & (uint8_t)(1 << 4))
    {
      m_c = true;
    }
  m_cid = bitfield & 0xF;
  i.Next (2);
  m_validTime = i.ReadNtohU16 ();

  if (GetContextLen () <= 64)
    {
      i.Read (buf, 8);
    }
  else
    {
      i.Read (buf, 16);
    }
  m_prefix = Ipv6Prefix (buf, m_contextLen);

  if (m_contextLen > 64)
    {
      SetLength (3);
    }
  else
    {
      SetLength (2);
    }

  return GetSerializedSize ();
}


NS_OBJECT_ENSURE_REGISTERED (Icmpv6OptionAuthoritativeBorderRouter);

Icmpv6OptionAuthoritativeBorderRouter::Icmpv6OptionAuthoritativeBorderRouter ()
{
  NS_LOG_FUNCTION (this);
  SetType (Icmpv6Header::ICMPV6_OPT_AUTHORITATIVE_BORDER_ROUTER);
  SetLength (3);
  m_version = 0;
  m_validTime = 0;
  m_routerAddress = Ipv6Address ("::");
}

Icmpv6OptionAuthoritativeBorderRouter::Icmpv6OptionAuthoritativeBorderRouter (uint32_t version, uint16_t time, Ipv6Address address)
{
  NS_LOG_FUNCTION (this);
  SetType (Icmpv6Header::ICMPV6_OPT_AUTHORITATIVE_BORDER_ROUTER);
  SetLength (3);
  m_version = version;
  m_validTime = time;
  m_routerAddress = address;
}

Icmpv6OptionAuthoritativeBorderRouter::~Icmpv6OptionAuthoritativeBorderRouter ()
{
  NS_LOG_FUNCTION (this);
}

TypeId Icmpv6OptionAuthoritativeBorderRouter::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::Icmpv6OptionAuthoritativeBorderRouter")
        .SetParent<Icmpv6OptionHeader> ()
        .SetGroupName ("Internet")
        .AddConstructor<Icmpv6OptionAuthoritativeBorderRouter> ()
        ;
  return tid;
}

TypeId Icmpv6OptionAuthoritativeBorderRouter::GetInstanceTypeId () const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

uint32_t Icmpv6OptionAuthoritativeBorderRouter::GetVersion () const
{
  NS_LOG_FUNCTION (this);
  return m_version;
}

void Icmpv6OptionAuthoritativeBorderRouter::SetVersion (uint32_t version)
{
  NS_LOG_FUNCTION (this << version);
  m_version = version;
}

uint16_t Icmpv6OptionAuthoritativeBorderRouter::GetValidLifeTime () const
{
  NS_LOG_FUNCTION (this);
  return m_validTime;
}

void Icmpv6OptionAuthoritativeBorderRouter::SetValidLifeTime (uint16_t time)
{
  NS_LOG_FUNCTION (this << time);
  m_validTime = time;
}

Ipv6Address Icmpv6OptionAuthoritativeBorderRouter::GetRouterAddress () const
{
  NS_LOG_FUNCTION (this);
  return m_routerAddress;
}

void Icmpv6OptionAuthoritativeBorderRouter::SetRouterAddress (Ipv6Address router)
{
  NS_LOG_FUNCTION (this << router);
  m_routerAddress = router;
}

void Icmpv6OptionAuthoritativeBorderRouter::Print (std::ostream& os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "( type = " << (uint32_t)GetType () << " length = " << (uint32_t)GetLength () << " version = " << m_version << " lifetime = " << m_validTime << " router address = " << m_routerAddress << ")";
}

uint32_t Icmpv6OptionAuthoritativeBorderRouter::GetSerializedSize () const
{
  NS_LOG_FUNCTION (this);
  return 24;
}

void Icmpv6OptionAuthoritativeBorderRouter::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  uint8_t buf[16];
  uint32_t versionL = m_version;
  uint32_t versionH = m_version;

  memset (buf, 0x0000, sizeof (buf));

  i.WriteU8 (GetType ());
  i.WriteU8 (GetLength ());

  versionL &= 0xFFFF;
  i.WriteU16 ((uint16_t)versionL);
  versionH >>= 16;
  versionH &= 0xFFFF;
  i.WriteU16 ((uint16_t)versionH);

  i.WriteU16 (m_validTime);

  m_routerAddress.Serialize (buf);
  i.Write (buf, 16);
}

uint32_t Icmpv6OptionAuthoritativeBorderRouter::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  uint8_t buf[16];
  uint32_t versionL;
  uint32_t versionH;

  memset (buf, 0x0000, sizeof (buf));

  SetType (i.ReadU8 ());
  SetLength (i.ReadU8 ());

  versionL = (uint32_t)i.ReadU16 ();
  versionH = (uint32_t)i.ReadU16 ();
  versionH <<= 16;
  m_version = (versionL &= 0xFFFF) + (versionH &= 0xFFFF0000);

  m_validTime = i.ReadU16 ();

  i.Read (buf, 16);
  m_routerAddress = Ipv6Address::Deserialize (buf);

  return GetSerializedSize ();
}

} /* namespace ns3 */
