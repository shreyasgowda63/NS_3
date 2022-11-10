
Writing a new congestion control algorithm
++++++++++++++++++++++++++++++++++++++++++

Writing (or porting) a congestion control algorithms from scratch (or from
other systems) is a process completely separated from the internals of
TcpSocketBase.

All operations that are delegated to a congestion control are contained in
the class TcpCongestionOps. It mimics the structure tcp_congestion_ops of
Linux, and the following operations are defined:

.. code-block:: c++

  virtual std::string GetName () const;
  virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight);
  virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,const Time& rtt);
  virtual Ptr<TcpCongestionOps> Fork ();
  virtual void CwndEvent (Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCaEvent_t event);

The most interesting methods to write are GetSsThresh and IncreaseWindow.
The latter is called when TcpSocketBase decides that it is time to increase
the congestion window. Much information is available in the Transmission
Control Block, and the method should increase cWnd and/or ssThresh based
on the number of segments acked.

GetSsThresh is called whenever the socket needs an updated value of the
slow start threshold. This happens after a loss; congestion control algorithms
are then asked to lower such value, and to return it.

PktsAcked is used in case the algorithm needs timing information (such as
RTT), and it is called each time an ACK is received.

CwndEvent is used in case the algorithm needs the state of socket during different
congestion window event.

TCP SACK and non-SACK
+++++++++++++++++++++
To avoid code duplication and the effort of maintaining two different versions
of the TCP core, namely RFC 6675 (TCP-SACK) and RFC 5681 (TCP congestion control),
we have merged RFC 6675 in the current code base. If the receiver supports the
option, the sender bases its retransmissions over the received SACK information.
However, in the absence of that option, the best it can do is to follow the RFC
5681 specification (on Fast Retransmit/Recovery) and employing NewReno
modifications in case of partial ACKs.

A similar concept is used in Linux with the function tcp_add_reno_sack.
Our implementation resides in the TcpTxBuffer class that implements a scoreboard
through two different lists of segments. TcpSocketBase actively uses the API
provided by TcpTxBuffer to query the scoreboard; please refer to the Doxygen
documentation (and to in-code comments) if you want to learn more about this
implementation.

For an academic peer-reviewed paper on the SACK implementation in ns-3,
please refer to https://dl.acm.org/citation.cfm?id=3067666.

Loss Recovery Algorithms
++++++++++++++++++++++++
The following loss recovery algorithms are supported in ns-3 TCP.  The current
default (as of ns-3.32 release) is Proportional Rate Reduction (PRR), while
the default for ns-3.31 and earlier was Classic Recovery.

Classic Recovery
^^^^^^^^^^^^^^^^
Classic Recovery refers to the combination of NewReno algorithm described in
RFC 6582 along with SACK based loss recovery algorithm mentioned in RFC 6675.
SACK based loss recovery is used when sender and receiver support SACK options.
In the case when SACK options are disabled, the NewReno modification handles
the recovery.

At the start of recovery phase the congestion window is reduced diffently for
NewReno and SACK based recovery. For NewReno the reduction is done as given below:

.. math::  cWnd = ssThresh

For SACK based recovery, this is done as follows:

.. math::   cWnd = ssThresh + (dupAckCount * segmentSize)

While in the recovery phase, the congestion window is inflated by segmentSize
on arrival of every ACK when NewReno is used. The congestion window is kept
same when SACK based loss recovery is used.

Proportional Rate Reduction
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Proportional Rate Reduction (PRR) is a loss recovery algorithm described in
RFC 6937 and currently used in Linux. The design of PRR helps in avoiding
excess window adjustments and aims to keep the congestion window as close as
possible to ssThresh.

PRR updates the congestion window by comparing the values of bytesInFlight and
ssThresh. If the value of bytesInFlight is greater than ssThresh, congestion window
is updated as shown below:

.. math::  sndcnt = CEIL(prrDelivered * ssThresh / RecoverFS) - prrOut
.. math::  cWnd = pipe + sndcnt

where ``RecoverFS`` is the value of bytesInFlight at the start of recovery phase,
``prrDelivered`` is the total bytes delivered during recovery phase,
``prrOut`` is the total bytes sent during recovery phase and
``sndcnt`` represents the number of bytes to be sent in response to each ACK.

