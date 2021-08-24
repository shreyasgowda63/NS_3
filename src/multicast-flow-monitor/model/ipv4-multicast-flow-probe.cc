/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2009 INESC Porto
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: Caleb Bowers <caleb.bowers@nrl.navy.mil>
//

#include "ns3/ipv4-multicast-flow-probe.h"
#include "ns3/ipv4-multicast-flow-classifier.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/multicast-flow-monitor.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/config.h"
#include "ns3/flow-id-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv4MulticastFlowProbe");

//////////////////////////////////////
// Ipv4MulticastFlowProbeTag class implementation //
//////////////////////////////////////

/**
 * \ingroup multicast-flow-monitor
 *
 * \brief Tag used to allow a fast identification of the packet
 *
 * This tag is added by FlowMonitor when a packet is seen for
 * the first time, and it is then used to classify the packet in
 * the following hops.
 */
class Ipv4MulticastFlowProbeTag : public Tag
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
  Ipv4MulticastFlowProbeTag ();
  /**
  * \brief Consructor
  * \param flowId the flow identifier
  * \param packetId the packet identifier
  * \param packetSize the packet size
  * \param src packet source address
  * \param dst packet destination address
  */
  Ipv4MulticastFlowProbeTag (uint32_t flowId, uint32_t packetId, uint32_t packetSize, Ipv4Address src, Ipv4Address dst);
  /**
  * \brief Set the flow identifier
  * \param flowId the flow identifier
  */
  void SetMulticastFlowId (uint32_t flowId);
  /**
  * \brief Set the packet identifier
  * \param packetId the packet identifier
  */
  void SetMulticastPacketId (uint32_t packetId);
  /**
  * \brief Set the packet size
  * \param packetSize the packet size
  */
  void SetMulticastPacketSize (uint32_t packetSize);
  /**
  * \brief Set the flow identifier
  * \returns the flow identifier
  */
  uint32_t GetMulticastFlowId (void) const;
  /**
  * \brief Set the packet identifier
  * \returns the packet identifier
  */
  uint32_t GetMulticastPacketId (void) const;
  /**
  * \brief Get the packet size
  * \returns the packet size
  */
  uint32_t GetMulticastPacketSize (void) const;
  /**
  * \brief Checks if the addresses stored in tag are matching
  * the arguments.
  *
  * This check is important for IP over IP encapsulation.
  *
  * \param src Source address.
  * \param dst Destination address.
  * \returns True if the addresses are matching.
  */
  bool IsSrcDstValid (Ipv4Address src, Ipv4Address dst) const;

private:
  uint32_t m_mcastFlowId;        //!< flow identifier
  uint32_t m_mcastPacketId;      //!< packet identifier
  uint32_t m_mcastPacketSize;    //!< packet size
  Ipv4Address m_src;        //!< IP source
  Ipv4Address m_dst;        //!< IP destination
};

TypeId
Ipv4MulticastFlowProbeTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ipv4MulticastFlowProbeTag")
    .SetParent<Tag> ()
    .SetGroupName ("MulticastFlowMonitor")
    .AddConstructor<Ipv4MulticastFlowProbeTag> ()
  ;
  return tid;
}
TypeId
Ipv4MulticastFlowProbeTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t
Ipv4MulticastFlowProbeTag::GetSerializedSize (void) const
{
  return 4 + 4 + 4 + 8;
}
void
Ipv4MulticastFlowProbeTag::Serialize (TagBuffer buf) const
{
  buf.WriteU32 (m_mcastFlowId);
  buf.WriteU32 (m_mcastPacketId);
  buf.WriteU32 (m_mcastPacketSize);

  uint8_t tBuf[4];
  m_src.Serialize (tBuf);
  buf.Write (tBuf, 4);
  m_dst.Serialize (tBuf);
  buf.Write (tBuf, 4);
}
void
Ipv4MulticastFlowProbeTag::Deserialize (TagBuffer buf)
{
  m_mcastFlowId = buf.ReadU32 ();
  m_mcastPacketId = buf.ReadU32 ();
  m_mcastPacketSize = buf.ReadU32 ();

  uint8_t tBuf[4];
  buf.Read (tBuf, 4);
  m_src = Ipv4Address::Deserialize (tBuf);
  buf.Read (tBuf, 4);
  m_dst = Ipv4Address::Deserialize (tBuf);
}
void
Ipv4MulticastFlowProbeTag::Print (std::ostream &os) const
{
  os << "MulticastFlowId=" << m_mcastFlowId;
  os << " MulticastPacketId=" << m_mcastPacketId;
  os << " MulticastPacketSize=" << m_mcastPacketSize;
}
Ipv4MulticastFlowProbeTag::Ipv4MulticastFlowProbeTag ()
  : Tag ()
{}

