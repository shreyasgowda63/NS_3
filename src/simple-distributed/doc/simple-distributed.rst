.. include:: replace.txt
.. highlight:: cpp

SimpleDistributed NetDevice
---------------------------

Overview of the SimpleDistributed model
***************************************

The |ns3| simple distributed model is a model of a very abstract fully
connected network which enables nodes to be distributed across
processors.  A motivating use case was a desire to simulate large
wireless networks in parallel.  It can be used where you want to
achieve parallelism and can tolerate a very abstract channel model
without represenstaion of interference.  The model is similiar to
fully connecting a set of nodes with PointToPointNetChannels between
the node but avoids the instantiation of N net devices on every node.
The design is based off the SimpleNetDevice model with some added
complexity to the delay model to enable use of node positions from a
mobility model to calculate network delays and an option to limit
transmission distances.

The entire topology must represented on all ranks but only owning
rank needs to install applications and mobility models.

Model Description
*****************

The simple distributed net devices are connected via an
SimpleDistributedChannel. This channel models a fully connected
network between the attached net devices.

The delay model has a delay component that can be specified at the
sending net device and a delay component specified in the channel.
The delay model was designed to be an abstract and generic delay that can
include node location effects.  The channel model does not directly
represent any specific real network technology.

Model Attributes
****************

The SimpleDistributedNetDevice provides following Attributes:

* Delay:     An ns3::Time specifying the propagation delay for the net device.
* DataRate:  An ns3::Rate specifying the propagation delay for the net device.

The SimpleDistributedChannel provides following Attributes:

* Delay:      An ns3::Time specifying the propagation delay for the channel.
* DataRate:   An ns3::Rate specifying the propagation delay for the channel.
* DelayModel: An ns3::ChannelDelayModel to enable a user specified method of propagation delay on the channel.

The full network delay is computed using the net device and channel values::

Time totalDelay = netdevice.Delay + netdevice.Rate(packet) +
   channel.Delay + channel.Rate(packet) +
   DelayModel.ComputeDelay (packet, sourcePosition, destinationNetDevice);

Most use cases will not require all the attributes, the complexity is
present to support a wide veriety of use cases with a single numeric
model.  Delay and rate attributes not required for a specific model
should be set to 0.

Using the SimpleDistributedNetDevice
************************************

The SimpleDistributed net devices and channels are typically created
and configured using the associated ``SimpleDistributedHelper``
object. The various ns3 device helpers generally work in a similar
way, and their use is seen in many of our example programs and is also
covered in the |ns3| tutorial.

The conceptual model of interest is that of a bare computer "husk" into which
you plug net devices. The bare computers are created using a ``NodeContainer``
helper. You just ask this helper to create as many computers (we call them
``Nodes``) as you need on your network::

  NodeContainer nodes;
  nodes.Create (2);

Once you have your nodes, you need to instantiate a `` SimpleDistributedHelper`` and
set any attributes you may want to change. Note that since this is a::

  SimpleDistributedHelper simpleDistributed;
  simpleDistributed.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  simpleDistributed.SetChannelAttribute ("Delay", StringValue ("2ms"));
 
Once the attributes are set, all that remains is to create the devices
and install them on the required nodes, and to connect the devices
together using a SimpleDistributed channel. When we create the net
devices, we add them to a container to allow you to use them in the
future. This all takes just one line of code.::

  NetDeviceContainer devices = simpleDistributed.Install (nodes);

SimpleDistributed Tracing
********************

Like all |ns3| devices, the SimpleDistributed Model provides a number of trace
sources. These trace sources can be hooked using your own custom trace code, or
you can use our helper functions to arrange for tracing to be enabled on devices
you specify.

Upper-Level (MAC) Hooks
+++++++++++++++++++++++

From the point of view of tracing in the net device, there are several
interesting points to insert trace hooks. A convention inherited from other
simulators is that packets destined for transmission onto attached networks pass
through a single "transmit queue" in the net device. We provide trace hooks at
this point in packet flow, which corresponds (abstractly) only to a transition
from the network to data link layer, and call them collectively
the device MAC hooks.

When a packet is sent to the simple distributed net device for transmission it
always passes through the transmit queue. The transmit queue in the
SimpleDistributedNetDevice inherits from Queue, and therefore inherits three trace
sources:

* An Enqueue operation source (see ns3::Queue::m_traceEnqueue);
* A Dequeue operation source (see ns3::Queue::m_traceDequeue);
* A Drop operation source (see ns3::Queue::m_traceDrop).

The upper-level (MAC) trace hooks for the SimpleDistributedNetDevice are, in fact, 
exactly these three trace sources on the single transmit queue of the device.  

The m_traceEnqueue event is triggered when a packet is placed on the transmit
queue. This happens at the time that ns3::PointTtoPointNetDevice::Send or 
ns3::SimpleDistributedNetDevice::SendFrom is called by a higher layer to queue a 
packet for transmission. An Enqueue trace event firing should be interpreted
as only indicating that a higher level protocol has sent a packet to the device.

The m_traceDequeue event is triggered when a packet is removed from the transmit
queue. Dequeues from the transmit queue can happen in two situations:  1) If the
underlying channel is idle when SimpleDistributedNetDevice::Send is called, a packet
is dequeued from the transmit queue and immediately transmitted;  2) a packet
may be dequeued and immediately transmitted in an internal TransmitCompleteEvent
that functions much  like a transmit complete interrupt service routine. An
Dequeue trace event firing may be viewed as indicating that the
SimpleDistributedNetDevice has begun transmitting a packet.

The m_traceDrop event is fired when a packet cannot be enqueued on the
transmit queue because it is full. This event only fires if the queue
is full and we do not overload this event to indicate that the
SimpleDistributedChannel is "full."


Lower-Level (PHY) Hooks
+++++++++++++++++++++++

Similar to the upper level trace hooks, there are trace hooks available at the
lower levels of the net device. We call these the PHY hooks. These events fire
from the device methods that talk directly to the SimpleDistributedChannel.

The trace source m_dropTrace is called to indicate a packet that is dropped by
the device. This happens when a packet is discarded as corrupt due to a receive
error model indication (see ns3::ErrorModel and the associated attribute
"ReceiveErrorModel").

A low-level trace source also fires on reception of a packet (see
ns3::SimpleDistributedNetDevice::m_rxTrace) from the channel.

The channel also has a low level trace source m_phyRxDropTrace that is
called to indicate a packet dropped by the channel.  This happens when
a packed is discarded as corrupt do to a channel error model
indication (see ns3::ChannelErrorModel and the channel atttributed
"ErrorModel").

