Support for Explicit Congestion Notification (ECN)
++++++++++++++++++++++++++++++++++++++++++++++++++

ECN provides end-to-end notification of network congestion without dropping
packets. It uses two bits in the IP header: ECN Capable Transport (ECT bit)
and Congestion Experienced (CE bit), and two bits in the TCP header: Congestion
Window Reduced (CWR) and ECN Echo (ECE).

More information is available in RFC 3168: https://tools.ietf.org/html/rfc3168

The following ECN states are declared in ``src/internet/model/tcp-socket-state.h``

::

  typedef enum
    {
      ECN_DISABLED = 0, //!< ECN disabled traffic
      ECN_IDLE,         //!< ECN is enabled but currently there is no action pertaining to ECE or CWR to be taken
      ECN_CE_RCVD,      //!< Last packet received had CE bit set in IP header
      ECN_SENDING_ECE,  //!< Receiver sends an ACK with ECE bit set in TCP header
      ECN_ECE_RCVD,     //!< Last ACK received had ECE bit set in TCP header
      ECN_CWR_SENT      //!< Sender has reduced the congestion window, and sent a packet with CWR bit set in TCP header. This is used for tracing.
    } EcnStates_t;

Current implementation of ECN is based on RFC 3168 and is referred as Classic ECN.

The following enum represents the mode of ECN:

::

  typedef enum
    {
      ClassicEcn,  //!< ECN functionality as described in RFC 3168.
      DctcpEcn,    //!< ECN functionality as described in RFC 8257. Note: this mode is specific to DCTCP.
    } EcnMode_t;

The following are some important ECN parameters:

::

  // ECN parameters
  EcnMode_t              m_ecnMode {ClassicEcn}; //!< ECN mode
  UseEcn_t               m_useEcn {Off};         //!< Socket ECN capability

Enabling ECN
^^^^^^^^^^^^

By default, support for ECN is disabled in TCP sockets. To enable, change
the value of the attribute ``ns3::TcpSocketBase::UseEcn`` to ``On``.
Following are supported values for the same, this functionality is aligned with
Linux: https://www.kernel.org/doc/Documentation/networking/ip-sysctl.txt

::

  typedef enum
    {
      Off        = 0,   //!< Disable
      On         = 1,   //!< Enable
      AcceptOnly = 2,   //!< Enable only when the peer endpoint is ECN capable
    } UseEcn_t;

For example:

::

  Config::SetDefault ("ns3::TcpSocketBase::UseEcn", StringValue ("On"))

ECN negotiation
^^^^^^^^^^^^^^^

ECN capability is negotiated during the three-way TCP handshake:

1. Sender sends SYN + CWR + ECE

::

    if (m_useEcn == UseEcn_t::On)
      {
        SendEmptyPacket (TcpHeader::SYN | TcpHeader::ECE | TcpHeader::CWR);
      }
    else
      {
        SendEmptyPacket (TcpHeader::SYN);
      }
    m_ecnState = ECN_DISABLED;

2. Receiver sends SYN + ACK + ECE

::

    if (m_useEcn != UseEcn_t::Off && (tcpHeader.GetFlags () & (TcpHeader::CWR | TcpHeader::ECE)) == (TcpHeader::CWR | TcpHeader::ECE))
      {
        SendEmptyPacket (TcpHeader::SYN | TcpHeader::ACK |TcpHeader::ECE);
        m_ecnState = ECN_IDLE;
      }
    else
      {
        SendEmptyPacket (TcpHeader::SYN | TcpHeader::ACK);
        m_ecnState = ECN_DISABLED;
      }

3. Sender sends ACK

::

    if (m_useEcn != UseEcn_t::Off &&  (tcpHeader.GetFlags () & (TcpHeader::CWR | TcpHeader::ECE)) == (TcpHeader::ECE))
      {
        m_ecnState = ECN_IDLE;
      }
    else
      {
        m_ecnState = ECN_DISABLED;
      }

