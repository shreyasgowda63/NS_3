/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 NITK Surathkal
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
 * Author: Deepak Kumaraswamy <deepakkavoor99@gmail.com>
 *
 */

#include "ns3/tcp-prague.h"
#include "ns3/log.h"
#include "math.h"
#include "ns3/tcp-socket-state.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpPrague");

NS_OBJECT_ENSURE_REGISTERED (TcpPrague);

TypeId TcpPrague::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpPrague")
    .SetParent<TcpCongestionOps> ()
    .AddConstructor<TcpPrague> ()
    .SetGroupName ("Internet")
    .AddAttribute ("PragueShiftG",
                   "Parameter G for updating prague_alpha",
                   DoubleValue (0.0625),
                   MakeDoubleAccessor (&TcpPrague::m_g),
                   MakeDoubleChecker<double> (0, 1))
    .AddAttribute ("PragueAlphaOnInit",
                   "Initial alpha value",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&TcpPrague::SetPragueAlpha),
                   MakeDoubleChecker<double> (0, 1))
    .AddAttribute ("UseEct0",
                   "Use ECT(0) for ECN codepoint, if false use ECT(1)",
                   BooleanValue (false),
                   MakeBooleanAccessor (&TcpPrague::m_useEct0),
                   MakeBooleanChecker ())
    .AddAttribute ("RttTarget",
                   "Target RTT to achieve",
                   TimeValue (MilliSeconds (15)),
                   MakeTimeAccessor (&TcpPrague::GetDefaultRttTarget,
                                     &TcpPrague::SetDefaultRttTarget),
                   MakeTimeChecker ())
    .AddAttribute ("RttTransitionDelay",
                   "Number of rounds post Slow Start after which RTT independence is enabled",
                   UintegerValue (100),
                   MakeUintegerAccessor (&TcpPrague::m_rttTransitionDelay),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("RttScalingMode", "RTT Independence Scaling Heuristic",
                   EnumValue (TcpPrague::RTT_CONTROL_NONE),
                   MakeEnumAccessor (&TcpPrague::SetRttScalingMode),
                   MakeEnumChecker (TcpPrague::RTT_CONTROL_NONE, "None",
                                    TcpPrague::RTT_CONTROL_RATE, "Rate",
                                    TcpPrague::RTT_CONTROL_SCALABLE, "Scalable",
                                    TcpPrague::RTT_CONTROL_ADDITIVE, "Additive"))
  ;
  return tid;
}

std::string TcpPrague::GetName () const
{
  return "TcpPrague";
}

TcpPrague::TcpPrague ()
  : TcpCongestionOps ()
{
  NS_LOG_FUNCTION (this);
  m_ackedBytesEcn = 0;
  m_ackedBytesTotal = 0;
  m_priorRcvNxt = SequenceNumber32 (0);
  m_priorRcvNxtFlag = false;
  m_nextSeq = SequenceNumber32 (0);
  m_nextSeqFlag = false;
  m_ceState = false;
  m_delayedAckReserved = false;
}

TcpPrague::TcpPrague (const TcpPrague& sock)
  : TcpCongestionOps (sock),
    m_ackedBytesEcn (sock.m_ackedBytesEcn),
    m_ackedBytesTotal (sock.m_ackedBytesTotal),
    m_priorRcvNxt (sock.m_priorRcvNxt),
    m_priorRcvNxtFlag (sock.m_priorRcvNxtFlag),
    m_alpha (sock.m_alpha),
    m_nextSeq (sock.m_nextSeq),
    m_nextSeqFlag (sock.m_nextSeqFlag),
    m_ceState (sock.m_ceState),
    m_delayedAckReserved (sock.m_delayedAckReserved),
    m_g (sock.m_g),
    m_useEct0 (sock.m_useEct0)
{
  NS_LOG_FUNCTION (this);
}

TcpPrague::~TcpPrague (void)
{
  NS_LOG_FUNCTION (this);
}

Ptr<TcpCongestionOps> TcpPrague::Fork (void)
{
  NS_LOG_FUNCTION (this);
  return CopyObject<TcpPrague> (this);
}


void
TcpPrague::Init (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);
  NS_LOG_INFO (this << "Enabling DctcpEcn for TCP Prague");
  tcb->m_useEcn = TcpSocketState::On;
  tcb->m_ecnMode = TcpSocketState::DctcpEcn;
  tcb->m_ectCodePoint = m_useEct0 ? TcpSocketState::Ect0 : TcpSocketState::Ect1;
  tcb->m_pacing = true;
  tcb->m_pacingCaRatio = 100;

  // related to rtt independence
  m_round = 0;
  m_alphaStamp = Simulator::Now ();
  NewRound (tcb);
}

