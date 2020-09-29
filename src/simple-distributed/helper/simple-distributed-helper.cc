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
 */

#include "simple-distributed-helper.h"

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/object-factory.h"
#include "ns3/queue.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/simple-distributed-net-device.h"
#include "ns3/simple-distributed-channel.h"
#include "ns3/config.h"
#include "ns3/packet.h"
#include "ns3/names.h"
#include "ns3/boolean.h"
#include "ns3/trace-helper.h"

#ifdef NS3_MPI
#include "ns3/mpi-interface.h"
#include "ns3/mpi-receiver.h"
#endif

#include <string>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimpleDistributedHelper");

SimpleDistributedHelper::SimpleDistributedHelper ()
{
  m_queueFactory.SetTypeId ("ns3::DropTailQueue<Packet>");
  m_deviceFactory.SetTypeId ("ns3::SimpleDistributedNetDevice");
  m_channelFactory.SetTypeId ("ns3::SimpleDistributedChannel");
  m_pointToPointMode = false;
}

void 
SimpleDistributedHelper::SetQueue (std::string type,
                                 std::string n1, const AttributeValue &v1,
                                 std::string n2, const AttributeValue &v2,
                                 std::string n3, const AttributeValue &v3,
                                 std::string n4, const AttributeValue &v4)
{
  QueueBase::AppendItemTypeIfNotPresent (type, "Packet");

  m_queueFactory.SetTypeId (type);
  m_queueFactory.Set (n1, v1);
  m_queueFactory.Set (n2, v2);
  m_queueFactory.Set (n3, v3);
  m_queueFactory.Set (n4, v4);
}

void
SimpleDistributedHelper::SetChannel (std::string type,
                                   std::string n1, const AttributeValue &v1,
                                   std::string n2, const AttributeValue &v2,
                                   std::string n3, const AttributeValue &v3,
                                   std::string n4, const AttributeValue &v4)
{
  m_channelFactory.SetTypeId (type);
  m_channelFactory.Set (n1, v1);
  m_channelFactory.Set (n2, v2);
  m_channelFactory.Set (n3, v3);
  m_channelFactory.Set (n4, v4);
}

void
SimpleDistributedHelper::SetDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  m_deviceFactory.Set (n1, v1);
}

void
SimpleDistributedHelper::SetChannelAttribute (std::string n1, const AttributeValue &v1)
{
  m_channelFactory.Set (n1, v1);
}

void
SimpleDistributedHelper::SetNetDevicePointToPointMode (bool pointToPointMode)
{
  m_pointToPointMode = pointToPointMode;
}

NetDeviceContainer
SimpleDistributedHelper::Install (Ptr<Node> node) const
{
  Ptr<SimpleDistributedChannel> channel = m_channelFactory.Create<SimpleDistributedChannel> ();
  return Install (node, channel);
}

NetDeviceContainer
SimpleDistributedHelper::Install (Ptr<Node> node, Ptr<SimpleDistributedChannel> channel) const
{
  return NetDeviceContainer (InstallPriv (node, channel));
}

NetDeviceContainer 
SimpleDistributedHelper::Install (const NodeContainer &c) const
{
  Ptr<SimpleDistributedChannel> channel = m_channelFactory.Create<SimpleDistributedChannel> ();

  return Install (c, channel);
}

NetDeviceContainer 
SimpleDistributedHelper::Install (const NodeContainer &c, Ptr<SimpleDistributedChannel> channel) const
{
  NetDeviceContainer devs;

  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); i++)
    {
      devs.Add (InstallPriv (*i, channel));
    }

  return devs;
}

Ptr<NetDevice>
SimpleDistributedHelper::InstallPriv (Ptr<Node> node, Ptr<SimpleDistributedChannel> channel) const
{
  Ptr<SimpleDistributedNetDevice> device = m_deviceFactory.Create<SimpleDistributedNetDevice> ();
  device->SetAttribute ("PointToPointMode", BooleanValue (m_pointToPointMode));
  device->SetAddress (Mac48Address::Allocate ());
  node->AddDevice (device);
  device->SetChannel (channel);
  Ptr<Queue<Packet> > queue = m_queueFactory.Create<Queue<Packet> > ();
  device->SetQueue (queue);

  Ptr<NetDeviceQueueInterface> ndqi = CreateObject<NetDeviceQueueInterface> ();
  ndqi->GetTxQueue (0)->ConnectQueueTraces (queue);
  device->AggregateObject (ndqi);

#ifdef NS3_MPI
  auto mpiReceiver = CreateObject<MpiReceiver> ();
  mpiReceiver->SetReceiveCallback (MakeCallback (&SimpleDistributedNetDevice::ReceiveRemote, device));
  device->AggregateObject (mpiReceiver);

  // If this channel spans systems need to bound lookahead.
  if (node->GetSystemId () != Simulator::GetSystemId ())
    {
      Simulator::BoundLookahead (channel->GetMinimumDelay ());
    }
#endif
  
  return device;
}