Once the ECN-negotiation is successful, the sender sends data packets with ECT
bits set in the IP header.

Note: As mentioned in `Section 6.1.1 of RFC 3168 <https://tools.ietf.org/html/rfc3168#section-6.1.1>`_, ECT bits should not be set
during ECN negotiation. The ECN negotiation implemented in |ns3| follows
this guideline.

ECN State Transitions
^^^^^^^^^^^^^^^^^^^^^

1. Initially both sender and receiver have their m_ecnState set as ECN_DISABLED
2. Once the ECN negotiation is successful, their states are set to ECN_IDLE
3. The receiver's state changes to ECN_CE_RCVD when it receives a packet with
   CE bit set. The state then moves to ECN_SENDING_ECE when the receiver sends
   an ACK with ECE set. This state is retained until a CWR is received
   , following which, the state changes to ECN_IDLE.
4. When the sender receives an ACK with ECE bit set from receiver, its state
   is set as ECN_ECE_RCVD
5. The sender's state changes to ECN_CWR_SENT when it sends a packet with
   CWR bit set. It remains in this state until an ACK with valid ECE is received
   (i.e., ECE is received for a packet that belongs to a new window),
   following which, its state changes to ECN_ECE_RCVD.

RFC 3168 compliance
^^^^^^^^^^^^^^^^^^^

Based on the suggestions provided in RFC 3168, the following behavior has
been implemented:

1. Pure ACK packets should not have the ECT bit set (`Section 6.1.4 <https://tools.ietf.org/html/rfc3168#section-6.1.4>`_).
2. In the current implementation, the sender only sends ECT(0) in the IP header.
3. The sender should should reduce the congestion window only once in each
   window (`Section 6.1.2 <https://tools.ietf.org/html/rfc3168#section-6.1.2>`_).
4. The receiver should ignore the CE bits set in a packet arriving out of
   window (`Section 6.1.5 <https://tools.ietf.org/html/rfc3168#section-6.1.5>`_).
5. The sender should ignore the ECE bits set in the packet arriving out of
   window (`Section 6.1.2 <https://tools.ietf.org/html/rfc3168#section-6.1.2>`_).

Open issues
^^^^^^^^^^^

The following issues are yet to be addressed:

1. Retransmitted packets should not have the CWR bit set (`Section 6.1.5 <https://tools.ietf.org/html/rfc3168#section-6.1.5>`_).

2. Despite the congestion window size being 1 MSS, the sender should reduce its
   congestion window by half when it receives a packet with the ECE bit set. The
   sender must reset the retransmit timer on receiving the ECN-Echo packet when
   the congestion window is one. The sending TCP will then be able to send a
   new packet only when the retransmit timer expires (`Section 6.1.2 <https://tools.ietf.org/html/rfc3168#section-6.1.2>`_).

3. Support for separately handling the enabling of ECN on the incoming and
   outgoing TCP sessions (e.g. a TCP may perform ECN echoing but not set the
   ECT codepoints on its outbound data segments).

Support for Dynamic Pacing
++++++++++++++++++++++++++

TCP pacing refers to the sender-side practice of scheduling the transmission
of a burst of eligible TCP segments across a time interval such as
a TCP RTT, to avoid or reduce bursts.  Historically,
TCP used the natural ACK clocking mechanism to pace segments, but some
network paths introduce aggregation (bursts of ACKs arriving) or ACK
thinning, either of which disrupts ACK clocking.
Some latency-sensitive congestion controls under development (Prague, BBR)
require pacing to operate effectively.

Until recently, the state of the art in Linux was to support pacing in one
of two ways:

1) fq/pacing with sch_fq
2) TCP internal pacing

The presentation by Dumazet and Cheng at IETF 88 summarizes:
https://www.ietf.org/proceedings/88/slides/slides-88-tcpm-9.pdf

