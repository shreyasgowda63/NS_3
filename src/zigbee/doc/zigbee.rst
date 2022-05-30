.. include:: replace.txt
.. highlight:: cpp


Zigbee Pro Specification
------------------------

This chapter describes the implementation of |ns3| models for Zigbee
protocol stack (Zigbee Pro) as specified by
the CSA Zigbee Specification Revision 22 1.0 (2017).


Model Description
*****************

The source code for the new module lives in the directory ``zigbee``.

The model described in the present document, represents the uncertified simulation models of Zigbee (TM)
technology to be used in |ns3| for research and educational purposes. The objective is to provide
|ns3| with a non-IP dependent layer 3 routing alternative to be used with the ``lr-wpan`` (IEEE 802.15.4-2011) module.

Design
======

.. _fig-ZigbeeStackArch:

.. figure:: figures/zigbeeStackArch.*

    Zigbee Stack Architecture in ns-3

The Zigbee stack implementation is meant to be use on top of an existing LrWpan netdevice (IEEE 802.15.4-2011 PHY & MAC stack).
The current scope of the project includes only the NWK layer in the Zigbee stack. However, the project can be later extended
to support higher layers like the Application Sublayer (APS) and the Zigbee Cluster Library (ZCL). Communication between layers
is done via "primitives". These primitives are implemented in |ns3| using callbacks.

Scope and Limitations
=====================

The current implementation focus on adding the Zigbee NWK layer to |ns3| to provide a non-IP dependent routing alternative to the lr-wpan module.
At the moment, the Zigbee Application Support Sub-Layer (APS) is not planned but future support is considered.
Likewise, higher layers such s Zigbee Device Profile (ZDP) or Zigbee Cluster Library (ZCL) are not planned at the moment.

In no particular order, the summary of goals for the current implementation are:

- [x] Implement `ZigbeeStack` to serve as an encapsulating class for easy management of Zigbee layers.
- [x] Implement `ZigbeeHelper` for easy installation and configuration of the ZigbeeStack with the underlying LrWpan MAC layer.
- [x] Creation of `ZigbeeStackContainer` used by the `ZigbeeHelper` for quick installation of ZigbeeStacks on top LrWpanNetDevices.
- [x] Implement the Zigbee network header (`ZigbeeNwkHeader`) used by all the frames passing through the NWK layer.
- [x] Implementation of Zigbee network payload headers (Currently, only RREQ and RREP command frames).
- [x] Creation of routing tables and neighbor tables to support routing discovery and route creation.
- [ ] Implement Network Layer Data Entity (NLDE) and the Network Layer Management Entity (NLME) primitives:
  - [x] Implementation of NLME-NETWORK-DISCOVERY primitives (Request, Confirm)
  - [ ] Implementation of NLME-ROUTE-DISCOVERY primitives (Request, Confirm)
  - [x] Implementation of NLME-NETWORK-FORMATION primitives (Request, Confirm)
  - [ ] Implementation of NLME-ED-SCAN primitives (Request, Confirm)
  - [x] Implementation of NLME-JOIN primitives (Request, Confirm, Indication)
       * [x] Using MAC association procedure
       * [x] Using the orphaning procedure
  - [x] Implementation of NLME-DIRECT-JOIN primitives (Request, Confirm)
  - [x] Implementation of NLME-START-ROUTER primitives (Request, Confirm)
  - [ ] Implementation of NLDE-DATA primitives (Request, Confirm, Indication)
- [ ] Examples of model use and test NWK routing capabilities (UNICAST)

Not considered for the initial implementation of the module:
- Multicast support of any kind.
- Security.
- Zigbee Application Support Sub-Layer (APS)
- Zigbee Cluster Library (ZCL)
- Zigbee Device Object (ZDO)
- Application layer
- Zigbee Stack profile 0x01 (Tree topology and distributed address assignment)

References
==========

* Zigbee Pro Specification 2017, https://csa-iot.org/developer-resource/specifications-download-request/

Usage
*****

Enabling Zigbee
================

Add ``zigbee`` to the list of modules built with |ns3|.


Helpers
=======

The model includes a ``ZigbeeHelper`` used to quickly configure and install the NWK layer
on top of an existing Lr-wpan MAC layer. In essence, ``ZigbeeHelper`` creates a  ``ZigbeeNwk`` object
wraps it in a ``ZigbeeStack`` and connects this stack to another existing
``LrwpanNetDevice``. All the necessary callback hooks are taken care of to ensure communication between
the Zigbee NWK layer and the Lr-wpan MAC layer is possible.


Examples
========

* ``zigbee-direct-join.cc``:  A simple example showing the NWK join process of devices using the orphaning procedure (direct join).
* ``zigbee-association-join.cc``:  A simple example showing the NWK join process of 3 devices in a zigbee network (MAC association).

Troubleshooting
===============

Zigbee stack is designed to work on top of IEEE 802.15.4-2011. In ns-3, this means that the zigbee module will only work on top of a correctly
installed and configured lr-wpan module.

Validation
**********

No formal validation has been done.