uint32_t
TcpPrague::GetSsThresh (Ptr<const TcpSocketState> state,
                        uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this << state << bytesInFlight);

  return state->m_ssThresh;
}

void
TcpPrague::ReduceCwnd (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);

  uint32_t cwnd_segs = tcb->m_cWnd / tcb->m_segmentSize;
  double_t reduction = m_alpha * cwnd_segs / 2.0;
  m_cWndCnt -= reduction;
}

uint32_t
TcpPrague::SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  uint32_t cwnd = std::min (((uint32_t)tcb->m_cWnd + (segmentsAcked * tcb->m_segmentSize)), (uint32_t)tcb->m_ssThresh);
  segmentsAcked -= ((cwnd - tcb->m_cWnd) / tcb->m_segmentSize);
  tcb->m_cWnd = cwnd;
  NS_LOG_INFO ("In SlowStart, updated to cwnd " << tcb->m_cWnd << " ssthresh " << tcb->m_ssThresh);
  return segmentsAcked;
}

void
TcpPrague::UpdateCwnd (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  bool updateCwnd = true;
  if (m_inLoss)
    {
      updateCwnd = false;
    }
  if (updateCwnd)
    {
      uint32_t acked = segmentsAcked;
      if (tcb->m_cWnd < tcb->m_ssThresh)
        {
          // Slow Start similar to ns3::TcpLinuxReno
          acked = SlowStart (tcb, segmentsAcked);
          if (!acked)
            {
              CwndChanged (tcb);
              return;
            }
        }
      // Congestion Avoidance
      uint32_t cwnd_segs = tcb->m_cWnd / tcb->m_segmentSize;
      double_t increase = 1.0 * acked * m_aiAckIncrease / cwnd_segs;
      m_cWndCnt += increase;
    }

  if (m_cWndCnt <= -1)
    {
      m_cWndCnt++;
      tcb->m_cWnd -= tcb->m_segmentSize;
      if (tcb->m_cWnd < 2 * tcb->m_segmentSize)
        {
          tcb->m_cWnd = 2 * tcb->m_segmentSize;
          m_cWndCnt = 0;
        }
      tcb->m_ssThresh = tcb->m_cWnd;
      CwndChanged (tcb);
    }
  else if (m_cWndCnt >= 1)
    {
      m_cWndCnt--;
      tcb->m_cWnd += tcb->m_segmentSize;
      CwndChanged (tcb);
    }
}

void
TcpPrague::UpdateAlpha (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (m_sawCE)
    {
      if (m_nextSeqFlag == false)
        {
          m_nextSeq = tcb->m_nextTxSequence;
          m_nextSeqFlag = true;
        }
      if (tcb->m_lastAckedSeq >= m_nextSeq)
        {
          double bytesEcn = 0.0;
          if (m_ackedBytesTotal >  0)
            {
              bytesEcn = static_cast<double> (m_ackedBytesEcn * 1.0 / m_ackedBytesTotal);
            }
          m_alpha = (1.0 - m_g) * m_alpha + m_g * bytesEcn;
          NS_LOG_INFO (this << "bytesEcn " << bytesEcn << ", m_alpha " << m_alpha);

          m_alphaStamp = Simulator::Now ();
          Reset (tcb);
        }
    }
  NewRound (tcb);
}

void
TcpPrague::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time &rtt)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked << rtt);

  // This method is similar to prague_cong_control() in Linux

  m_ackedBytesTotal += segmentsAcked * tcb->m_segmentSize;
  if (tcb->m_ecnState == TcpSocketState::ECN_ECE_RCVD)
    {
      m_sawCE = true;
      m_ackedBytesEcn += segmentsAcked * tcb->m_segmentSize;
    }

  UpdateCwnd (tcb, segmentsAcked);
  if (ShouldUpdateEwma (tcb))
    {
      UpdateAlpha (tcb, segmentsAcked);
    }
}

void
TcpPrague::NewRound (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);

  if (tcb->m_cWnd >= tcb->m_ssThresh)
    {
      ++m_round;
    }
  AiAckIncrease (tcb);
}

void
TcpPrague::SetPragueAlpha (double alpha)
{
  NS_LOG_FUNCTION (this << alpha);

  m_alpha = alpha;
}