Otherwise, the congestion window is updated by either using Conservative Reduction
Bound (CRB) or Slow Start Reduction Bound (SSRB) with SSRB being the default
Reduction Bound. Each Reduction Bound calculates a maximum data sending limit.
For CRB, the limit is calculated as shown below:

.. math::  limit = prrDelivered - prr out

For SSRB, it is calculated as:

.. math::  limit = MAX(prrDelivered - prrOut, DeliveredData) + MSS

where ``DeliveredData`` represets the total number of bytes delivered to the
receiver as indicated by the current ACK and ``MSS`` is the maximum segment size.

After limit calculation, the cWnd is updated as given below:

.. math::  sndcnt = MIN (ssThresh - pipe, limit)
.. math::  cWnd = pipe + sndcnt

More information (paper): https://dl.acm.org/citation.cfm?id=2068832

More information (RFC): https://tools.ietf.org/html/rfc6937

Adding a new loss recovery algorithm in ns-3
++++++++++++++++++++++++++++++++++++++++++++

Writing (or porting) a loss recovery algorithms from scratch (or from
other systems) is a process completely separated from the internals of
TcpSocketBase.

All operations that are delegated to a loss recovery are contained in
the class TcpRecoveryOps and are given below:

.. code-block:: c++

  virtual std::string GetName () const;
  virtual void EnterRecovery (Ptr<const TcpSocketState> tcb, uint32_t unAckDataCount,
                              bool isSackEnabled, uint32_t dupAckCount,
                              uint32_t bytesInFlight, uint32_t lastDeliveredBytes);
  virtual void DoRecovery (Ptr<const TcpSocketState> tcb, uint32_t unAckDataCount,
                           bool isSackEnabled, uint32_t dupAckCount,
                           uint32_t bytesInFlight, uint32_t lastDeliveredBytes);
  virtual void ExitRecovery (Ptr<TcpSocketState> tcb, uint32_t bytesInFlight);
  virtual void UpdateBytesSent (uint32_t bytesSent);
  virtual Ptr<TcpRecoveryOps> Fork ();

EnterRecovery is called when packet loss is detected and recovery is triggered.
While in recovery phase, each time when an ACK arrives, DoRecovery is called which
performs the necessary congestion window changes as per the recovery algorithm.
ExitRecovery is called just prior to exiting recovery phase in order to perform the
required congestion window ajustments. UpdateBytesSent is used to keep track of
bytes sent and is called whenever a data packet is sent during recovery phase.

Delivery Rate Estimation
++++++++++++++++++++++++
Current TCP implementation measures the approximate value of the delivery rate of
inflight data based on Delivery Rate Estimation.

As high level idea, keep in mind that the algorithm keeps track of 2 variables:

1. `delivered`: Total amount of data delivered so far.

2. `deliveredStamp`: Last time `delivered` was updated.

When a packet is transmitted, the value of `delivered (d0)` and `deliveredStamp (t0)`
is stored in its respective TcpTxItem.

When an acknowledgement comes for this packet, the value of `delivered` and `deliveredStamp`
is updated to `d1` and `t1` in the same TcpTxItem.

After processing the acknowledgement, the rate sample is calculated and then passed
to a congestion avoidance algorithm:

.. math:: delivery_rate = (d1 - d0)/(t1 - t0)


The implementation to estimate delivery rate is a joint work between TcpTxBuffer and TcpRateOps.
For more information, please take a look at their Doxygen documentation.

The implementation follows the Internet draft (Delivery Rate Estimation):
https://tools.ietf.org/html/draft-cheng-iccrg-delivery-rate-estimation-00

Current limitations
+++++++++++++++++++

* TcpCongestionOps interface does not contain every possible Linux operation

.. _Writing-tcp-tests:

Writing TCP tests
+++++++++++++++++

The TCP subsystem supports automated test
cases on both socket functions and congestion control algorithms. To show
how to write tests for TCP, here we explain the process of creating a test
case that reproduces the `Bug #1571 <https://www.nsnam.org/bugzilla/show_bug.cgi?id=1571>`_.

The bug concerns the zero window situation, which happens when the receiver
cannot handle more data. In this case, it advertises a zero window, which causes
the sender to pause transmission and wait for the receiver to increase the
window.

