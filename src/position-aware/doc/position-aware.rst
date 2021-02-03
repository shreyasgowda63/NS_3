Position Aware
----------------------------

.. include:: replace.txt
.. highlight:: cpp

.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

Position Aware module enables higher level modules to be aware of when a mobile node moves
outside a certain radius of a given reference position. This allows modules to take action only 
when the change in position is significant.

Model Description
*****************

The source code for the new module lives in the directory ``src/position-aware``.

Design
======

Given a mobility model, whenvever the velocity is updated, an event is scheduled for 
the time when the new velocity would have the object move outside the prescribed radius.
If a new velocity change occurs before that time, then the old event is cancelled and a
new one is created. If the departure takes place later than a designated 'timeout' interval
then the timeout is scheduled instead.


Dependencies
===================

This Module will error if mobility is not installed on a node

Uses
========

This module allows spatial indexing to be utitilized for clipping reception events

This module allows for more efficient simulation of geographic protoocols.