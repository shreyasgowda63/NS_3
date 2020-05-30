/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007-2009 Strasbourg University
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
 * Author: Sebastien Vincent <vincent@clarinet.u-strasbg.fr>
 *         David Gross <gdavid.devel@gmail.com>
 *         Mehdi Benamor <benamor.mehdi@ensi.rnu.tn>
 *         Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "ns3/log.h"
#include "ns3/ptr.h"
#include "ns3/assert.h"
#include "ns3/abort.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/boolean.h"
#include "ns3/ipv6-routing-protocol.h"
#include "ns3/ipv6-route.h"
#include "ns3/mac64-address.h"
#include "ns3/mac48-address.h"
#include "ns3/mac16-address.h"
#include "ns3/ndisc-cache.h"
#include "ns3/pointer.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/sgi-hashmap.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/ipv6-interface.h"
#include "ns3/packet.h"
#include "ns3/integer.h"
#include "ns3/nstime.h"

#include "sixlowpan-nd-protocol.h"
#include "sixlowpan-ndisc-cache.h"
#include "sixlowpan-nd-context.h"
#include "sixlowpan-nd-prefix.h"
#include "sixlowpan-nd-header.h"
#include "sixlowpan-net-device.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SixLowPanNdProtocol");

NS_OBJECT_ENSURE_REGISTERED (SixLowPanNdProtocol);

const uint16_t SixLowPanNdProtocol::MIN_CONTEXT_CHANGE_DELAY = 300;

const uint8_t SixLowPanNdProtocol::MAX_RTR_ADVERTISEMENTS = 3;
const uint8_t SixLowPanNdProtocol::MIN_DELAY_BETWEEN_RAS = 10;
const uint8_t SixLowPanNdProtocol::MAX_RA_DELAY_TIME = 2;
const uint8_t SixLowPanNdProtocol::TENTATIVE_NCE_LIFETIME = 20;

const uint8_t SixLowPanNdProtocol::MULTIHOP_HOPLIMIT = 64;

const uint8_t SixLowPanNdProtocol::RTR_SOLICITATION_INTERVAL = 10;
const uint8_t SixLowPanNdProtocol::MAX_RTR_SOLICITATIONS = 3;
const uint8_t SixLowPanNdProtocol::MAX_RTR_SOLICITATION_INTERVAL = 60;

SixLowPanNdProtocol::SixLowPanNdProtocol ()
  : Icmpv6L4Protocol ()
{
  NS_LOG_FUNCTION (this);

  m_rsRetransmit = 0;
  m_aroRetransmit = 0;
  m_nodeRole = SixLowPanNode;
  m_version = 0;
}

SixLowPanNdProtocol::~SixLowPanNdProtocol ()
{
  NS_LOG_FUNCTION (this);
}

