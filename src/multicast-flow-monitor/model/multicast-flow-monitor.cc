/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "multicast-flow-monitor.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include <fstream>
#include <sstream>

#define PERIODIC_CHECK_INTERVAL (Seconds (1))

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MulticastFlowMonitor");

NS_OBJECT_ENSURE_REGISTERED (MulticastFlowMonitor);

TypeId
MulticastFlowMonitor::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MulticastFlowMonitor")
    .SetParent<Object> ()
    .SetGroupName ("MulticastFlowMonitor")
    .AddConstructor<MulticastFlowMonitor> ()
    .AddAttribute ("MaxPerHopDelay", ("The maximum per-hop delay that should be considered.  "
                                      "Packets still not received after this delay are to be considered lost."),
                   TimeValue (Seconds (10.0)),
                   MakeTimeAccessor (&MulticastFlowMonitor::m_maxPerHopDelay),
                   MakeTimeChecker ())
    .AddAttribute ("StartTime", ("The time when the monitoring starts."),
                   TimeValue (Seconds (0.0)),
                   MakeTimeAccessor (&MulticastFlowMonitor::Start),
                   MakeTimeChecker ())
  ;
  return tid;
}

TypeId
MulticastFlowMonitor::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

MulticastFlowMonitor::MulticastFlowMonitor ()
  : m_enabled (false)
{
  NS_LOG_FUNCTION (this);
}

void
MulticastFlowMonitor::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_startEvent);
  Simulator::Cancel (m_stopEvent);
  for (std::list<Ptr<MulticastFlowClassifier> >::iterator iter = m_mcastClassifiers.begin ();
       iter != m_mcastClassifiers.end ();
       iter++)
    {
      *iter = 0;
    }
  for (uint32_t i = 0; i < m_multicastFlowProbes.size (); i++)
    {
      m_multicastFlowProbes[i]->Dispose ();
      m_multicastFlowProbes[i] = 0;
    }
  Object::DoDispose ();
}

inline MulticastFlowMonitor::MulticastFlowStats&
MulticastFlowMonitor::GetStatsForMulticastFlow (MulticastFlowId flowId)
{
  NS_LOG_FUNCTION (this);
  MulticastFlowStatsContainerI iter;
  iter = m_multicastFlowStats.find (flowId);
  if (iter == m_multicastFlowStats.end ())
    {
      MulticastFlowMonitor::MulticastFlowStats &ref = m_multicastFlowStats[flowId];
      ref.delaySum.clear ();
      ref.jitterSum.clear ();
      ref.lastDelay.clear ();
      ref.txBytes = 0;
      ref.rxBytes.clear ();
      ref.txPackets = 0;
      ref.rxPackets.clear ();
      ref.lostPackets.clear ();
      ref.nodeLostPackets.clear ();
      ref.timesForwarded.clear ();
      ref.dupsDropped.clear ();
      ref.groupNodeIds.clear ();
      ref.numHops.clear ();
      ref.packetDelay.clear ();
      // ref.delayHistogram.SetDefaultBinWidth (m_delayBinWidth);
      // ref.jitterHistogram.SetDefaultBinWidth (m_jitterBinWidth);
      // ref.packetSizeHistogram.SetDefaultBinWidth (m_packetSizeBinWidth);
      // ref.flowInterruptionsHistogram.SetDefaultBinWidth (m_flowInterruptionsBinWidth);
      return ref;
    }
  else
    {
      return iter->second;
    }
}

inline MulticastFlowMonitor::MulticastFlowStats&
MulticastFlowMonitor::GetStatsForMulticastFlow (MulticastFlowId flowId, std::vector<uint32_t> groupNodes)
{
  NS_LOG_FUNCTION (this);
  MulticastFlowStatsContainerI iter;
  iter = m_multicastFlowStats.find (flowId);
  if (iter == m_multicastFlowStats.end ())
    {
      MulticastFlowMonitor::MulticastFlowStats &ref = m_multicastFlowStats[flowId];
      ref.txPackets = 0;
      ref.txBytes = 0;
      ref.groupNodeIds = groupNodes;
      std::vector<uint32_t>::iterator it;
      for (it = ref.groupNodeIds.begin (); it != ref.groupNodeIds.end (); it++)
        {
          // ref.delaySum[*(it)] = Seconds(0);
          // ref.jitterSum[*(it)] = Seconds(0);
          ref.lastDelay[*(it)] = Seconds (0);

          ref.timesForwarded[*(it)] = 0;
          ref.rxBytes[*(it)] = 0;

          ref.rxPackets[*(it)] = 0;
          ref.lostPackets[*(it)] = 0;

          ref.dupsDropped[*(it)] = 0;
        }
      return ref;
    }
  else
    {
      return iter->second;
    }
}

