/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005 INRIA
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
 * Author: Emmanuelle Laprise <emmanuelle.laprise@bluekazoo.ca>
 */

#include "ethernet-utils.h"

#include "ns3/enum.h"
#include "ns3/llc-snap-header.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/type-id.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EthernetEncap");

void
EthernetEncap (Ptr<Packet> packet, Mac48Address source, Mac48Address dest, uint16_t protocolNumber,
               Ethernet::EncapMode mode)
{
  NS_LOG_FUNCTION (packet << source << dest << protocolNumber);

  EthernetHeader header (false);
  header.SetSource (source);
  header.SetDestination (dest);

  EthernetTrailer trailer;

  NS_LOG_LOGIC ("packet->GetSize () = " << packet->GetSize ());
  NS_LOG_LOGIC ("m_mode = " << static_cast<int> (mode));

  uint16_t lengthType = 0;
  switch (mode)
    {
    case Ethernet::EncapMode::DIX:
      NS_LOG_LOGIC ("Encapsulating packet as DIX (type interpretation)");
      //
      // This corresponds to the type interpretation of the lengthType field as
      // in the old Ethernet Blue Book.
      //
      lengthType = protocolNumber;

      //
      // All Ethernet frames must carry a minimum payload of 46 bytes.  We need
      // to pad out if we don't have enough bytes.  These must be real bytes
      // since they will be written to pcap files and compared in regression
      // trace files.
      //
      if (packet->GetSize () < 46)
        {
          uint8_t buffer[46];
          memset (buffer, 0, 46);
          Ptr<Packet> padd = Create<Packet> (buffer, 46 - packet->GetSize ());
          packet->AddAtEnd (padd);
        }
      break;
      case Ethernet::EncapMode::LLC: {
        NS_LOG_LOGIC ("Encapsulating packet as LLC (length interpretation)");

        LlcSnapHeader llc;
        llc.SetType (protocolNumber);
        packet->AddHeader (llc);

        //
        // This corresponds to the length interpretation of the lengthType
        // field but with an LLC/SNAP header added to the payload as in
        // IEEE 802.2
        //
        lengthType = packet->GetSize ();

        //
        // All Ethernet frames must carry a minimum payload of 46 bytes.  The
        // LLC SNAP header counts as part of this payload.  We need to padd out
        // if we don't have enough bytes.  These must be real bytes since they
        // will be written to pcap files and compared in regression trace files.
        //
        if (packet->GetSize () < 46)
          {
            uint8_t buffer[46];
            memset (buffer, 0, 46);
            Ptr<Packet> padd = Create<Packet> (buffer, 46 - packet->GetSize ());
            packet->AddAtEnd (padd);
          }
      }
      break;
    case Ethernet::EncapMode::ILLEGAL:
    default:
      NS_FATAL_ERROR ("CsmaNetDevice::AddHeader(): Unknown packet encapsulation mode");
      break;
    }

  NS_LOG_LOGIC ("header.SetLengthType (" << lengthType << ")");
  header.SetLengthType (lengthType);
  packet->AddHeader (header);

  if (Node::ChecksumEnabled ())
    {
      trailer.EnableFcs (true);
    }
  trailer.CalcFcs (packet);
  packet->AddTrailer (trailer);
}

bool
EthernetDecap (Ptr<Packet> packet, uint16_t &protocol, EthernetHeader &header)
{
  EthernetTrailer trailer;
  packet->RemoveTrailer (trailer);
  if (Node::ChecksumEnabled ())
    {
      trailer.EnableFcs (true);
    }

  bool crcGood = trailer.CheckFcs (packet);
  if (!crcGood)
    {
      NS_LOG_INFO ("CRC error on Packet " << packet);
      return false;
    }

  packet->RemoveHeader (header);

  NS_LOG_LOGIC ("Pkt source is " << header.GetSource ());
  NS_LOG_LOGIC ("Pkt destination is " << header.GetDestination ());

  //
  // If the length/type is less than 1500, it corresponds to a length
  // interpretation packet.  In this case, it is an 802.3 packet and
  // will also have an 802.2 LLC header.  If greater than 1500, we
  // find the protocol number (Ethernet type) directly.
  //
  if (header.GetLengthType () <= 1500)
    {
      NS_ASSERT (packet->GetSize () >= header.GetLengthType ());
      uint32_t padlen = packet->GetSize () - header.GetLengthType ();
      NS_ASSERT (padlen <= 46);
      if (padlen > 0)
        {
          packet->RemoveAtEnd (padlen);
        }

      LlcSnapHeader llc;
      packet->RemoveHeader (llc);
      protocol = llc.GetType ();
    }
  else
    {
      protocol = header.GetLengthType ();
    }
  return true;
}

} // namespace ns3