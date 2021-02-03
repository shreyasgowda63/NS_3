.. include:: replace.txt
.. highlight:: cpp
.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

.. _position-aware:

Position Aware
--------------

PositionAware enables higher level modules to be aware of when a mobile node moves
outside a certain radius (delta position) of a given reference position. This allows modules to take action only 
when the change in position is significant. It effectively sets a tolerance on position to reduce unnecessary events.

Model Description
*****************

The source code for the new functionality lives in the directory ``src/mobility``.

Design
======

PositionAware is designed to be aggregated to an ns3::Object (most typically
a Node) that already has a MobilityModel object aggregated to it.

Given a mobility model, whenever the velocity is updated, an event is scheduled for 
the time when the new velocity would have the object move outside the prescribed radius.
If a new velocity change occurs before that time, then the old event is cancelled and a
new one is created. If the departure will take place later than a designated 'timeout' interval,
then the timeout is scheduled instead.


Dependencies
============

PositionAware will cause a fatal simulation error if a mobility model
was not previously aggregated to the same object.

Uses
====

PositionAware allows spatial indexing to be utilized for clipping reception events.

PositionAware allows for more efficient simulation of geographic protocols.

Utilizing
*****************
:code:
      `
      NodeContainer nodes;
      nodes.Create(NUMNODES);
      MobilityHelper mobility;
      mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
      mobility.Install(nodes);
      PositionAwareHelper pos_aware(ns3::Seconds(10),100);
      pos_aware.Install(nodes);
      `