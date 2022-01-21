.. include:: replace.txt
.. highlight:: cpp
.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

.. _clipping:

Clipping
-------------
Clipping is a high level optimization implemented in the YansWifiChannel, SingleModelSpectrumChannel, and MultiModelSpectrumChannel.
It is intended to be a coarse filter to reduce unnecessary scheduling of events.
The idea of clipping is reduce the number of receive events that need to be scheduled by using :ref:`spatial indexing<spatial-indexing>`.
The goal is to set to the clip range to exclude receivers where the impact of the reception would be negligible; ie dropped before 
contributing to noise. Care should be taken to set the clip range to balance performance and fidelity. When setting this clipping range, 
it is important to consider the precision of the spatial indexing set by the PositionAware module described below. A typical clip range
is around 2 to 3 times the max transmission range.


PositionAware
---------------
Clipping is dependant on :ref:`PositionAware<position-aware`. Position aware provides a way to adjust the precision with which the 
spatial indexing used by clipping tracks the position of the nodes. There is a trade off between the runtime cost of updating the 
position and the precision of the position in the spatial-indexing. 

Things to consider when setting this precision could include:
*  velocity
*  transmission range
*  complexity of movement.

The goal is to set this precision via the 'DeltaPosition' attribute on position aware such that potential receivers are included in
any queries to the clipping module. A useful starting point is the typical reliable transmission range used in the simulation.

References
==========

For details on the theory behind this implementation see:
* A. Ganti, U. Onunkwo, B. Van Leeuwen, M. P. Scoggin and R. Schroeppel, "A Novel Approach to Exponential Speedup of Simulation Events in Wireless Networks," 2018 International Conference on Computing, Networking and Communications (ICNC), Maui, HI, 2018, pp. 682-688, doi: 10.1109/ICCNC.2018.8390357.

It should be noted that the concept described in the paper is split between :ref:`SpatialIndexing<spatial-indexing>` and :ref:`PositionAware<position-aware>` modules.