The first option was most often used when offloading (TSO) was enabled and
when the sch_fq scheduler was used at the traffic control (qdisc) sublayer.  In
this case, TCP was responsible for setting the socket pacing rate, but
the qdisc sublayer would enforce it.  When TSO was enabled, the kernel
would break a large burst into smaller chunks, with dynamic sizing based
on the pacing rate, and hand off the segments to the fq qdisc for
pacing.

The second option was used if sch_fq was not enabled; TCP would be
responsible for internally pacing.

In 2018, Linux switched to an Early Departure Model (EDM): https://lwn.net/Articles/766564/.

TCP pacing in Linux was added in kernel 3.12, and authors chose to allow
a pacing rate of 200% against the current rate, to allow probing for
optimal throughput even during slow start phase.  Some refinements were
added in https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=43e122b014c9,
in which Google reported that it was better to apply
a different ratio (120%) in Congestion Avoidance phase.  Furthermore,
authors found that after cwnd reduction, it was helpful to become more
conservative and switch to the conservative ratio (120%) as soon as
cwnd >= ssthresh/2, as the initial ramp up (when ssthresh is infinite) still
allows doubling cwnd every other RTT.  Linux also does not pace the initial
window (IW), typically 10 segments in practice.

Linux has also been observed to not pace if the number of eligible segments
to be sent is exactly two; they will be sent back to back.  If three or
more, the first two are sent immediately, and additional segments are paced
at the current pacing rate.

In ns-3, the model is as follows.  There is no TSO/sch_fq model; only
internal pacing according to current Linux policy.

Pacing may be enabled for any TCP congestion control, and a maximum
pacing rate can be set.  Furthermore, dynamic pacing is enabled for
all TCP variants, according to the following guidelines.

* Pacing of the initial window (IW) is not done by default but can be
  separately enabled.

* Pacing of the initial slow start, after IW, is done according to the
  pacing rate of 200% of the current rate, to allow for window growth
  This pacing rate can be configured to a different value than 200%.

* Pacing of congestion avoidance phase is done at a pacing rate of 120% of
  current rate.  This can be configured to a different value than 120%.

* Pacing of subsequent slow start is done according to the following
  heuristic.  If cwnd < ssthresh/2, such as after a timeout or idle period,
  pace at the slow start rate (200%).  Otherwise, pace at the congestion
  avoidance rate.

Dynamic pacing is demonstrated by the example program ``examples/tcp/tcp-pacing.cc``.

Validation
++++++++++

The following tests are found in the ``src/internet/test`` directory. In
general, TCP tests inherit from a class called :cpp:class:`TcpGeneralTest`,
which provides common operations to set up test scenarios involving TCP
objects. For more information on how to write new tests, see the
section below on :ref:`Writing-tcp-tests`.

* **tcp:** Basic transmission of string of data from client to server
* **tcp-bytes-in-flight-test:** TCP correctly estimates bytes in flight under loss conditions
* **tcp-cong-avoid-test:** TCP congestion avoidance for different packet sizes
* **tcp-datasentcb:** Check TCP's 'data sent' callback
* **tcp-endpoint-bug2211-test:** A test for an issue that was causing stack overflow
* **tcp-fast-retr-test:** Fast Retransmit testing
* **tcp-header:** Unit tests on the TCP header
* **tcp-highspeed-test:** Unit tests on the HighSpeed congestion control
* **tcp-htcp-test:** Unit tests on the H-TCP congestion control
* **tcp-hybla-test:** Unit tests on the Hybla congestion control
* **tcp-vegas-test:** Unit tests on the Vegas congestion control
* **tcp-veno-test:** Unit tests on the Veno congestion control
* **tcp-scalable-test:** Unit tests on the Scalable congestion control
* **tcp-bic-test:** Unit tests on the BIC congestion control
* **tcp-yeah-test:** Unit tests on the YeAH congestion control
* **tcp-illinois-test:** Unit tests on the Illinois congestion control
* **tcp-ledbat-test:** Unit tests on the LEDBAT congestion control
* **tcp-lp-test:** Unit tests on the TCP-LP congestion control
* **tcp-dctcp-test:** Unit tests on the DCTCP congestion control
* **tcp-bbr-test:** Unit tests on the BBR congestion control
* **tcp-option:** Unit tests on TCP options
* **tcp-pkts-acked-test:** Unit test the number of time that PktsAcked is called
* **tcp-rto-test:** Unit test behavior after a RTO occurs
* **tcp-rtt-estimation-test:** Check RTT calculations, including retransmission cases
* **tcp-slow-start-test:** Check behavior of slow start
* **tcp-timestamp:** Unit test on the timestamp option
* **tcp-wscaling:** Unit test on the window scaling option
* **tcp-zero-window-test:** Unit test persist behavior for zero window conditions
* **tcp-close-test:** Unit test on the socket closing: both receiver and sender have to close their socket when all bytes are transferred
* **tcp-ecn-test:** Unit tests on Explicit Congestion Notification
* **tcp-pacing-test:** Unit tests on dynamic TCP pacing rate