void
TcpPrague::CwndChanged (Ptr<TcpSocketState> tcb)
{
  // This method is similar to prague_cwnd_changed() in Linux
  NS_LOG_FUNCTION (this << tcb);

  AiAckIncrease (tcb);
}

void
TcpPrague::Reset (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);

  m_nextSeq = tcb->m_nextTxSequence;
  m_ackedBytesEcn = 0;
  m_ackedBytesTotal = 0;
}

void
TcpPrague::EnterLoss (Ptr<TcpSocketState> tcb)
{
  // This method is similar to prague_enter_loss() in Linux
  NS_LOG_FUNCTION (this << tcb);

  m_cWndCnt -= (1.0 * tcb->m_cWnd / tcb->m_segmentSize) / 2;
  m_inLoss = true;
  CwndChanged (tcb);
}

void
TcpPrague::CeState0to1 (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);

  if (!m_ceState && m_delayedAckReserved && m_priorRcvNxtFlag)
    {
      SequenceNumber32 tmpRcvNxt;
      /* Save current NextRxSequence. */
      tmpRcvNxt = tcb->m_rxBuffer->NextRxSequence ();

      /* Generate previous ACK without ECE */
      tcb->m_rxBuffer->SetNextRxSequence (m_priorRcvNxt);
      tcb->m_sendEmptyPacketCallback (TcpHeader::ACK);

      /* Recover current RcvNxt. */
      tcb->m_rxBuffer->SetNextRxSequence (tmpRcvNxt);
    }

  if (m_priorRcvNxtFlag == false)
    {
      m_priorRcvNxtFlag = true;
    }
  m_priorRcvNxt = tcb->m_rxBuffer->NextRxSequence ();
  m_ceState = true;
  tcb->m_ecnState = TcpSocketState::ECN_CE_RCVD;
}

void
TcpPrague::CeState1to0 (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);

  if (m_ceState && m_delayedAckReserved && m_priorRcvNxtFlag)
    {
      SequenceNumber32 tmpRcvNxt;
      /* Save current NextRxSequence. */
      tmpRcvNxt = tcb->m_rxBuffer->NextRxSequence ();

      /* Generate previous ACK with ECE */
      tcb->m_rxBuffer->SetNextRxSequence (m_priorRcvNxt);
      tcb->m_sendEmptyPacketCallback (TcpHeader::ACK | TcpHeader::ECE);

      /* Recover current RcvNxt. */
      tcb->m_rxBuffer->SetNextRxSequence (tmpRcvNxt);
    }

  if (m_priorRcvNxtFlag == false)
    {
      m_priorRcvNxtFlag = true;
    }
  m_priorRcvNxt = tcb->m_rxBuffer->NextRxSequence ();
  m_ceState = false;

  if (tcb->m_ecnState == TcpSocketState::ECN_CE_RCVD || tcb->m_ecnState == TcpSocketState::ECN_SENDING_ECE)
    {
      tcb->m_ecnState = TcpSocketState::ECN_IDLE;
    }
}

void
TcpPrague::UpdateAckReserved (Ptr<TcpSocketState> tcb,
                              const TcpSocketState::TcpCAEvent_t event)
{
  NS_LOG_FUNCTION (this << tcb << event);

  switch (event)
    {
      case TcpSocketState::CA_EVENT_DELAYED_ACK:
        if (!m_delayedAckReserved)
          {
            m_delayedAckReserved = true;
          }
        break;
      case TcpSocketState::CA_EVENT_NON_DELAYED_ACK:
        if (m_delayedAckReserved)
          {
            m_delayedAckReserved = false;
          }
        break;
      default:
        /* Don't care for the rest. */
        break;
    }
}

void
TcpPrague::CwndEvent (Ptr<TcpSocketState> tcb,
                      const TcpSocketState::TcpCAEvent_t event)
{
  NS_LOG_FUNCTION (this << tcb << event);

  switch (event)
    {
      case TcpSocketState::CA_EVENT_ECN_IS_CE:
        CeState0to1 (tcb);
        break;
      case TcpSocketState::CA_EVENT_ECN_NO_CE:
        CeState1to0 (tcb);
        break;
      case TcpSocketState::CA_EVENT_DELAYED_ACK:
      case TcpSocketState::CA_EVENT_NON_DELAYED_ACK:
        UpdateAckReserved (tcb, event);
        break;
      case TcpSocketState::CA_RECOVERY:
        EnterLoss (tcb);
        break;
      case TcpSocketState::CA_OPEN:
        m_inLoss = false;
        break;
      default:
        /* Don't care for the rest. */
        break;
    }
}

