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

#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/ipv6-address.h"
#include "ns3/mac64-address.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/net-device.h"
#include "ns3/uinteger.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/ipv6.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/icmpv6-l4-protocol.h"
#include "ns3/ipv6-interface.h"
#include "ns3/ipv6-raw-socket-factory.h"
#include "ns3/ipv6-packet-info-tag.h"
#include "ns3/ipv6-header.h"
#include "ns3/icmpv6-header.h"
#include "ns3/string.h"
#include "ns3/pointer.h"

#include "sixlowpan-nd-header.h"
#include "sixlowpan-radvd.h"

#include "sixlowpan-nd-context.h"
#include "sixlowpan-nd-dad-entry.h"
#include "sixlowpan-nd-interface.h"


namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("SixLowPanRadvdApplication");

NS_OBJECT_ENSURE_REGISTERED (SixLowPanRadvd);

TypeId SixLowPanRadvd::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::SixLowPanRadvd")
        .SetParent<Application> ()
        .SetGroupName("SixLowPanpan")
        .AddConstructor<SixLowPanRadvd> ();
  ;
  return tid;
}

SixLowPanRadvd::SixLowPanRadvd ()
{
  NS_LOG_FUNCTION (this);
  m_recvSocket = 0;
}

SixLowPanRadvd::~SixLowPanRadvd ()
{
  NS_LOG_FUNCTION (this);
  for (SixLowPanRadvdInterfaceListI it = m_sixlowConfs.begin (); it != m_sixlowConfs.end (); ++it)
    {
      *it = 0;
    }
  m_sixlowConfs.clear ();
  m_recvSocket = 0;
}

void SixLowPanRadvd::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_recvSocket->Close ();
  m_recvSocket = 0;

  for (SocketMapI it = m_sendSockets.begin (); it != m_sendSockets.end (); ++it)
    {
      it->second->Close ();
      it->second = 0;
    }

  Application::DoDispose ();
}

void SixLowPanRadvd::StartApplication ()
{
  NS_LOG_FUNCTION (this);

  TypeId tid = TypeId::LookupByName ("ns3::Ipv6RawSocketFactory");

  if (!m_recvSocket)
    {
      m_recvSocket = Socket::CreateSocket (GetNode (), tid);

      NS_ASSERT (m_recvSocket);

      m_recvSocket->Bind (Inet6SocketAddress (Ipv6Address::GetAllRoutersMulticast (), 0));
      m_recvSocket->SetAttribute ("Protocol", UintegerValue (Ipv6Header::IPV6_ICMPV6));
      m_recvSocket->SetRecvCallback (MakeCallback (&SixLowPanRadvd::HandleRead, this));
      m_recvSocket->ShutdownSend ();
      m_recvSocket->SetRecvPktInfo (true);
    }

  for (SixLowPanRadvdInterfaceListI it = m_sixlowConfs.begin (); it != m_sixlowConfs.end (); it++)
    {
      if (m_sendSockets.find ((*it)->GetInterface ()) == m_sendSockets.end ())
        {
          Ptr<Ipv6L3Protocol> ipv6 = GetNode ()->GetObject<Ipv6L3Protocol> ();
          Ptr<Ipv6Interface> iFace = ipv6->GetInterface ((*it)->GetInterface ());

          m_sendSockets[(*it)->GetInterface ()] = Socket::CreateSocket (GetNode (), tid);
          m_sendSockets[(*it)->GetInterface ()]->Bind (Inet6SocketAddress (iFace->GetLinkLocalAddress ().GetAddress (), 0));
          m_sendSockets[(*it)->GetInterface ()]->SetAttribute ("Protocol", UintegerValue (Ipv6Header::IPV6_ICMPV6));
          m_sendSockets[(*it)->GetInterface ()]->ShutdownRecv ();
        }
    }
}

void SixLowPanRadvd::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_recvSocket)
    {
      m_recvSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }

  for (EventIdMapI it = m_solicitedEventIds.begin (); it != m_solicitedEventIds.end (); ++it)
    {
      Simulator::Cancel ((*it).second);
    }
  m_solicitedEventIds.clear ();
}