Several tests have dependencies outside of the ``internet`` module, so they
are located in a system test directory called ``src/test/ns3tcp``.

* **ns3-tcp-loss:** Check behavior of ns-3 TCP upon packet losses
* **ns3-tcp-no-delay:** Check that ns-3 TCP Nagle's algorithm works correctly and that it can be disabled
* **ns3-tcp-socket:** Check that ns-3 TCP successfully transfers an application data write of various sizes
* **ns3-tcp-state:** Check the operation of the TCP state machine for several cases

Several TCP validation test results can also be found in the
`wiki page <http://www.nsnam.org/wiki/New_TCP_Socket_Architecture>`_
describing this implementation.

The ns-3 implementation of TCP Linux Reno was validated against the NewReno
implementation of Linux kernel 4.4.0 using ns-3 Direct Code Execution (DCE).
DCE is a framework which allows the users to run kernel space protocol inside
ns-3 without changing the source code.

In this validation, cwnd traces of DCE Linux ``reno`` were compared to those of
ns-3 Linux Reno and NewReno for a delayed acknowledgement configuration of 1
segment (in the ns-3 implementation; Linux does not allow direct configuration
of this setting). It can be observed that cwnd traces for ns-3 Linux Reno are
closely overlapping with DCE ``reno``, while
for ns-3 NewReno there was deviation in the congestion avoidance phase.

.. _fig-dce-Linux-reno-vs-ns3-linux-reno:

.. figure:: figures/dce-linux-reno-vs-ns3-linux-reno.*
   :scale: 70%
   :align: center

   DCE Linux Reno vs. ns-3 Linux Reno

.. _fig-dce-Linux-reno-vs-ns3-new-reno:

.. figure:: figures/dce-linux-reno-vs-ns3-new-reno.*
   :scale: 70%
   :align: center

   DCE Linux Reno vs. ns-3 NewReno

The difference in the cwnd in the early stage of this flow is because of the
way cwnd is plotted.  As ns-3 provides a trace source for cwnd, an ns-3 Linux
Reno cwnd simple is obtained every time the cwnd value changes, whereas for
DCE Linux Reno, the kernel does not have a corresponding trace source.
Instead, we use the "ss" command of the Linux kernel to obtain
cwnd values. The "ss" samples cwnd at an interval of 0.5 seconds.

Figure :ref:`fig-dctcp-10ms-50mbps-tcp-throughput` shows a long-running
file transfer using DCTCP over a 50 Mbps bottleneck (running CoDel queue
disc with a 1ms CE threshold setting) with a 10 ms base RTT.  The figure
shows that DCTCP reaches link capacity very quickly and stays there for
the duration with minimal change in throughput.  In contrast, Figure
:ref:`fig-dctcp-80ms-50mbps-tcp-throughput` plots the throughput for
the same configuration except with an 80 ms base RTT.  In this case,
the DCTCP exits slow start early and takes a long time to build the
flow throughput to the bottleneck link capacity.  DCTCP is not intended
to be used at such a large base RTT, but this figure highlights the
sensitivity to RTT (and can be reproduced using the Linux implementation).

