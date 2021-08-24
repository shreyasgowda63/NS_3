Example Module Documentation
----------------------------

.. include:: replace.txt
.. highlight:: cpp

.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

Model Description
*****************

The source code for the module lives in the directory ``multicast-flow-monitor``.

The Multicast Flow Monitor module goal is to provide a flexible system to measure the
performance of multicast network protocols. The existing flow monitor is designed specifically for
unicast transmissions and makes assumptions that traffic transmitting across the network is unicast traffic. 
I have removed this assumption by forcing the user to define the multicast flows which they wish to monitor (more below).
The module uses probes, installed in network nodes, to track the packets exchanged by the nodes, and it will measure
a number of parameters. Packets are divided according to the flow they belong
to, where each flow is defined according to the probe's characteristics (e.g.,
for IP, a flow is defined as the packets with the same {protocol, source (IP, port),
destination (IP, port)} tuple.

The assumption is that the user will access the probes directly to request specific stats about each flow.
Future work will be performed to serialize to CSV format.

Design
======

The Multicast Flow Monitor module is designed in a modular way. It can be extended by subclassing
``ns3::MulticastFlowProbe`` and ``ns3::MulticastFlowClassifier``.
Typically, a subclass of ``ns3::MulticastFlowProbe`` works by listening to the appropriate
class Traces, and then uses its own ``ns3::FlowClassifier`` subclass to classify
the packets passing though each node.

Each Probe can try to listen to other classes traces (e.g., ``ns3::Ipv4FlowProbe``
will try to use any ``ns3::NetDevice`` trace named ``TxQueue/Drop``) but this
is something that the user should not rely into blindly, because the trace is not
guaranteed to be in every type of ``ns3::NetDevice``. As an example,
``CsmaNetDevice`` and ``PointToPointNetDevice`` have a ``TxQueue/Drop`` trace, while
``WiFiNetDevice`` does not.

Scope and Limitations
=====================

At the moment, probes and classifiers are available only for IPv4 multicast.

IPv4 proves will classify packets in five points:

* When a packet is sent (SendOutgoing IPv4 traces)
* When a packet is forwarded (MulticastForward IPv4 traces)
* When a packet is received (LocalDeliver IPv4 traces)
* When a packet is dropped in general (Drop IPv6 traces)

Only UDP packets are supported for Multicast right now, so there should not be any retransmissions. Any additional 
transport protocols

A Tag will be added to the packet (``ns3::Ipv[4,6]FlowProbeTag``). The tag will carry
basic packet's data, useful for the packet's classification.

It must be underlined that only L4 UDP packets are classified for multicast since no other multicast protocols exist in ns3.

The data collected for each flow by group destination node are:

* timeFirstTxPacket: when the first packet in the flow was transmitted;
* timeLastTxPacket: when the last packet in the flow was transmitted;
* timeFirstRxPacket: when the first packet in the flow was received by an end node;
* timeLastRxPacket: when the last packet in the flow was received;
* delaySum: the sum of all end-to-end delays for all received packets of the flow;
* jitterSum: the sum of all end-to-end delay jitter (delay variation) values for all received packets of the flow, as defined in :rfc:`3393`;
* packetDelay: The delay for each packet in a flow;
* dupsDropped: The number of duplicate packets dropped;
* timesForwarded: The number of times the packet has been forwarded;
* txBytes, txPackets: total number of transmitted bytes / packets for the flow;
* rxBytes, rxPackets: total number of received bytes / packets for the flow;
* lostPackets: total number of packets that are assumed to be lost (not reported over 10 seconds);
* timesForwarded: the number of times a packet has been reportedly forwarded;
* delayHistogram, jitterHistogram, packetSizeHistogram: histogram versions for the delay, jitter, and packet sizes, respectively;
* packetsDropped, bytesDropped: the number of lost packets and bytes, divided according to the loss reason code (defined in the probe).

The data collected for each packet are:
* numHops: The number of hops a packet has encountered;

It is worth pointing out that the probes measure the packet bytes including IP headers.
The L2 headers are not included in the measure.

Future Work will endeavor to provide a method to write these stats in CSV format upon request.

The "lost"packets problem
#########################
The lost packets problem present in the unicast FlowMonitor exists here as well. Packets not received after 10 seconds are assumed to be lost. When creating a scenario and running a simulation, the user should allow for longer than ten seconds (or some user specified value) to pass after last transmission for increased certainty regarding packet loss, i.e., stop the traffic application ten seconds before simulation stop time.



References
==========

Forthcoming

Usage
*****

The module usage is extremely simple. The helper will take care of about everything.

