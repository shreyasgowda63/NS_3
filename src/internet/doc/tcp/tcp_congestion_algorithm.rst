Congestion Control Algorithms
+++++++++++++++++++++++++++++
Here follows a list of supported TCP congestion control algorithms. For an
academic paper on many of these congestion control algorithms, see
http://dl.acm.org/citation.cfm?id=2756518 .

NewReno
^^^^^^^
NewReno algorithm introduces partial ACKs inside the well-established Reno
algorithm. This and other modifications are described in RFC 6582. We have two
possible congestion window increment strategy: slow start and congestion
avoidance. Taken from RFC 5681:

  During slow start, a TCP increments cwnd by at most SMSS bytes for
  each ACK received that cumulatively acknowledges new data. Slow
  start ends when cwnd exceeds ssthresh (or, optionally, when it
  reaches it, as noted above) or when congestion is observed. While
  traditionally TCP implementations have increased cwnd by precisely
  SMSS bytes upon receipt of an ACK covering new data, we RECOMMEND
  that TCP implementations increase cwnd, per Equation :eq:`newrenocongavoid`,
  where N is the number of previously unacknowledged bytes acknowledged
  in the incoming ACK.

.. math:: cwnd += min (N, SMSS)
   :label: newrenocongavoid

During congestion avoidance, cwnd is incremented by roughly 1 full-sized
segment per round-trip time (RTT), and for each congestion event, the slow
start threshold is halved.

CUBIC
^^^^^
CUBIC (class :cpp:class:`TcpCubic`) is the default TCP congestion control
in Linux, macOS (since 2014), and Microsoft Windows (since 2017).
CUBIC has two main differences with respect to
a more classic TCP congestion control such as NewReno.  First, during the
congestion avoidance phase, the window size grows according to a cubic
function (concave, then convex) with the latter convex portion designed
to allow for bandwidth probing.  Second, a hybrid slow start (HyStart)
algorithm uses observations of delay increases in the slow start
phase of window growth to try to exit slow start before window growth
causes queue overflow.

CUBIC is documented in :rfc:`8312`, and the |ns3| implementation is based
on the RFC more so than the Linux implementation, although the Linux 4.4
kernel implementation (through the Direct Code Execution environment) has
been used to validate the behavior and is fairly well aligned (see below
section on validation).

Linux Reno
^^^^^^^^^^
TCP Linux Reno (class :cpp:class:`TcpLinuxReno`) is designed to provide a
Linux-like implementation of
TCP NewReno. The implementation of class :cpp:class:`TcpNewReno` in ns-3
follows RFC standards, and increases cwnd more conservatively than does Linux Reno.
Linux Reno modifies slow start and congestion avoidance algorithms to
increase cwnd based on the number of bytes being acknowledged by each
arriving ACK, rather than by the number of ACKs that arrive.  Another major
difference in implementation is that Linux maintains the congestion window
in units of segments, while the RFCs define the congestion window in units of
bytes.

In slow start phase, on each incoming ACK at the TCP sender side cwnd
is increased by the number of previously unacknowledged bytes ACKed by the
incoming acknowledgment. In contrast, in ns-3 NewReno, cwnd is increased
by one segment per acknowledgment.  In standards terminology, this
difference is referred to as Appropriate Byte Counting (RFC 3465); Linux
follows Appropriate Byte Counting while ns-3 NewReno does not.

.. math:: cwnd += segAcked * segmentSize
   :label: linuxrenoslowstart

.. math:: cwnd += segmentSize
   :label: newrenoslowstart

In congestion avoidance phase, the number of bytes that have been ACKed at
the TCP sender side are stored in a 'bytes_acked' variable in the TCP control
block. When 'bytes_acked' becomes greater than or equal to the value of the
cwnd, 'bytes_acked' is reduced by the value of cwnd. Next, cwnd is incremented
by a full-sized segment (SMSS).  In contrast, in ns-3 NewReno, cwnd is increased
by (1/cwnd) with a rounding off due to type casting into int.

.. code-block:: c++

   if (m_cWndCnt >= w)
    {
      uint32_t delta = m_cWndCnt / w;

      m_cWndCnt -= delta * w;
      tcb->m_cWnd += delta * tcb->m_segmentSize;
      NS_LOG_DEBUG ("Subtracting delta * w from m_cWndCnt " << delta * w);
    }

   :label: linuxrenocongavoid