void
MulticastFlowMonitor::ReportFirstTx (Ptr<MulticastFlowProbe> probe,
                                     uint32_t flowId,
                                     uint32_t packetId,
                                     uint32_t packetSize,
                                     uint32_t txNodeId,
                                     uint32_t ttl,
                                     std::vector<uint32_t> groupNodeIds)
{
  NS_LOG_FUNCTION (this << probe << flowId << packetId << packetSize);
  if (!m_enabled)
    {
      NS_LOG_DEBUG ("MulticastFlowMonitor not enabled; returning");
      return;
    }
  Time now = Simulator::Now ();
  TrackedPacket &tracked = m_trackedPackets[std::make_pair (flowId, packetId)];
  tracked.firstSeenTime = now;
  tracked.lastSeenTime = tracked.firstSeenTime;
  tracked.timesForwarded = 0;
  tracked.nodesSeen = 1;
  tracked.initial_ttl = ttl;
  NS_LOG_DEBUG ("ReportFirstTx: adding tracked packet (flowId=" << flowId << ", packetId=" << packetId
                                                                << ").");

  probe->AddPacketStats (flowId, packetSize, Seconds (0), txNodeId);
  MulticastFlowStats &stats = GetStatsForMulticastFlow (flowId, groupNodeIds);
  stats.groupNodeIds = groupNodeIds;
  std::vector<uint32_t>::iterator it;
  stats.txBytes += packetSize;
  stats.txPackets++;
  if (stats.txPackets == 1)
    {
      stats.timeFirstTxPacket = now;
    }
  stats.timeLastTxPacket = now;
}

void
MulticastFlowMonitor::ReportForwarding (Ptr<MulticastFlowProbe> probe,
                                        uint32_t flowId,
                                        uint32_t packetId,
                                        uint32_t packetSize,
                                        uint32_t nodeId)
{
  NS_LOG_FUNCTION (this << probe << flowId << packetId << packetSize);
  if (!m_enabled)
    {
      NS_LOG_DEBUG ("MulticastFlowMonitor not enabled; returning");
      return;
    }
  std::pair<MulticastFlowId, MulticastFlowPacketId> key (flowId, packetId);
  TrackedPacketMap::iterator tracked = m_trackedPackets.find (key);
  if (tracked == m_trackedPackets.end ())
    {
      NS_LOG_WARN ("Received packet forward report (flowId=" << flowId << ", packetId=" << packetId
                                                             << ") but not known to be transmitted.");
      return;
    }

  tracked->second.timesForwarded++;
  tracked->second.nodesSeen++;
  tracked->second.lastSeenTime = Simulator::Now ();

  Time delay = (Simulator::Now () - tracked->second.firstSeenTime);
  probe->AddPacketStats (flowId, packetSize, delay, nodeId);
}