Ipv4MulticastFlowProbeTag::Ipv4MulticastFlowProbeTag (uint32_t flowId, uint32_t packetId, uint32_t packetSize, Ipv4Address src, Ipv4Address dst)
  : Tag (), m_mcastFlowId (flowId), m_mcastPacketId (packetId), m_mcastPacketSize (packetSize), m_src (src), m_dst (dst)
{}

void
Ipv4MulticastFlowProbeTag::SetMulticastFlowId (uint32_t id)
{
  m_mcastFlowId = id;
}
void
Ipv4MulticastFlowProbeTag::SetMulticastPacketId (uint32_t id)
{
  m_mcastPacketId = id;
}
void
Ipv4MulticastFlowProbeTag::SetMulticastPacketSize (uint32_t size)
{
  m_mcastPacketSize = size;
}
uint32_t
Ipv4MulticastFlowProbeTag::GetMulticastFlowId (void) const
{
  return m_mcastFlowId;
}
uint32_t
Ipv4MulticastFlowProbeTag::GetMulticastPacketId (void) const
{
  return m_mcastPacketId;
}
uint32_t
Ipv4MulticastFlowProbeTag::GetMulticastPacketSize (void) const
{
  return m_mcastPacketSize;
}
bool
Ipv4MulticastFlowProbeTag::IsSrcDstValid (Ipv4Address src, Ipv4Address dst) const
{
  return ((m_src == src) && (m_dst == dst));
}

////////////////////////////////////////
// Ipv4MulticastFlowProbe class implementation //
////////////////////////////////////////

Ipv4MulticastFlowProbe::Ipv4MulticastFlowProbe (Ptr<MulticastFlowMonitor> monitor,
                                                Ptr<Ipv4MulticastFlowClassifier> classifier,
                                                Ptr<Node> node, std::map<Ipv4Address, std::vector<uint32_t> > addressGroups)
  : MulticastFlowProbe (monitor),
    m_classifier (classifier)
{
  NS_LOG_FUNCTION (this << node->GetId ());

  m_ipv4 = node->GetObject<Ipv4L3Protocol> ();

  if (!m_ipv4->TraceConnect ("SendOutgoing", std::to_string (node->GetId ()),
                             MakeCallback (&Ipv4MulticastFlowProbe::SendOutgoingLogger, Ptr<Ipv4MulticastFlowProbe> (this))))
    {
      NS_FATAL_ERROR ("trace fail");
    }
  if (!m_ipv4->TraceConnect ("MulticastForward", std::to_string (node->GetId ()),
                             MakeCallback (&Ipv4MulticastFlowProbe::ForwardLogger, Ptr<Ipv4MulticastFlowProbe> (this))))
    {
      NS_FATAL_ERROR ("trace fail");
    }
  if (!m_ipv4->TraceConnect ("LocalDeliver", std::to_string (node->GetId ()),
                             MakeCallback (&Ipv4MulticastFlowProbe::ForwardUpLogger, Ptr<Ipv4MulticastFlowProbe> (this))))
    {
      NS_FATAL_ERROR ("trace fail");
    }

  if (!m_ipv4->TraceConnect ("Drop", std::to_string (node->GetId ()),
                             MakeCallback (&Ipv4MulticastFlowProbe::DropLogger, Ptr<Ipv4MulticastFlowProbe> (this))))
    {
      NS_FATAL_ERROR ("trace fail");
    }

  std::ostringstream qd;
  qd << "/NodeList/" << node->GetId () << "/$ns3::TrafficControlLayer/RootQueueDiscList/*/Drop";
  Config::ConnectFailSafe (qd.str (), MakeCallback (&Ipv4MulticastFlowProbe::QueueDiscDropLogger, Ptr<Ipv4MulticastFlowProbe> (this)));

  // code copied from point-to-point-helper.cc
  std::ostringstream oss;
  oss << "/NodeList/" << node->GetId () << "/DeviceList/*/TxQueue/Drop";
  Config::ConnectFailSafe (oss.str (), MakeCallback (&Ipv4MulticastFlowProbe::QueueDropLogger, Ptr<Ipv4MulticastFlowProbe> (this)));

  m_flowGroupNodes = addressGroups;
}

