.. include:: replace.txt
.. highlight:: cpp

TCP models in ns-3
------------------

This chapter describes the TCP models available in |ns3|.

Overview of support for TCP
***************************

|ns3| was written to support multiple TCP implementations. The implementations
inherit from a few common header classes in the ``src/network`` directory, so that
user code can swap out implementations with minimal changes to the scripts.

There are three important abstract base classes:

* class :cpp:class:`TcpSocket`: This is defined in
  ``src/internet/model/tcp-socket.{cc,h}``. This class exists for hosting TcpSocket
  attributes that can be reused across different implementations. For instance,
  the attribute ``InitialCwnd`` can be used for any of the implementations
  that derive from class :cpp:class:`TcpSocket`.
* class :cpp:class:`TcpSocketFactory`: This is used by the layer-4 protocol
  instance to create TCP sockets of the right type.
* class :cpp:class:`TcpCongestionOps`: This supports different variants of
  congestion control-- a key topic of simulation-based TCP research.

There are presently two active implementations of TCP available for |ns3|.

* a natively implemented TCP for ns-3
* support for kernel implementations via `Direct Code Execution (DCE) <https://www.nsnam.org/overview/projects/direct-code-execution/>`__

Direct Code Execution is limited in its support for newer kernels; at
present, only Linux kernel 4.4 is supported.  However, the TCP implementations
in kernel 4.4 can still be used for ns-3 validation or for specialized
simulation use cases.

It should also be mentioned that various ways of combining virtual machines
with |ns3| makes available also some additional TCP implementations, but
those are out of scope for this chapter.

ns-3 TCP
********

In brief, the native |ns3| TCP model supports a full bidirectional TCP with
connection setup and close logic. Several congestion control algorithms
are supported, with CUBIC the default, and NewReno, Westwood, Hybla, HighSpeed,
Vegas, Scalable, Veno, Binary Increase Congestion Control (BIC), Yet Another
HighSpeed TCP (YeAH), Illinois, H-TCP, Low Extra Delay Background Transport
(LEDBAT), TCP Low Priority (TCP-LP), Data Center TCP (DCTCP) and Bottleneck
Bandwidth and RTT (BBR) also supported. The model also supports Selective
Acknowledgements (SACK), Proportional Rate Reduction (PRR) and Explicit
Congestion Notification (ECN). Multipath-TCP is not yet supported in the |ns3|
releases.

Model history
+++++++++++++

Until the ns-3.10 release, |ns3| contained a port of the TCP model from `GTNetS
<http://www.ece.gatech.edu/research/labs/MANIACS/GTNetS/index.html>`_,
developed initially by George Riley and ported to |ns3| by Raj Bhattacharjea.
This implementation was substantially rewritten by Adriam Tam for ns-3.10.
In 2015, the TCP module was redesigned in order to create a better
environment for creating and carrying out automated tests. One of the main
changes involves congestion control algorithms, and how they are implemented.

Before the ns-3.25 release, a congestion control was considered as a stand-alone TCP
through an inheritance relation: each congestion control (e.g. TcpNewReno) was
a subclass of TcpSocketBase, reimplementing some inherited methods. The
architecture was redone to avoid this inheritance,
by making each congestion control a separate class, and defining an interface
to exchange important data between TcpSocketBase and the congestion modules.
The Linux ``tcp_congestion_ops`` interface was used as the design reference.

Along with congestion control, Fast Retransmit and Fast Recovery algorithms
have been modified; in previous releases, these algorithms were delegated to
TcpSocketBase subclasses. Starting from ns-3.25, they have been merged inside
TcpSocketBase. In future releases, they can be extracted as separate modules,
following the congestion control design.

As of the ns-3.31 release, the default initial window was set to 10 segments
(in previous releases, it was set to 1 segment).  This aligns with current
Linux default, and is discussed further in :rfc:`6928`.

In the ns-3.32 release, the default recovery algorithm was set to
Proportional Rate Reduction (PRR) from the classic ack-clocked Fast
Recovery algorithm.

In the ns-3.34 release, the default congestion control algorithm was set
to CUBIC from NewReno.

Acknowledgments
+++++++++++++++

