Nix-Vector Routing Documentation
--------------------------------

.. include:: replace.txt
.. highlight:: cpp

.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)


Nix-vector routing is a simulation specific routing protocol and is 
intended for large network topologies.  The on-demand nature of this 
protocol as well as the low-memory footprint of the nix-vector provides 
improved performance in terms of memory usage and simulation run time 
when dealing with a large number of nodes.

Model Description
*****************

The source code for the NixVectorRouting module lives in
the directory ``src/nix-vector-routing``.

|ns3| nix-vector-routing performs on-demand route computation using 
a breadth-first search and an efficient route-storage data structure 
known as a nix-vector.

When a packet is generated at a node for transmission, the route is 
calculated, and the nix-vector is built. 
The nix-vector stores an index for each hop along the path, which 
corresponds to the neighbor-index.  This index is used to determine 
which net-device and gateway should be used.  To route a packet, the 
nix-vector must be transmitted with the packet. At each hop, the 
current node extracts the appropriate neighbor-index from the 
nix-vector and transmits the packet through the corresponding 
net-device.  This continues until the packet reaches the destination.

|ns3| supports IPv4 as well as IPv6 Nix-Vector routing.

Scope and Limitations
=====================

Currently, the |ns3| model of nix-vector routing supports IPv4 p2p links 
as well as CSMA links.  It does not (yet) provide support for 
efficient adaptation to link failures.  It simply flushes all nix-vector 
routing caches.

NixVectorRouting bases its routing decisions on the nodes addresses,
and it does **not** check that the nodes are in the proper subnets.
In other terms, using NixVectorRouting you could have an (apparently)
working network that violates every good practice in IP address assignments.

In case of IPv6, Nix assumes the link-local addresses assigned are **unique**.
When using the IPv6 stack, the link-local address allocation is unique by
default over the entire topology. However, if the link-local addresses are
assigned manually, this care should be taken of.


Usage
*****

The usage pattern is the one of all the Internet routing protocols.
Since NixVectorRouting is not installed by default in the 
Internet stack, it is necessary to set it in the Internet Stack 
helper by using ``InternetStackHelper::SetRoutingHelper``.

Remember to include the header file ``ns3/nix-vector-helper.h`` to
use IPv4 or IPv6 Nix-Vector routing.

.. note::
   The previous header file ``ns3/ipv4-nix-vector-helper.h`` is
   maintained for backward compatibility reasons. Therefore, the
   existing IPv4 Nix-Vector routing simulations should work fine.

*  Using IPv4 Nix-Vector Routing:

.. code-block:: c++

   Ipv4NixVectorHelper nixRouting;
   InternetStackHelper stack;
   stack.SetRoutingHelper (nixRouting);  // has effect on the next Install ()
   stack.Install (allNodes);             // allNodes is the NodeContainer

*  Using IPv6 Nix-Vector Routing:

.. code-block:: c++

   Ipv6NixVectorHelper nixRouting;
   InternetStackHelper stack;
   stack.SetRoutingHelper (nixRouting);  // has effect on the next Install ()
   stack.Install (allNodes);             // allNodes is the NodeContainer

.. note::
   The NixVectorHelper helper class helps to use NixVectorRouting functionality.
   The NixVectorRouting model can also be used directly to use Nix-Vector routing.
   To use it directly, the header file ``ns3/nix-vector-routing.h`` should be
   included. The previous header file ``ns3/ipv4-nix-vector-routing.h`` is maintained
   for backwards compatibility with any existing IPv4 Nix-Vector routing simulations.

Examples
========

The examples for the NixVectorRouting module lives in
the directory ``src/nix-vector-routing/examples``.

There are examples which use both IPv4 and IPv6 networking.

*  nix-simple.cc

   #. Using IPv4:
   .. code-block:: bash

      # By default IPv4 network is selected
      ./waf --run src/nix-vector-routing/examples/nix-simple
      # Alternatively, setting ip as "v4" explicitly also works
      ./waf --run "src/nix-vector-routing/examples/nix-simple --ip=v4"

   #. Using IPv6:
   .. code-block:: bash

      # Set ip as "v6" explicity
      ./waf --run "src/nix-vector-routing/examples/nix-simple --ip=v6"

* nms-p2p-nix.cc

This example demonstrates the advantage of Nix-Vector routing as Nix
performs source-based routing (BFS) to have faster routing.

   #. Using IPv4:
   .. code-block:: bash

      # By default IPv4 network is selected
      ./waf --run src/nix-vector-routing/examples/nms-p2p-nix
      # Alternatively, setting ip as "v4" explicitly also works
      ./waf --run "src/nix-vector-routing/examples/nms-p2p-nix --ip=v4"

   #. Using IPv6:
   .. code-block:: bash

      # Set ip as "v6" explicity
      ./waf --run "src/nix-vector-routing/examples/nms-p2p-nix --ip=v6"