TypeId SixLowPanNdProtocol::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::SixLowPanNdProtocol")
    .SetParent<Icmpv6L4Protocol> ()
    .SetGroupName ("Internet")
    .AddConstructor<SixLowPanNdProtocol> ()
    .AddAttribute ("RegistrationLifeTime", "The amount of time (units of 60 seconds) that the router should retain the NCE for the node.",
                    UintegerValue (60),
                    MakeUintegerAccessor (&SixLowPanNdProtocol::m_regTime),
                    MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("AdvanceTime", "The advance to perform maintaining of RA's information and registration.",
                    UintegerValue (5),
                    MakeUintegerAccessor (&SixLowPanNdProtocol::m_advance),
                    MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("DefaultRouterLifeTime", "The default router lifetime.",
                   TimeValue (Minutes (60)),
                   MakeTimeAccessor (&SixLowPanNdProtocol::m_routerLifeTime),
                   MakeTimeChecker (Time (0), Seconds(0xffff)))
    .AddAttribute ("DefaultPrefixInformationPreferredLifeTime", "The default Prefix Information preferred lifetime.",
                   TimeValue (Minutes (10)),
                   MakeTimeAccessor (&SixLowPanNdProtocol::m_pioPreferredLifeTime),
                   MakeTimeChecker ())
    .AddAttribute ("DefaultPrefixInformationValidLifeTime", "The default Prefix Information valid lifetime.",
                   TimeValue (Minutes (10)),
                   MakeTimeAccessor (&SixLowPanNdProtocol::m_pioValidLifeTime),
                   MakeTimeChecker ())
    .AddAttribute ("DefaultContextValidLifeTime", "The default Context valid lifetime [minutes].",
                   TimeValue (Minutes (10)),
                   MakeTimeAccessor (&SixLowPanNdProtocol::m_contextValidLifeTime),
                   MakeTimeChecker ())
    .AddAttribute ("DefaultAbroValidLifeTime", "The default ABRO Valid lifetime [minutes].",
                   TimeValue (Minutes (10)),
                   MakeTimeAccessor (&SixLowPanNdProtocol::m_abroValidLifeTime),
                   MakeTimeChecker ())
    ;
  return tid;
}

TypeId SixLowPanNdProtocol::GetInstanceTypeId () const
{
  return SixLowPanNdProtocol::GetTypeId ();
}

void SixLowPanNdProtocol::DoInitialize ()
{
  if (!m_raEntries.empty ())
    {
      m_nodeRole = SixLowPanBorderRouter;
    }

  Icmpv6L4Protocol::DoInitialize ();
}

Ptr<Packet> SixLowPanNdProtocol::ForgeNA (Ipv6Address src, Ipv6Address dst, Ipv6Address target, Address hardwareAddress, uint8_t flags)
{
  NS_LOG_FUNCTION (this << src << dst << hardwareAddress << (uint32_t)flags);
  Ptr<Packet> p = Create<Packet> ();
  Ipv6Header ipHeader;
  Icmpv6NA na;
  Icmpv6OptionLinkLayerAddress llOption (0, hardwareAddress);  /* we give our mac address in response */

  NS_LOG_LOGIC ("Send NA ( from " << src << " to " << dst << " target " << target << ")");

  /* forge the entire NA packet from IPv6 header to ICMPv6 link-layer option, so that the packet does not pass by Icmpv6L4Protocol::Lookup again */

  p->AddHeader (llOption);
  na.SetIpv6Target (target);

  if ((flags & 1))
    {
      na.SetFlagO (true);
    }
  if ((flags & 2) && src != Ipv6Address::GetAny ())
    {
      na.SetFlagS (true);
    }
  if ((flags & 4))
    {
      na.SetFlagR (true);
    }

  na.CalculatePseudoHeaderChecksum (src, dst, p->GetSize () + na.GetSerializedSize (), PROT_NUMBER);
  p->AddHeader (na);

  ipHeader.SetSourceAddress (src);
  ipHeader.SetDestinationAddress (dst);
  ipHeader.SetNextHeader (PROT_NUMBER);
  ipHeader.SetPayloadLength (p->GetSize ());
  ipHeader.SetHopLimit (255);

  p->AddHeader (ipHeader);

  return p;
}

void SixLowPanNdProtocol::SendNS (Ipv6Address src, Ipv6Address dst, Ipv6Address target,
                                  Address linkAddr)
{
  NS_LOG_FUNCTION (this << src << dst << target << linkAddr);

  Icmpv6L4Protocol::SendNS (src, dst, target, linkAddr);
  /*
  Ptr<Ipv6L3Protocol> ipv6 = GetNode ()->GetObject<Ipv6L3Protocol> ();
  uint32_t interfaceId = ipv6->GetInterfaceForAddress (src);
  Ptr<Ipv6Interface> interface = ipv6->GetInterface (interfaceId);
  Ptr<NdiscCache> cache = FindCache (interface->GetDevice ());

  if (src == Ipv6Address::GetAny ())
    {
      NS_ABORT_MSG ("An unspecified source address MUST NOT be used in NS messages.");
      return;
    }

  // only send NS if the target is a router.
  NdiscCache::Entry *entry = cache->Lookup (dst);
  if (entry)
    {
      if (entry->IsRouter ())
        {
          Icmpv6L4Protocol::SendNS (interface->GetLinkLocalAddress ().GetAddress (), dst,
                                    target, linkAddr);
        }
    }
  */
}

void SixLowPanNdProtocol::SendSixLowPanNsWithAro (Ipv6Address src, Ipv6Address dst, uint16_t time,
                                            Mac64Address eui, Address linkAddr, Ptr<NetDevice> sixDevice)
{
  NS_LOG_FUNCTION (this << src << dst << time << eui << linkAddr);

  /* an unspecified src MUST NOT be used in NS */

  NS_ASSERT_MSG (src != Ipv6Address::GetAny (), "An unspecified source address MUST NOT be used in ARO messages.");
  NS_ASSERT_MSG (!dst.IsMulticast (), "Destination address must not be a multicast address in ARO messages.");

  Ptr<Packet> p = Create<Packet> ();
  Icmpv6NS ns (dst);

  /* ARO (request) + SLLAO */
  Icmpv6OptionAddressRegistration arOption (time, eui);
  Icmpv6OptionLinkLayerAddress llOption (1, linkAddr);

  NS_LOG_LOGIC ("Send NS ( from " << src << " to " << dst << ")");

  p->AddHeader (arOption);
  p->AddHeader (llOption);

  Ptr<Ipv6L3Protocol> ipv6 = m_node->GetObject<Ipv6L3Protocol> ();
  NS_ASSERT (ipv6 != 0 && ipv6->GetRoutingProtocol () != 0);
  Ipv6Header header;
  SocketIpv6HopLimitTag tag;
  Socket::SocketErrno err;
  Ptr<Ipv6Route> route;

  header.SetDestinationAddress (dst);
  route = ipv6->GetRoutingProtocol ()->RouteOutput (p, header, sixDevice, err);

  if (route != 0)
    {
      NS_LOG_LOGIC ("Route exists");
      tag.SetHopLimit (255);
      p->AddPacketTag (tag);

      ns.CalculatePseudoHeaderChecksum (src, dst, p->GetSize () + ns.GetSerializedSize (), PROT_NUMBER);
      p->AddHeader (ns);
      m_downTarget (p, src, dst, PROT_NUMBER, route);
    }
  else
    {
      NS_LOG_WARN ("drop icmp message");
    }
}

void SixLowPanNdProtocol::SendSixLowPanNaWithAro (Ipv6Address src, Ipv6Address dst, uint8_t status,
                                            uint16_t time, Mac64Address eui, Ptr<NetDevice> sixDevice)
{
  NS_LOG_FUNCTION (this << src << dst << static_cast<uint32_t> (status) << time << eui);
  Ptr<Packet> p = Create<Packet> ();
  Icmpv6NA na;

  /* ARO (response) */
  Icmpv6OptionAddressRegistration arOption (status, time, eui);

  NS_LOG_LOGIC ("Send NA ( from " << src << " to " << dst << ")");

  na.SetIpv6Target (Ipv6Address::GetZero ());
  na.SetFlagO (false);
  na.SetFlagS (true);
  na.SetFlagR (true);

  p->AddHeader (arOption);
  na.CalculatePseudoHeaderChecksum (src, dst, p->GetSize () + na.GetSerializedSize (), PROT_NUMBER);
  p->AddHeader (na);

  SendMessage (p, src, dst, 255);
}

void SixLowPanNdProtocol::SendSixLowPanRA (Ipv6Address src, Ipv6Address dst, Ptr<Ipv6Interface> interface)
{
  NS_LOG_FUNCTION (this << src << dst << interface);

  Ptr<SixLowPanNdiscCache> sixCache = DynamicCast<SixLowPanNdiscCache> (FindCache (interface->GetDevice ()));
  NS_ASSERT_MSG (sixCache, "Can not find a SixLowPanNdiscCache");

  // Send out the Cached RA entries
  for (auto iter = m_raCache.begin (); iter != m_raCache.end (); iter++)
    {
      Icmpv6RA raHdr;
      Ipv6Header ipHeader;

      Ptr<Packet> p = Create<Packet> ();

      /* set RA header information */
      raHdr.SetFlagM (iter->second->IsManagedFlag ());
      raHdr.SetFlagO (iter->second->IsOtherConfigFlag ());
      raHdr.SetFlagH (iter->second->IsHomeAgentFlag ());
      raHdr.SetCurHopLimit (iter->second->GetCurHopLimit ());
      raHdr.SetLifeTime (iter->second->GetRouterLifeTime ());
      raHdr.SetReachableTime (iter->second->GetReachableTime ());
      raHdr.SetRetransmissionTime (iter->second->GetRetransTimer ());

      /* Add ABRO */
      Icmpv6OptionAuthoritativeBorderRouter abroHdr = iter->second->MakeAbro ();
      p->AddHeader (abroHdr);

      /* Add SLLAO */
      Address addr = interface->GetDevice ()->GetAddress ();
      Icmpv6OptionLinkLayerAddress llaHdr (1, addr);
      p->AddHeader (llaHdr);

      /* Add PIO */
      Ptr<SixLowPanNdPrefix> prefix = iter->second->GetPrefix ();
      Icmpv6OptionPrefixInformation prefixHdr;
      prefixHdr.SetPrefixLength (prefix->GetPrefixLength ());
      prefixHdr.SetFlags (prefix->GetFlags ());
      prefixHdr.SetValidTime (prefix->GetValidLifeTime ().GetSeconds ());
      prefixHdr.SetPreferredTime (prefix->GetPreferredLifeTime ().GetSeconds ());
      prefixHdr.SetPrefix (prefix->GetPrefix ());
      p->AddHeader (prefixHdr);

      /* Add 6COs */
      std::map<uint8_t, Ptr<SixLowPanNdContext> > contexts = iter->second->GetContexts ();
      for (std::map<uint8_t, Ptr<SixLowPanNdContext> >::iterator i = contexts.begin (); i != contexts.end (); i++)
        {
          Icmpv6OptionSixLowPanContext sixHdr;
          sixHdr.SetContextPrefix (i->second->GetContextPrefix ());
          sixHdr.SetFlagC (i->second->IsFlagC ());
          sixHdr.SetCid (i->second->GetCid ());

          Time difference = Simulator::Now () - i->second->GetLastUpdateTime ();
          double updatedValidTime = i->second->GetValidTime ().GetMinutes () - floor (difference.GetMinutes ());

          // we want to advertise only contexts with a remaining validity time greater than 1 minute.
          if (updatedValidTime > 1)
            {
              sixHdr.SetValidTime (updatedValidTime);
              p->AddHeader (sixHdr);
            }
        }

      raHdr.CalculatePseudoHeaderChecksum (src, dst, p->GetSize () + raHdr.GetSerializedSize (), PROT_NUMBER);
      p->AddHeader (raHdr);

      ipHeader.SetSourceAddress (src);
      ipHeader.SetDestinationAddress (dst);
      ipHeader.SetNextHeader (PROT_NUMBER);
      ipHeader.SetPayloadLength (p->GetSize ());
      ipHeader.SetHopLimit (255);

      /* send RA */
      NS_LOG_LOGIC ("Send RA to " << dst);

      interface->Send (p, ipHeader, dst);
    }

  // Send out the RA entries I know because I'm a 6LBR
  // Do something else like:
  Ptr<SixLowPanNetDevice> sixDevice = DynamicCast<SixLowPanNetDevice> (interface->GetDevice());
  auto iter = m_raEntries.find (sixDevice);
  if (m_raEntries.find (sixDevice) != m_raEntries.end ())
    {
      Icmpv6RA raHdr;
      Ipv6Header ipHeader;

      Ptr<Packet> p = Create<Packet> ();

      /* set RA header information */
      raHdr.SetFlagM (iter->second->IsManagedFlag ());
      raHdr.SetFlagO (iter->second->IsOtherConfigFlag ());
      raHdr.SetFlagH (iter->second->IsHomeAgentFlag ());
      raHdr.SetCurHopLimit (iter->second->GetCurHopLimit ());
      raHdr.SetLifeTime (iter->second->GetRouterLifeTime ());
      raHdr.SetReachableTime (iter->second->GetReachableTime ());
      raHdr.SetRetransmissionTime (iter->second->GetRetransTimer ());

      /* Add ABRO */
      Icmpv6OptionAuthoritativeBorderRouter abroHdr = iter->second->MakeAbro ();
      p->AddHeader (abroHdr);

      /* Add SLLAO */
      Address addr = interface->GetDevice ()->GetAddress ();
      Icmpv6OptionLinkLayerAddress llaHdr (1, addr);
      p->AddHeader (llaHdr);

      /* Add PIO */
      Ptr<SixLowPanNdPrefix> prefix = iter->second->GetPrefix ();
      Icmpv6OptionPrefixInformation prefixHdr;
      prefixHdr.SetPrefixLength (prefix->GetPrefixLength ());
      prefixHdr.SetFlags (prefix->GetFlags ());
      prefixHdr.SetValidTime (prefix->GetValidLifeTime ().GetSeconds ());
      prefixHdr.SetPreferredTime (prefix->GetPreferredLifeTime ().GetSeconds ());
      prefixHdr.SetPrefix (prefix->GetPrefix ());
      p->AddHeader (prefixHdr);

      /* Add 6COs */
      std::map<uint8_t, Ptr<SixLowPanNdContext> > contexts = iter->second->GetContexts ();
      for (std::map<uint8_t, Ptr<SixLowPanNdContext> >::iterator i = contexts.begin (); i != contexts.end (); i++)
        {
          Icmpv6OptionSixLowPanContext sixHdr;
          sixHdr.SetContextPrefix (i->second->GetContextPrefix ());
          sixHdr.SetFlagC (i->second->IsFlagC ());
          sixHdr.SetCid (i->second->GetCid ());

          Time difference = Simulator::Now () - i->second->GetLastUpdateTime ();
          double updatedValidTime = i->second->GetValidTime ().GetMinutes () - floor (difference.GetMinutes ());

          // we want to advertise only contexts with a remaining validity time greater than 1 minute.
          if (updatedValidTime > 1)
            {
              sixHdr.SetValidTime (updatedValidTime);
              p->AddHeader (sixHdr);
            }
        }

      raHdr.CalculatePseudoHeaderChecksum (src, dst, p->GetSize () + raHdr.GetSerializedSize (), PROT_NUMBER);
      p->AddHeader (raHdr);

      ipHeader.SetSourceAddress (src);
      ipHeader.SetDestinationAddress (dst);
      ipHeader.SetNextHeader (PROT_NUMBER);
      ipHeader.SetPayloadLength (p->GetSize ());
      ipHeader.SetHopLimit (255);

      /* send RA */
      NS_LOG_LOGIC ("Send RA to " << dst);

      interface->Send (p, ipHeader, dst);
    }
}

void SixLowPanNdProtocol::SendSixLowPanDAR (Ipv6Address src, Ipv6Address dst, uint16_t time, Mac64Address eui,
                                            Ipv6Address registered)
{
  NS_LOG_FUNCTION (this << src << dst << time << eui << registered);
  Ptr<Packet> p = Create<Packet> ();
  Icmpv6DuplicateAddress dar (time, eui, registered);

  NS_LOG_LOGIC ("Send DAR ( from " << src << " to " << dst << ")");

  dar.CalculatePseudoHeaderChecksum (src, dst, p->GetSize () + dar.GetSerializedSize (), PROT_NUMBER);
  p->AddHeader (dar);

  SendMessage (p, src, dst, MULTIHOP_HOPLIMIT);
}

enum IpL4Protocol::RxStatus SixLowPanNdProtocol::Receive (Ptr<Packet> packet, Ipv6Header const &header,
                                                          Ptr<Ipv6Interface> interface)
{
  NS_LOG_FUNCTION (this << packet << header.GetSourceAddress () << header.GetDestinationAddress () <<
                   interface);
  Ptr<Ipv6> ipv6 = GetNode ()->GetObject<Ipv6> ();
  Ptr<SixLowPanNetDevice> sixDevice = DynamicCast<SixLowPanNetDevice> (interface->GetDevice());
  NS_ASSERT_MSG (sixDevice != NULL, "SixLowPanNdProtocol cannot be installed on device different from SixLowPanNetDevice");

  uint8_t type;
  packet->CopyData (&type, sizeof(type));

  switch (type)
  {
    case Icmpv6Header::ICMPV6_ND_ROUTER_SOLICITATION:
      HandleSixLowPanRS (packet, header.GetSourceAddress (), header.GetDestinationAddress (), interface);
      break;
    case Icmpv6Header::ICMPV6_ND_ROUTER_ADVERTISEMENT:
      HandleSixLowPanRA (packet, header.GetSourceAddress (), header.GetDestinationAddress (), interface);
      break;
    case Icmpv6Header::ICMPV6_ND_NEIGHBOR_SOLICITATION:
      HandleSixLowPanNS (packet, header.GetSourceAddress (), header.GetDestinationAddress (), interface);
      break;
    case Icmpv6Header::ICMPV6_ND_NEIGHBOR_ADVERTISEMENT:
      HandleSixLowPanNA (packet, header.GetSourceAddress (), header.GetDestinationAddress (), interface);
      break;
    case Icmpv6Header::ICMPV6_ND_DUPLICATE_ADDRESS_CONFIRM:
      HandleSixLowPanDAC (packet, header.GetSourceAddress (), header.GetDestinationAddress (), interface);
      break;
    default:
      return Icmpv6L4Protocol::Receive (packet, header, interface);
      break;
  }

  return IpL4Protocol::RX_OK;
}

void SixLowPanNdProtocol::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  Icmpv6L4Protocol::DoDispose ();
}

