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

#include <algorithm>
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/ipv6.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/callback.h"
#include "ns3/icmpv6-l4-protocol.h"
#include "mipv6-mn.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/pointer.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/ipv6-interface.h"
#include "mipv6-header.h"
#include "mipv6-mobility.h"
#include "mipv6-demux.h"
#include "ns3/ipv6-option-header.h"
#include "ns3/ipv6-extension-header.h"
#include "mipv6-l4-protocol.h"
#include "ns3/ipv6-routing-protocol.h"
#include "ns3/ipv6-route.h"

using namespace std;

NS_LOG_COMPONENT_DEFINE ("Mipv6Mn");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (Mipv6Mn);

TypeId
Mipv6Mn::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Mipv6Mn")
    .SetParent<Mipv6Agent> ()
    .AddAttribute ("BList", "The binding list associated with this MN.",
                   PointerValue (),
                   MakePointerAccessor (&Mipv6Mn::m_buinf),
                   MakePointerChecker<BList> ())
    .AddTraceSource ("RxBA",
                     "Received BA packet from HA",
                     MakeTraceSourceAccessor (&Mipv6Mn::m_rxbaTrace),
                     "ns3::Mipv6Mn::RxBaTracedCallback")
    .AddTraceSource ("TxBU",
                     "Sent BU packet from MN",
                     MakeTraceSourceAccessor (&Mipv6Mn::m_txbuTrace),
                     "ns3::Mipv6Mn::TxBuTracedCallback")


    ;
  return tid;
}

Mipv6Mn::Mipv6Mn (std::list<Ipv6Address> haalist)
{
  m_Haalist = haalist;
  m_hsequence = 0;
  m_roflag = false;
}

Mipv6Mn::~Mipv6Mn ()
{
  delete this;
}

void Mipv6Mn::NotifyNewAggregate ()
{
  NS_LOG_FUNCTION (this);

  uint8_t buf1[8],buf2[16],buf[16];
  uint8_t i;

  if (GetNode () == 0)
    {
      Ptr<Node> node = this->GetObject<Node> ();
      SetNode (node);
      m_buinf = CreateObject<BList> (m_Haalist);
      m_buinf->SetNode (node);


      //Fetch any link-local address of the node
      Ptr<Ipv6> ip = GetNode ()->GetObject<Ipv6> ();
      Ipv6InterfaceAddress ads = ip->GetAddress (1,0);


      // Set HAA and Forming HoA from HAA Prefix

      if (m_Haalist.size ())
        {
          m_buinf->SetHA (m_Haalist.front ()); // The first address
          (m_buinf->GetHA ()).GetBytes (buf1); //Fetching Prefix
          (ads.GetAddress ()).GetBytes (buf2); //Fetching interface identifier
          for (i = 0; i < 8; i++)
            {
              buf[i] = buf1[i];
            }
          for (i = 0; i < 8; i++)
            {
              buf[i + 8] = buf2[i + 8];
            }
          Ipv6Address addr (buf);
          m_buinf->SetHoa (addr);
        }

      Ptr<Icmpv6L4Protocol> icmpv6l4 = GetNode ()->GetObject<Icmpv6L4Protocol> ();
      icmpv6l4->SetNewIPCallback (MakeCallback (&Mipv6Mn::HandleNewAttachment, this));
      icmpv6l4->SetCheckAddressCallback (MakeCallback (&Mipv6Mn::CheckAddresses, this));

      Ptr<Ipv6L3Protocol> ipv6l3 = GetNode ()->GetObject<Ipv6L3Protocol> ();
      ipv6l3->SetPrefixCallback (MakeCallback (&Mipv6Mn::SetDefaultRouterAddress, this));

      int n = ipv6l3->GetNInterfaces ();

      for (int i = 0; i<n; i++)
        {
          NS_LOG_INFO (this << " Setting callback in interface to check for home-link" << i);
          (ipv6l3->GetInterface (i))->SetHomeLinkCheck (MakeCallback (&Mipv6Mn::SetHomeLink, this));
        }

      Ptr<UdpL4Protocol> udpl4 = GetNode ()->GetObject<UdpL4Protocol> ();
      udpl4->SetMipv6Callback (MakeCallback (&BList::GetHoa, m_buinf));

      Ptr<TcpL4Protocol> tcpl4 = GetNode ()->GetObject<TcpL4Protocol> ();
      tcpl4->SetMipv6Callback (MakeCallback (&BList::GetHoa, m_buinf));
    }
  Mipv6Agent::NotifyNewAggregate ();
}