void SixLowPanRadvd::AddSixLowPanConfiguration (Ptr<SixLowPanNdInterface> routerInterface)
{
  NS_LOG_FUNCTION (this << routerInterface);
  m_sixlowConfs.push_back (routerInterface);
}

void SixLowPanRadvd::SendRA (Ptr<SixLowPanNdInterface> config, Ipv6Address dst)
{
  NS_LOG_FUNCTION (this << dst);

  Icmpv6RA raHdr;
  Icmpv6OptionLinkLayerAddress llaHdr;
  Icmpv6OptionPrefixInformation prefixHdr;
  Icmpv6OptionSixLowPanContext contextHdr;
  Icmpv6OptionAuthoritativeBorderRouter abroHdr;

  std::list<Ptr<SixLowPanNdContext> > contexts = config->GetContexts ();
  Ptr<Packet> p = Create<Packet> ();
  Ptr<Ipv6> ipv6 = GetNode ()->GetObject<Ipv6> ();

  /* set RA header information */
  raHdr.SetFlagM (0);
  raHdr.SetFlagO (0);
  raHdr.SetFlagH (0);
  raHdr.SetCurHopLimit (0); // unspecified by this router, see RFC 4861
  raHdr.SetLifeTime (config->GetDefaultLifeTime ());
  raHdr.SetReachableTime (0);  // unspecified by this router, see RFC 4861
  // \todo Set this to a meaningful value ?
  raHdr.SetRetransmissionTime (0);

  /* add SLLAO */
  Address addr = ipv6->GetNetDevice (config->GetInterface ())->GetAddress ();
  llaHdr = Icmpv6OptionLinkLayerAddress (true, addr);
  p->AddHeader (llaHdr);

  /* Add PIO */
  uint8_t flags = 0;
  prefixHdr = Icmpv6OptionPrefixInformation ();
  prefixHdr.SetPrefix (config->GetPioNetwork ());
  prefixHdr.SetPrefixLength (64);
  prefixHdr.SetValidTime (config->GetPioValidLifeTime ());
  prefixHdr.SetPreferredTime (config->GetPioPreferredLifeTime ());

  // The on-link flag must be set to zero.
  flags += 1 << 6; // Just set the autonomous address-configuration flag.

  prefixHdr.SetFlags (flags);
  p->AddHeader (prefixHdr);


  /* add list of 6CO */
  for (std::list<Ptr<SixLowPanNdContext> >::const_iterator it = contexts.begin (); it != contexts.end (); it++)
    {
      contextHdr = Icmpv6OptionSixLowPanContext ();
      contextHdr.SetContextLen ((*it)->GetContextLen ());
      contextHdr.SetFlagC ((*it)->IsFlagC ());
      contextHdr.SetCid ((*it)->GetCid ());
      contextHdr.SetValidTime ((*it)->GetValidTime ());
      contextHdr.SetContextPrefix ((*it)->GetContextPrefix ());

      p->AddHeader (contextHdr);
    }

  Address sockAddr;
  m_sendSockets[config->GetInterface ()]->GetSockName (sockAddr);
  Ipv6Address src = Inet6SocketAddress::ConvertFrom (sockAddr).GetIpv6 ();
  NS_ABORT_MSG_IF (src.IsLinkLocal (), "Address is not link-local " << src);

  /* add ABRO */
  abroHdr = Icmpv6OptionAuthoritativeBorderRouter ();
  abroHdr.SetVersion (config->GetAbroVersion ());
  abroHdr.SetValidTime (config->GetAbroValidLifeTime ());
  abroHdr.SetRouterAddress (src);
  p->AddHeader (abroHdr);

  /* as we know interface index that will be used to send RA and 
   * we always send RA with router's link-local address, we can 
   * calculate checksum here.
   */
  raHdr.CalculatePseudoHeaderChecksum (src, dst, p->GetSize () + raHdr.GetSerializedSize (), Icmpv6L4Protocol::PROT_NUMBER);
  p->AddHeader (raHdr);

  /* Router advertisements MUST always have a ttl of 255
   * The ttl value should be set as a socket option, but this is not yet implemented
   */
  SocketIpTtlTag ttl;
  ttl.SetTtl (255);
  p->AddPacketTag (ttl);

  /* send RA */
  NS_LOG_LOGIC ("Send RA to " << dst);
  m_sendSockets[config->GetInterface ()]->SendTo (p, 0, Inet6SocketAddress (dst, 0));
}