void SixLowPanNdProtocol::HandleSixLowPanNS (Ptr<Packet> pkt, Ipv6Address const &src, Ipv6Address const &dst,
                                             Ptr<Ipv6Interface> interface)
{
  NS_LOG_FUNCTION (this << pkt << src << dst << interface);

  Ptr<Packet> packet = pkt->Copy ();
  Icmpv6NS nsHdr;
  packet->RemoveHeader (nsHdr);
  Ipv6Address target = nsHdr.GetIpv6Target ();

  Ptr<SixLowPanNetDevice> sixDevice = DynamicCast<SixLowPanNetDevice> (interface->GetDevice());
  NS_ASSERT_MSG (sixDevice != NULL, "SixLowPanNdProtocol cannot be installed on device different from SixLowPanNetDevice");

 if (src == Ipv6Address::GetAny ())
    {
      NS_ABORT_MSG ("An unspecified source address MUST NOT be used in NS messages.");
      return;
    }

  Icmpv6OptionLinkLayerAddress llaHdr (1); /* SLLAO */
  Icmpv6OptionAddressRegistration aroHdr;

  bool next = true;
  bool hasLla = false;
  bool hasAro = false;

   /* search all options following the NS header */
  while (next == true)
    {
      uint8_t type;
      packet->CopyData (&type, sizeof(type));

      switch (type)
      {
        case Icmpv6Header::ICMPV6_OPT_LINK_LAYER_SOURCE:
          if (!hasLla)
            {
              packet->RemoveHeader (llaHdr);
              hasLla = true;
            }
          break;
        case Icmpv6Header::ICMPV6_OPT_ADDRESS_REGISTRATION:
          if (!hasAro)
            {
              packet->RemoveHeader (aroHdr);
              hasAro = true;
            }
          break;
        default:
          /* unknow option, quit */
          next = false;
      }
      if (packet->GetSize () == 0)
        next = false;
    }

  if (!hasLla) /* error! */
    {
      NS_LOG_ERROR ("NS message MUST have source link layer option.");
      return;
    }
  else /* NS + SLLAO */
    {
      /* Update NDISC table with information of src */
      Ptr<NdiscCache> cache = FindCache (sixDevice);

      NdiscCache::Entry* entry = 0;
      entry = cache->Lookup (src);

      if (!entry && src.IsLinkLocal())
        {
          entry = cache->Add (src);
          uint8_t buf[16];
          src.GetBytes (buf);
          Mac16Address address;
          address.CopyFrom (buf+14);
          entry->SetRouter (false);
          entry->SetMacAddress (llaHdr.GetAddress ());
          entry->MarkReachable ();
        }
      else if (!entry)
        {
          entry = cache->Add (src);
          entry->SetRouter (false);
          entry->MarkStale (llaHdr.GetAddress ());
        }
      else if (entry->GetMacAddress () != llaHdr.GetAddress ())
        {
          entry->MarkStale (llaHdr.GetAddress ());
        }
    }

  // \todo Here there's a bug (probably). If a node asks for a NS, shouldn't we forward it to the 6LBR ?
  if (!hasAro) /* NS + SLLAO */
    {
      /* Send NA to src with information requested about target (in NDISC table) */
      Ptr<NdiscCache> cache = FindCache (sixDevice);

      NdiscCache::Entry* entry = 0;

      // First we check if the NS is for ourself.

      uint32_t nb = interface->GetNAddresses ();
      uint32_t i = 0;
      Ipv6InterfaceAddress ifaddr;

      for (i = 0; i < nb; i++)
        {
          ifaddr = interface->GetAddress (i);

          if (ifaddr.GetAddress () == target)
            {
              HandleNS (pkt, src, dst, interface);
              return;
            }
        }

      entry = cache->Lookup (target);
      uint8_t flags;

      if (!entry)
        {
          NS_LOG_LOGIC ("No entry with address " << target);
          return;
        }
      else
        {
          if (entry->IsRouter ())
            {
              flags = 7; /* R + S + O flag */
            }
          else
            {
              flags = 3; /* S + O flags */
            }

          /* NA + TLLAO */
          Ptr<Packet> p = ForgeNA (interface->GetLinkLocalAddress ().GetAddress (), src, target,
                                    entry->GetMacAddress (), flags);
          Ipv6Header header;
          p->RemoveHeader (header);
          interface->Send (p, header, src);
        }
    }
  else /* NS + SLLAO + ARO */
    {
      /* Update NCE with information of src and ARO */
      Ptr<SixLowPanNdiscCache> neighborCache = DynamicCast<SixLowPanNdiscCache> (FindCache (sixDevice));
      NS_ASSERT_MSG (neighborCache, "Can not find a SixLowPanNdiscCache");

      SixLowPanNdiscCache::SixLowPanEntry* neighborEntry = 0;
      neighborEntry = dynamic_cast <SixLowPanNdiscCache::SixLowPanEntry*> (neighborCache->Lookup (src));

      if (!neighborEntry) /* no entry, creating */
        {
          if (aroHdr.GetRegTime () != 0)
            {
              neighborEntry = dynamic_cast<SixLowPanNdiscCache::SixLowPanEntry*> (neighborCache->Add (src));
              neighborEntry->SetRouter (false);
              neighborEntry->SetMacAddress (llaHdr.GetAddress ());
              neighborEntry->SetEui64 (aroHdr.GetEui64 ());
              neighborEntry->MarkReachable ();

              if (m_multihopDad) /* multihop DAD used */
                {
                  neighborEntry->MarkTentative ();
                  for (auto iter = m_raCache.begin (); iter != m_raCache.end (); iter++)
                    {
                      Ipv6Address destination = iter->second->GetAbroBorderRouterAddress ();
                      SendSixLowPanDAR (interface->GetAddressMatchingDestination (destination).GetAddress (), destination,
                                        aroHdr.GetRegTime (), aroHdr.GetEui64 (), src);
                    }
                  return;
                }
              else  /* multihop DAD NOT used */
                {
                  neighborEntry->MarkRegistered (aroHdr.GetRegTime ());
                  SendSixLowPanNaWithAro (interface->GetLinkLocalAddress ().GetAddress (), src, 0, aroHdr.GetRegTime (),
                                    aroHdr.GetEui64 (), sixDevice);
                  return;
                }
            }
        }
      else if (neighborEntry->GetEui64 () == aroHdr.GetEui64 ()) /* entry found, same EUI-64, updating */
        {
          if (aroHdr.GetRegTime () != 0)
            {
              neighborEntry->SetRouter (false);
              neighborEntry->SetMacAddress (llaHdr.GetAddress ());
              neighborEntry->MarkReachable ();
              neighborEntry->MarkRegistered (aroHdr.GetRegTime ());
              SendSixLowPanNaWithAro (interface->GetLinkLocalAddress ().GetAddress (), src, 0, aroHdr.GetRegTime (),
                                aroHdr.GetEui64 (), sixDevice);

              if (m_multihopDad)
                {
                  for (auto iter = m_raCache.begin (); iter != m_raCache.end (); iter++)
                    {
                      Ipv6Address destination = iter->second->GetAbroBorderRouterAddress ();

                      /* Send request to update entry from DAD table */
                      SendSixLowPanDAR (interface->GetAddressMatchingDestination (destination).GetAddress (), destination,
                                        aroHdr.GetRegTime (), aroHdr.GetEui64 (), src);
                    }
                }

              return;
            }
          else /* ARO's Registration Lifetime = 0 */
            {
              if (m_multihopDad)
                {
                  for (auto iter = m_raCache.begin (); iter != m_raCache.end (); iter++)
                    {
                      Ipv6Address destination = iter->second->GetAbroBorderRouterAddress ();

                      /* Send request to remove entry from DAD table */
                      SendSixLowPanDAR (interface->GetAddressMatchingDestination (destination).GetAddress (), destination,
                                        aroHdr.GetRegTime (), aroHdr.GetEui64 (), src);
                    }
                }
              /* Remove the entry from Neighbor Cache */
              neighborCache->Remove (neighborEntry);
              return;
            }
        }
      else /* entry found, different EUI-64 */
        {
          if (m_multihopDad && neighborEntry->IsRegistered ())
            {
              SendSixLowPanNaWithAro (interface->GetLinkLocalAddress ().GetAddress (),
                                Ipv6Address::MakeAutoconfiguredLinkLocalAddress (aroHdr.GetEui64 ()),
                                // sixDevice->MakeLinkLocalAddressFromMac (aroHdr.GetEui64 ()),
                                1,
                                aroHdr.GetRegTime (), aroHdr.GetEui64 (), sixDevice);
            }
          else if (!m_multihopDad)
            {
              SendSixLowPanNaWithAro (interface->GetLinkLocalAddress ().GetAddress (),
                                Ipv6Address::MakeAutoconfiguredLinkLocalAddress (aroHdr.GetEui64 ()),
                                // sixDevice->MakeLinkLocalAddressFromMac (aroHdr.GetEui64 ()),
                                1,
                                aroHdr.GetRegTime (), aroHdr.GetEui64 (), sixDevice);
            }
          return;
        }
    }
}