void
TcpPrague::SetDefaultRttTarget (Time targetRtt)
{
  m_rttTarget = targetRtt;
}

void
TcpPrague::SetRttTransitionDelay (uint32_t rounds)
{
  m_rttTransitionDelay = rounds;
}

void
TcpPrague::SetRttScalingMode (TcpPrague::RttScalingMode_t scalingMode)
{
  m_rttScalingMode = scalingMode;
}

bool
TcpPrague::IsRttIndependent (Ptr<TcpSocketState> tcb)
{
  // This method is similar to prague_is_rtt_indep in Linux
  NS_LOG_FUNCTION (this << tcb);

  if (m_rttScalingMode != RttScalingMode_t::RTT_CONTROL_NONE
      && !(tcb->m_cWnd < tcb->m_ssThresh) && m_round >= m_rttTransitionDelay)
    {
      return true;
    }
  return false;
}

double_t
TcpPrague::GetCwndCnt (void)
{
  return m_cWndCnt;
}

Time
TcpPrague::GetDefaultRttTarget (void) const
{
  return m_rttTarget;
}

Time
TcpPrague::GetTargetRtt (Ptr<TcpSocketState> tcb)
{
  // This method is similar to prague_target_rtt in Linux
  NS_LOG_FUNCTION (this << tcb);

  /* Referred from TcpOptionTS::NowToTsValue */
  Time target = m_rttTarget;
  if (m_rttScalingMode != RttScalingMode_t::RTT_CONTROL_ADDITIVE)
    {
      return target;
    }
  Time lastRtt = tcb->m_lastRtt;
  target += lastRtt;
  return target;
}

bool
TcpPrague::ShouldUpdateEwma (Ptr<TcpSocketState> tcb)
{
  // This method is similar to prague_should_update_ewma in Linux
  NS_LOG_FUNCTION (this << tcb);

  if (m_rttScalingMode == RttScalingMode_t::RTT_CONTROL_NONE)
    {
      return true;
    }
  bool e2eRttElapsed = !(tcb->m_nextTxSequence < m_nextSeq);

  if (!e2eRttElapsed)
    {
      return false;
    }

  // Instead of Linux-like tcp_mstamp, use simulator time
  bool targetRttElapsed = (GetTargetRtt (tcb).GetSeconds () <= std::max (Simulator::Now ().GetSeconds () - m_alphaStamp.GetSeconds (), 0.0));
  return !IsRttIndependent (tcb) || targetRttElapsed;
}

void
TcpPrague::AiAckIncrease (Ptr<TcpSocketState> tcb)
{
  // This method is similar to prague_ai_ack_increase in Linux
  NS_LOG_FUNCTION (this << tcb);

  Time lastRtt = tcb->m_lastRtt;
  Time maxScaledRtt = MilliSeconds (100);
  if (m_rttScalingMode == RttScalingMode_t::RTT_CONTROL_NONE || m_round < m_rttTransitionDelay || lastRtt > maxScaledRtt)
    {
      m_aiAckIncrease = 1;
      return;
    }

  // Use other heuristics
  if (m_rttScalingMode == RttScalingMode_t::RTT_CONTROL_RATE || m_rttScalingMode == RttScalingMode_t::RTT_CONTROL_ADDITIVE)
    {
      // Linux would call prague_rate_scaled_ai_ack_increase
      Time target = GetTargetRtt (tcb);
      if (lastRtt.GetSeconds () > target.GetSeconds ())
        {
          m_aiAckIncrease = 1;
          return;
        }
      m_aiAckIncrease = 1.0 * lastRtt.GetSeconds () * lastRtt.GetSeconds () / (target.GetSeconds () * target.GetSeconds ());
    }
  else
    {
      // Linux would call prague_scalable_ai_ack_increase
      Time R0 = Seconds (0.016), R1 = Seconds (0.0015); // 16ms and 1.5ms
      double_t increase = R0.GetSeconds () / 8 + std::min (std::max (lastRtt.GetSeconds () - R1.GetSeconds (), 0.0), R0.GetSeconds ());
      increase = increase * lastRtt.GetSeconds () / R0.GetSeconds () * R0.GetSeconds ();
      m_aiAckIncrease = increase;
    }
}


} // namespace ns3