Ipv4MulticastFlowProbe::~Ipv4MulticastFlowProbe ()
{}

/* static */
TypeId
Ipv4MulticastFlowProbe::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ipv4MulticastFlowProbe")
    .SetParent<MulticastFlowProbe> ()
    .SetGroupName ("MulticastFlowMonitor")
    // No AddConstructor because this class has no default constructor.
  ;

  return tid;
}

void
Ipv4MulticastFlowProbe::DoDispose ()
{
  m_ipv4 = 0;
  m_classifier = 0;
  MulticastFlowProbe::DoDispose ();
}

uint32_t
Ipv4MulticastFlowProbe::GetNodeFromContext (std::string context, std::string type)
{
  std::string sub = context.substr (10);
  uint32_t pos = sub.find (type);
  uint32_t nodeId = atoi (sub.substr (0, pos).c_str ());
  return nodeId;
}

void
Ipv4MulticastFlowProbe::SendOutgoingLogger (std::string nodeId,
                                            const Ipv4Header &ipHeader,
                                            Ptr<const Packet> ipPayload,
                                            uint32_t interface)
{
  MulticastFlowId flowId;
  MulticastFlowPacketId packetId;

  uint32_t txNodeId = static_cast<uint32_t> (std::stoi (nodeId));

  Ipv4MulticastFlowProbeTag fTag;
  bool found = ipPayload->FindFirstMatchingByteTag (fTag);
  if (found)
    {
      return;
    }

  if (m_classifier->Classify (ipHeader, ipPayload, &flowId, &packetId))
    {
      uint32_t size = (ipPayload->GetSize () + ipHeader.GetSerializedSize ());
      NS_LOG_DEBUG ("ReportFirstTx (" << this << ", " << flowId << ", " << packetId << ", " << size << "); "
                                      << ipHeader << *ipPayload);
      uint32_t ttl = ipHeader.GetTtl ();
      m_multicastFlowMonitor->ReportFirstTx (this, flowId, packetId, size, txNodeId, ttl, m_flowGroupNodes[ipHeader.GetDestination ()]);

      // tag the packet with the flow id and packet id, so that the packet can be identified even
      // when Ipv4Header is not accessible at some non-IPv4 protocol layer
      Ipv4MulticastFlowProbeTag fTag (flowId, packetId, size, ipHeader.GetSource (), ipHeader.GetDestination ());
      ipPayload->AddByteTag (fTag);
    }
}

