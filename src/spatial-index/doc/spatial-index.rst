.. include:: replace.txt
.. highlight:: cpp

.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)
.. _spatial-index:

Spatial Index
----------------------------

The Spatial Index Module stores the positions of mobile nodes in a
spatial index, enabling quick spatial queries.  This capability
is used in conjunction with the Position Aware Module to enable "clipping"
where receive events are only scheduled on nodes within some given range
of a transmitting node.  However, this functionality may be useful in other
contexts as well.

Model Description
*****************

The source code for the new module lives in the directory ``src/spatial-index``.

Design
======

The base class (SpatialIndexing) can be inherited by various spatial
indexing implementations.  Currently the k-d-tree and brute force implementations
are provided.  The brute force implementation is only for comparison and
can be used to demonstrate the benefit of using a spatial index.

Nodes can be added, removed from, and updated in the spatial index, and each
implementation provides methods for these.  When called, the getNodesInRange()
method returns a vector of pointers to Nodes that are within the given range of
the given position.  Since nodes are stored in a spatial index, this is an
efficient operation.

References
==========

For information on k-d trees and spatial indexing see:
* Bentley, J. L. (1975). "Multidimensional binary search trees used for associative searching". Communications of the ACM. 18 (9): 509–517. doi:10.1145/361002.361007
* https://gistbok.ucgis.org/bok-topics/spatial-indexing

For details on the theory behind this implementation see:
* A. Ganti, U. Onunkwo, B. Van Leeuwen, M. P. Scoggin and R. Schroeppel, "A Novel Approach to Exponential Speedup of Simulation Events in Wireless Networks," 2018 International Conference on Computing, Networking and Communications (ICNC), Maui, HI, 2018, pp. 682-688, doi: 10.1109/ICCNC.2018.8390357.

Dependencies
===================

This Module uses an implementation of k-d tree included in the module source.

Examples
========

Two examples of using spatial-index for clipping are provided:
* clipping-example simulates a grid of nodes with each node will sending a
  UDP packet to the broadcast address over a Spectrum Wifi Channel.  The
  distances between nodes have been strategically set such that the
  wifi packets will only successfully propagate to direct neighbors
  (not diagonal).  With clipping enabled simulation time is drastically reduced
  as receive events are only placed on the queue for nodes within the
  chosen clipping range, yielding the same results in much less time.
* mobile-clipping simulates nodes with their starting positions on a similar
  grid.  The nodes are mobile, however, and select a random direction of
  travel every 15 seconds and travel in that direction at 5 m/s.
  During the simulation Each node will send a UDP packet to the
  broadcast address over a Wifi Channel.  The identical motions are repeated
  with and without clipping enabled, and the speedup using clipping and
  fidelity are computed.  With clipping enabled the simulation time is
  drastically reduced as receive events are only placed on the queue for nodes
  within the clipping range, yielding nearly the same, if not identical results
  in much less time.