.. code-block:: c++

   if (segmentsAcked > 0)
    {
      double adder = static_cast<double> (tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd.Get ();
      adder = std::max (1.0, adder);
      tcb->m_cWnd += static_cast<uint32_t> (adder);
      NS_LOG_INFO ("In CongAvoid, updated to cwnd " << tcb->m_cWnd <<
                   " ssthresh " << tcb->m_ssThresh);
    }

   :label: newrenocongavoid

So, there are two main difference between the TCP Linux Reno and TCP NewReno
in ns-3:
1) In TCP Linux Reno, delayed acknowledgement configuration does not affect
congestion window growth, while in TCP NewReno, delayed acknowledgments cause
a slower congestion window growth.
2) In congestion avoidance phase, the arithmetic for counting the number of
segments acked and deciding when to increment the cwnd is different for TCP
Linux Reno and TCP NewReno.

Following graphs shows the behavior of window growth in TCP Linux Reno and
TCP NewReno with delayed acknowledgement of 2 segments:

.. _fig-ns3-new-reno-vs-ns3-linux-reno:

.. figure:: figures/ns3-new-reno-vs-ns3-linux-reno.*
   :scale: 70%
   :align: center

   ns-3 TCP NewReno vs. ns-3 TCP Linux Reno

HighSpeed
^^^^^^^^^
TCP HighSpeed is designed for high-capacity channels or, in general, for
TCP connections with large congestion windows.
Conceptually, with respect to the standard TCP, HighSpeed makes the
cWnd grow faster during the probing phases and accelerates the
cWnd recovery from losses.
This behavior is executed only when the window grows beyond a
certain threshold, which allows TCP HighSpeed to be friendly with standard
TCP in environments with heavy congestion, without introducing new dangers
of congestion collapse.

Mathematically:

.. math::  cWnd = cWnd + \frac{a(cWnd)}{cWnd}
   :label: highspeedcwndincrement

The function a() is calculated using a fixed RTT the value 100 ms (the
lookup table for this function is taken from RFC 3649). For each congestion
event, the slow start threshold is decreased by a value that depends on the
size of the slow start threshold itself. Then, the congestion window is set
to such value.

.. math::   cWnd = (1-b(cWnd)) \cdot cWnd
   :label: highspeedcwnddecrement

The lookup table for the function b() is taken from the same RFC.
More information at: http://dl.acm.org/citation.cfm?id=2756518

Hybla
^^^^^
The key idea behind TCP Hybla is to obtain for long RTT connections the same
instantaneous transmission rate of a reference TCP connection with lower RTT.
With analytical steps, it is shown that this goal can be achieved by
modifying the time scale, in order for the throughput to be independent from
the RTT. This independence is obtained through the use of a coefficient rho.

This coefficient is used to calculate both the slow start threshold
and the congestion window when in slow start and in congestion avoidance,
respectively.

More information at: http://dl.acm.org/citation.cfm?id=2756518

Westwood
^^^^^^^^
The main idea of Westwood TCP, which is an end-to-end bandwidth estimation for setting control windows after congestion, was proposed by Saverio Mascolo. It leverages the information provided by TCP ACKs to derive more useful measurements. Since the sequence number in a TCP header represents the first byte being carried in the segment, Every ACK arrival indicates that a few bytes were delivered successfully at the receiver. 

Bandwidth is Estimated using the cumulative acknowledgment time difference and the corresponding amount of data delivered by. 

.. math:: b_k=\frac{d_{k}}{t_k - t_{k-1}}

Average is taken by (Tustin approximation)

.. math:: \widehat{b_k} = \frac{ \frac{2\tau}{t_k-t_{k-1}} -1}{ \frac{2\tau}{t_k-t_{k-1}} +1}\widehat{b_{k-1}} + \frac{b_k + b_{k-1}}{\frac{2\tau}{t_k-t_{k-1}} +1}

By considering a constant interarrival time: :math:`t_k - t_{k-1} = \Delta_k = \tau/10`.

Average bandwidth reduces to,