The sender has a timer to periodically check the receiver's window: however, in
modern TCP implementations, when the receiver has freed a "significant" amount
of data, the receiver itself sends an "active" window update, meaning that
the transmission could be resumed. Nevertheless, the sender timer is still
necessary because window updates can be lost.

.. note::
   During the text, we will assume some knowledge about the general design
   of the TCP test infrastructure, which is explained in detail into the
   Doxygen documentation. As a brief summary, the strategy is to have a class
   that sets up a TCP connection, and that calls protected members of itself.
   In this way, subclasses can implement the necessary members, which will
   be called by the main TcpGeneralTest class when events occur. For example,
   after processing an ACK, the method ProcessedAck will be invoked. Subclasses
   interested in checking some particular things which must have happened during
   an ACK processing, should implement the ProcessedAck method and check
   the interesting values inside the method. To get a list of available methods,
   please check the Doxygen documentation.

We describe the writing of two test cases, covering both situations: the
sender's zero-window probing and the receiver "active" window update. Our focus
will be on dealing with the reported problems, which are:

* an ns-3 receiver does not send "active" window update when its receive buffer
  is being freed;
* even if the window update is artificially crafted, the transmission does not
  resume.

However, other things should be checked in the test:

* Persistent timer setup
* Persistent timer teardown if rWnd increases

To construct the test case, one first derives from the TcpGeneralTest class:

The code is the following:

.. code-block:: c++

   TcpZeroWindowTest::TcpZeroWindowTest (const std::string &desc)
      : TcpGeneralTest (desc)
   {
   }

Then, one should define the general parameters for the TCP connection, which
will be one-sided (one node is acting as SENDER, while the other is acting as
RECEIVER):

* Application packet size set to 500, and 20 packets in total (meaning a stream
  of 10k bytes)
* Segment size for both SENDER and RECEIVER set to 500 bytes
* Initial slow start threshold set to UINT32_MAX
* Initial congestion window for the SENDER set to 10 segments (5000 bytes)
* Congestion control: NewReno

We have also to define the link properties, because the above definition does
not work for every combination of propagation delay and sender application behavior.

* Link one-way propagation delay: 50 ms
* Application packet generation interval: 10 ms
* Application starting time: 20 s after the starting point

To define the properties of the environment (e.g. properties which should be
set before the object creation, such as propagation delay) one next implements
the method ConfigureEnvironment:

.. code-block:: c++

   void
   TcpZeroWindowTest::ConfigureEnvironment ()
   {
     TcpGeneralTest::ConfigureEnvironment ();
     SetAppPktCount (20);
     SetMTU (500);
     SetTransmitStart (Seconds (2.0));
     SetPropagationDelay (MilliSeconds (50));
   }

For other properties, set after the object creation, one can use
ConfigureProperties ().
The difference is that some values, such as initial congestion window
or initial slow start threshold, are applicable only to a single instance, not
to every instance we have. Usually, methods that requires an id and a value
are meant to be called inside ConfigureProperties (). Please see the Doxygen
documentation for an exhaustive list of the tunable properties.

.. code-block:: c++

   void
   TcpZeroWindowTest::ConfigureProperties ()
   {
     TcpGeneralTest::ConfigureProperties ();
     SetInitialCwnd (SENDER, 10);
   }

To see the default value for the experiment, please see the implementation of
both methods inside TcpGeneralTest class.

.. note::
   If some configuration parameters are missing, add a method called
   "SetSomeValue" which takes as input the value only (if it is meant to be
   called inside ConfigureEnvironment) or the socket and the value (if it is
   meant to be called inside ConfigureProperties).

To define a zero-window situation, we choose (by design) to initiate the connection
with a 0-byte rx buffer. This implies that the RECEIVER, in its first SYN-ACK,
advertises a zero window. This can be accomplished by implementing the method
CreateReceiverSocket, setting an Rx buffer value of 0 bytes (at line 6 of the
following code):

