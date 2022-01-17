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

#ifndef B_LIST_H
#define B_LIST_H

#include <list>
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/ipv6-address.h"
#include "ns3/ptr.h"
#include "ns3/timer.h"
#include "ns3/node.h"

namespace ns3 {

/**
 * \class BList
 * \brief Binding List of Mobile Node
 */
class BList : public Object
{
public:
  /**
   * \brief Get the type identifier.
   * \return type identifier
   */
  static TypeId GetTypeId ();
  /**
   * \brief constructor.
   * \param haalist home agent address list
   */
  BList (std::list<Ipv6Address> haalist);
  /**
   * \brief destructor
   */
  ~BList ();
  
  /**
   * \brief Return the home address
   * \return HoA
   */
  Ipv6Address GetHoa () const;

};

} /* ns3 */

#endif /* BINDING_UPDATE_LIST_H */