void
MulticastFlowMonitor::ReportRx (Ptr<MulticastFlowProbe> probe, uint32_t flowId, uint32_t packetId, uint32_t packetSize, uint32_t nodeId, uint32_t ttl)
{
  NS_LOG_FUNCTION (this << probe << flowId << packetId << packetSize);
  if (!m_enabled)
    {
      NS_LOG_DEBUG ("FlowMonitor not enabled; returning");
      return;
    }
  TrackedPacketMap::iterator tracked = m_trackedPackets.find (std::make_pair (flowId, packetId));
  if (tracked == m_trackedPackets.end ())
    {
      NS_LOG_WARN ("Received packet last-tx report (flowId=" << flowId << ", packetId=" << packetId
                                                             << ") but not known to be transmitted.");
      return;
    }

  Time now = Simulator::Now ();
  Time delay = (now - tracked->second.firstSeenTime);
  probe->AddPacketStats (flowId, packetSize, delay, nodeId);

  MulticastFlowStats &stats = GetStatsForMulticastFlow (flowId);
  stats.delaySum[nodeId] += delay;
  if (stats.rxPackets[nodeId] > 0 )
    {
      Time jitter = stats.lastDelay[nodeId] - delay;
      if (jitter > Seconds (0))
        {
          stats.jitterSum[nodeId] += jitter;
        }
      else
        {
          stats.jitterSum[nodeId] -= jitter;
        }
    }
  stats.packetDelay[nodeId][packetId] = delay;
  stats.numHops[nodeId][packetId] = tracked->second.initial_ttl - (ttl - 1);
  stats.lastDelay[nodeId] = delay;

  stats.rxBytes[nodeId] += packetSize;
  stats.rxPackets[nodeId]++;
  if (stats.rxPackets[nodeId] == 1)
    {
      stats.timeFirstRxPacket[nodeId] = now;
    }
  stats.timeLastRxPacket[nodeId] = now;
  stats.timesForwarded[nodeId] += tracked->second.timesForwarded;

  stats.groupDelivered[nodeId][packetId] = true;

  std::vector<uint32_t>::iterator it;
  bool all_del = true;
  for (it = stats.groupNodeIds.begin (); it != stats.groupNodeIds.end (); it++)
    {
      NS_LOG_DEBUG ("FlowID: " << flowId << " NodeID: " << *(it) << " PacketId: " << packetId << " delivered: " << stats.groupDelivered[*(it)][packetId] << ".");
      if (stats.groupDelivered[*(it)][packetId])
        {
          continue;
        }
      else
        {
          all_del = false;
        }
    }

  if (all_del)      // all group nodes got packet delivered
    {
      NS_LOG_DEBUG ("ReportRx: removing tracked packet (flowId="
                    << flowId << ", packetId=" << packetId << ").");

      m_trackedPackets.erase (tracked);           // we don't need to track this packet anymore
    }
}

void
MulticastFlowMonitor::ReportDrop (Ptr<MulticastFlowProbe> probe, uint32_t flowId, uint32_t packetId, uint32_t packetSize,
                                  uint32_t reasonCode, uint32_t nodeId)
{
  NS_LOG_FUNCTION (this << probe << flowId << packetId << packetSize << reasonCode);
  if (!m_enabled)
    {
      NS_LOG_DEBUG ("FlowMonitor not enabled; returning");
      return;
    }

  probe->AddPacketDropStats (flowId, packetSize, reasonCode, nodeId);

  MulticastFlowStats &stats = GetStatsForMulticastFlow (flowId);
  if (stats.packetsDropped[nodeId].size () < reasonCode + 1)
    {
      stats.packetsDropped[nodeId].resize (reasonCode + 1, 0);
      stats.bytesDropped[nodeId].resize (reasonCode + 1, 0);
    }
  ++stats.packetsDropped[nodeId][reasonCode];
  stats.bytesDropped[nodeId][reasonCode] += packetSize;
  NS_LOG_DEBUG ("++stats.packetsDropped[" << reasonCode << "]; // becomes: " << stats.packetsDropped[nodeId][reasonCode]);

  stats.groupDropped[nodeId][packetId] = true;

  std::vector<uint32_t>::iterator it;
  bool all_drop = true;
  for (it = stats.groupNodeIds.begin (); it != stats.groupNodeIds.end (); it++)
    {
      if (stats.groupDropped[*(it)][packetId])
        {
          continue;
        }
      else
        {
          all_drop = false;
        }
    }

  if (all_drop)
    {
      stats.lostPackets[nodeId]++;
      TrackedPacketMap::iterator tracked = m_trackedPackets.find (std::make_pair (flowId, packetId));
      if (tracked != m_trackedPackets.end ())
        {
          // we don't need to track this packet anymor
          NS_LOG_DEBUG ("ReportDrop: removing tracked packet (flowId="
                        << flowId << ", packetId=" << packetId << ").");
          m_trackedPackets.erase (tracked);
        }
    }
}

void
MulticastFlowMonitor::ReportDupDrop (Ptr<MulticastFlowProbe> probe, uint32_t flowId, uint32_t packetId, uint32_t packetSize,
                                     uint32_t nodeId)
{
  NS_LOG_FUNCTION (this << probe << flowId << packetId << packetSize);
  if (!m_enabled)
    {
      NS_LOG_DEBUG ("FlowMonitor not enabled; returning");
      return;
    }

  MulticastFlowStats &stats = GetStatsForMulticastFlow (flowId);

  stats.dupsDropped[nodeId]++;
}

