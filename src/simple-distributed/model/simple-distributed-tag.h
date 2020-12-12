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
#ifndef SIMPLE_DISTRIBUTED_TAG_H
#define SIMPLE_DISTRIBUTED_TAG_H

#include "ns3/tag.h"
#include "ns3/mac48-address.h"
#include "ns3/vector.h"

namespace ns3 {

/**
 * \ingroup simple-distributed
 *
 * Tag class to enable attaching source and destination information to
 * a packet during packet serializing when sending to a remote
 * processor.
 */
class SimpleDistributedTag : public Tag
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
    
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
    
  SimpleDistributedTag()
  {
  }
    
  SimpleDistributedTag(Mac48Address src, uint32_t srcId, Vector srcPosition, Mac48Address dst, uint16_t proto) :
    m_src(src),
    m_dst(dst),
    m_srcId(srcId),
    m_srcPosition(srcPosition),
    m_protocolNumber(proto)
  {
  }
    
  /**
   * Get the source address
   * \return the source address
   */
  Mac48Address GetSrc (void) const;
    
  /**
   * Get the source node ID
   * \return the source node ID
   */
  uint32_t GetSrcId (void) const;

  /**
   * Get the source position
   * \return the source position
   */
  Vector GetSrcPosition (void) const;

  /**
   * Get the destination address
   * \return the destination address
   */
  Mac48Address GetDst (void) const;

  /**
   * Get the protocol number
   * \return the protocol number
   */
  uint16_t GetProto (void) const;

  void Print (std::ostream &os) const;

private:
  Mac48Address m_src; //!< source address
  Mac48Address m_dst; //!< destination address
  uint32_t m_srcId; //!< source node id
  Vector m_srcPosition; //!< source position
  uint16_t m_protocolNumber; //!< protocol number
};

} // namespace ns3

#endif