void SixLowPanRadvd::SendDAC (uint32_t interfaceIndex, Ipv6Address dst, uint8_t status, uint16_t time,
                           Mac64Address eui, Ipv6Address registered)
{
  NS_LOG_FUNCTION (this << interfaceIndex << dst << status << time << eui << registered);
  Ptr<Packet> p = Create<Packet> ();
  Icmpv6DuplicateAddress dac (status, time, eui, registered);

  Address sockAddr;
  m_sendSockets[interfaceIndex]->GetSockName (sockAddr);
  Ipv6Address src = Inet6SocketAddress::ConvertFrom (sockAddr).GetIpv6 ();

  dac.CalculatePseudoHeaderChecksum (src, dst, p->GetSize () + dac.GetSerializedSize (), Icmpv6L4Protocol::PROT_NUMBER);
  p->AddHeader (dac);

  /* The ttl value should be set as a socket option, but this is not yet implemented */
  SocketIpTtlTag ttl;
  ttl.SetTtl (64); /* SixLowPanpanNdProtocol::MULTIHOP_HOPLIMIT */
  p->AddPacketTag (ttl);

  /* send DAC */
  NS_LOG_LOGIC ("Send DAC to " << dst);
  m_sendSockets[interfaceIndex]->SendTo (p, 0, Inet6SocketAddress (dst, 0));
}
// fatto (aggiungi SixLowPanpanNdProtocol::MULTIHOP_HOPLIMIT)

void SixLowPanRadvd::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet = 0;
  Address from;

  while ((packet = socket->RecvFrom (from)))
    {
      if (Inet6SocketAddress::IsMatchingType (from))
        {
          Ipv6PacketInfoTag interfaceInfo;
          if (!packet->RemovePacketTag (interfaceInfo))
            {
              NS_ABORT_MSG ("No incoming interface on RADVD message, aborting.");
            }

          Ipv6Header hdr;

          packet->RemoveHeader (hdr);
          uint8_t type;
          packet->CopyData (&type, sizeof(type));

          switch (type)
          {
            case Icmpv6Header::ICMPV6_ND_ROUTER_SOLICITATION:
              HandleRS (packet, hdr.GetSourceAddress (), interfaceInfo);
              break;
            case Icmpv6Header::ICMPV6_ND_DUPLICATE_ADDRESS_REQUEST:
              HandleDAR (packet, hdr.GetSourceAddress (), interfaceInfo);
              break;
            default:
              break;
          }
        }
    }
}