void SixLowPanNdProtocol::HandleSixLowPanNA (Ptr<Packet> packet, Ipv6Address const &src, Ipv6Address const &dst,
                                             Ptr<Ipv6Interface> interface)
{
  NS_LOG_FUNCTION (this << packet << src << dst << interface);

  Ptr<Packet> p = packet->Copy ();
  Icmpv6NS naHdr;
  packet->RemoveHeader (naHdr);
  Ipv6Address target = naHdr.GetIpv6Target ();

  Icmpv6OptionLinkLayerAddress llaHdr (0); /* TLLAO */
  Icmpv6OptionAddressRegistration aroHdr;

  uint8_t type;
  packet->CopyData (&type, sizeof(type));

  switch (type)
  {
    case Icmpv6Header::ICMPV6_OPT_LINK_LAYER_TARGET: /* NA + TLLAO */

      HandleNA (p, target, dst, interface); /* Handle response of Address Resolution */

      break;
    case Icmpv6Header::ICMPV6_OPT_ADDRESS_REGISTRATION: /* NA + ARO */

      packet->RemoveHeader (aroHdr);

      m_aroRetransmit = 0;

      if (aroHdr.GetStatus () == 0) /* status=0, success! */
        {
          /* schedule a new ARO to maintain NCE in routers */

          Simulator::Schedule (Time (Minutes (aroHdr.GetRegTime () - m_advance)), &SixLowPanNdProtocol::RetransmitARO, this,
                               dst, src, aroHdr.GetRegTime (), aroHdr.GetEui64 (), interface->GetDevice ()->GetAddress (), interface->GetDevice ());
        }
      else /* status NOT 0, fail! */
        {
          NS_LOG_LOGIC ("ARO status is NOT 0, registration failed!");
          /// \todo implement method to remove address that generated error.
          return;
        }
      break;
    default:
      NS_LOG_ERROR ("NA message MUST have option.");
      return;
  }
}

