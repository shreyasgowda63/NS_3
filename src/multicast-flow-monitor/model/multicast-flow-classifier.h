/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) YEAR COPYRIGHTHOLDER
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
 * Author: Caleb Bowers <caleb.bowers@nrl.navy.mil>
 */

#ifndef MULTICAST_FLOW_CLASSIFIER_H
#define MULTICAST_FLOW_CLASSIFIER_H

#include "ns3/simple-ref-count.h"
#include <ostream>

namespace ns3 {

/**
 * \ingroup multicast-flow-monitor
 * \brief Abstract identifier of a packet flow
 */
typedef uint32_t MulticastFlowId;

/**
 * \ingroup multicast-flow-monitor
 * \brief Abstract identifier of a packet within a flow
 */
typedef uint32_t MulticastFlowPacketId;


/// \ingroup multicast-flow-monitor
/// Provides a method to translate raw packet data into abstract
/// `multicast flow identifier` and `packet identifier` parameters.  These
/// identifiers are unsigned 32-bit integers that uniquely identify a
/// flow and a packet within that flow, respectively, for the whole
/// simulation, regardless of the point in which the packet was
/// captured.  These abstract identifiers are used in the
/// communication between MulticastFlowProbe and MulticastFlowMonitor, and all collected
/// statistics reference only those abstract identifiers in order to
/// keep the core architecture generic and not tied down to any
/// particular flow capture method or classification system.
class MulticastFlowClassifier : public SimpleRefCount<MulticastFlowClassifier>
{
private:
  MulticastFlowId m_lastNewFlowId; //!< Last known Multicast Flow ID

  /// Defined and not implemented to avoid misuse
  MulticastFlowClassifier (MulticastFlowClassifier const &);
  /// Defined and not implemented to avoid misuse
  /// \returns
  MulticastFlowClassifier& operator= (MulticastFlowClassifier const &);

public:

  MulticastFlowClassifier ();
  virtual ~MulticastFlowClassifier ();

protected:
  /// Returns a new, unique Multicast Flow Identifier
  /// \returns a new MulticastFlowId
  MulticastFlowId GetNewMulticastFlowId ();

  ///
  /// \brief Add a number of spaces for indentation purposes.
  /// \param os The stream to write to.
  /// \param level The number of spaces to add.
  void Indent (std::ostream &os, uint16_t level) const;

};

inline void
MulticastFlowClassifier::Indent (std::ostream &os, uint16_t level) const
{
  for (uint16_t __xpto = 0; __xpto < level; __xpto++)
    {
      os << ' ';
    }
}


} // namespace ns3

#endif /* MULTICAST_FLOW_CLASSIFIER_H */