.. math:: \widehat{b_k}= a\widehat{b_{k-1}} +  \frac{(1-a)}{2}[b_k+b_{k-1}] 
   
where a=0.9 


Algorithms:

.. code-block:: c++

   packets_acked = current_ack_seqno - last_ack_seqno;
   if (packets_acked == 0)
   {
      dup_ack_counter = dup_ack_counter + 1;
      packets_acked = 1;
   }
   else if(packets_acked > 1)
   {
      if (dup_ack_counter > packets_acked)
      {
         dup_ack_counter = dup_ack_counter - packets_acked;
         packets_acked = 1;
      }
      else if (dup_ack_counter < packets_acked)
      {
         packets_acked = packets_acked - dup_ack_counter;
         dup_ack_counter = 0;
      }
   }
   last_ack_seqno = current_ack_seqno;
   return (packets_acked);

   :label: westwood packet acked count algorithm


.. code-block:: c++

   if (‘n’ duplicate ACKs are received)
   {
      slow_start_thresh = (BWE * RTTmin) / segment_size;
      if (congestion_window > slow_start_thresh)
         congestion_window = slow_start_thresh;
   }

   :label: Westwood ‘n’ duplicate ACKs algorithm

where RTTmin indicates the ‘smallest’ RTT observed over the duration of the connection

.. code-block:: c++

   if (the retransmission timer expires)
   {
      slow_start_thresh = (BWE * RTTmin) / segment_size;
      if (slow_start_thresh < 2)
         Slow_start_thresh = 2;
      congestion_window = 1;
   }

   :label: tcp westwood timeout algorithm

TCP Westwood+ is a extension of TCP Westwood and is built on top of TCP Westwood. The major limitation of TCP Westwood was that bandwidth estimation failed to work in the presence of ACK compression. In TCP Westwood, it was assumed that ACKs don’t undergo queueing delay. But In complex networks, ACKs do undergo queueing delay; thereby, ACKs are received in a burst. Since in TCP Westwood, bandwidth calculation is done once per ACK by,

.. math:: b_k =\frac{ d_k } {t_k - t_{k-1}}

The ACKs are received at burst at a small time gap. Therefore bandwidth estimated is large. 
In contrast, The ACKs went to compression because of the congestion in the network, Therefore, bandwidth should be decreased. To overcome this bandwidth overestimation limitation of the TCP Westwood, in TCP westwood+, bandwidth estimation is done once per RTT.


The modified bandwidth estimation is,

.. math:: b_k = \frac{d_k} {\Delta_k}

where :math:`\Delta_k` is RTT.

The second modification of westwood+ over Westwood is that, In westwood+, The bandwidth average is estimated by exponential moving weighted average(EWMA) by,

.. math:: \widehat{b_k} =\alpha* \widehat{b_{k-1}} + (1-\alpha)b_k

where :math:`\alpha` is 0.9.

WARNING: this TCP model lacks validation and regression tests; use with caution.

More information at: http://dl.acm.org/citation.cfm?id=381704 and
http://dl.acm.org/citation.cfm?id=2512757

Vegas
^^^^^
TCP Vegas is a pure delay-based congestion control algorithm implementing a
proactive scheme that tries to prevent packet drops by maintaining a small
backlog at the bottleneck queue. Vegas continuously samples the RTT and computes
the actual throughput a connection achieves using Equation :eq:`vegasactual` and compares it
with the expected throughput calculated in Equation :eq:`vegasexpected`. The difference between
these 2 sending rates in Equation :eq:`vegasdiff` reflects the amount of extra packets being
queued at the bottleneck.

.. math::   actual &= \frac{cWnd}{RTT}
   :label: vegasactual

.. math::   expected &= \frac{cWnd}{BaseRTT}
   :label: vegasexpected

.. math::   diff &= expected - actual
   :label: vegasdiff

To avoid congestion, Vegas linearly increases/decreases its congestion window
to ensure the diff value falls between the two predefined thresholds, alpha and
beta. diff and another threshold, gamma, are used to determine when Vegas
should change from its slow-start mode to linear increase/decrease mode.
Following the implementation of Vegas in Linux, we use 2, 4, and 1 as the
default values of alpha, beta, and gamma, respectively, but they can be
modified through the Attribute system.