.. code-block:: c++
   :linenos:
   :emphasize-lines: 6,7,8

   Ptr<TcpSocketMsgBase>
   TcpZeroWindowTest::CreateReceiverSocket (Ptr<Node> node)
   {
     Ptr<TcpSocketMsgBase> socket = TcpGeneralTest::CreateReceiverSocket (node);

     socket->SetAttribute("RcvBufSize", UintegerValue (0));
     Simulator::Schedule (Seconds (10.0),
                          &TcpZeroWindowTest::IncreaseBufSize, this);

     return socket;
   }

Even so, to check the active window update, we should schedule an increase
of the buffer size. We do this at line 7 and 8, scheduling the function
IncreaseBufSize.

.. code-block:: c++

   void
   TcpZeroWindowTest::IncreaseBufSize ()
   {
     SetRcvBufSize (RECEIVER, 2500);
   }

Which utilizes the SetRcvBufSize method to edit the RxBuffer object of the
RECEIVER. As said before, check the Doxygen documentation for class TcpGeneralTest
to be aware of the various possibilities that it offers.

.. note::
   By design, we choose to maintain a close relationship between TcpSocketBase
   and TcpGeneralTest: they are connected by a friendship relation. Since
   friendship is not passed through inheritance, if one discovers that one
   needs to access or to modify a private (or protected) member of TcpSocketBase,
   one can do so by adding a method in the class TcpGeneralSocket. An example
   of such method is SetRcvBufSize, which allows TcpGeneralSocket subclasses
   to forcefully set the RxBuffer size.

   .. code-block:: c++

      void
      TcpGeneralTest::SetRcvBufSize (SocketWho who, uint32_t size)
      {
        if (who == SENDER)
          {
            m_senderSocket->SetRcvBufSize (size);
          }
        else if (who == RECEIVER)
          {
            m_receiverSocket->SetRcvBufSize (size);
          }
        else
          {
            NS_FATAL_ERROR ("Not defined");
          }
      }

Next, we can start to follow the TCP connection:

#. At time 0.0 s the connection is opened sender side, with a SYN packet sent from
   SENDER to RECEIVER
#. At time 0.05 s the RECEIVER gets the SYN and replies with a SYN-ACK
#. At time 0.10 s the SENDER gets the SYN-ACK and replies with a SYN.

While the general structure is defined, and the connection is started,
we need to define a way to check the rWnd field on the segments. To this aim,
we can implement the methods Rx and Tx in the TcpGeneralTest subclass,
checking each time the actions of the RECEIVER and the SENDER. These methods are
defined in TcpGeneralTest, and they are attached to the Rx and Tx traces in the
TcpSocketBase. One should write small tests for every detail that one wants to ensure during the
connection (it will prevent the test from changing over the time, and it ensures
that the behavior will stay consistent through releases). We start by ensuring that
the first SYN-ACK has 0 as advertised window size:

.. code-block:: c++

   void
   TcpZeroWindowTest::Tx(const Ptr<const Packet> p, const TcpHeader &h, SocketWho who)
   {
     ...
     else if (who == RECEIVER)
       {
         NS_LOG_INFO ("\tRECEIVER TX " << h << " size " << p->GetSize());

         if (h.GetFlags () & TcpHeader::SYN)
           {
             NS_TEST_ASSERT_MSG_EQ (h.GetWindowSize(), 0,
                                    "RECEIVER window size is not 0 in the SYN-ACK");
           }
       }
       ....
    }

Pratically, we are checking that every SYN packet sent by the RECEIVER has the
advertised window set to 0. The same thing is done also by checking, in the Rx
method, that each SYN received by SENDER has the advertised window set to 0.
Thanks to the log subsystem, we can print what is happening through messages.
If we run the experiment, enabling the logging, we can see the following:

.. code-block:: bash

   ./ns3 shell
   gdb --args ./build/utils/ns3-dev-test-runner-debug --test-name=tcp-zero-window-test --stop-on-failure --fullness=QUICK --assert-on-failure --verbose
   (gdb) run

   0.00s TcpZeroWindowTestSuite:Tx(): 0.00	SENDER TX 49153 > 4477 [SYN] Seq=0 Ack=0 Win=32768 ns3::TcpOptionWinScale(2) ns3::TcpOptionTS(0;0) size 36
   0.05s TcpZeroWindowTestSuite:Rx(): 0.05	RECEIVER RX 49153 > 4477 [SYN] Seq=0 Ack=0 Win=32768 ns3::TcpOptionWinScale(2) ns3::TcpOptionTS(0;0) ns3::TcpOptionEnd(EOL) size 0
   0.05s TcpZeroWindowTestSuite:Tx(): 0.05	RECEIVER TX 4477 > 49153 [SYN|ACK] Seq=0 Ack=1 Win=0 ns3::TcpOptionWinScale(0) ns3::TcpOptionTS(50;0) size 36
   0.10s TcpZeroWindowTestSuite:Rx(): 0.10	SENDER RX 4477 > 49153 [SYN|ACK] Seq=0 Ack=1 Win=0 ns3::TcpOptionWinScale(0) ns3::TcpOptionTS(50;0) ns3::TcpOptionEnd(EOL) size 0
   0.10s TcpZeroWindowTestSuite:Tx(): 0.10	SENDER TX 49153 > 4477 [ACK] Seq=1 Ack=1 Win=32768 ns3::TcpOptionTS(100;50) size 32
   0.15s TcpZeroWindowTestSuite:Rx(): 0.15	RECEIVER RX 49153 > 4477 [ACK] Seq=1 Ack=1 Win=32768 ns3::TcpOptionTS(100;50) ns3::TcpOptionEnd(EOL) size 0
   (...)

The output is cut to show the threeway handshake. As we can see from the headers,
the rWnd of RECEIVER is set to 0, and thankfully our tests are not failing.
Now we need to test for the persistent timer, which should be started by
the SENDER after it receives the SYN-ACK. Since the Rx method is called before
any computation on the received packet, we should utilize another method, namely
ProcessedAck, which is the method called after each processed ACK. In the
following, we show how to check if the persistent event is running after the
processing of the SYN-ACK:

.. code-block:: c++

   void
   TcpZeroWindowTest::ProcessedAck (const Ptr<const TcpSocketState> tcb,
                                    const TcpHeader& h, SocketWho who)
   {
     if (who == SENDER)
       {
         if (h.GetFlags () & TcpHeader::SYN)
           {
             EventId persistentEvent = GetPersistentEvent (SENDER);
             NS_TEST_ASSERT_MSG_EQ (persistentEvent.IsRunning (), true,
                                    "Persistent event not started");
           }
       }
    }

Since we programmed the increase of the buffer size after 10 simulated seconds,
we expect the persistent timer to fire before any rWnd changes. When it fires,
the SENDER should send a window probe, and the receiver should reply reporting
again a zero window situation. At first, we investigates on what the sender sends:

..  code-block:: c++
    :linenos:
    :emphasize-lines: 1,6,7,11

      if (Simulator::Now ().GetSeconds () <= 6.0)
        {
          NS_TEST_ASSERT_MSG_EQ (p->GetSize () - h.GetSerializedSize(), 0,
                                 "Data packet sent anyway");
        }
      else if (Simulator::Now ().GetSeconds () > 6.0 &&
               Simulator::Now ().GetSeconds () <= 7.0)
        {
          NS_TEST_ASSERT_MSG_EQ (m_zeroWindowProbe, false, "Sent another probe");

          if (! m_zeroWindowProbe)
            {
              NS_TEST_ASSERT_MSG_EQ (p->GetSize () - h.GetSerializedSize(), 1,
                                     "Data packet sent instead of window probe");
              NS_TEST_ASSERT_MSG_EQ (h.GetSequenceNumber(), SequenceNumber32 (1),
                                     "Data packet sent instead of window probe");
              m_zeroWindowProbe = true;
            }
        }

We divide the events by simulated time. At line 1, we check everything that
happens before the 6.0 seconds mark; for instance, that no data packets are sent,
and that the state remains OPEN for both sender and receiver.

Since the persist timeout is initialized at 6 seconds (exercise left for the
reader: edit the test, getting this value from the Attribute system), we need
to check (line 6) between 6.0 and 7.0 simulated seconds that the probe is sent.
Only one probe is allowed, and this is the reason for the check at line 11.

.. code-block:: c++
   :linenos:
   :emphasize-lines: 6,7

   if (Simulator::Now ().GetSeconds () > 6.0 &&
       Simulator::Now ().GetSeconds () <= 7.0)
     {
       NS_TEST_ASSERT_MSG_EQ (h.GetSequenceNumber(), SequenceNumber32 (1),
                              "Data packet sent instead of window probe");
       NS_TEST_ASSERT_MSG_EQ (h.GetWindowSize(), 0,
                              "No zero window advertised by RECEIVER");
     }