void SixLowPanNdProtocol::HandleSixLowPanRS (Ptr<Packet> packet, Ipv6Address const &src, Ipv6Address const &dst,
                                             Ptr<Ipv6Interface> interface)
{
  NS_LOG_FUNCTION (this << packet << src << dst << interface);

  if (m_nodeRole == SixLowPanNode)
    {
      NS_LOG_LOGIC ("Discarding a RS because I'm a simple node");
      return;
    }

  Ptr<SixLowPanNetDevice> sixDevice = DynamicCast<SixLowPanNetDevice> (interface->GetDevice());
  NS_ASSERT_MSG (sixDevice != NULL, "SixLowPanNdProtocol cannot be installed on device different from SixLowPanNetDevice");

  Icmpv6RS rsHeader;
  Icmpv6OptionLinkLayerAddress lla (1);

  packet->RemoveHeader (rsHeader);

  if (src != Ipv6Address::GetAny ())
    {
      uint8_t type;
      packet->CopyData (&type, sizeof(type));

      if (type != Icmpv6Header::ICMPV6_OPT_LINK_LAYER_SOURCE)
        {
          NS_LOG_ERROR ("RS message MUST have source link layer option.");
          return;
        }

      packet->RemoveHeader (lla);

      /* Update Neighbor Cache */
      Ptr<SixLowPanNdiscCache> sixCache = DynamicCast<SixLowPanNdiscCache> (FindCache (sixDevice));
      NS_ASSERT_MSG (sixCache, "Can not find a SixLowPanNdiscCache");
      SixLowPanNdiscCache::SixLowPanEntry* sixEntry = 0;
      sixEntry = dynamic_cast <SixLowPanNdiscCache::SixLowPanEntry*> (sixCache->Lookup (src));
      if (!sixEntry)
        {
          sixEntry = dynamic_cast<SixLowPanNdiscCache::SixLowPanEntry*> (sixCache->Add (src));
          sixEntry->SetRouter (false);
          sixEntry->MarkStale (lla.GetAddress ());
          sixEntry->MarkTentative ();
          NS_LOG_LOGIC ("Tentative entry created from RS");
        }
      else if (sixEntry->GetMacAddress () != lla.GetAddress ())
        {
          sixEntry->MarkStale (lla.GetAddress ());
        }

      /* Update "ARPv6" table */
      Ptr<NdiscCache> cache = FindCache (sixDevice);
      NdiscCache::Entry* entry = 0;
      entry = cache->Lookup (src);
      if (!entry)
        {
          entry = cache->Add (src);
          entry->SetRouter (false);
          entry->MarkStale (lla.GetAddress ());
        }
      else if (entry->GetMacAddress () != lla.GetAddress ())
        {
          entry->MarkStale (lla.GetAddress ());
        }
    }

  Address addr = lla.GetAddress ();
  Ipv6Address destination = Ipv6Address::MakeAutoconfiguredLinkLocalAddress (addr);

  SendSixLowPanRA (interface->GetLinkLocalAddress ().GetAddress (), destination, interface);
}

