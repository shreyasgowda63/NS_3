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
#ifndef SEQUENCE_TAG_H
#define SEQUENCE_TAG_H

#include "ns3/tag.h"

namespace ns3 {

class SequenceTag : public Tag
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer buf) const;
  virtual void Deserialize (TagBuffer buf);
  virtual void Print (std::ostream &os) const;
  SequenceTag ();

  /**
   *  Constructs a SequenceTag with the given flow id
   *
   *  \param seq Sequence number
   */
  SequenceTag (uint32_t seq);
  /**
   *  Sets the sequence for this tag.
   *  \param seq Sequence number to assign to the tag
   */
  void SetSequence (uint32_t seq);
  /**
   *  Returns the sequence for the tag
   *  \returns Current sequence for this tag
   */
  uint32_t GetSequence (void) const;

private:
  uint32_t m_sequence; //!< Sequence
};

} // namespace ns3

#endif /* SEQUENCE_TAG_H */