For the RECEIVER, the interval between 6 and 7 seconds is when the zero-window
segment is sent.

Other checks are redundant; the safest approach is to deny any other packet
exchange between the 7 and 10 seconds mark.

.. code-block:: c++

   else if (Simulator::Now ().GetSeconds () > 7.0 &&
            Simulator::Now ().GetSeconds () < 10.0)
     {
       NS_FATAL_ERROR ("No packets should be sent before the window update");
     }

The state checks are performed at the end of the methods, since they are valid
in every condition:

.. code-block:: c++

   NS_TEST_ASSERT_MSG_EQ (GetCongStateFrom (GetTcb(SENDER)), TcpSocketState::CA_OPEN,
                          "Sender State is not OPEN");
   NS_TEST_ASSERT_MSG_EQ (GetCongStateFrom (GetTcb(RECEIVER)), TcpSocketState::CA_OPEN,
                          "Receiver State is not OPEN");

Now, the interesting part in the Tx method is to check that after the 10.0
seconds mark (when the RECEIVER sends the active window update) the value of
the window should be greater than zero (and precisely, set to 2500):

.. code-block:: c++

   else if (Simulator::Now().GetSeconds() >= 10.0)
     {
       NS_TEST_ASSERT_MSG_EQ (h.GetWindowSize(), 2500,
                              "Receiver window not updated");
     }

To be sure that the sender receives the window update, we can use the Rx
method:

.. code-block:: c++
   :linenos:
   :emphasize-lines: 5

   if (Simulator::Now().GetSeconds() >= 10.0)
     {
       NS_TEST_ASSERT_MSG_EQ (h.GetWindowSize(), 2500,
                              "Receiver window not updated");
       m_windowUpdated = true;
     }

We check every packet after the 10 seconds mark to see if it has the
window updated. At line 5, we also set to true a boolean variable, to check
that we effectively reach this test.

Last but not least, we implement also the NormalClose() method, to check that
the connection ends with a success:

.. code-block:: c++

   void
   TcpZeroWindowTest::NormalClose (SocketWho who)
   {
     if (who == SENDER)
       {
         m_senderFinished = true;
       }
     else if (who == RECEIVER)
       {
         m_receiverFinished = true;
       }
   }

The method is called only if all bytes are transmitted successfully. Then, in
the method FinalChecks(), we check all variables, which should be true (which
indicates that we have perfectly closed the connection).

.. code-block:: c++

   void
   TcpZeroWindowTest::FinalChecks ()
   {
     NS_TEST_ASSERT_MSG_EQ (m_zeroWindowProbe, true,
                            "Zero window probe not sent");
     NS_TEST_ASSERT_MSG_EQ (m_windowUpdated, true,
                            "Window has not updated during the connection");
     NS_TEST_ASSERT_MSG_EQ (m_senderFinished, true,
                            "Connection not closed successfully (SENDER)");
     NS_TEST_ASSERT_MSG_EQ (m_receiverFinished, true,
                            "Connection not closed successfully (RECEIVER)");
   }

To run the test, the usual way is

.. code-block:: bash

   ./test.py -s tcp-zero-window-test

   PASS: TestSuite tcp-zero-window-test
   1 of 1 tests passed (1 passed, 0 skipped, 0 failed, 0 crashed, 0 valgrind errors)

To see INFO messages, use a combination of ./ns3 shell and gdb (really useful):

.. code-block:: bash


    ./ns3 shell && gdb --args ./build/utils/ns3-dev-test-runner-debug --test-name=tcp-zero-window-test --stop-on-failure --fullness=QUICK --assert-on-failure --verbose

and then, hit "Run".

.. note::
   This code magically runs without any reported errors; however, in real cases,
   when you discover a bug you should expect the existing test to fail (this
   could indicate a well-written test and a bad-writted model, or a bad-written
   test; hopefully the first situation). Correcting bugs is an iterative
   process. For instance, commits created to make this test case running without
   errors are 11633:6b74df04cf44, (others to be merged).