As mentioned above, |ns3| TCP has had multiple authors and maintainers over
the years. Several publications exist on aspects of |ns3| TCP, and users
of |ns3| TCP are requested to cite one of the applicable papers when
publishing new work.

A general reference on the current architecture is found in the following paper:

* Maurizio Casoni, Natale Patriciello, Next-generation TCP for ns-3 simulator, Simulation Modelling Practice and Theory, Volume 66, 2016, Pages 81-93. (http://www.sciencedirect.com/science/article/pii/S1569190X15300939)

For an academic peer-reviewed paper on the SACK implementation in ns-3,
please refer to:

* Natale Patriciello. 2017. A SACK-based Conservative Loss Recovery Algorithm for ns-3 TCP: a Linux-inspired Proposal. In Proceedings of the Workshop on ns-3 (WNS3 '17). ACM, New York, NY, USA, 1-8. (https://dl.acm.org/citation.cfm?id=3067666)

Usage
+++++

In many cases, usage of TCP is set at the application layer by telling
the |ns3| application which kind of socket factory to use.

Using the helper functions defined in ``src/applications/helper`` and
``src/network/helper``, here is how one would create a TCP receiver::

  // Create a packet sink on the star "hub" to receive these packets
  uint16_t port = 50000;
  Address sinkLocalAddress(InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApp = sinkHelper.Install (serverNode);
  sinkApp.Start (Seconds (1.0));
  sinkApp.Stop (Seconds (10.0));

Similarly, the below snippet configures OnOffApplication traffic source to use
TCP::

  // Create the OnOff applications to send TCP to the server
  OnOffHelper clientHelper ("ns3::TcpSocketFactory", Address ());

The careful reader will note above that we have specified the TypeId of an
abstract base class :cpp:class:`TcpSocketFactory`. How does the script tell
|ns3| that it wants the native |ns3| TCP vs. some other one? Well, when
internet stacks are added to the node, the default TCP implementation that is
aggregated to the node is the |ns3| TCP.  So, by default, when using the |ns3|
helper API, the TCP that is aggregated to nodes with an Internet stack is the
native |ns3| TCP.

To configure behavior of TCP, a number of parameters are exported through the
|ns3| attribute system. These are documented in the `Doxygen
<http://www.nsnam.org/doxygen/classns3_1_1_tcp_socket.html>`_ for class
:cpp:class:`TcpSocket`. For example, the maximum segment size is a
settable attribute.

To set the default socket type before any internet stack-related objects are
created, one may put the following statement at the top of the simulation
program::

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));

For users who wish to have a pointer to the actual socket (so that
socket operations like Bind(), setting socket options, etc. can be
done on a per-socket basis), Tcp sockets can be created by using the
``Socket::CreateSocket()`` method. The TypeId passed to CreateSocket()
must be of type :cpp:class:`ns3::SocketFactory`, so configuring the underlying
socket type must be done by twiddling the attribute associated with the
underlying TcpL4Protocol object. The easiest way to get at this would be
through the attribute configuration system. In the below example,
the Node container "n0n1" is accessed to get the zeroth element, and a socket is
created on this node::

  // Create and bind the socket...
  TypeId tid = TypeId::LookupByName ("ns3::TcpNewReno");
  Config::Set ("/NodeList/*/$ns3::TcpL4Protocol/SocketType", TypeIdValue (tid));
  Ptr<Socket> localSocket =
    Socket::CreateSocket (n0n1.Get (0), TcpSocketFactory::GetTypeId ());

Above, the "*" wild card for node number is passed to the attribute
configuration system, so that all future sockets on all nodes are set to
NewReno, not just on node 'n0n1.Get (0)'. If one wants to limit it to just
the specified node, one would have to do something like::

  // Create and bind the socket...
  TypeId tid = TypeId::LookupByName ("ns3::TcpNewReno");
  std::stringstream nodeId;
  nodeId << n0n1.Get (0)->GetId ();
  std::string specificNode = "/NodeList/" + nodeId.str () + "/$ns3::TcpL4Protocol/SocketType";
  Config::Set (specificNode, TypeIdValue (tid));
  Ptr<Socket> localSocket =
    Socket::CreateSocket (n0n1.Get (0), TcpSocketFactory::GetTypeId ());

Once a TCP socket is created, one will want to follow conventional socket logic
and either connect() and send() (for a TCP client) or bind(), listen(), and
accept() (for a TCP server).
Please note that applications usually create the sockets they use automatically,
and so is not straightforward to connect directly to them using pointers. Please
refer to the source code of your preferred application to discover how and when
it creates the socket.

TCP Socket interaction and interface with Application layer
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In the following there is an analysis on the public interface of the TCP socket,
and how it can be used to interact with the socket itself. An analysis of the
callback fired by the socket is also carried out. Please note that, for
the sake of clarity, we will use the terminology "Sender" and "Receiver" to clearly
divide the functionality of the socket. However, in TCP these two roles can be
applied at the same time (i.e. a socket could be a sender and a receiver at the
same time): our distinction does not lose generality, since the following
definition can be applied to both sockets in case of full-duplex mode.

----------

**TCP state machine (for commodity use)**

.. _fig-tcp-state-machine:

.. figure:: figures/tcp-state-machine.*
   :align: center

   TCP State machine

In ns-3 we are fully compliant with the state machine depicted in
Figure :ref:`fig-tcp-state-machine`.

----------

**Public interface for receivers (e.g. servers receiving data)**

*Bind()*
  Bind the socket to an address, or to a general endpoint. A general endpoint
  is an endpoint with an ephemeral port allocation (that is, a random port
  allocation) on the 0.0.0.0 IP address. For instance, in current applications,
  data senders usually binds automatically after a *Connect()* over a random
  port. Consequently, the connection will start from this random port towards
  the well-defined port of the receiver. The IP 0.0.0.0 is then translated by
  lower layers into the real IP of the device.

*Bind6()*
  Same as *Bind()*, but for IPv6.

*BindToNetDevice()*
  Bind the socket to the specified NetDevice, creating a general endpoint.

*Listen()*
  Listen on the endpoint for an incoming connection. Please note that this
  function can be called only in the TCP CLOSED state, and transit in the
  LISTEN state. When an incoming request for connection is detected (i.e. the
  other peer invoked *Connect()*) the application will be signaled with the
  callback *NotifyConnectionRequest* (set in *SetAcceptCallback()* beforehand).
  If the connection is accepted (the default behavior, when the associated
  callback is a null one) the Socket will fork itself, i.e. a new socket is
  created to handle the incoming data/connection, in the state SYN_RCVD. Please
  note that this newly created socket is not connected anymore to the callbacks
  on the "father" socket (e.g. DataSent, Recv); the pointer of the newly
  created socket is provided in the Callback *NotifyNewConnectionCreated* (set
  beforehand in *SetAcceptCallback*), and should be used to connect new
  callbacks to interesting events (e.g. Recv callback). After receiving the ACK
  of the SYN-ACK, the socket will set the congestion control, move into
  ESTABLISHED state, and then notify the application with
  *NotifyNewConnectionCreated*.

*ShutdownSend()*
  Signal a termination of send, or in other words prevents data from being added
  to the buffer. After this call, if buffer is already empty, the socket
  will send a FIN, otherwise FIN will go when buffer empties. Please note
  that this is useful only for modeling "Sink" applications. If you have
  data to transmit, please refer to the *Send()* / *Close()* combination of
  API.

*GetRxAvailable()*
  Get the amount of data that could be returned by the Socket in one or multiple
  call to Recv or RecvFrom. Please use the Attribute system to configure the
  maximum available space on the receiver buffer (property "RcvBufSize").

*Recv()*
  Grab data from the TCP socket. Please remember that TCP is a stream socket,
  and it is allowed to concatenate multiple packets into bigger ones. If no data
  is present (i.e. *GetRxAvailable* returns 0) an empty packet is returned.
  Set the callback *RecvCallback* through *SetRecvCallback()* in order to have
  the application automatically notified when some data is ready to be read.
  It's important to connect that callback to the newly created socket in case
  of forks.

*RecvFrom()*
  Same as Recv, but with the source address as parameter.

-------------------

**Public interface for senders (e.g. clients uploading data)**

*Connect()*
  Set the remote endpoint, and try to connect to it. The local endpoint should
  be set before this call, or otherwise an ephemeral one will be created. The
  TCP then will be in the SYN_SENT state. If a SYN-ACK is received, the TCP will
  setup the congestion control, and then call the callback
  *ConnectionSucceeded*.

*GetTxAvailable()*
  Return the amount of data that can be stored in the TCP Tx buffer. Set this
  property through the Attribute system ("SndBufSize").

*Send()*
  Send the data into the TCP Tx buffer. From there, the TCP rules will decide
  if, and when, this data will be transmitted. Please note that, if the tx
  buffer has enough data to fill the congestion (or the receiver) window, dynamically
  varying the rate at which data is injected in the TCP buffer does not have any
  noticeable effect on the amount of data transmitted on the wire, that will
  continue to be decided by the TCP rules.

*SendTo()*
  Same as *Send()*.

*Close()*
  Terminate the local side of the connection, by sending a FIN (after all data
  in the tx buffer has been transmitted). This does not prevent the socket in
  receiving data, and employing retransmit mechanism if losses are detected. If
  the application calls *Close()* with unread data in its rx buffer, the socket
  will send a reset. If the socket is in the state SYN_SENT, CLOSING, LISTEN,
  FIN_WAIT_2, or LAST_ACK, after that call the application will be notified with
  *NotifyNormalClose()*. In other cases, the notification is delayed
  (see *NotifyNormalClose()*).

-----------------------------------------

**Public callbacks**

These callbacks are called by the TCP socket to notify the application of
interesting events. We will refer to these with the protected name used in
socket.h, but we will provide the API function to set the pointers to these
callback as well.

*NotifyConnectionSucceeded*: *SetConnectCallback*, 1st argument
  Called in the SYN_SENT state, before moving to ESTABLISHED. In other words, we
  have sent the SYN, and we received the SYN-ACK: the socket prepares the
  sequence numbers, sends the ACK for the SYN-ACK, tries to send out more data (in
  another segment) and then invokes this callback. After this callback, it
  invokes the NotifySend callback.

*NotifyConnectionFailed*: *SetConnectCallback*, 2nd argument
  Called after the SYN retransmission count goes to 0. SYN packet is lost
  multiple times, and the socket gives up.

*NotifyNormalClose*: *SetCloseCallbacks*, 1st argument
  A normal close is invoked. A rare case is when we receive an RST segment (or a
  segment with bad flags) in normal states. All other cases are:
  - The application tries to *Connect()* over an already connected socket
  - Received an ACK for the FIN sent, with or without the FIN bit set (we are in LAST_ACK)
  - The socket reaches the maximum amount of retries in retransmitting the SYN (*)
  - We receive a timeout in the LAST_ACK state
  - Upon entering the TIME_WAIT state, before waiting the 2*Maximum Segment Lifetime seconds to finally deallocate the socket.

*NotifyErrorClose*: *SetCloseCallbacks*, 2nd argument
  Invoked when we send an RST segment (for whatever reason) or we reached the
  maximum amount of data retries.

*NotifyConnectionRequest*: *SetAcceptCallback*, 1st argument
  Invoked in the LISTEN state, when we receive a SYN. The return value indicates
  if the socket should accept the connection (return true) or should ignore it
  (return false).

*NotifyNewConnectionCreated*: *SetAcceptCallback*, 2nd argument
  Invoked when from SYN_RCVD the socket passes to ESTABLISHED, and after setting
  up the congestion control, the sequence numbers, and processing the incoming
  ACK. If there is some space in the buffer, *NotifySend* is called shortly
  after this callback. The Socket pointer, passed with this callback, is the
  newly created socket, after a Fork().

*NotifyDataSent*: *SetDataSentCallback*
  The Socket notifies the application that some bytes have been transmitted on
  the IP level. These bytes could still be lost in the node (traffic control
  layer) or in the network.

*NotifySend*: *SetSendCallback*
  Invoked if there is some space in the tx buffer when entering the ESTABLISHED
  state (e.g. after the ACK for SYN-ACK is received), after the connection
  succeeds (e.g. after the SYN-ACK is received) and after each new ACK (i.e.
  that advances SND.UNA).

*NotifyDataRecv*: *SetRecvCallback*
  Called when in the receiver buffer there are in-order bytes, and when in
  FIN_WAIT_1 or FIN_WAIT_2 the socket receive a in-sequence FIN (that can carry
  data).