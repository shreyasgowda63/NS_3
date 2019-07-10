.. include:: replace.txt
.. highlight:: bash

Sjf queue disc
---------------------

Model Description
*****************

SjfQueueDisc implements a Shortest-Job-First (SJF) policy. The packet from the flow with the
smallest flow size tag value is prioritized during dequeue. The internal queue data structure of 
SjfQueueDisc is PriorityQueue which sorts the packet based on the flow size tag value. The packet
with the smallest flow size tag value is at the front of the queue and will be served first. Packets
of the same flow size tag value will be sorted based on the First-In-First-Out (FIFO) policy.

SJF is an ideal algorithm: it assumes that the source applications know the total flow size a priori and
such information would be tagged to the packets prior transmission.

To fully implement SJF policy, the source applications should tag the packet with the associated flow size. 
Meanwhile, all nodes of the network should intall SjfQueueDisc. If the tagging process is absent, the 
SjfQueueDisc would function as the simple FifoQueueDisc since all packets would be served based on FIFO policy.

Attributes
==========

The MlfqQueueDisc class holds the following attribute:

* ``MaxSize:`` The maximum number of packets/bytes the queue disc can hold. The default value is 1000 packets.

Examples
========

An example of SJF scheduling is given as `mlfq-sjf-example.cc` located in ``src/traffic-control/examples``. 

.. sourcecode:: bash

   $ ./waf configure --enable-examples
   $ ./waf --run "mlfq-example --PrintHelp"
   $ # Run SjfQueueDisc
   $ NS_LOG="MlfqSjfExample" ./waf --run "mlfq-sjf-example --queueDiscName=SjfQueueDisc"

Validation
**********

SjfQueueDisc is tested using :cpp:class:`SjfQueueDiscTestSuite` class defined in 
``src/traffic-control/test/sjf-queue-disc-test-suite.cc``. It includes 3 test cases:

* Test 1: Check that packets with different flow size tag values are dequeued in the non-decreasing order.
* Test 2: Check that packets with the same flow size tag values are dequeued based on the FIFO policy.
* Test 3: Check that when the queue disc size is full, the default drop tail policy is applied. That is, any
new incoming packet will be dropped regardless of its tag value.

The test suite can be run using the following commands:

.. sourcecode:: bash

  $ ./waf configure --enable-tests
  $ ./waf build
  $ ./test.py -s sjf-queue-disc

or

.. sourcecode:: bash

  $ ./waf configure --enable-tests
  $ NS_LOG="SjfQueueDisc:PriorityQueue" ./waf --run "test-runner --suite=sjf-queue-disc"