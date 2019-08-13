.. include:: replace.txt
.. highlight:: cpp

DCell topology
--------------

DCell topolgy is a scalable, recursively defined data center network architecture.
In DCell, a server is connected to several other servers and a mini-switch via bidirectional links. 
A high-level DCell is constructed from low-level DCells.
DCell0 is the building block to construct larger DCells. It has n servers and a mini-switch. 
All servers in DCell0 are connected to the mini-switch.
DCell1 is constructed using n + 1 DCell0s. In DCell1, each DCell0 is connected to all the other DCell0s with one link.
In DCell1, each DCell0, if treated as a virtual node, is fully connected with every other virtual node to form a
complete graph.
For a level 2 or higher DCellk, it is constructed in the same way to the above DCell1 construction.
Therefore, the number of level k-1 DCell in a DCellk, (i.e., g_{k}), and the total number of servers in a DCellk (i.e.,
t_{k}) follow the following equations:
g_{k} = t_{k-1} + 1;
t_{k} = g_{k} * t_{k-1}
for k > 0. DCell0 is a special case when g_{0} = 1 and t_{0} = n, with n being the number of servers in a DCell0.

.. figure:: figures/dcell-animation.*
   :align: center
   :width: 500px
   :height: 400px

   An Example of DCell Topology with n=4, level k=1

Using the DCell
----------------------------

The DcellHelper object can be instantiated by following statement.
  DcellHelper dcell (numLevels, numServers);
  where,
  numLevels is the number of levels (k) and numServers is the number of servers in each DCell0

Examples
========

The DCell topology example could be found at ``src/netanim/examples/dcell-animation.cc``.

.. sourcecode:: bash

   $ ./waf configure --enable-examples
   $ NS_LOG="DCellAnimation" ./waf --run "dcell-animation"

References
**********

Link to the Paper: http://www.sigcomm.org/sites/default/files/ccr/papers/2008/October/1402946-1402968.pdf