void
Ipv4MulticastFlowProbe::ForwardLogger (std::string nodeId, const Ipv4Header &ipHeader, Ptr<const Packet> ipPayload, uint32_t interface)
{
  Ipv4MulticastFlowProbeTag fTag;
  bool found = ipPayload->FindFirstMatchingByteTag (fTag);

  uint32_t fwdNodeId = static_cast<uint32_t> (std::stoi (nodeId));

  if (found)
    {
      if (!ipHeader.IsLastFragment () || ipHeader.GetFragmentOffset () != 0)
        {
          NS_LOG_WARN ("Not counting fragmented packets");
          return;
        }
      if (!fTag.IsSrcDstValid (ipHeader.GetSource (), ipHeader.GetDestination ()))
        {
          NS_LOG_LOGIC ("Not reporting encapsulated packet");
          return;
        }

      MulticastFlowId flowId = fTag.GetMulticastFlowId ();
      MulticastFlowPacketId packetId = fTag.GetMulticastPacketId ();

      uint32_t size = (ipPayload->GetSize () + ipHeader.GetSerializedSize ());
      NS_LOG_DEBUG ("ReportForwarding (" << this << ", " << flowId << ", " << packetId << ", " << size << ");");
      m_multicastFlowMonitor->ReportForwarding (this, flowId, packetId, size, fwdNodeId);
    }
}

void
Ipv4MulticastFlowProbe::ForwardUpLogger (std::string nodeId, const Ipv4Header &ipHeader, Ptr<const Packet> ipPayload, uint32_t interface)
{
  Ipv4MulticastFlowProbeTag fTag;
  bool found = ipPayload->FindFirstMatchingByteTag (fTag);

  uint32_t rxNodeId = static_cast<uint32_t> (std::stoi (nodeId));

  if (found)
    {
      if (!fTag.IsSrcDstValid (ipHeader.GetSource (), ipHeader.GetDestination ()))
        {
          NS_LOG_LOGIC ("Not reporting encapsulated packet");
          return;
        }

      MulticastFlowId flowId = fTag.GetMulticastFlowId ();
      MulticastFlowPacketId packetId = fTag.GetMulticastPacketId ();

      uint32_t size = (ipPayload->GetSize () + ipHeader.GetSerializedSize ());
      NS_LOG_DEBUG ("ReportRx (" << this << ", " << flowId << ", " << packetId << ", " << size << "); "
                                 << ipHeader << *ipPayload);
      uint32_t ttl = ipHeader.GetTtl ();
      // std::cout << "TTL = " << ttl << std::endl;
      m_multicastFlowMonitor->ReportRx (this, flowId, packetId, size, rxNodeId, ttl);
    }
}

