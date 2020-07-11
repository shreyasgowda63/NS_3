.. include:: replace.txt
.. highlight:: cpp
.. highlight:: bash

ACK Filter
------------------

This chapter describes the Ack-Filtering feature implementation in ns3.

This feature is one of the four components introduced in the paper Piece of `CAKE <https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=8475045>`_ : A Comprehensive Queue
Management Solution for Home Gateways. TCP ACK filtering is mainly an optimization technique that
seems very useful in highly asymmetric networks. The technique
involves filtering TCP acknowledgment (ACK)
packets by inspecting queues and dropping ACKs if a TCP
flow has several consecutive ACKs queued. This technique involves going
through flow queues and dropping ACKs such that it will not result in any information
loss for TCP. This can improve the performance very significantly, especially when the reverse path
does not have sufficient bandwidth to send ACK produced due to the forward path.

Model Description
*****************

The below section explains the implementation of TCP ACK Filter in ns3. The implementation is inspired by the Linux code implementation for the CAKE module. The source code for the ACK filter model is located in the directory ``src/traffic-control/model`` and consists of 2 files `ack-filter.h`
and `ack-filter.cc`. The ack-filter class has been implemented in a modularized way so that if any QueueDisc requires it, an object of this class can be instantiated and the corresponding method can be called. The important methods of the AckFilter class are explained below: 

* class :cpp:class:ack-filter : This class implements the ACK Filter feature:

  * ``bool AckFilterMain (Ptr<Queue<QueueDiscItem>> queue, Ptr<QueueDiscItem> item)``: This is the primary method, which will be used in any QueueDisc, we need to call this method on the arrival of every packet in the queue. To apply the ACK Filter feature on any queue, we need to pass a pointer to the queue as an argument. It handles iterating through the rest of the queue and checks the required conditions. It internally calls AckFilterMayDrop and AckFilterSackCompare methods. It returns true if ACKs were dropped from the queue, else false.

  * ``bool AckFilterMayDrop (Ptr<QueueDiscItem> item, uint32_t tstamp, uint32_t tsecr)``: tsecr): This method used to determine if a QueueDiscItem is elligible to being dropped from the queue, thus it returns a boolean value. If it returns false then the item is not dropped, but if it returns true, item might be dropped. It basically checks various TCP flags like ACK, CWR, ECE and also checks TCP options and validates if the item is eligible to being dropped.

  * ``int AckFilterSackCompare (Ptr<QueueDiscItem> item a, Ptr<QueueDiscItem> item b)``: In this method we compare 2 SACK blocks to determine which one SACKs more bytes. If A SACKs more bytes or bytes not SACKED by B, then A will be considered greater and vice-versa. If both A and B SACK same bytes then they will be considered equal. It will return :
   
    1. -1 if SACK Block of 'item a' is greater than SACK Block of item b. 
    2. 0 if SACK Block of item a is equal SACK Block of item b. 
    3. -1 if SACK Block of item a is smaller SACK Block of item b.


Examples
========

To implement the ACK Filter feature on any queue, we will need a pointer to the target queue, an object of the ack-filter class and call the 

.. sourcecode:: cpp

  Ptr<QueueDiscItem> item;                                 //item to be enqueued
  Ptr<Queue<QueueDiscItem>> queue = GetInternalQueue (0);  //queue to enqueue it in
  bool retval;

  AckFilter ack;
  retval = ack.AckFilterMain(queue, item);
  


Validation
**********

The AckFilter model is tested using :cpp:class:`CobaltQueueDiscTestSuite` class defined in `src/traffic-control/test/cobalt-queue-disc-test-suite.cc.cc`. In each test case, we enqueue 2 ACK packets and check for different scenarios. The suite has multiple test cases, out of which 5 test cases are associated with AckFilter: 
  1. AckFilterEceCwrFlagTest: In this test case, we verify if the AckFilter model is performing correctly when the ECE, CWR flags are the same for the enqueued item and an item already in the queue.
  2. AckFilterSackPermittedTest: Here we test the behavior of the model when the already enqueued packet has SACK PERMITTED TCP option enabled.
  3. AckFilterUdpEnqueueTest: AckFilter model does not apply to UDP packets, this characteristic is tested in the feature.
  4. AckFilterUrgFlagTest: An item in the queue with URG flag enabled shouldn't be filtered, this aspect is tested in this test case.
  5. AckFilterDropHeadTest: This test is designed to verify the basic functionality of TCP ACK Filter. The already enqueued TCP packet has only ACK Flag enabled and the packet being enqueued has a higher sequence number. The packet at HEAD of the queue will be dropped as this does not result in any information loss at TCP Sender Side.

The test suite can be run using the following commands::````
  $ NS_LOG="CobaltQueueDisc" ./waf --run "test-runner --suite=cobalt-queue-disc"  

  $ NS_LOG="CobaltQueueDisc" ./waf --run dumbbell