void SixLowPanRadvd::HandleRS (Ptr<Packet> packet, Ipv6Address const &src, Ipv6PacketInfoTag interfaceInfo)
{
  uint32_t incomingIf = interfaceInfo.GetRecvIf ();
  Ptr<NetDevice> dev = GetNode ()->GetDevice (incomingIf);
  Ptr<Ipv6> ipv6 = GetNode ()->GetObject<Ipv6> ();
  uint32_t ipInterfaceIndex = ipv6->GetInterfaceForDevice (dev);

  Icmpv6RS rsHdr;
  uint64_t delay = 0;
  Time t;
  packet->RemoveHeader (rsHdr);

  NS_LOG_INFO ("Received ICMPv6 Router Solicitation from " << src << " code = " << (uint32_t)rsHdr.GetCode ());

  for (SixLowPanRadvdInterfaceListCI it = m_sixlowConfs.begin (); it != m_sixlowConfs.end (); it++)
    {
      if (ipInterfaceIndex == (*it)->GetInterface ())
        {
          /* calculate minimum delay between RA */
          delay = static_cast<uint64_t> (m_jitter->GetValue (0, MAX_RA_DELAY_TIME * 1000));

          /* Check if there is an already scheduled RA */
          // \todo THIS IS WRONG, RA are sent as unicast.
          bool scheduleIt = true;

          if (m_solicitedEventIds.find ((*it)->GetInterface ()) != m_solicitedEventIds.end ())
            {
              if (m_solicitedEventIds[(*it)->GetInterface ()].IsRunning ())
                {
                  scheduleIt = false;
                }
            }

          if (scheduleIt)
            {
              NS_LOG_INFO ("schedule new RA");
              EventId newEvent = Simulator::Schedule (MilliSeconds (delay), &SixLowPanRadvd::SendRA,
                                                      this, (*it), src);
              m_solicitedEventIds.insert (std::make_pair ((*it)->GetInterface (), newEvent));
            }
        }
    }
}

void SixLowPanRadvd::HandleDAR (Ptr<Packet> packet, Ipv6Address const &src, Ipv6PacketInfoTag interfaceInfo)
{
  uint32_t incomingIf = interfaceInfo.GetRecvIf ();
  Ptr<NetDevice> dev = GetNode ()->GetDevice (incomingIf);
  Ptr<Ipv6> ipv6 = GetNode ()->GetObject<Ipv6> ();
  uint32_t ipInterfaceIndex = ipv6->GetInterfaceForDevice (dev);

  Icmpv6DuplicateAddress darHdr (1);
  packet->RemoveHeader (darHdr);

  NS_LOG_INFO ("Received ICMPv6 Duplicate Address Request from " << src << " code = " << (uint32_t)darHdr.GetCode ());

  Ipv6Address reg = darHdr.GetRegAddress ();

  if (!reg.IsMulticast () && src != Ipv6Address::GetAny () && !src.IsMulticast ())
    {
      Ptr<SixLowPanNdDadEntry> entry = 0;
      for (SixLowPanRadvdInterfaceListCI it = m_sixlowConfs.begin (); it != m_sixlowConfs.end (); it++)
        {
          if (ipInterfaceIndex == (*it)->GetInterface ())
            {
              for (SixLowPanNdInterface::DadTableI jt = (*it)->GetDadTable ().begin (); jt != (*it)->GetDadTable ().end (); jt++)
                {
                  if ((*jt)->GetRegAddress () == reg)
                    {
                      entry = (*jt);
                    }
                }
              if (entry)
                {
                  if (entry->GetEui64 () == darHdr.GetRovr ())
                    {
                      NS_LOG_LOGIC ("No duplicate, same EUI-64. Entry updated.");

                      entry->SetRegTime (darHdr.GetRegTime ());

                      SendDAC (ipInterfaceIndex, src, 0, darHdr.GetRegTime (), darHdr.GetRovr (), darHdr.GetRegAddress ());
                    }
                  else
                    {
                      NS_LOG_LOGIC ("Duplicate, different EUI-64.");
                      SendDAC (ipInterfaceIndex, src, 1, darHdr.GetRegTime (), darHdr.GetRovr (), darHdr.GetRegAddress ());
                    }
                }
              else
                {
                  NS_LOG_LOGIC ("Entry did not exist. Entry created.");

                  entry = new SixLowPanNdDadEntry;
                  entry->SetRegTime (darHdr.GetRegTime ());
                  entry->SetEui64 (darHdr.GetRovr ());
                  entry->SetRegAddress (darHdr.GetRegAddress ());

                  (*it)->AddDadEntry (entry);

                  SendDAC (ipInterfaceIndex, src, 0, darHdr.GetRegTime (), darHdr.GetRovr (), darHdr.GetRegAddress ());
                }
            }
        }
    }
  else
    {
      NS_LOG_ERROR ("Validity checks for DAR not satisfied.");
    } 
}

} /* namespace ns3 */