More information at: http://dx.doi.org/10.1109/49.464716

Scalable
^^^^^^^^
Scalable improves TCP performance to better utilize the available bandwidth of
a highspeed wide area network by altering NewReno congestion window adjustment
algorithm. When congestion has not been detected, for each ACK received in an
RTT, Scalable increases its cwnd per:

.. math::  cwnd = cwnd + 0.01
   :label: scalablecwndincrement

Following Linux implementation of Scalable, we use 50 instead of 100 to account
for delayed ACK.

On the first detection of congestion in a given RTT, cwnd is reduced based on
the following equation:

.. math::  cwnd = cwnd - ceil(0.125 \cdot cwnd)
   :label: scalablecwnddecrement

More information at: http://dl.acm.org/citation.cfm?id=956989

Veno
^^^^

TCP Veno enhances Reno algorithm for more effectively dealing with random
packet loss in wireless access networks by employing Vegas's method in
estimating the backlog at the bottleneck queue to distinguish between
congestive and non-congestive states.

The backlog (the number of packets accumulated at the bottleneck queue) is
calculated using Equation :eq:`venoN`:

.. math::
   N &= Actual \cdot (RTT - BaseRTT) \\
     &= Diff \cdot BaseRTT
   :label: venoN

where:

.. math::
   Diff &= Expected - Actual \\
        &= \frac{cWnd}{BaseRTT} - \frac{cWnd}{RTT}
   :label: venoDiff

Veno makes decision on cwnd modification based on the calculated N and its
predefined threshold beta.

Specifically, it refines the additive increase algorithm of Reno so that the
connection can stay longer in the stable state by incrementing cwnd by
1/cwnd for every other new ACK received after the available bandwidth has
been fully utilized, i.e. when N exceeds beta. Otherwise, Veno increases
its cwnd by 1/cwnd upon every new ACK receipt as in Reno.

In the multiplicative decrease algorithm, when Veno is in the non-congestive
state, i.e. when N is less than beta, Veno decrements its cwnd by only 1/5
because the loss encountered is more likely a corruption-based loss than a
congestion-based. Only when N is greater than beta, Veno halves its sending
rate as in Reno.

More information at: http://dx.doi.org/10.1109/JSAC.2002.807336

BIC
^^^
BIC (class :cpp:class:`TcpBic`) is a predecessor of TCP CUBIC.
In TCP BIC the congestion control problem is viewed as a search
problem. Taking as a starting point the current window value
and as a target point the last maximum window value
(i.e. the cWnd value just before the loss event) a binary search
technique can be used to update the cWnd value at the midpoint between
the two, directly or using an additive increase strategy if the distance from
the current window is too large.

This way, assuming a no-loss period, the congestion window logarithmically
approaches the maximum value of cWnd until the difference between it and cWnd
falls below a preset threshold. After reaching such a value (or the maximum
window is unknown, i.e. the binary search does not start at all) the algorithm
switches to probing the new maximum window with a 'slow start' strategy.

If a loss occur in either these phases, the current window (before the loss)
can be treated as the new maximum, and the reduced (with a multiplicative
decrease factor Beta) window size can be used as the new minimum.

More information at: http://ieeexplore.ieee.org/xpl/articleDetails.jsp?arnumber=1354672

YeAH
^^^^

YeAH-TCP (Yet Another HighSpeed TCP) is a heuristic designed to balance various
requirements of a state-of-the-art congestion control algorithm:


1. fully exploit the link capacity of high BDP networks while inducing a small number of congestion events
2. compete friendly with Reno flows
3. achieve intra and RTT fairness
4. robust to random losses
5. achieve high performance regardless of buffer size

YeAH operates between 2 modes: Fast and Slow mode. In the Fast mode when the queue
occupancy is small and the network congestion level is low, YeAH increments
its congestion window according to the aggressive HSTCP rule. When the number of packets
in the queue grows beyond a threshold and the network congestion level is high, YeAH enters
its Slow mode, acting as Reno with a decongestion algorithm. YeAH employs Vegas' mechanism
for calculating the backlog as in Equation :eq:`q_yeah`. The estimation of the network congestion
level is shown in Equation :eq:`l_yeah`.

