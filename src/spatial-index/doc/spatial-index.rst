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

Usage
======
How the information used by each call will vary by implementation.

SpatialIndexing was designed to be used with :ref:`PositionAware<position-aware>` and PositionAware is 
strictly dependent on :ref:`MobilityModel<mobility>`. While in the abstract, SpatialIndexing implementations are not
required to use PositionAware, the shipped implementation 'kd-tree' uses callbacks to automatically update.

The first step is install mobility and position aware on an object
```
    auto o=CreateObject<AnyObject>;
    MobilityHelper mobility;
    mobility.Install(o);
    PositionAwareHelper pos_aware(ns3::Seconds(10),100);
    pos_aware.Install(o);
```

The next step is to add objects to the spatial indexing using add
```
    auto o=CreateObject<AnyObject>;
    SpatialIndexImplementation si;
    Vector3D position = {0.0,0.1,2};
    si.Add(o,position);
```

Once spatialIndexing has all the information you want it to have, you can get nodes within range.
```
  auto results = si.GetNodesInRange(100.0,Vector3D(5.0,6.0,7.0),o);
```
This will return a vector that can be iterated over. we pass in a source object in the event that the particular implementation can use it. 
The range (100) and the point of reference are use to calculate the list.

If the position of an object changes, it is intended that an 'update' be performed. If you did not use PositionAware with kd-tree, these 
updates will need to be done explicitly.
```
  Vector3D newPosition = {100.0,1000.0,1.0};
  si.Update(o,newPosition);
```

If no updates are done, the intention is that the list returned by identical calls getNodesInRange should contain the same objects. 
This is not enforced.

If at any time the use wishes to remove an object from the spatial indexing system, call remove
```
  si.Remove(o);
```



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
* spatial-index-test-suite.cc provides an example of how to use spatialIndexing in it's pure form.
* SingleModelSpectrumChannel and MultiModelSpectrumChannel both implement clipping using spatial indexing
* YansWifiChannel also implements clipping using spatial indexing.