void Mipv6Mn::HandleNewAttachment (Ipv6Address ipr)
{
  NS_LOG_FUNCTION (this << ipr);
  if (!ipr.IsLinkLocal () )
    {
      Ipv6Address coa = ipr;
      m_buinf->SetCoa (coa);

      ClearTunnelAndRouting ();
      Ptr<Ipv6> ipv6 = GetNode ()->GetObject<Ipv6> ();
      NS_ASSERT (ipv6);

      //preset header information
      m_buinf->SetHomeLastBindingUpdateSequence (GetHomeBUSequence ());
      //Cut to micro-seconds
      m_buinf->SetHomeLastBindingUpdateTime (MicroSeconds (Simulator::Now ().GetMicroSeconds ()));
      //reset (for the first registration)
      m_buinf->ResetHomeRetryCount ();

      Ptr<Packet> p;
      if(m_homelink)
        {
          p = BuildHomeBU (true, true, true, true, 0, true);

          Ptr<Ipv6L3Protocol> ipv6l3 = (GetNode ())->GetObject <Ipv6L3Protocol> ();

          NS_ASSERT (ipv6l3 != 0 && ipv6l3->GetRoutingProtocol () != 0);

          Ipv6Header header;
          SocketIpTtlTag tag;
          Socket::SocketErrno err;
          Ptr<Ipv6Route> route;
          Ptr<NetDevice> oif = ipv6l3->GetNetDevice (m_IfIndex);

          header.SetSourceAddress (ipr);
          header.SetDestinationAddress (m_defaultrouteraddress);
          route = ipv6l3->GetRoutingProtocol ()->RouteOutput (p, header, oif, err);

          if (route != 0)
            {
              tag.SetTtl (64);
              p->AddPacketTag (tag);

              ipv6->Send (p, ipr, m_defaultrouteraddress, 135, route);
              NS_LOG_LOGIC ("route found and send hmipv6 message");
            }
          else
            {
              NS_LOG_LOGIC ("no route.. drop mipv6 message");
            }
            
            return;
        }
      else
        {
          p = BuildHomeBU (true, true, true, true, Mipv6L4Protocol::MAX_BINDING_LIFETIME, true);
          SendMessage (p->Copy (), m_buinf->GetHA (), 64);
        }

      //save packet
      m_buinf->SetHomeBUPacket (p);


      //send BU
      NS_LOG_FUNCTION (this << p->GetSize ());
      
      Ptr<Packet> pkt = p->Copy ();
      m_txbuTrace (pkt, m_buinf->GetCoa (), m_buinf->GetHA ());


      m_buinf->StartHomeRetransTimer ();

      if (m_buinf->IsHomeReachable ())
        {
          m_buinf->MarkHomeRefreshing ();
        }
      else
        {
          m_buinf->MarkHomeUpdating ();
        }
    }
}

Ptr<Packet> Mipv6Mn::BuildHomeBU (bool flagA, bool flagH, bool flagL, bool flagK, uint16_t lifetime, bool extn)
{
  NS_LOG_FUNCTION (this << flagA << flagH << flagL << flagK << lifetime << extn);

  Ptr<Packet> p = Create<Packet> ();

  if(extn)
    {
      //Adding home address option

      Ipv6ExtensionDestinationHeader destextnhdr;
      Ipv6HomeAddressOptionHeader homeopt;
      homeopt.SetHomeAddress (m_buinf->GetHoa ());
      destextnhdr.AddOption (homeopt);

      destextnhdr.SetNextHeader (59);
      p->AddHeader (destextnhdr);
    }

  Ipv6MobilityBindingUpdateHeader bu;


  bu.SetSequence (m_buinf->GetHomeLastBindingUpdateSequence ());
  bu.SetFlagA (flagA);
  bu.SetFlagH (flagH);
  bu.SetFlagL (flagL);
  bu.SetFlagK (flagK);

  bu.SetLifetime (lifetime);


  p->AddHeader (bu);

  return p;
}