.. math::  Q = (RTT - BaseRTT) \cdot \frac{cWnd}{RTT}
   :label: q_yeah

.. math::  L = \frac{RTT - BaseRTT}{BaseRTT}
   :label: l_yeah

To ensure TCP friendliness, YeAH also implements an algorithm to detect the presence of legacy
Reno flows. Upon the receipt of 3 duplicate ACKs, YeAH decreases its slow start threshold
according to Equation :eq:`yeahssthresh` if it's not competing with Reno flows. Otherwise, the ssthresh is
halved as in Reno:

.. math::  ssthresh = min(max(\frac{cWnd}{8}, Q), \frac{cWnd}{2})
   :label: yeahssthresh

More information: http://www.csc.lsu.edu/~sjpark/cs7601/4-YeAH_TCP.pdf

Illinois
^^^^^^^^

TCP Illinois is a hybrid congestion control algorithm designed for
high-speed networks. Illinois implements a Concave-AIMD (or C-AIMD)
algorithm that uses packet loss as the primary congestion signal to
determine the direction of window update and queueing delay as the
secondary congestion signal to determine the amount of change.

The additive increase and multiplicative decrease factors (denoted as
alpha and beta, respectively) are functions of the current average queueing
delay da as shown in Equations :eq:`illinoisalpha` and :eq:`illinoisbeta`. To improve the protocol
robustness against sudden fluctuations in its delay sampling,
Illinois allows the increment of alpha to alphaMax
only if da stays below d1 for a some (theta) amount of time.

.. math::
   alpha &=
   \begin{cases}
      \quad alphaMax              & \quad \text{if } da <= d1 \\
      \quad k1 / (k2 + da)        & \quad \text{otherwise} \\
   \end{cases}
   :label: illinoisalpha

.. math::
   beta &=
   \begin{cases}
      \quad betaMin               & \quad \text{if } da <= d2 \\
      \quad k3 + k4 \, da         & \quad \text{if } d2 < da < d3 \\
      \quad betaMax               & \quad \text{otherwise}
   \end{cases}
   :label: illinoisbeta

where the calculations of k1, k2, k3, and k4 are shown in the following:

.. math::   k1 &= \frac{(dm - d1) \cdot alphaMin \cdot alphaMax}{alphaMax - alphaMin}
   :label: illinoisk1

.. math::   k2 &= \frac{(dm - d1) \cdot alphaMin}{alphaMax - alphaMin} - d1
   :label: illinoisk2

.. math::   k3 &= \frac{alphaMin \cdot d3 - alphaMax \cdot d2}{d3 - d2}
   :label: illinoisk3

.. math::   k4 &= \frac{alphaMax - alphaMin}{d3 - d2}
   :label: illinoisk4

Other parameters include da (the current average queueing delay), and
Ta (the average RTT, calculated as sumRtt / cntRtt in the implementation) and
Tmin (baseRtt in the implementation) which is the minimum RTT ever seen.
dm is the maximum (average) queueing delay, and Tmax (maxRtt in the
implementation) is the maximum RTT ever seen.

.. math::   da &= Ta - Tmin
   :label: illinoisda

.. math::   dm &= Tmax - Tmin
   :label: illinoisdm

.. math::   d_i &= eta_i \cdot dm
   :label: illinoisdi

Illinois only executes its adaptation of alpha and beta when cwnd exceeds a threshold
called winThresh. Otherwise, it sets alpha and beta to the base values of 1 and 0.5,
respectively.

Following the implementation of Illinois in the Linux kernel, we use the following
default parameter settings:

* alphaMin = 0.3      (0.1 in the Illinois paper)
* alphaMax = 10.0
* betaMin = 0.125
* betaMax = 0.5
* winThresh = 15      (10 in the Illinois paper)
* theta = 5
* eta1 = 0.01
* eta2 = 0.1
* eta3 = 0.8

More information: http://www.doi.org/10.1145/1190095.1190166

H-TCP
^^^^^

H-TCP has been designed for high BDP (Bandwidth-Delay Product) paths. It is
a dual mode protocol. In normal conditions, it works like traditional TCP
with the same rate of increment and decrement for the congestion window.
However, in high BDP networks, when it finds no congestion on the path
after ``deltal`` seconds, it increases the window size based on the alpha
function in the following:

.. math::   alpha(delta)=1+10(delta-deltal)+0.5(delta-deltal)^2
   :label: htcpalpha

where ``deltal`` is a threshold in seconds for switching between the modes and
``delta`` is the elapsed time from the last congestion. During congestion,
it reduces the window size by multiplying by beta function provided
in the reference paper. The calculated throughput between the last two
consecutive congestion events is considered for beta calculation.

The transport ``TcpHtcp`` can be selected in the program
``examples/tcp/tcp-variants-comparison.cc`` to perform an experiment with H-TCP,
although it is useful to increase the bandwidth in this example (e.g.
to 20 Mb/s) to create a higher BDP link, such as

::

  ./ns3 run "tcp-variants-comparison --transport_prot=TcpHtcp --bandwidth=20Mbps --duration=10"

More information (paper): http://www.hamilton.ie/net/htcp3.pdf

More information (Internet Draft): https://tools.ietf.org/html/draft-leith-tcp-htcp-06

LEDBAT
^^^^^^

Low Extra Delay Background Transport (LEDBAT) is an experimental delay-based
congestion control algorithm that seeks to utilize the available bandwidth on
an end-to-end path while limiting the consequent increase in queueing delay
on that path. LEDBAT uses changes in one-way delay measurements to limit
congestion that the flow itself induces in the network.

As a first approximation, the LEDBAT sender operates as shown below:

On receipt of an ACK:

::
       currentdelay = acknowledgement.delay
       basedelay = min (basedelay, currentdelay)
       queuingdelay = currentdelay - basedelay
       offtarget = (TARGET - queuingdelay) / TARGET
       cWnd += GAIN * offtarget * bytesnewlyacked * MSS / cWnd

``TARGET`` is the maximum queueing delay that LEDBAT itself may introduce in the
network, and ``GAIN`` determines the rate at which the cwnd responds to changes in
queueing delay; ``offtarget`` is a normalized value representing the difference between
the measured current queueing delay and the predetermined TARGET delay. offtarget can
be positive or negative; consequently, cwnd increases or decreases in proportion to
offtarget.

Following the recommendation of RFC 6817, the default values of the parameters are:

* TargetDelay = 100
* baseHistoryLen = 10
* noiseFilterLen = 4
* Gain = 1

To enable LEDBAT on all TCP sockets, the following configuration can be used:

::

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpLedbat::GetTypeId ()));

To enable LEDBAT on a chosen TCP socket, the following configuration can be used:

::

  Config::Set ("$ns3::NodeListPriv/NodeList/1/$ns3::TcpL4Protocol/SocketType", TypeIdValue (TcpLedbat::GetTypeId ()));

The following unit tests have been written to validate the implementation of LEDBAT:

* LEDBAT should operate same as NewReno during slow start
* LEDBAT should operate same as NewReno if timestamps are disabled
* Test to validate cwnd increment in LEDBAT

In comparison to RFC 6817, the scope and limitations of the current LEDBAT
implementation are:

* It assumes that the clocks on the sender side and receiver side are synchronised
* In line with Linux implementation, the one-way delay is calculated at the sender side by using the timestamps option in TCP header
* Only the MIN function is used for noise filtering

More information about LEDBAT is available in RFC 6817: https://tools.ietf.org/html/rfc6817

TCP-LP
^^^^^^

TCP-Low Priority (TCP-LP) is a delay based congestion control protocol in which the low
priority data utilizes only the excess bandwidth available on an end-to-end path.
TCP-LP uses one way delay measurements as an indicator of congestion as it does
not influence cross-traffic in the reverse direction.

On receipt of an ACK:

.. math::

  One way delay = Receiver timestamp - Receiver timestamp echo reply
  Smoothed one way delay = 7/8 * Old Smoothed one way delay + 1/8 * one way delay
  If smoothed one way delay > owdMin + 15 * (owdMax - owdMin) / 100
    if LP_WITHIN_INF
      cwnd = 1
    else
      cwnd = cwnd / 2
    Inference timer is set

