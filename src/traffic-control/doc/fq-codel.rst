.. include:: replace.txt
.. highlight:: cpp
.. highlight:: bash

FqCoDel queue disc
------------------

This chapter describes the FqCoDel ([Hoe16]_) queue disc implementation in |ns3|.

The FlowQueue-CoDel (FQ-CoDel) algorithm is a combined packet scheduler and
Active Queue Management (AQM) algorithm developed as part of the
bufferbloat-fighting community effort ([Buf16]_).
Each queue is managed by the CoDel AQM algorithm.

FqCoDel is installed by default on single-queue NetDevices (such as PointToPoint,
Csma and Simple). Also, on multi-queue devices (such as Wifi), the default root
qdisc is Mq with as many FqCoDel child queue discs as the number of device queues.

Model Description
*****************

The source code for the FqCoDel queue disc is located in the directory
``src/traffic-control/model`` and consists of 2 files `fq-codel-queue-disc.h`
and `fq-codel-queue-disc.cc` defining a FqCoDelQueueDisc class. 
Documentation about the Flow Queue part is available at src/traffic-control/doc/fq.rst
The code was ported to |ns3| based on Linux kernel code implemented by Eric Dumazet.

In |ns3|, packet classification is performed in the same way as in Linux.
Neither internal queues nor classes can be configured for an FqCoDel
queue disc.


Possible next steps
===================

* what to do if ECT(1) and either/both ECT(0) and NotECT are in the same flow queue (hash collisions or tunnels)-- our L4S traffic flows will avoid this situation by supporting AccECN and ECN++ (and if it happens in practice, the CoDel logic will just apply two separate thresholds)
* adding a ramp marking response instead of step threshold
* adding a floor value (to suppress marks if the queue length is below a certain number of bytes or packets)
* adding a heuristic such as in PIE to avoid marking a packet if it arrived to an empty flow queue (check on ingress, remember at egress time)


References
==========

.. [Hoe16] T. Hoeiland-Joergensen, P. McKenney, D. Taht, J. Gettys and E. Dumazet, The FlowQueue-CoDel Packet Scheduler and Active Queue Management Algorithm, IETF draft.  Available online at `<https://tools.ietf.org/html/draft-ietf-aqm-fq-codel>`_

.. [Buf16] Bufferbloat.net.  Available online at `<http://www.bufferbloat.net/>`_.


Attributes
==========

The key attributes that the FqCoDelQueue class holds include the following:

* ``UseEcn:`` True to use ECN (packets are marked instead of being dropped)
* ``Interval:`` The interval parameter to be used on the CoDel queues. The default value is 100 ms.
* ``Target:`` The target parameter to be used on the CoDel queues. The default value is 5 ms.
* ``MaxSize:`` The limit on the maximum number of packets stored by FqCoDel.
* ``Flows:`` The number of flow queues managed by FqCoDel.
* ``DropBatchSize:`` The maximum number of packets dropped from the fat flow.
* ``Perturbation:`` The salt used as an additional input to the hash function used to classify packets.
* ``CeThreshold`` The FqCoDel CE threshold for marking packets
* ``UseL4s`` True to use L4S (only ECT1 packets are marked at CE threshold)
* ``EnableSetAssociativeHash:`` The parameter used to enable set associative hash.

Examples
========

A typical usage pattern is to create a traffic control helper and to configure type
and attributes of queue disc and filters from the helper. For example, FqCodel
can be configured as follows:

.. sourcecode:: cpp

  TrafficControlHelper tch;
  tch.SetRootQueueDisc ("ns3::FqCoDelQueueDisc", "DropBatchSize", UintegerValue (1)
                                                 "Perturbation", UintegerValue (256));
  QueueDiscContainer qdiscs = tch.Install (devices);

The example for FqCoDel's L4S mode is `FqCoDel-L4S-example.cc` located in ``src/traffic-control/examples``.  To run the file (the first invocation below shows the available
command-line options):
.. sourcecode:: bash
   $ ./waf --run "FqCoDel-L4S-example --PrintHelp"
   $ ./waf --run "FqCoDel-L4S-example --scenarioNum=5" 
The expected output from the previous command are .dat files.

Validation
**********

The FqCoDel model is tested using :cpp:class:`FqCoDelQueueDiscTestSuite` class defined in `src/test/ns3tc/codel-queue-test-suite.cc`.  The suite includes 2 test cases:

* Test 1: The sixth test checks that the packets are marked correctly.
* Test 2: The eighth test checks the L4S mode of FqCoDel where ECT1 packets are marked at CE threshold (target delay does not matter) while ECT0 packets continue to be marked at target delay (CE threshold does not matter).

The test suite can be run using the following commands::

  $ ./waf configure --enable-examples --enable-tests
  $ ./waf build
  $ ./test.py -s fq-codel-queue-disc

or::

  $ NS_LOG="FqCoDelQueueDisc" ./waf --run "test-runner --suite=fq-codel-queue-disc"