The typical use is::

  MulticastFlowMonitorHelper flowmon;
  Ptr<MulticastFlowMonitor> monitor = flowmon.InstallAll (mcast_groups);

  Simulator::Stop (Seconds (endTime + 1));
  Simulator::Run ();

  Ptr<Ipv4MulticastFlowClassifier> classifier = DynamicCast<Ipv4MulticastFlowClassifier> (flowmon.GetClassifier ());
  MulticastFlowMonitor::MulticastFlowStatsContainer stats = monitor->GetMulticastFlowStats ();

From there the stats object can be accessed::

   std::ofstream flow_out ("FlowStats.csv");
  flow_out << "flow_id,flow_src,flow_dst,flow_dst_addr,avg_hop_count,tx_packets," <<
    "tx_bytes,tx_rate_kbps,rx_packets,rx_bytes,rx_tput_kbps," <<
    "dups_dropped,times_fwded,lost_packets,avg_delay\n";

  std::ofstream pack_out ("PacketStats.csv");
  pack_out << "flow_id,flow_src,flow_dst,flow_dst_addr,packetSeq,delay,hop_count\n";

  for (std::map<MulticastFlowId, MulticastFlowMonitor::MulticastFlowStats>::iterator i = stats.begin (); i != stats.end (); ++i)
    {
      Ipv4MulticastFlowClassifier::FiveTuple t = classifier->FindMulticastFlow (i->first);
      // std::vector<uint32_t> groupNodeIds
      std::vector<uint32_t>::const_iterator it;
      for (it = i->second.groupNodeIds.begin (); it != i->second.groupNodeIds.end (); it++)
        {
          uint32_t node = *(it);
          flow_out << i->first << ","; //flow_id
          flow_out << t.sourceAddress << ","; // flow_src node
          flow_out << node << ","; // flow_dst node
          flow_out << t.destinationAddress << ","; //flow_dst addr

          std::map<uint32_t, uint32_t>::iterator h_it;
          uint32_t tot_hops = 0;
          for (h_it = i->second.numHops[node].begin (); h_it != i->second.numHops[node].end (); h_it++)
            {
              tot_hops = tot_hops + h_it->second;
            }
          flow_out << static_cast<double> (tot_hops) / static_cast<double> (i->second.numHops[node].size ()) << ","; // avg_hop_count
          flow_out << i->second.txPackets << ","; // num_tx_packets
          flow_out << i->second.txBytes << ","; // tx_bytes
          flow_out << i->second.txBytes * 8.0 / (endTime - start_time) / 1000.0 << ","; //tx_throughput_kbps
          flow_out << i->second.rxPackets[node] << ","; //num_rx_packets
          flow_out << i->second.rxBytes[node] << ","; //rx_bytes
          flow_out << i->second.rxBytes[node] * 8.0 / (endTime - start_time) / 1000.0 << ","; //rx_throughput_kbps
          flow_out << i->second.dupsDropped[node] << ",";
          flow_out << i->second.timesForwarded[node] << ","; // times_forwarded
          flow_out << i->second.lostPackets[node] << ",";
          flow_out << i->second.delaySum[node].GetSeconds () / i->second.txPackets << "\n";

          std::map<uint32_t, Time>::iterator p_it;
          for (p_it = i->second.packetDelay[node].begin (); p_it != i->second.packetDelay[node].end (); p_it++)
            {
              uint32_t pack_id = p_it->first;
              Time pack_delay = p_it->second;
              pack_out << i->first << ","; //flow_id
              pack_out << t.sourceAddress << ","; // flow_src node
              pack_out << node << ","; // flow_dst node
              pack_out << t.destinationAddress << ","; //flow_dst addr
              pack_out << pack_id << ","; // packet id/seq
              pack_out << pack_delay.GetSeconds () << ",";
              pack_out << i->second.numHops[node][pack_id] << "\n";
            }
        }


Helpers
=======

The helper API follows the pattern usage of normal helpers.
Through the helper you can install the monitor in the nodes, set the monitor attributes, and
print the statistics.

One important thing is: the :cpp:class:`ns3::MulticastFlowMonitorHelper` must be instantiated only
once in the main.

Attributes
==========

The module provides the following attributes in :cpp:class:`ns3::MulticastFlowMonitor`:

* MaxPerHopDelay (Time, default 10s): The maximum per-hop delay that should be considered;
* StartTime (Time, default 0s): The time when the monitoring starts.

Output
======

The main output can be seen above in :ref:`Usage`


Examples
========

The examples are located in `src/multicast-flow-monitor/examples`.

Troubleshooting
===============

Do not define more than one :cpp:class:`ns3::MulticastFlowMonitorHelper` in the simulation.

Validation
**********

The forthcoming paper in the references contains a description of the module validation against
a representative test network.