void SixLowPanNdProtocol::HandleSixLowPanRA (Ptr<Packet> packet, Ipv6Address const &src, Ipv6Address const &dst,
                                             Ptr<Ipv6Interface> interface)
{
  NS_LOG_FUNCTION (this << packet << src << dst << interface);

  m_rsRetransmit = 0;
  m_receivedRA = true;

  std::cout << "** begin ** SixLowPanNdProtocol::HandleSixLowPanRA" << std::endl;
  std::cout << "   the RA was from " << src << std::endl;

//  ** begin ** SixLowPanNdProtocol::HandleSixLowPanRA

  Ptr<SixLowPanNetDevice> sixDevice = DynamicCast<SixLowPanNetDevice> (interface->GetDevice());
  NS_ASSERT_MSG (sixDevice != NULL, "SixLowPanNdProtocol cannot be installed on device different from SixLowPanNetDevice");

  Address macAddr = sixDevice->GetAddress ();

  Icmpv6RA raHeader;
  Ptr<Ipv6L3Protocol> ipv6 = GetNode ()->GetObject<Ipv6L3Protocol> ();

  Icmpv6OptionAuthoritativeBorderRouter abroHdr; /* ABRO */
  Icmpv6OptionLinkLayerAddress llaHdr (1); /* SLLAO */

  bool next = true;
  Ptr<SixLowPanNdiscCache> sixCache = DynamicCast<SixLowPanNdiscCache> (FindCache (sixDevice));
  NS_ASSERT_MSG (sixCache, "Can not find a SixLowPanNdiscCache");

  Ptr<NdiscCache> cache = FindCache (sixDevice);

  uint32_t version = 0;
  Ipv6Address border = Ipv6Address::GetAny ();

  packet->RemoveHeader (raHeader);

  Ipv6Address defaultRouter = Ipv6Address::GetZero ();

  if (raHeader.GetLifeTime())
    {
      defaultRouter = src;
    }

  std::list<Icmpv6OptionPrefixInformation> prefixList;
  std::list<Icmpv6OptionSixLowPanContext> contextList;
  Icmpv6OptionPrefixInformation prefixHdr;
  Icmpv6OptionSixLowPanContext contextHdr;

  bool hasAbro = false;
  bool hasOptLinkLayerSource = false;

  while (next == true)
    {
      uint8_t type = 0;
      packet->CopyData (&type, sizeof(type));

      switch (type)
      {
        case Icmpv6Header::ICMPV6_OPT_PREFIX:
          packet->RemoveHeader (prefixHdr);
          prefixList.push_back (prefixHdr);
          break;
        case Icmpv6Header::ICMPV6_OPT_SIXLOWPAN_CONTEXT:
          packet->RemoveHeader (contextHdr);
          contextList.push_back (contextHdr);
          break;
        case Icmpv6Header::ICMPV6_OPT_AUTHORITATIVE_BORDER_ROUTER:
          packet->RemoveHeader (abroHdr);
          version = abroHdr.GetVersion ();
          border = abroHdr.GetRouterAddress ();
          hasAbro = true;
          break;
        case Icmpv6Header::ICMPV6_OPT_LINK_LAYER_SOURCE:
          packet->RemoveHeader (llaHdr);
          ReceiveLLA (llaHdr, src, dst, interface); // generates an entry in NDISC table with m_router = true
          hasOptLinkLayerSource = true;
          break;
        default:
          /* Unknown option, stop processing */
          next = false;
          break;
      }
      if (packet->GetSize () == 0)
        {
          next = false;
        }
    }

  if (prefixList.size () != 1)
    {
      // RAs should contain one (and only one) PIO
      NS_LOG_LOGIC ("SixLowPanNdProtocol::HandleSixLowPanRA - wrong number of PIOs in the RA (" << prefixList.size () << ") - ignoring RA");
      return;
    }
  if (hasOptLinkLayerSource == false)
    {
      // RAs should contain one (and only one) PIO
      NS_LOG_LOGIC ("SixLowPanNdProtocol::HandleSixLowPanRA - no Option LinkLayerSource -  ignoring RA");
      return;
    }
  if (hasAbro == false)
    {
      // RAs should contain one (and only one) PIO
      NS_LOG_LOGIC ("SixLowPanNdProtocol::HandleSixLowPanRA - no ABRO -  ignoring RA");
      return;
    }

  if (border == Ipv6Address::GetAny ())
    {
      NS_LOG_LOGIC ("SixLowPanNdProtocol::HandleSixLowPanRA - border router address is set to Any - ignoring RA");
      return;
    }

  auto it = m_raCache.find (border);
  if (it == m_raCache.end ()) // Can't find the entry, create a new one
    {
      NS_LOG_LOGIC ("SixLowPanNdProtocol new RA, adding it to the cache");

      Ptr<SixLowPanRaEntry> ra = Create<SixLowPanRaEntry> ();
      m_raCache[border] = ra;
      ra->SetManagedFlag (raHeader.GetFlagM ());
      ra->SetOtherConfigFlag (raHeader.GetFlagO ());
      ra->SetHomeAgentFlag (raHeader.GetFlagH ());
      ra->SetReachableTime (raHeader.GetReachableTime ());
      ra->SetRouterLifeTime (raHeader.GetLifeTime ());
      ra->SetRetransTimer (raHeader.GetRetransmissionTime ());
      ra->SetCurHopLimit (raHeader.GetCurHopLimit ());
      ra->ParseAbro (abroHdr);

      NS_ASSERT (m_regTime);

      for (std::list<Icmpv6OptionSixLowPanContext>::iterator jt = contextList.begin (); jt != contextList.end (); jt++)
        {
          Ptr<SixLowPanNdContext> context = new SixLowPanNdContext;
          context->SetCid ((*jt).GetCid ());
          context->SetFlagC ((*jt).IsFlagC ());
          context->SetValidTime (Minutes ((*jt).GetValidTime ()));
          context->SetContextPrefix ((*jt).GetContextPrefix ());
          context->SetLastUpdateTime (Simulator::Now ());

          ra->AddContext (context);
        }

      Ptr<SixLowPanNdPrefix> prefix = Create<SixLowPanNdPrefix> ();
      prefix->SetPrefix (prefixHdr.GetPrefix ());
      prefix->SetPrefixLength (prefixHdr.GetPrefixLength ());
      prefix->SetPreferredLifeTime (Seconds (prefixHdr.GetPreferredTime ()));
      prefix->SetValidLifeTime (Seconds (prefixHdr.GetValidTime ()));

      ra->SetPrefix (prefix);

      int32_t interfaceId = ipv6->GetInterfaceForDevice (sixDevice);
      Ptr<Ipv6Interface> ipInterface = ipv6->GetInterface (interfaceId);

      Ipv6Address newIpAddr = Ipv6Address::MakeAutoconfiguredAddress (macAddr, prefixHdr.GetPrefix ());

      uint8_t addrBuffer[16];
      newIpAddr.GetBytes (addrBuffer);
      Mac64Address eui64;
      eui64.CopyFrom (addrBuffer+8);

      SendSixLowPanNsWithAro (newIpAddr, src, m_regTime, eui64, macAddr, sixDevice);
      // \todo
      // The address is not even registered in the interface - any packet sent to it will be discarded.
      // We now need to prepare for a registration result (any result) and to retransmit the registration.


    }
  else // found an entry, try to update it.
    {
      Ptr<SixLowPanRaEntry> ra = it->second;
      if (version > (it->second->GetAbroVersion ())) // Update existing entry from 6LBR with new information
        {
          for (std::list<Icmpv6OptionPrefixInformation>::iterator iter = prefixList.begin (); iter != prefixList.end (); iter++)
            {
              if (ra->GetPrefix ()->GetPrefix () != (*iter).GetPrefix () ) /* prefix NOT matching */
                {
                  NS_LOG_WARN ("A 6LBR is advertising a prefix different than the one we already had. Ignoring");
                  return;
                }
            }

          it->second->SetManagedFlag (raHeader.GetFlagM ());
          it->second->SetOtherConfigFlag (raHeader.GetFlagO ());
          it->second->SetHomeAgentFlag (raHeader.GetFlagH ());
          it->second->SetReachableTime (raHeader.GetReachableTime ());
          it->second->SetRouterLifeTime (raHeader.GetLifeTime ());
          it->second->SetRetransTimer (raHeader.GetRetransmissionTime ());
          it->second->SetCurHopLimit (raHeader.GetCurHopLimit ());
          it->second->ParseAbro (abroHdr);

          for (std::list<Icmpv6OptionSixLowPanContext>::iterator jt = contextList.begin (); jt != contextList.end (); jt++)
            {
              if (ra->GetContexts ().find ((*jt).GetCid ()) == ra->GetContexts ().end ()) /* context NOT found */
                {
                  Ptr<SixLowPanNdContext> context = new SixLowPanNdContext;
                  context->SetCid ((*jt).GetCid ());
                  context->SetFlagC ((*jt).IsFlagC ());
                  context->SetValidTime (Minutes ((*jt).GetValidTime ()));
                  context->SetContextPrefix ((*jt).GetContextPrefix ());
                  context->SetLastUpdateTime (Simulator::Now ());

                  ra->AddContext (context);
                }
              else
                {
                  Ptr<SixLowPanNdContext> context = (ra->GetContexts ().find ((*jt).GetCid ()))->second;

                  context->SetFlagC ((*jt).IsFlagC ());
                  context->SetValidTime (Minutes ((*jt).GetValidTime ()));
                  context->SetContextPrefix ((*jt).GetContextPrefix ());
                  context->SetLastUpdateTime (Simulator::Now ());
                }
            }

          // this is to update the address timers.
          ipv6->AddAutoconfiguredAddress (ipv6->GetInterfaceForDevice (sixDevice),
                                          prefixHdr.GetPrefix (), prefixHdr.GetPrefixLength (),
                                          prefixHdr.GetFlags (), prefixHdr.GetValidTime (),
                                          prefixHdr.GetPreferredTime (), defaultRouter);

        }
    }

  // \todo Da cambiare di brutto

  uint32_t t = raHeader.GetLifeTime ();

  for (std::list<Icmpv6OptionPrefixInformation>::iterator it = prefixList.begin (); it != prefixList.end (); it++)
    {
      t = t < ((*it).GetValidTime ()) ? t : ((*it).GetValidTime ());
    }
  for (std::list<Icmpv6OptionSixLowPanContext>::iterator jt = contextList.begin (); jt != contextList.end (); jt++)
    {
      t = (60 * ((*jt).GetValidTime ())) < t ? (60 * ((*jt).GetValidTime ())) : t;
    }

  t -= (60 * m_advance);

  Simulator::Schedule (Time (Seconds (t - 1)), &SixLowPanNdProtocol::SetReceivedRA, this, false);
  Simulator::Schedule (Time (Seconds (t)), &SixLowPanNdProtocol::RetransmitRS, this,
                       interface->GetLinkLocalAddress ().GetAddress (), src, macAddr);
}

void SixLowPanNdProtocol::HandleSixLowPanDAC (Ptr<Packet> packet, Ipv6Address const &src, Ipv6Address const &dst,
                                              Ptr<Ipv6Interface> interface)
{
  NS_LOG_FUNCTION (this << packet << src << dst << interface);

  Ptr<SixLowPanNetDevice> sixDevice = DynamicCast<SixLowPanNetDevice> (interface->GetDevice());
  NS_ASSERT_MSG (sixDevice != NULL, "SixLowPanNdProtocol cannot be installed on device different from SixLowPanNetDevice");

  Icmpv6DuplicateAddress dacHdr (0);
  packet->RemoveHeader (dacHdr);

  if (m_multihopDad)
    {
      Ipv6Address reg = dacHdr.GetRegAddress ();

      if (!reg.IsMulticast () && src != Ipv6Address::GetAny () && !src.IsMulticast ())
        {
          Ptr<SixLowPanNdiscCache> cache = DynamicCast<SixLowPanNdiscCache> (FindCache (sixDevice));
          NS_ASSERT_MSG (cache, "Can not find a SixLowPanNdiscCache");

          SixLowPanNdiscCache::SixLowPanEntry* entry = 0;
          entry = dynamic_cast <SixLowPanNdiscCache::SixLowPanEntry*> (cache->Lookup (reg));

          if (dacHdr.GetStatus () == 0) /* mark the entry as registered, send ARO with status=0 */
            {
              entry->MarkRegistered (dacHdr.GetRegTime ());

              SendSixLowPanNaWithAro (dst, dacHdr.GetRegAddress (), dacHdr.GetStatus (), dacHdr.GetRegTime (),
                                dacHdr.GetEui64 (), sixDevice);
            }
          else /* remove the tentative entry, send ARO with error code */
            {
              cache->Remove (entry);

              Ipv6Address address = Ipv6Address::MakeAutoconfiguredLinkLocalAddress (dacHdr.GetEui64 ());
              // Ipv6Address address = sixDevice->MakeLinkLocalAddressFromMac (dacHdr.GetEui64 ());

              SendSixLowPanNaWithAro (dst, address, dacHdr.GetStatus (), dacHdr.GetRegTime (), dacHdr.GetEui64 (), sixDevice);
            }
        }
      else
        {
          NS_LOG_ERROR ("Validity checks for DAR not satisfied.");
          return;
        }
    }
}