void 
SimpleDistributedHelper::EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, bool promiscuous, bool explicitFilename)
{
  //
  // All of the Pcap enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type SimpleDistributedNetDevice.
  //
  Ptr<SimpleDistributedNetDevice> device = nd->GetObject<SimpleDistributedNetDevice> ();
  if (device == 0)
    {
      NS_LOG_INFO ("SimpleDistributedHelper::EnablePcapInternal: Device " << device << " not of type ns3::SimpleDistributedNetDevice");
      return;
    }

  Ptr<Node> node = device -> GetNode ();

  uint32_t systemId = Simulator::GetSystemId ();
  
  if (node)
    {
      // Only enable capturing on node's owned by this rank.
      if (node -> GetSystemId () == systemId)
        {
          PcapHelper pcapHelper;

          std::string filename;
          if (explicitFilename)
            {
              filename = prefix;
            }
          else
            {
              filename = pcapHelper.GetFilenameFromDevice (prefix, device);
            }
          
          Ptr<PcapFileWrapper> file = pcapHelper.CreateFile (filename, std::ios::out, 
                                                             PcapHelper::DLT_EN10MB);
          pcapHelper.HookDefaultSink<SimpleDistributedNetDevice> (device, "PromiscSniffer", file);
        }
    }
  else
    {
      NS_LOG_INFO ("SimpleDistributedHelper::EnablePcapInternal: Node has not been assigned to net device");
      return;
    }

}

void 
SimpleDistributedHelper::EnableAsciiInternal (
  Ptr<OutputStreamWrapper> stream, 
  std::string prefix, 
  Ptr<NetDevice> nd,
  bool explicitFilename)
{
  //
  // All of the ascii enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type SimpleDistributedNetDevice.
  //
  Ptr<SimpleDistributedNetDevice> device = nd->GetObject<SimpleDistributedNetDevice> ();
  if (device == 0)
    {
      NS_LOG_INFO ("SimpleDistributedHelper::EnableAsciiInternal(): Device " << device << 
                   " not of type ns3::SimpleDistributedNetDevice");
      return;
    }

  Ptr<Node> node = device -> GetNode ();

  uint32_t systemId = Simulator::GetSystemId ();
  
  if (node)
    {
      // Only enable capturing on node's owned by this rank.
      if (node -> GetSystemId () == systemId)
        {

          //
          // Our default trace sinks are going to use packet printing, so we have to 
          // make sure that is turned on.
          //
          Packet::EnablePrinting ();
          
          //
          // If we are not provided an OutputStreamWrapper, we are expected to create 
          // one using the usual trace filename conventions and do a Hook*WithoutContext
          // since there will be one file per context and therefore the context would
          // be redundant.
          //
          if (stream == 0)
            {
              //
              // Set up an output stream object to deal with private ofstream copy 
              // constructor and lifetime issues.  Let the helper decide the actual
              // name of the file given the prefix.
              //
              AsciiTraceHelper asciiTraceHelper;
              
              std::string filename;
              if (explicitFilename)
                {
                  filename = prefix;
                }
              else
                {
                  filename = asciiTraceHelper.GetFilenameFromDevice (prefix, device);
                }
              
              Ptr<OutputStreamWrapper> theStream = asciiTraceHelper.CreateFileStream (filename);
              
              //
              // The MacRx trace source provides our "r" event.
              //
              asciiTraceHelper.HookDefaultReceiveSinkWithoutContext<SimpleDistributedNetDevice> (device, "MacRx", theStream);
              
              //
              // The "+", '-', and 'd' events are driven by trace sources actually in the
              // transmit queue.
              //
              Ptr<Queue<Packet> > queue = device->GetQueue ();
              asciiTraceHelper.HookDefaultEnqueueSinkWithoutContext<Queue<Packet> > (queue, "Enqueue", theStream);
              asciiTraceHelper.HookDefaultDropSinkWithoutContext<Queue<Packet> > (queue, "Drop", theStream);
              asciiTraceHelper.HookDefaultDequeueSinkWithoutContext<Queue<Packet> > (queue, "Dequeue", theStream);
              
              // PhyRxDrop trace source for "d" event
              asciiTraceHelper.HookDefaultDropSinkWithoutContext<SimpleDistributedNetDevice> (device, "PhyRxDrop", theStream);
              
              return;
            }
          
          //
          // If we are provided an OutputStreamWrapper, we are expected to use it, and
          // to providd a context.  We are free to come up with our own context if we
          // want, and use the AsciiTraceHelper Hook*WithContext functions, but for 
          // compatibility and simplicity, we just use Config::Connect and let it deal
          // with the context.
          //
          // Note that we are going to use the default trace sinks provided by the 
          // ascii trace helper.  There is actually no AsciiTraceHelper in sight here,
          // but the default trace sinks are actually publicly available static 
          // functions that are always there waiting for just such a case.
          //
          uint32_t nodeid = nd->GetNode ()->GetId ();
          uint32_t deviceid = nd->GetIfIndex ();
          std::ostringstream oss;
          
          oss << "/NodeList/" << nd->GetNode ()->GetId () << "/DeviceList/" << deviceid << "/$ns3::SimpleDistributedNetDevice/MacRx";
          Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultReceiveSinkWithContext, stream));
          
          oss.str ("");
          oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::SimpleDistributedNetDevice/TxQueue/Enqueue";
          Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultEnqueueSinkWithContext, stream));
          
          oss.str ("");
          oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::SimpleDistributedNetDevice/TxQueue/Dequeue";
          Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDequeueSinkWithContext, stream));
          
          oss.str ("");
          oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::SimpleDistributedNetDevice/TxQueue/Drop";
          Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));
          
          oss.str ("");
          oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::SimpleDistributedNetDevice/PhyRxDrop";
          Config::Connect (oss.str (), MakeBoundCallback (&AsciiTraceHelper::DefaultDropSinkWithContext, stream));
        }
    }
}
      
} // namespace ns3