const MulticastFlowMonitor::MulticastFlowStatsContainer&
MulticastFlowMonitor::GetMulticastFlowStats () const
{
  return m_multicastFlowStats;
}

void
MulticastFlowMonitor::CheckForLostPackets (Time maxDelay)
{
  NS_LOG_FUNCTION (this << maxDelay.As (Time::S));
  Time now = Simulator::Now ();

  for (TrackedPacketMap::iterator iter = m_trackedPackets.begin ();
       iter != m_trackedPackets.end (); )
    {
      uint32_t flow_id = iter->first.first;
      uint32_t p_id = iter->first.second;
      bool all_lost = true;
      if (now - iter->second.lastSeenTime >= maxDelay)
        {
          MulticastFlowStatsContainerI flow = m_multicastFlowStats.find (flow_id);
          NS_ASSERT (flow != m_multicastFlowStats.end ());
          std::vector<uint32_t>::iterator it;
          for (it = flow->second.groupNodeIds.begin (); it != flow->second.groupNodeIds.end (); it++)
            {
              if (flow->second.groupDelivered[*(it)][p_id])
                {
                  all_lost = false;
                  continue;
                }
              else if (flow->second.nodeLostPackets[*(it)][p_id])
                {
                  continue;
                }
              else
                {
                  flow->second.nodeLostPackets[*(it)][p_id] = true;
                  flow->second.lostPackets[*(it)]++;
                }
            }
          if (all_lost)
            {
              m_trackedPackets.erase (iter++);
            }
          else
            {
              iter++;
            }
        }
      else
        {
          iter++;
        }
    }
}

void
MulticastFlowMonitor::CheckForLostPackets ()
{
  CheckForLostPackets (m_maxPerHopDelay);
}

void
MulticastFlowMonitor::PeriodicCheckForLostPackets ()
{
  CheckForLostPackets ();
  Simulator::Schedule (PERIODIC_CHECK_INTERVAL, &MulticastFlowMonitor::PeriodicCheckForLostPackets, this);
}

void
MulticastFlowMonitor::NotifyConstructionCompleted ()
{
  Object::NotifyConstructionCompleted ();
  Simulator::Schedule (PERIODIC_CHECK_INTERVAL, &MulticastFlowMonitor::PeriodicCheckForLostPackets, this);
}

void
MulticastFlowMonitor::AddProbe (Ptr<MulticastFlowProbe> probe)
{
  m_multicastFlowProbes.push_back (probe);
}

const MulticastFlowMonitor::MulticastFlowProbeContainer&
MulticastFlowMonitor::GetAllMulticastProbes () const
{
  return m_multicastFlowProbes;
}

void
MulticastFlowMonitor::Start (const Time &time)
{
  NS_LOG_FUNCTION (this << time.As (Time::S));
  if (m_enabled)
    {
      NS_LOG_DEBUG ("MulticastFlowMonitor already enabled; returning");
      return;
    }
  Simulator::Cancel (m_startEvent);
  NS_LOG_DEBUG ("Scheduling start at " << time.As (Time::S));
  m_startEvent = Simulator::Schedule (time, &MulticastFlowMonitor::StartRightNow, this);
}

void
MulticastFlowMonitor::Stop (const Time &time)
{
  NS_LOG_FUNCTION (this << time.As (Time::S));
  Simulator::Cancel (m_stopEvent);
  NS_LOG_DEBUG ("Scheduling stop at " << time.As (Time::S));
  m_stopEvent = Simulator::Schedule (time, &MulticastFlowMonitor::StopRightNow, this);
}

void
MulticastFlowMonitor::StartRightNow ()
{
  NS_LOG_FUNCTION (this);
  if (m_enabled)
    {
      NS_LOG_DEBUG ("MulticastFlowMonitor already enabled; returning");
      return;
    }
  m_enabled = true;
}

void
MulticastFlowMonitor::StopRightNow ()
{
  NS_LOG_FUNCTION (this);
  if (!m_enabled)
    {
      NS_LOG_DEBUG ("MulticastFlowMonitor not enabled; returning");
      return;
    }
  m_enabled = false;
  CheckForLostPackets ();
}

void
MulticastFlowMonitor::AddMulticastFlowClassifier (Ptr<MulticastFlowClassifier> classifier)
{
  m_mcastClassifiers.push_back (classifier);
}

}