where owdMin and owdMax are the minimum and maximum one way delays experienced
throughout the connection, LP_WITHIN_INF indicates if TCP-LP is in inference
phase or not

More information (paper): http://cs.northwestern.edu/~akuzma/rice/doc/TCP-LP.pdf

Data Center TCP (DCTCP)
^^^^^^^^^^^^^^^^^^^^^^^^

DCTCP, specified in RFC 8257 and implemented in Linux, is a TCP congestion
control algorithm for data center networks.  It leverages Explicit Congestion
Notification (ECN) to provide more fine-grained congestion
feedback to the end hosts, and is intended to work with routers that
implement a shallow congestion marking threshold (on the order of a
few milliseconds) to achieve high throughput and low latency in the
datacenter.  However, because DCTCP does not react in the same way to
notification of congestion experienced, there are coexistence (fairness)
issues between it and legacy TCP congestion controllers, which is why it
is recommended to only be used in controlled networking environments such
as within data centers.

DCTCP extends the Explicit Congestion Notification signal
to estimate the fraction of bytes that encounter congestion, rather than simply
detecting that the congestion has occurred. DCTCP then scales the congestion
window based on this estimate. This approach achieves high burst tolerance, low
latency, and high throughput with shallow-buffered switches.

* *Receiver functionality:* If CE is observed in the IP header of an incoming
  packet at the TCP receiver, the receiver sends congestion notification to
  the sender by setting ECE in TCP header. This processing is different
  from standard receiver ECN processing which sets and holds the ECE bit
  for every ACK until it observes a CWR signal from the TCP sender.

* *Sender functionality:* The sender makes use of the modified receiver
  ECE semantics to maintain an estimate of the fraction of packets marked
  (:math:`\alpha`) by using the exponential weighted moving average (EWMA) as
  shown below:

.. math::

               \alpha = (1 - g) * \alpha + g * F

In the above EWMA:

* *g* is the estimation gain (between 0 and 1)
* *F* is the fraction of packets marked in current RTT.

For send windows in which at least one ACK was received with ECE set,
the sender should respond by reducing the congestion
window as follows, once for every window of data:

.. math::

               cwnd = cwnd * (1 - \alpha / 2)

Following the recommendation of RFC 8257, the default values of the parameters are:

.. math::

  g = 0.0625 (i.e., 1/16)

  initial alpha (\alpha) = 1


To enable DCTCP on all TCP sockets, the following configuration can be used:

::

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpDctcp::GetTypeId ()));

To enable DCTCP on a selected node, one can set the "SocketType" attribute
on the TcpL4Protocol object of that node to the TcpDctcp TypeId.

The ECN is enabled automatically when DCTCP is used, even if the user
has not explicitly enabled it.

DCTCP depends on a simple queue management algorithm in routers / switches to
mark packets. The current implementation of DCTCP in ns-3 can use RED with
a simple
configuration to achieve the behavior of desired queue management algorithm.

To configure RED router for DCTCP:

::

  Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (1.0));
  Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (16));
  Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (16));

There is also the option, when running CoDel or FqCoDel, to enable ECN
on the queue and to set the "CeThreshold" value to a low value such as 1ms.
The following example uses CoDel:

::

  Config::SetDefault ("ns3::CoDelQueueDisc::UseEcn", BooleanValue (true));
  Config::SetDefault ("ns3::CoDelQueueDisc::CeThreshold", TimeValue (MilliSeconds (1)));

The following unit tests have been written to validate the implementation of DCTCP:

* ECT flags should be set for SYN, SYN+ACK, ACK and data packets for DCTCP traffic
* ECT flags should not be set for SYN, SYN+ACK and pure ACK packets, but should be set on data packets for ECN enabled traditional TCP flows
* ECE should be set only when CE flags are received at receiver and even if sender doesn’t send CWR, receiver should not send ECE if it doesn’t receive packets with CE flags

An example program, ``examples/tcp/tcp-validation.cc``, can be used to
experiment with DCTCP for long-running flows with different bottleneck
link bandwidth, base RTTs, and queuing disciplines.  A variant of this
program has also been run using the |ns3| Direct Code Execution
environment using DCTCP from Linux kernel 4.4, and the results were
compared against |ns3| results.