Ptr<NdiscCache> SixLowPanNdProtocol::CreateCache (Ptr<NetDevice> device, Ptr<Ipv6Interface> interface)
{
  NS_LOG_FUNCTION (this << device << interface);

  Ptr<SixLowPanNdiscCache> cache = CreateObject<SixLowPanNdiscCache> ();

  cache->SetDevice (device, interface, this);
  device->AddLinkChangeCallback (MakeCallback (&NdiscCache::Flush, cache));
  
  // in case a cache was previously created by Icmpv6L4Protocol, remove it.
  for (auto iter = m_cacheList.begin (); iter != m_cacheList.end (); iter ++)
    {
      if ((*iter)->GetDevice () == device)
        {
          m_cacheList.erase (iter);
        }
    } 
  m_cacheList.push_back (cache);
  return cache;
}

void SixLowPanNdProtocol::RetransmitARO (Ipv6Address src, Ipv6Address dst, uint16_t time,
                                         Mac64Address eui, Address linkAddr, Ptr<NetDevice> sixDevice)
{
  NS_LOG_FUNCTION (this << src << dst << time << eui << linkAddr);

  IntegerValue maxUnicastSolicit;
  GetAttribute ("MaxUnicastSolicit", maxUnicastSolicit);
  if (m_aroRetransmit < maxUnicastSolicit.Get ())
    {
      m_aroRetransmit++;

      SendSixLowPanNsWithAro (src, dst, time, eui, linkAddr, sixDevice);

      TimeValue retransmissionTime;
      GetAttribute ("RetransmissionTime", retransmissionTime);

      Simulator::Schedule (retransmissionTime.Get (), &SixLowPanNdProtocol::RetransmitARO, this,
                           src, dst, time, eui, linkAddr, sixDevice);
      return;
    }
  else
    {
      return;
    }
}

void SixLowPanNdProtocol::RetransmitRS (Ipv6Address src, Ipv6Address dst, Address linkAddr)
{
  NS_LOG_FUNCTION (this << src << dst << linkAddr);

  /* if the source is NOT unspec, send a RS message + SLLA option */
  if (src != Ipv6Address::GetAny ())
    {
      if (!m_receivedRA && m_rsRetransmit < MAX_RTR_SOLICITATIONS)
        {
          m_rsRetransmit++;

          SendRS (src, dst, linkAddr);

          Simulator::Schedule (Time (Seconds (RTR_SOLICITATION_INTERVAL)), &SixLowPanNdProtocol::RetransmitRS, this,
                               src, dst, linkAddr);
          return;
        }
      else if (!m_receivedRA)
        {
          m_rsRetransmit++;

          Ipv6Address destination = Ipv6Address::GetAllRoutersMulticast ();

          SendRS (src, destination, linkAddr);

          Simulator::Schedule (Time (Seconds (MAX_RTR_SOLICITATION_INTERVAL)), &SixLowPanNdProtocol::RetransmitRS, this,
                               src, destination, linkAddr);
          /* inserire truncated binary exponential backoff */
          return;
        }
      else
        {
          return;
        }
    }
  else
    {
      NS_LOG_ERROR ("An unspecified source address MUST NOT be used in RS messages.");
      return;
    }
}
/// \todo da finire!! (truncated binary exponential backoff)

void SixLowPanNdProtocol::SetReceivedRA (bool received)
{
  NS_LOG_FUNCTION (this << received);
  m_receivedRA = received;
}

void SixLowPanNdProtocol::SetInterfaceAs6lbr (Ptr<SixLowPanNetDevice> device)
{
  NS_LOG_FUNCTION (device);

  if (m_raEntries.find (device) != m_raEntries.end ())
    {
      NS_LOG_LOGIC ("Not going to re-configure an interface");
      return;
    }

  Ptr<SixLowPanRaEntry> newRa = Create<SixLowPanRaEntry> ();
  newRa->SetManagedFlag (false);
  newRa->SetHomeAgentFlag (false);
  newRa->SetOtherConfigFlag (false);
  newRa->SetOtherConfigFlag (false);
  newRa->SetCurHopLimit (0); // unspecified by this router
  newRa->SetRetransTimer (0); // unspecified by this router

  newRa->SetReachableTime (0); // unspecified by this router

  uint64_t routerLifetime = ceil (m_routerLifeTime.GetMinutes ());
  if (routerLifetime > 0xffff)
    routerLifetime = 0xffff;

  newRa->SetRouterLifeTime (routerLifetime);

  Ptr<Ipv6L3Protocol> ipv6 = GetNode ()->GetObject<Ipv6L3Protocol> ();
  int32_t interfaceId = ipv6->GetInterfaceForDevice (device);
  Ipv6Address borderAddress = Ipv6Address::GetAny ();
  for (uint32_t i=0; i<ipv6->GetNAddresses (interfaceId); i++)
    {
      if (ipv6->GetAddress (interfaceId, i).GetScope () == Ipv6InterfaceAddress::GLOBAL)
        {
          borderAddress = ipv6->GetAddress (interfaceId, i).GetAddress ();
          continue;
        }
    }
  NS_ABORT_MSG_IF (borderAddress == Ipv6Address::GetAny (), "Can not set a 6LBR because I can't find a global address associated with the interface");
  newRa->SeAbroBorderRouterAddress (borderAddress);
  newRa->SetAbroVersion (0x66);
  newRa->SetAbroValidLifeTime (m_abroValidLifeTime.GetSeconds());

  m_raEntries[device] = newRa;
}

void SixLowPanNdProtocol::SetAdvertisedPrefix (Ptr<SixLowPanNetDevice> device, Ipv6Prefix prefix)
{
  NS_LOG_FUNCTION (device << prefix);

  if (m_raEntries.find (device) == m_raEntries.end ())
    {
      NS_LOG_LOGIC ("Not adding a prefix to a non-configured interface");
      return;
    }

  Ptr<SixLowPanNdPrefix> newPrefix = Create<SixLowPanNdPrefix> (prefix.ConvertToIpv6Address (), prefix.GetPrefixLength (),
                                                                m_pioPreferredLifeTime, m_pioValidLifeTime, 64);

  m_raEntries[device]->SetPrefix (newPrefix);
}

void SixLowPanNdProtocol::AddAdvertisedContext (Ptr<SixLowPanNetDevice> device, Ipv6Prefix context)
{
  NS_LOG_FUNCTION (device << context);

  if (m_raEntries.find (device) == m_raEntries.end ())
    {
      NS_LOG_LOGIC ("Not adding a context to a non-configured interface");
      return;
    }
  auto contextMap = m_raEntries[device]->GetContexts ();

  bool found = false;
  for (auto iter=contextMap.begin (); iter!=contextMap.end (); iter++)
    {
      if (iter->second->GetContextPrefix () == context)
        {
          found = true;
          break;
        }
    }
  if (found)
    {
      NS_LOG_WARN ("Not adding an already existing context - remove the old one first " << context);
      return;
    }

  uint8_t unusedCid;
  for (unusedCid=0; unusedCid<16; unusedCid++)
    {
      if (contextMap.count(unusedCid) == 0)
        {
          break;
        }
    }

  Ptr<SixLowPanNdContext> newContext = Create<SixLowPanNdContext> (true, unusedCid, m_contextValidLifeTime, context);
  newContext->SetLastUpdateTime (Simulator::Now ());

  m_raEntries[device]->AddContext (newContext);
}


