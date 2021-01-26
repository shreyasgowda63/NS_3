/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013
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
 * Author: Ghada Badawy <gbadawy@gmail.com>
 *         Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/log.h"
#include "ns3/packet.h"
#include "mpdu-aggregator.h"
#include "ampdu-subframe-header.h"
#include "wifi-phy.h"
#include "wifi-tx-vector.h"
#include "wifi-remote-station-manager.h"
#include "mac-low.h"
#include "wifi-mac-queue-item.h"
#include "wifi-mac-queue.h"
#include "msdu-aggregator.h"
#include "wifi-net-device.h"
#include "ht-capabilities.h"
#include "vht-capabilities.h"
#include "he-capabilities.h"
#include "regular-wifi-mac.h"
#include "ctrl-headers.h"
#include "wifi-mac-trailer.h"
#include "wifi-tx-parameters.h"

NS_LOG_COMPONENT_DEFINE ("MpduAggregator");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (MpduAggregator);

TypeId
MpduAggregator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MpduAggregator")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddConstructor<MpduAggregator> ()
  ;
  return tid;
}

MpduAggregator::MpduAggregator ()
{
}

MpduAggregator::~MpduAggregator ()
{
}

void
MpduAggregator::DoDispose ()
{
  m_mac = 0;
  Object::DoDispose ();
}

void
MpduAggregator::SetWifiMac (const Ptr<RegularWifiMac> mac)
{
  NS_LOG_FUNCTION (this << mac);
  m_mac = mac;
}

void
MpduAggregator::Aggregate (Ptr<const WifiMacQueueItem> mpdu, Ptr<Packet> ampdu, bool isSingle)
{
  NS_LOG_FUNCTION (mpdu << ampdu << isSingle);
  NS_ASSERT (ampdu);
  // if isSingle is true, then ampdu must be empty
  NS_ASSERT (!isSingle || ampdu->GetSize () == 0);

  // pad the previous A-MPDU subframe if the A-MPDU is not empty
  if (ampdu->GetSize () > 0)
    {
      uint8_t padding = CalculatePadding (ampdu->GetSize ());

      if (padding)
        {
          Ptr<Packet> pad = Create<Packet> (padding);
          ampdu->AddAtEnd (pad);
        }
    }

  // add MPDU header and trailer
  Ptr<Packet> tmp = mpdu->GetPacket ()->Copy ();
  tmp->AddHeader (mpdu->GetHeader ());
  AddWifiMacTrailer (tmp);

  // add A-MPDU subframe header and MPDU to the A-MPDU
  AmpduSubframeHeader hdr = GetAmpduSubframeHeader (static_cast<uint16_t> (tmp->GetSize ()), isSingle);

  tmp->AddHeader (hdr);
  ampdu->AddAtEnd (tmp);
}

uint32_t
MpduAggregator::GetSizeIfAggregated (uint32_t mpduSize, uint32_t ampduSize)
{
  NS_LOG_FUNCTION (mpduSize << ampduSize);

  return ampduSize + CalculatePadding (ampduSize) + 4 + mpduSize;
}