An example program based on an experimental topology found in the original
DCTCP SIGCOMM paper is provided in ``examples/tcp/dctcp-example.cc``.
This example uses a simple topology consisting of forty DCTCP senders
and receivers and two ECN-enabled switches to examine throughput,
fairness, and queue delay properties of the network.

This implementation was tested extensively against a version of DCTCP in
the Linux kernel version 4.4 using the ns-3 direct code execution (DCE)
environment. Some differences were noted:

* Linux maintains its congestion window in segments and not bytes, and
  the arithmetic is not floating point, so small differences in the
  evolution of congestion window have been observed.
* Linux uses pacing, where packets to be sent are paced out at regular
  intervals. However, if at any instant the number of segments that can
  be sent are less than two, Linux does not pace them and instead sends
  them back-to-back. Currently, ns-3 paces out all packets eligible to
  be sent in the same manner.

More information about DCTCP is available in the RFC 8257:
https://tools.ietf.org/html/rfc8257

BBR
^^^
BBR (class :cpp:class:`TcpBbr`) is a congestion control algorithm that
regulates the sending rate by deriving an estimate of the bottleneck's
available bandwidth and RTT of the path. It seeks to operate at an optimal
point where sender experiences maximum delivery rate with minimum RTT. It
creates a network model comprising maximum delivery rate with minimum RTT
observed so far, and then estimates BDP (maximum bandwidth * minimum RTT)
to control the maximum amount of inflight data. BBR controls congestion by
limiting the rate at which packets are sent. It caps the cwnd to one BDP
and paces out packets at a rate which is adjusted based on the latest estimate
of delivery rate. BBR algorithm is agnostic to packet losses and ECN marks.

pacing_gain controls the rate of sending data and cwnd_gain controls the amount
of data to send.

The following is a high level overview of BBR congestion control algorithm:

On receiving an ACK:
    rtt = now - packet.sent_time
    update_minimum_rtt (rtt)
    delivery_rate = estimate_delivery_rate (packet)
    update_maximum_bandwidth (delivery_rate)

After transmitting a data packet:
    bdp = max_bandwidth * min_rtt
    if (cwnd * bdp < inflight)
      return
    if (now > nextSendTime)
      {
        transmit (packet)
        nextSendTime = now + packet.size / (pacing_gain * max_bandwidth)
      }
    else
      return
    Schedule (nextSendTime, Send)

To enable BBR on all TCP sockets, the following configuration can be used:

::

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpBbr::GetTypeId ()));

To enable BBR on a chosen TCP socket, the following configuration can be used
(note that an appropriate Node ID must be used instead of 1):

::

  Config::Set ("$ns3::NodeListPriv/NodeList/1/$ns3::TcpL4Protocol/SocketType", TypeIdValue (TcpBbr::GetTypeId ()));

The ns-3 implementation of BBR is based on its Linux implementation. Linux 5.4
kernel implementation has been used to validate the behavior of ns-3
implementation of BBR (See below section on Validation).

In addition, the following unit tests have been written to validate the
implementation of BBR in ns-3:

* BBR should enable (if not already done) TCP pacing feature.
* Test to validate the values of pacing_gain and cwnd_gain in different phases
of BBR.

An example program, examples/tcp/tcp-bbr-example.cc, is provided to experiment
with BBR for one long running flow. This example uses a simple topology
consisting of one sender, one receiver and two routers to examine congestion
window, throughput and queue control. A program similar to this has been run
using the Network Stack Tester (NeST) using BBR from Linux kernel 5.4, and the
results were compared against ns-3 results.

More information about BBR is available in the following Internet Draft:
https://tools.ietf.org/html/draft-cardwell-iccrg-bbr-congestion-control-00

More information about Delivery Rate Estimation is in the following draft:
https://tools.ietf.org/html/draft-cheng-iccrg-delivery-rate-estimation-00

For an academic peer-reviewed paper on the BBR implementation in ns-3,
please refer to:

* Vivek Jain, Viyom Mittal and Mohit P. Tahiliani. "Design and Implementation of TCP BBR in ns-3." In Proceedings of the 10th Workshop on ns-3, pp. 16-22. 2018. (https://dl.acm.org/doi/abs/10.1145/3199902.3199911)