void SixLowPanNdProtocol::RemoveAdvertisedContext (Ptr<SixLowPanNetDevice> device, Ipv6Prefix context)
{
  NS_LOG_FUNCTION (device << context);

  if (m_raEntries.find (device) == m_raEntries.end ())
    {
      NS_LOG_LOGIC ("Not removing a context to a non-configured interface");
      return;
    }

  auto contextMap = m_raEntries[device]->GetContexts ();

  for (auto iter=contextMap.begin (); iter!=contextMap.end (); iter++)
    {
      if (iter->second->GetContextPrefix () == context)
        {
          m_raEntries[device]->RemoveContext (iter->second);
          return;
        }
    }
  NS_LOG_WARN ("Not removing a non-existing context " << context);

}

bool SixLowPanNdProtocol::IsBorderRouterOnInterface (Ptr<SixLowPanNetDevice> device) const
{
  NS_LOG_FUNCTION (device);

  if (m_raEntries.find (device) == m_raEntries.end ())
    {
      return false;
    }
  return true;
}

//
// SixLowPanRaEntry class
//

//NS_LOG_COMPONENT_DEFINE ("SixLowPanRaEntry");

SixLowPanNdProtocol::SixLowPanRaEntry::SixLowPanRaEntry ()
{
  NS_LOG_FUNCTION (this);
}

SixLowPanNdProtocol::SixLowPanRaEntry::~SixLowPanRaEntry ()
{
  NS_LOG_FUNCTION (this);
}

void SixLowPanNdProtocol::SixLowPanRaEntry::SetPrefix (Ptr<SixLowPanNdPrefix> prefix)
{
  NS_LOG_FUNCTION (this << prefix);
  m_prefix = prefix;
}

Ptr<SixLowPanNdPrefix> SixLowPanNdProtocol::SixLowPanRaEntry::GetPrefix () const
{
  NS_LOG_FUNCTION (this);
  return m_prefix;
}

void SixLowPanNdProtocol::SixLowPanRaEntry::AddContext (Ptr<SixLowPanNdContext> context)
{
  NS_LOG_FUNCTION (this << context);
  m_contexts.insert (std::pair<uint8_t, Ptr<SixLowPanNdContext> > (context->GetCid (), context));
}

void SixLowPanNdProtocol::SixLowPanRaEntry::RemoveContext (Ptr<SixLowPanNdContext> context)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_contexts.erase (context->GetCid ());
}

std::map<uint8_t, Ptr<SixLowPanNdContext> > SixLowPanNdProtocol::SixLowPanRaEntry::GetContexts () const
{
  NS_LOG_FUNCTION (this);
  return m_contexts;
}

bool SixLowPanNdProtocol::SixLowPanRaEntry::IsManagedFlag () const
{
  NS_LOG_FUNCTION (this);
  return m_managedFlag;
}

void SixLowPanNdProtocol::SixLowPanRaEntry::SetManagedFlag (bool managedFlag)
{
  NS_LOG_FUNCTION (this << managedFlag);
  m_managedFlag = managedFlag;
}

bool SixLowPanNdProtocol::SixLowPanRaEntry::IsOtherConfigFlag () const
{
  NS_LOG_FUNCTION (this);
  return m_otherConfigFlag;
}

void SixLowPanNdProtocol::SixLowPanRaEntry::SetOtherConfigFlag (bool otherConfigFlag)
{
  NS_LOG_FUNCTION (this << otherConfigFlag);
  m_otherConfigFlag = otherConfigFlag;
}

bool SixLowPanNdProtocol::SixLowPanRaEntry::IsHomeAgentFlag () const
{
  NS_LOG_FUNCTION (this);
  return m_homeAgentFlag;
}

void SixLowPanNdProtocol::SixLowPanRaEntry::SetHomeAgentFlag (bool homeAgentFlag)
{
  NS_LOG_FUNCTION (this << homeAgentFlag);
  m_homeAgentFlag = homeAgentFlag;
}

uint32_t SixLowPanNdProtocol::SixLowPanRaEntry::GetReachableTime () const
{
  NS_LOG_FUNCTION (this);
  return m_reachableTime;
}

void SixLowPanNdProtocol::SixLowPanRaEntry::SetReachableTime (uint32_t time)
{
  NS_LOG_FUNCTION (this << time);
  m_reachableTime = time;
}

uint32_t SixLowPanNdProtocol::SixLowPanRaEntry::GetRouterLifeTime () const
{
  NS_LOG_FUNCTION (this);
  return m_routerLifeTime;
}

void SixLowPanNdProtocol::SixLowPanRaEntry::SetRouterLifeTime (uint32_t time)
{
  NS_LOG_FUNCTION (this << time);
  m_routerLifeTime = time;
}

uint32_t SixLowPanNdProtocol::SixLowPanRaEntry::GetRetransTimer () const
{
  NS_LOG_FUNCTION (this);
  return m_retransTimer;
}

void SixLowPanNdProtocol::SixLowPanRaEntry::SetRetransTimer (uint32_t timer)
{
  NS_LOG_FUNCTION (this << timer);
  m_retransTimer = timer;
}

uint8_t SixLowPanNdProtocol::SixLowPanRaEntry::GetCurHopLimit () const
{
  NS_LOG_FUNCTION (this);
  return m_curHopLimit;
}

void SixLowPanNdProtocol::SixLowPanRaEntry::SetCurHopLimit (uint8_t curHopLimit)
{
  NS_LOG_FUNCTION (this << curHopLimit);
  m_curHopLimit = curHopLimit;
}

uint32_t SixLowPanNdProtocol::SixLowPanRaEntry::GetAbroVersion () const
{
  NS_LOG_FUNCTION (this);
  return m_abroVersion;
}

void SixLowPanNdProtocol::SixLowPanRaEntry::SetAbroVersion (uint32_t version)
{
  NS_LOG_FUNCTION (this << version);
  m_abroVersion = version;
}

uint16_t SixLowPanNdProtocol::SixLowPanRaEntry::GetAbroValidLifeTime () const
{
  NS_LOG_FUNCTION (this);
  return m_abroValidLifeTime;
}

void SixLowPanNdProtocol::SixLowPanRaEntry::SetAbroValidLifeTime (uint16_t time)
{
  NS_LOG_FUNCTION (this << time);
  m_abroValidLifeTime = time;
}

Ipv6Address SixLowPanNdProtocol::SixLowPanRaEntry::GetAbroBorderRouterAddress () const
{
  NS_LOG_FUNCTION (this);
  return m_abroBorderRouter;
}

void SixLowPanNdProtocol::SixLowPanRaEntry::SeAbroBorderRouterAddress (Ipv6Address border)
{
  NS_LOG_FUNCTION (this << border);
  m_abroBorderRouter = border;
}

bool SixLowPanNdProtocol::SixLowPanRaEntry::ParseAbro (Icmpv6OptionAuthoritativeBorderRouter abro)
{
  Ipv6Address addr = abro.GetRouterAddress ();
  if (addr == Ipv6Address::GetAny ())
    {
      return false;
    }
  m_abroBorderRouter = addr;

  m_abroVersion = abro.GetVersion ();
  m_abroValidLifeTime = abro.GetValidLifeTime ();
  return true;
}

Icmpv6OptionAuthoritativeBorderRouter SixLowPanNdProtocol::SixLowPanRaEntry::MakeAbro ()
{
  Icmpv6OptionAuthoritativeBorderRouter abro;

  abro.SetRouterAddress (m_abroBorderRouter);
  abro.SetValidLifeTime (m_abroValidLifeTime);
  abro.SetVersion (m_abroVersion);

  return abro;
}

} /* namespace ns3 */