uint32_t
MpduAggregator::GetMaxAmpduSize (Mac48Address recipient, uint8_t tid,
                                 WifiModulationClass modulation) const
{
  NS_LOG_FUNCTION (this << recipient << +tid << modulation);

  AcIndex ac = QosUtilsMapTidToAc (tid);

  // Find the A-MPDU max size configured on this device
  UintegerValue size;

  switch (ac)
    {
      case AC_BE:
        m_mac->GetAttribute ("BE_MaxAmpduSize", size);
        break;
      case AC_BK:
        m_mac->GetAttribute ("BK_MaxAmpduSize", size);
        break;
      case AC_VI:
        m_mac->GetAttribute ("VI_MaxAmpduSize", size);
        break;
      case AC_VO:
        m_mac->GetAttribute ("VO_MaxAmpduSize", size);
        break;
      default:
        NS_ABORT_MSG ("Unknown AC " << ac);
        return 0;
    }

  uint32_t maxAmpduSize = size.Get ();

  if (maxAmpduSize == 0)
    {
      NS_LOG_DEBUG ("A-MPDU Aggregation is disabled on this station for AC " << ac);
      return 0;
    }

  Ptr<WifiRemoteStationManager> stationManager = m_mac->GetWifiRemoteStationManager ();
  NS_ASSERT (stationManager);

  // Retrieve the Capabilities elements advertised by the recipient
  Ptr<const HeCapabilities> heCapabilities = stationManager->GetStationHeCapabilities (recipient);
  Ptr<const VhtCapabilities> vhtCapabilities = stationManager->GetStationVhtCapabilities (recipient);
  Ptr<const HtCapabilities> htCapabilities = stationManager->GetStationHtCapabilities (recipient);

  // Determine the constraint imposed by the recipient based on the PPDU
  // format used to transmit the A-MPDU
  if (modulation == WIFI_MOD_CLASS_HE)
    {
      NS_ABORT_MSG_IF (!heCapabilities, "HE Capabilities element not received");

      maxAmpduSize = std::min (maxAmpduSize, heCapabilities->GetMaxAmpduLength ());
    }
  else if (modulation == WIFI_MOD_CLASS_VHT)
    {
      NS_ABORT_MSG_IF (!vhtCapabilities, "VHT Capabilities element not received");

      maxAmpduSize = std::min (maxAmpduSize, vhtCapabilities->GetMaxAmpduLength ());
    }
  else if (modulation == WIFI_MOD_CLASS_HT)
    {
      NS_ABORT_MSG_IF (!htCapabilities, "HT Capabilities element not received");

      maxAmpduSize = std::min (maxAmpduSize, htCapabilities->GetMaxAmpduLength ());
    }
  else  // non-HT PPDU
    {
      NS_LOG_DEBUG ("A-MPDU aggregation is not available for non-HT PHYs");

      maxAmpduSize = 0;
    }

  return maxAmpduSize;
}

uint8_t
MpduAggregator::CalculatePadding (uint32_t ampduSize)
{
  return (4 - (ampduSize % 4 )) % 4;
}

AmpduSubframeHeader
MpduAggregator::GetAmpduSubframeHeader (uint16_t mpduSize, bool isSingle)
{
  AmpduSubframeHeader hdr;
  hdr.SetLength (mpduSize);
  if (isSingle)
    {
      hdr.SetEof (1);
    }
  return hdr;
}

std::vector<Ptr<WifiMacQueueItem>>
MpduAggregator::GetNextAmpdu (Ptr<const WifiMacQueueItem> mpdu, WifiTxVector txVector,
                              Time ppduDurationLimit) const
{
  NS_LOG_FUNCTION (this << *mpdu << ppduDurationLimit);
  std::vector<Ptr<WifiMacQueueItem>> mpduList;
  Mac48Address recipient = mpdu->GetHeader ().GetAddr1 ();

  NS_ASSERT (mpdu->GetHeader ().IsQosData () && !recipient.IsGroup ());

  uint8_t tid = GetTid (mpdu->GetPacket (), mpdu->GetHeader ());
  Ptr<QosTxop> qosTxop = m_mac->GetQosTxop (tid);
  NS_ASSERT (qosTxop != 0);

  WifiModulationClass modulation = txVector.GetModulationClass ();
  uint32_t maxAmpduSize = GetMaxAmpduSize (recipient, tid, modulation);

  if (maxAmpduSize == 0)
    {
      NS_LOG_DEBUG ("A-MPDU aggregation disabled");
      return mpduList;
    }

  //Have to make sure that the block ack agreement is established before sending an A-MPDU
  if (qosTxop->GetBaAgreementEstablished (recipient, tid))
    {
      /* here is performed MPDU aggregation */
      uint16_t startingSequenceNumber = qosTxop->GetBaStartingSequence (recipient, tid);
      Ptr<WifiMacQueueItem> nextMpdu;
      uint16_t maxMpdus = qosTxop->GetBaBufferSize (recipient, tid);
      uint32_t currentAmpduSize = 0;

      // check if the received MPDU meets the size and duration constraints
      if (qosTxop->GetLow ()->IsWithinSizeAndTimeLimits (mpdu, txVector, 0, ppduDurationLimit))
        {
          // MPDU can be aggregated
          nextMpdu = Copy (mpdu);
        }

      while (nextMpdu != 0)
        {
          /* if we are here, nextMpdu can be aggregated to the A-MPDU.
           * nextMpdu may be any of the following:
           * (a) an A-MSDU (with all the constituent MSDUs dequeued from
           *     the EDCA queue)
           * (b) an MSDU dequeued from the EDCA queue
           * (c) a retransmitted MSDU or A-MSDU dequeued from the BA Manager queue
           * (d) an MPDU that was aggregated in an A-MPDU which was not
           *     transmitted (e.g., because the RTS/CTS exchange failed)
           */

          currentAmpduSize = GetSizeIfAggregated (nextMpdu->GetSize (), currentAmpduSize);

          NS_LOG_DEBUG ("Adding packet with sequence number " << nextMpdu->GetHeader ().GetSequenceNumber ()
                        << " to A-MPDU, packet size = " << nextMpdu->GetSize ()
                        << ", A-MPDU size = " << currentAmpduSize);

          // Always use the Normal Ack policy (Implicit Block Ack), for now
          nextMpdu->GetHeader ().SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);

          mpduList.push_back (nextMpdu);

          // If allowed by the BA agreement, get the next MPDU
          nextMpdu = 0;

          Ptr<const WifiMacQueueItem> peekedMpdu;
          peekedMpdu = qosTxop->PeekNextFrame (tid, recipient);
          if (peekedMpdu != 0)
            {
              uint16_t currentSequenceNumber = peekedMpdu->GetHeader ().GetSequenceNumber ();

              if (IsInWindow (currentSequenceNumber, startingSequenceNumber, maxMpdus))
                {
                  // dequeue the frame if constraints on size and duration limit are met.
                  // Note that the dequeued MPDU differs from the peeked MPDU if A-MSDU
                  // aggregation is performed during the dequeue
                  NS_LOG_DEBUG ("Trying to aggregate another MPDU");
                  nextMpdu = qosTxop->DequeuePeekedFrame (peekedMpdu, txVector, true,
                                                                 currentAmpduSize, ppduDurationLimit);
                }
            }
        }
      if (mpduList.size () == 1)
        {
          // return an empty vector if it was not possible to aggregate at least two MPDUs
          mpduList.clear ();
        }
    }
  return mpduList;
}

