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
 *
 */
#include "simple-distributed-channel.h"
#include "simple-distributed-net-device.h"
#include "simple-distributed-tag.h"
#include "channel-error-model.h"
#include "channel-delay-model.h"
#include "ns3/mobility-model.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/double.h"
#include "ns3/boolean.h"

#ifdef NS3_MPI
#include "ns3/mpi-interface.h"
#endif

#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimpleDistributedChannel");

NS_OBJECT_ENSURE_REGISTERED (SimpleDistributedChannel);

TypeId
SimpleDistributedChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimpleDistributedChannel")
    .SetParent<Channel> ()
    .SetGroupName ("SimpleDistributed")
    .AddConstructor<SimpleDistributedChannel> ()
    .AddAttribute ("Delay", "Transmission delay through the channel",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&SimpleDistributedChannel::m_delay),
                   MakeTimeChecker ())
    .AddAttribute ("DataRate",
                   "The default data rate for the channel. Zero means infinite bandwidth",
                   DataRateValue (DataRate ("0b/s")),
                   MakeDataRateAccessor (&SimpleDistributedChannel::m_dataRate),
                   MakeDataRateChecker ())
    .AddAttribute ("DelayModel",
                   "The optional delay model used to calculate packet delays",
                   PointerValue (),
                   MakePointerAccessor (&SimpleDistributedChannel::m_delayModel),
                   MakePointerChecker<ChannelDelayModel> ())
    .AddAttribute ("PromiscuousMode", "Promiscous mode all nodes receive packets on the channel; will impact parallel performance if enabled",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SimpleDistributedChannel::m_promiscuous),
                   MakeBooleanChecker ())
    .AddAttribute ("Distance", "Limit transmission to specified distance from sender node. Negative value indicates don't use distance",
                   DoubleValue (-1.0),
                   MakeDoubleAccessor (&SimpleDistributedChannel::m_distance),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ErrorModel",
                   "The receiver error model used to simulate packet loss",
                   PointerValue (),
                   MakePointerAccessor (&SimpleDistributedChannel::m_errorModel),
                   MakePointerChecker<ChannelErrorModel> ())
    .AddTraceSource ("PhyRxDrop",
                     "Trace source indicating a packet has been dropped "
                     "by the channel",
                     MakeTraceSourceAccessor (&SimpleDistributedChannel::m_phyRxDropTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

SimpleDistributedChannel::SimpleDistributedChannel ()
{
  NS_LOG_FUNCTION (this);
}


void
SimpleDistributedChannel::Send (Ptr<Packet> p, uint16_t protocol,
                                Mac48Address to, Mac48Address from,
                                Ptr<SimpleDistributedNetDevice> srcDevice,
                                Ptr<SimpleDistributedNetDevice> dstDevice,
                                Ptr<MobilityModel> srcMobilityModel,
                                Time txTime
                                )
{
  NS_LOG_FUNCTION (this << p << protocol << to << from << srcDevice << dstDevice << srcMobilityModel << txTime);
  
  auto dstNode = dstDevice->GetNode ();
  auto mySysId = Simulator::GetSystemId ();
  auto remoteSysId = dstNode->GetSystemId ();

  // Check if within specified distance.
  Ptr<MobilityModel> dstMobilityModel = dstNode->GetObject<MobilityModel> ();

  if (m_distance > 0 && dstMobilityModel && srcMobilityModel)
    {
      double distanceToDst = srcMobilityModel->GetDistanceFrom (dstMobilityModel);
      if (distanceToDst > m_distance)
        {
          NS_LOG_LOGIC("Dropping packet due to distance " << p);
          return;
        }
    }

  // Calculate delay from channel, added to delay from the net device
  Time delay = txTime + TransmitDelaySendSide(p, to, from, srcDevice);

  Vector srcPosition (0,0,0);
  auto srcNode = srcDevice->GetNode ();
  if (srcMobilityModel)
    {
      srcPosition = srcMobilityModel->GetPosition ();
    }
  
  
  if (mySysId == remoteSysId)
    {
      
      if (m_errorModel && m_errorModel->IsCorrupt (p, srcNode -> GetId (), srcPosition, dstDevice))
        {
          m_phyRxDropTrace (p);
          return;
        }
      
      delay += TransmitDelayReceiveSide(p, srcNode -> GetId (), srcPosition, dstDevice);
      
      NS_LOG_LOGIC("Schedule receive for node " << dstNode->GetId () << " delay " << delay);
      Simulator::ScheduleWithContext (dstNode->GetId (), delay,
                                      &SimpleDistributedNetDevice::Receive, dstDevice, p->Copy (), protocol, to, from);
    }
  else
    {
#ifdef NS3_MPI
      auto sendPacket = p->Copy ();
      SimpleDistributedTag tag(from, srcNode->GetId (), srcPosition, to, protocol);
      sendPacket->AddPacketTag (tag);
      
      // Calculate the rxTime (absolute)
      Time rxTime = Simulator::Now () + delay;
      // A performance enhancement for broadcasts would be to send the
      // to an MPI rank and then all net devices on the channel to
      // avoid multiple MPI messages to the same rank.  This would
      // require some changes to MpiInterface.
      MpiInterface::SendPacket (sendPacket, rxTime, dstNode->GetId (), dstDevice->GetIfIndex ());
#else
      NS_ASSERT_MSG (mySysId == remoteSysId, "If MPI is not used in compilation; all nodes must be on same rank.");
#endif
  }
}  

void
SimpleDistributedChannel::Send (Ptr<Packet> p, uint16_t protocol,
                                Mac48Address to, Mac48Address from,
                                Ptr<SimpleDistributedNetDevice> sender,
                                Time txTime
                                )
{
  NS_LOG_FUNCTION (this << p << protocol << to << from << sender << txTime);

  Ptr<MobilityModel> srcMobilityModel = sender->GetNode ()->GetObject<MobilityModel> ();

  if (m_promiscuous || to.IsBroadcast ())
    {
      // Send to all devices on the channel.
      for (std::vector<Ptr<SimpleDistributedNetDevice> >::const_iterator i = m_devicesVector.begin (); i != m_devicesVector.end (); ++i)
        {
          Ptr<SimpleDistributedNetDevice> dstDevice = *i;
          if (dstDevice == sender)
            {
              continue;
            }

          Send (p, protocol,
                to, from,
                sender,
                dstDevice,
                srcMobilityModel,
                txTime
                );
        }
    }
  else 
    {
      // not promiscuous || broadcast only send to the destination
      DeviceMap::const_iterator got = m_devicesMap.find (to);
      Ptr<SimpleDistributedNetDevice> dstDevice = got->second;

      Send (p, protocol,
            to, from,
            sender, dstDevice,
            srcMobilityModel,
            txTime
            );
    }
}

void
SimpleDistributedChannel::Add (Ptr<SimpleDistributedNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  m_devicesMap.insert (std::make_pair (Mac48Address::ConvertFrom (device->GetAddress ()), device));
  m_devicesVector.push_back (device);
}

std::size_t
SimpleDistributedChannel::GetNDevices (void) const
{
  NS_LOG_FUNCTION (this);
  return m_devicesVector.size ();
}

Ptr<NetDevice>
SimpleDistributedChannel::GetDevice (std::size_t i) const
{
  NS_LOG_FUNCTION (this << i);
  return m_devicesVector[i];
}

void
SimpleDistributedChannel::SetDelayModel (Ptr<ChannelDelayModel> delay_model)
{
  m_delayModel = delay_model;
}

Ptr<ChannelDelayModel>
SimpleDistributedChannel::GetDelayModel (void)
{
  return m_delayModel;
}

Time
SimpleDistributedChannel::GetMinimumDelay (void)
{

  Time delay = m_delay;

  if (m_delayModel)
    {
      delay += m_delayModel -> GetMinimumDelay ();
    }

  return delay;
}

void
SimpleDistributedChannel::SetErrorModel (Ptr<ChannelErrorModel> error_model)
{
  m_errorModel = error_model;
}

Ptr<ChannelErrorModel>
SimpleDistributedChannel::GetErrorModel (void)
{
  return m_errorModel;
}

Time SimpleDistributedChannel::TransmitDelaySendSide (Ptr<Packet> p,
                                                      Mac48Address to,
                                                      Mac48Address from,
                                                      Ptr<SimpleDistributedNetDevice> sender)
{
  NS_LOG_FUNCTION (this << p << to << from << sender);

  Time delay = m_delay;

  NS_LOG_DEBUG ("m_delay = " << m_delay);

  if (m_dataRate > DataRate (0))
    {
      delay += m_dataRate.CalculateBytesTxTime (p->GetSize ());
    }

  NS_LOG_DEBUG ("send side transmit delay = " << delay);
  return delay;
}


Time SimpleDistributedChannel::TransmitDelayReceiveSide (Ptr<Packet> p,
                                                         uint32_t srcNodeId,
                                                         Vector srcPosition,
                                                         Ptr<SimpleDistributedNetDevice> dstDevice
                                                         )
{
  NS_LOG_FUNCTION (this << p << srcNodeId << srcPosition << dstDevice);

  Time delay;

  if (m_delayModel)
    {
      delay += m_delayModel->ComputeDelay (p, srcNodeId, srcPosition, dstDevice);
    }
  
  NS_LOG_DEBUG ("receive side transmit delay = " << delay);
  return delay;
}

} // namespace ns3