.. _fig-dctcp-10ms-50mbps-tcp-throughput:

.. figure:: figures/dctcp-10ms-50mbps-tcp-throughput.*
   :scale: 80 %
   :align: center

   DCTCP throughput for 10ms/50Mbps bottleneck, 1ms CE threshold

.. _fig-dctcp-80ms-50mbps-tcp-throughput:

.. figure:: figures/dctcp-80ms-50mbps-tcp-throughput.*
   :scale: 80 %
   :align: center

   DCTCP throughput for 80ms/50Mbps bottleneck, 1ms CE threshold

Similar to DCTCP, TCP CUBIC has been tested against the Linux kernel version
4.4 implementation.  Figure :ref:`fig-cubic-50ms-50mbps-tcp-cwnd-no-ecn`
compares the congestion window evolution between ns-3 and Linux for a single
flow operating over a 50 Mbps link with 50 ms base RTT and the CoDel AQM.
Some differences can be observed between the peak of slow start window
growth (ns-3 exits slow start earlier due to its HyStart implementation),
and the window growth is a bit out-of-sync (likely due to different
implementations of the algorithm), but the cubic concave/convex window
pattern, and the signs of TCP CUBIC fast convergence algorithm
(alternating patterns of cubic and concave window growth) can be observed.
The |ns3| congestion window is maintained in bytes (unlike Linux which uses
segments) but has been normalized to segments for these plots.
Figure :ref:`fig-cubic-50ms-50mbps-tcp-cwnd-ecn` displays the outcome of
a similar scenario but with ECN enabled throughout.

.. _fig-cubic-50ms-50mbps-tcp-cwnd-no-ecn:

.. figure:: figures/cubic-50ms-50mbps-tcp-cwnd-no-ecn.*
   :scale: 80 %
   :align: center

   CUBIC cwnd evolution for 50ms/50Mbps bottleneck, no ECN

.. _fig-cubic-50ms-50mbps-tcp-cwnd-ecn:

.. figure:: figures/cubic-50ms-50mbps-tcp-cwnd-ecn.*
   :scale: 80 %
   :align: center

   CUBIC cwnd evolution for 50ms/50Mbps bottleneck, with ECN


TCP ECN operation is tested in the ARED and RED tests that are documented in the traffic-control
module documentation.

Like DCTCP and TCP CUBIC, the ns-3 implementation of TCP BBR was validated
against the BBR implementation of Linux kernel 5.4 using Network Stack Tester
(NeST). NeST is a python package which allows the users to emulate kernel space
protocols using Linux network namespaces. Figure :ref:`fig-ns3-bbr-vs-linux-bbr`
compares the congestion window evolution between ns-3 and Linux for a single
flow operating over a 10 Mbps link with 10 ms base RTT and FIFO queue
discipline.

.. _fig-ns3-bbr-vs-linux-bbr:

.. figure:: figures/ns3-bbr-vs-linux-bbr.*
   :scale: 80 %
   :align: center

   Congestion window evolution: ns-3 BBR vs. Linux BBR (using NeST)

It can be observed that the congestion window traces for ns-3 BBR closely
overlap with Linux BBR. The periodic drops in congestion window every 10
seconds depict the PROBE_RTT phase of the BBR algorithm. In this phase, BBR
algorithm keeps the congestion window fixed to 4 segments.

The example program, examples/tcp-bbr-example.cc has been used to obtain the
congestion window curve shown in Figure :ref:`fig-ns3-bbr-vs-linux-bbr`. The
detailed instructions to reproduce ns-3 plot and NeST plot can be found at:
https://github.com/mohittahiliani/BBR-Validation