std::vector<Ptr<WifiMacQueueItem>>
MpduAggregator::GetNextAmpdu (Ptr<WifiMacQueueItem> mpdu, WifiTxParameters& txParams,
                              Time availableTime, WifiMacQueueItem::QueueIteratorPair queueIt) const
{
  NS_LOG_FUNCTION (this << *mpdu << &txParams << availableTime);

  std::vector<Ptr<WifiMacQueueItem>> mpduList;

  Mac48Address recipient = mpdu->GetHeader ().GetAddr1 ();
  NS_ASSERT (mpdu->GetHeader ().IsQosData () && !recipient.IsBroadcast ());
  uint8_t tid = mpdu->GetHeader ().GetQosTid ();

  Ptr<QosTxop> qosTxop = m_mac->GetQosTxop (tid);
  NS_ASSERT (qosTxop != 0);

  //Have to make sure that the block ack agreement is established and A-MPDU is enabled
  if (qosTxop->GetBaAgreementEstablished (recipient, tid)
      && GetMaxAmpduSize (recipient, tid, txParams.m_txVector.GetModulationClass ()) > 0)
    {
      /* here is performed MPDU aggregation */
      Ptr<WifiMacQueueItem> nextMpdu = mpdu;

      while (nextMpdu != 0)
        {
          // if we are here, nextMpdu can be aggregated to the A-MPDU.
          NS_LOG_DEBUG ("Adding packet with sequence number " << nextMpdu->GetHeader ().GetSequenceNumber ()
                        << " to A-MPDU, packet size = " << nextMpdu->GetSize ()
                        << ", A-MPDU size = " << txParams.GetSize (recipient));

          mpduList.push_back (nextMpdu);

          // If allowed by the BA agreement, get the next MPDU
          nextMpdu = 0;

          Ptr<const WifiMacQueueItem> peekedMpdu;
          peekedMpdu = qosTxop->PeekNextMpdu (queueIt, tid, recipient);
          if (peekedMpdu != 0)
            {
              // PeekNextMpdu() does not return an MPDU that is beyond the transmit window
              NS_ASSERT (IsInWindow (peekedMpdu->GetHeader ().GetSequenceNumber (),
                                     qosTxop->GetBaStartingSequence (recipient, tid),
                                     qosTxop->GetBaBufferSize (recipient, tid)));

              // get the next MPDU to aggregate, provided that the constraints on size
              // and duration limit are met. Note that the returned MPDU differs from
              // the peeked MPDU if A-MSDU aggregation is enabled.
              NS_LOG_DEBUG ("Trying to aggregate another MPDU");
              nextMpdu = qosTxop->GetNextMpdu (peekedMpdu, txParams, availableTime, false, queueIt);
            }
        }

      if (mpduList.size () == 1)
        {
          // return an empty vector if it was not possible to aggregate at least two MPDUs
          mpduList.clear ();
        }
    }

  return mpduList;
}

} //namespace ns3