uint8_t Mipv6Mn::HandleBA (Ptr<Packet> packet, const Ipv6Address &src, const Ipv6Address &dst, Ptr<Ipv6Interface> interface)
{

  NS_LOG_FUNCTION (this << packet << src << dst << interface << "HANDLE BACK");

  Ptr<Packet> p = packet->Copy ();
  Ptr<Packet> pkt = packet->Copy ();
  m_rxbaTrace (pkt, src, dst, interface);

  Ipv6MobilityBindingAckHeader ba;
  Ipv6ExtensionType2RoutingHeader exttype2;

  p->RemoveHeader (ba);
  p->RemoveHeader (exttype2);

  Ptr<Mipv6Demux> mipv6Demux = GetNode ()->GetObject<Mipv6Demux> ();
  NS_ASSERT (mipv6Demux);

  Ptr<Mipv6Mobility> ipv6Mobility = mipv6Demux->GetMobility (ba.GetMhType ());
  NS_ASSERT (ipv6Mobility);

  if (IsHomeMatch (src) && (m_buinf->GetHoa ()) == exttype2.GetHomeAddress ())
    {
      if (m_buinf->GetHomeLastBindingUpdateSequence () != ba.GetSequence ())
        {
          NS_LOG_LOGIC ("Sequence mismatch. Ignored. this: "
                        << m_buinf->GetHomeLastBindingUpdateSequence ()
                        << ", from: "
                        << ba.GetSequence ());

          return 0;
        }

      //check status code
      switch (ba.GetStatus ())
        {
        case Mipv6Header::BA_STATUS_BINDING_UPDATE_ACCEPTED:
          {
            m_buinf->StopHomeRetransTimer ();
            m_buinf->SetHomeAddressRegistered (true);
            m_buinf->SetHomeBUPacket (0);
            m_buinf->SetHomeReachableTime (Seconds (ba.GetLifetime ()));


            if (ba.GetLifetime () > 0)
              {
                if (!(m_buinf->GetHoa () ==  m_buinf->GetCoa ()))
                  {
                    SetupTunnelAndRouting ();
                  }

                m_buinf->MarkHomeReachable ();

                //Setup lifetime
                m_buinf->StopHomeRefreshTimer ();
                m_buinf->StartHomeRefreshTimer ();
                m_buinf->StopHomeReachableTimer ();
                m_buinf->StartHomeReachableTimer ();
                
                // Route Optimization stuff here
              }
            else
              {
                NS_LOG_INFO (this << "BA lifetime  is 0");
                ClearTunnelAndRouting ();

                Address replyMacAddress = interface->GetDevice ()->GetMulticast (dst);

                uint8_t flags = 1;

                /* send a NA to src */
                Ptr<Ipv6L3Protocol> ipv6 = GetNode ()->GetObject<Ipv6L3Protocol> ();
                Ptr<Icmpv6L4Protocol> icmp = GetNode ()->GetObject<Icmpv6L4Protocol> ();

                if (ipv6->IsForwarding (ipv6->GetInterfaceForDevice (interface->GetDevice ())))
                  {
                    flags += 4; /* R flag */
                  }

                Address hardwareAddress = interface->GetDevice ()->GetAddress ();
                Ipv6Address hoa = GetHomeAddress ();
                NdiscCache::Ipv6PayloadHeaderPair p = icmp->ForgeNA (hoa,
                                                              interface->GetLinkLocalAddress ().GetAddress (),
                                                              &hardwareAddress,
                                                              flags );

                // We must bypass the IPv6 layer, as a NA must be sent regardless of the NCE status (and not change it beyond what we did already).
                Ptr<Packet> pkt = p.first;
                pkt->AddHeader (p.second);
                interface->GetDevice ()->Send (pkt, replyMacAddress, Ipv6L3Protocol::PROT_NUMBER);
              }



            break;
          }

        default:
          NS_LOG_LOGIC ("Error occurred code=" << ba.GetStatus ());

        }

      return 0;
    }
    
  else
    {
      NS_LOG_LOGIC ("Error occurred code, No source found");
    }

  return 0;
}

bool Mipv6Mn::IsHomeMatch (Ipv6Address addr)
{
  NS_LOG_FUNCTION (this << addr);
  std::list<Ipv6Address>::iterator iter = std::find (m_Haalist.begin (), m_Haalist.end (), addr);
  if ( m_Haalist.end () == iter )
    {
      return false;
    }
  else
    {
      return true;
    }
}

uint16_t Mipv6Mn::GetHomeBUSequence ()
{
  NS_LOG_FUNCTION (this);

  return ++m_hsequence;
}

Ipv6Address Mipv6Mn::GetCoA ()
{
  NS_LOG_FUNCTION (this);
  return m_buinf->GetCoa ();
}

bool Mipv6Mn::SetupTunnelAndRouting ()
{
  NS_LOG_FUNCTION (this);
  // TODO: Set Tunnel and Routing
  return true;

}

void Mipv6Mn::ClearTunnelAndRouting ()
{
  NS_LOG_FUNCTION (this);
  // Clear Tunnel and Routing
}

void Mipv6Mn::SetRouteOptimizationRequiredField (bool roflag)
{
  m_roflag = roflag;
}


bool Mipv6Mn::IsRouteOptimizationRequired ()
{
  return m_roflag;
}

bool Mipv6Mn::SetHomeLink (Ipv6Address prefix, Ipv6Prefix mask) {
  NS_LOG_FUNCTION (this << prefix << mask);

  m_homelink = false;

  if (mask.IsMatch (prefix, GetHomeAddress ()))
    {
      NS_LOG_INFO (this << " prefix matched, in home link");
      m_homelink = true;
      return m_homelink;
    }

  NS_LOG_INFO (this << " not in home link");
  return m_homelink;
}

void Mipv6Mn::SetDefaultRouterAddress (Ipv6Address addr,  uint32_t index)
{
  NS_LOG_FUNCTION (this << addr << index);
  m_defaultrouteraddress = addr;
  m_IfIndex = index;
}

bool Mipv6Mn::CheckAddresses (Ipv6Address ha, Ipv6Address hoa)
{
  NS_LOG_FUNCTION (this << ha << hoa);
  if (ha == m_buinf->GetHA () && hoa == m_buinf->GetHoa ())
    {
      return true;
    }
  return false;
}

Ipv6Address Mipv6Mn::GetHomeAddress ()
{
  NS_LOG_FUNCTION (this);
  return m_buinf->GetHoa ();
}

} /* namespace ns3 */