void
Ipv4MulticastFlowProbe::DropLogger (std::string nodeId, const Ipv4Header &ipHeader, Ptr<const Packet> ipPayload,
                                    Ipv4L3Protocol::DropReason reason, Ptr<Ipv4> ipv4, uint32_t ifIndex)
{
#if 0
  switch (reason)
    {
      case Ipv4L3Protocol::DROP_NO_ROUTE:
        break;

      case Ipv4L3Protocol::DROP_TTL_EXPIRED:
      case Ipv4L3Protocol::DROP_BAD_CHECKSUM:
        Ipv4Address addri = m_ipv4->GetAddress (ifIndex);
        Ipv4Mask maski = m_ipv4->GetNetworkMask (ifIndex);
        Ipv4Address bcast = addri.GetSubnetDirectedBroadcast (maski);
        if (ipHeader.GetDestination () == bcast) // we don't want broadcast packets
          {
            return;
          }
    }
#endif

  Ipv4MulticastFlowProbeTag fTag;
  bool found = ipPayload->FindFirstMatchingByteTag (fTag);

  uint32_t dropNodeId = static_cast<uint32_t> (std::stoi (nodeId));

  if (found)
    {
      MulticastFlowId flowId = fTag.GetMulticastFlowId ();
      MulticastFlowPacketId packetId = fTag.GetMulticastPacketId ();

      uint32_t size = (ipPayload->GetSize () + ipHeader.GetSerializedSize ());
      NS_LOG_DEBUG ("Drop (" << this << ", " << flowId << ", " << packetId << ", " << size << ", " << reason
                             << ", destIp=" << ipHeader.GetDestination () << "); "
                             << "HDR: " << ipHeader << " PKT: " << *ipPayload);

      DropReason myReason;


      switch (reason)
        {
          case Ipv4L3Protocol::DROP_TTL_EXPIRED:
            myReason = DROP_TTL_EXPIRED;
            NS_LOG_DEBUG ("DROP_TTL_EXPIRED");
            break;
          case Ipv4L3Protocol::DROP_NO_ROUTE:
            myReason = DROP_NO_ROUTE;
            NS_LOG_DEBUG ("DROP_NO_ROUTE");
            break;
          case Ipv4L3Protocol::DROP_BAD_CHECKSUM:
            myReason = DROP_BAD_CHECKSUM;
            NS_LOG_DEBUG ("DROP_BAD_CHECKSUM");
            break;
          case Ipv4L3Protocol::DROP_INTERFACE_DOWN:
            myReason = DROP_INTERFACE_DOWN;
            NS_LOG_DEBUG ("DROP_INTERFACE_DOWN");
            break;
          case Ipv4L3Protocol::DROP_ROUTE_ERROR:
            myReason = DROP_ROUTE_ERROR;
            NS_LOG_DEBUG ("DROP_ROUTE_ERROR");
            break;
          case Ipv4L3Protocol::DROP_FRAGMENT_TIMEOUT:
            myReason = DROP_FRAGMENT_TIMEOUT;
            NS_LOG_DEBUG ("DROP_FRAGMENT_TIMEOUT");
            break;
          case Ipv4L3Protocol::DROP_DUPLICATE:
            myReason = DROP_DUPLICATE;
            NS_LOG_DEBUG ("DROP_DUPLICATE");
            break;


          default:
            myReason = DROP_INVALID_REASON;
            NS_FATAL_ERROR ("Unexpected drop reason code " << reason);
        }

      if (myReason == DROP_DUPLICATE)
        {
          m_multicastFlowMonitor->ReportDupDrop (this, flowId, packetId, size, dropNodeId);
        }
      else
        {
          m_multicastFlowMonitor->ReportDrop (this, flowId, packetId, size, myReason, dropNodeId);
        }
    }
}

void
Ipv4MulticastFlowProbe::QueueDropLogger (std::string context, Ptr<const Packet> ipPayload)
{
  Ipv4MulticastFlowProbeTag fTag;
  bool tagFound = ipPayload->FindFirstMatchingByteTag (fTag);

  if (!tagFound)
    {
      return;
    }

  uint32_t node = GetNodeFromContext (context, "/DeviceList");

  MulticastFlowId flowId = fTag.GetMulticastFlowId ();
  MulticastFlowPacketId packetId = fTag.GetMulticastPacketId ();
  uint32_t size = fTag.GetMulticastPacketSize ();

  NS_LOG_DEBUG ("Drop (" << this << ", " << flowId << ", " << packetId << ", " << size << ", " << DROP_QUEUE
                         << "); ");

  m_multicastFlowMonitor->ReportDrop (this, flowId, packetId, size, DROP_QUEUE, node);
}

void
Ipv4MulticastFlowProbe::QueueDiscDropLogger (std::string context, Ptr<const QueueDiscItem> item)
{
  Ipv4MulticastFlowProbeTag fTag;
  bool tagFound = item->GetPacket ()->FindFirstMatchingByteTag (fTag);

  if (!tagFound)
    {
      return;
    }

  uint32_t node = GetNodeFromContext (context, "/$ns3::TrafficControlLayer");

  MulticastFlowId flowId = fTag.GetMulticastFlowId ();
  MulticastFlowPacketId packetId = fTag.GetMulticastPacketId ();
  uint32_t size = fTag.GetMulticastPacketSize ();

  NS_LOG_DEBUG ("Drop (" << this << ", " << flowId << ", " << packetId << ", " << size << ", " << DROP_QUEUE_DISC
                         << "); ");

  m_multicastFlowMonitor->ReportDrop (this, flowId, packetId, size, DROP_QUEUE_DISC, node);
}

} // namespace ns3


