.. include:: replace.txt
.. highlight:: cpp


Zigbee
------

This chapter describes the implementation of |ns3| models for Zigbee
protocol stack (Zigbee Pro) as specified by
the CSA Zigbee Specification Revision 22 1.0 (2017).

The model described in the present document, represents the uncertified simulation models of Zigbee (TM)
technology to be used in |ns3| for research and educational purposes. The objective is to provide
|ns3| with a non-IP dependent layer 3 routing alternative to be used with the ``lr-wpan`` (IEEE 802.15.4-2011) module.


The source code for the new module lives in the directory ``src\zigbee``.

.. _fig-ZigbeeStackArch:

.. figure:: figures/zigbeeStackArch.*

    Zigbee Stack Architecture in ns-3

The Zigbee stack implementation is meant to be use on top of an existing |ns3| ``LrWpanNetDevice`` (IEEE 802.15.4-2011 PHY & MAC stack).
The current scope of the project includes only the NWK layer in the Zigbee stack. However, the project can be later extended
to support higher layers like the Application Sublayer (APS) and the Zigbee Cluster Library (ZCL). Communication between layers
is done via "primitives". These primitives are implemented in |ns3|  with a combination of functions and callbacks.

The following is a list of NWK primitives supported:

- NLME-NETWORK-DISCOVERY (Request, Confirm)
- NLME-ROUTE-DISCOVERY (Request, Confirm)
- NLME-NETWORK-FORMATION (Request, Confirm)
- NLME-JOIN (Request, Confirm, Indication)
- NLME-DIRECT-JOIN primitives (Request, Confirm)
- NLME-START-ROUTER primitives (Request, Confirm)
- NLDE-DATA primitives (Request, Confirm, Indication)

The network layer (NWK)
***********************


Helpers
*******

The model includes a ``ZigbeeHelper`` used to quickly configure and install the NWK layer
on top of an existing Lr-wpan MAC layer. In essence, ``ZigbeeHelper`` creates a  ``ZigbeeNwk`` object
wraps it in a ``ZigbeeStack`` and connects this stack to another existing
``LrwpanNetDevice``. All the necessary callback hooks are taken care of to ensure communication between
the Zigbee NWK layer and the Lr-wpan MAC layer is possible.


Examples and Test
*****************

* ``zigbee-direct-join.cc``:  A simple example showing the NWK join process of devices using the orphaning procedure (direct join).
* ``zigbee-association-join.cc``:  A simple example showing the NWK join process of 3 devices in a zigbee network (MAC association).


Validation
**********

No formal validation has been done.

Scope and Limitations
*********************

- Multicast is not supported
- Security is not supported.
- Zigbee Application Support Sub-Layer (APS) is not implemented.
- Zigbee Cluster Library (ZCL) is not implemented.
- Zigbee Device Object (ZDO) is not implemented.
- Application layer is not implemented.
- Zigbee Stack profile 0x01 (Tree topology and distributed address assignment) is not supported.
- NLME-ED-SCAN primitives (Request, Confirm) not supported.
- NLME-SET and NLME-GET (Request, Confirm) not supported.

References
**********

[`1 <https://csa-iot.org/developer-resource/specifications-download-request/>`_] Zigbee Pro Specification 2017 (R22 1.0)

[`2 <https://dsr-iot.com/downloads/open-source-zigbee/>`_] DSR Z-boss stack 1.0

