/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/error-rate-model.h"
#include "ns3/frame-capture-model.h"
#include "ns3/preamble-detection-model.h"
#include "ns3/yans-wifi-phy.h"
#include "yans-wifi-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("YansWifiHelper");

YansWifiChannelHelper::YansWifiChannelHelper ()
{
}

YansWifiChannelHelper
YansWifiChannelHelper::Default (void)
{
  YansWifiChannelHelper helper;
  helper.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  helper.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
  return helper;
}

Ptr<YansWifiChannel>
YansWifiChannelHelper::Create (void) const
{
  Ptr<YansWifiChannel> channel = CreateObject<YansWifiChannel> ();
  Ptr<PropagationLossModel> prev = 0;
  for (std::vector<ObjectFactory>::const_iterator i = m_propagationLoss.begin (); i != m_propagationLoss.end (); ++i)
    {
      Ptr<PropagationLossModel> cur = (*i).Create<PropagationLossModel> ();
      if (prev != 0)
        {
          prev->SetNext (cur);
        }
      if (m_propagationLoss.begin () == i)
        {
          channel->SetPropagationLossModel (cur);
        }
      prev = cur;
    }
  Ptr<PropagationDelayModel> delay = m_propagationDelay.Create<PropagationDelayModel> ();
  channel->SetPropagationDelayModel (delay);
  return channel;
}

int64_t
YansWifiChannelHelper::AssignStreams (Ptr<YansWifiChannel> c, int64_t stream)
{
  return c->AssignStreams (stream);
}

YansWifiPhyHelper::YansWifiPhyHelper ()
  : m_channel (0)
{
  m_phy.SetTypeId ("ns3::YansWifiPhy");
  SetErrorRateModel ("ns3::TableBasedErrorRateModel");
}

void
YansWifiPhyHelper::SetChannel (Ptr<YansWifiChannel> channel)
{
  m_channel = channel;
}

void
YansWifiPhyHelper::SetChannel (std::string channelName)
{
  Ptr<YansWifiChannel> channel = Names::Find<YansWifiChannel> (channelName);
  m_channel = channel;
}

Ptr<WifiPhy>
YansWifiPhyHelper::Create (Ptr<Node> node, Ptr<NetDevice> device) const
{
  Ptr<YansWifiPhy> phy = m_phy.Create<YansWifiPhy> ();
  Ptr<ErrorRateModel> error = m_errorRateModel.Create<ErrorRateModel> ();
  phy->SetErrorRateModel (error);
  if (m_frameCaptureModel.IsTypeIdSet ())
    {
      Ptr<FrameCaptureModel> capture = m_frameCaptureModel.Create<FrameCaptureModel> ();
      phy->SetFrameCaptureModel (capture);
    }
  if (m_preambleDetectionModel.IsTypeIdSet ())
    {
      Ptr<PreambleDetectionModel> capture = m_preambleDetectionModel.Create<PreambleDetectionModel> ();
      phy->SetPreambleDetectionModel (capture);
    }
  phy->SetChannel (m_channel);
  phy->SetDevice (device);
  return phy;
}

} //namespace ns3
