.. include:: replace.txt
.. highlight:: cpp


MIPv6
#####



Model Description
******************

The source code for the new model lives in the directory ``src/mipv6``.

The MIPv6 class diagram is illustrated in figure :ref:`fig-mipv6-class`. The white boxes
represent the classes which already exist in |ns3| and the classes with colored boxes are
newly defined to implement MIPv6. These classes are divided into four models:


.. _fig-mipv6-class:
   
.. figure:: figures/mipv6-class.*
   :align: center

   MIPv6 class digram


header
------
In figure :ref:`fig-header`, the main data members of MIPv6 header class are shown.
As the type field of the options are used by the MIPv6 mobility messages, the existing |ns3|
Ipv6OptionHeader cannot be extended for the MIPv6 specific option headers.

Mobility header (BU, BA,HoT etc.) class inherits the Mipv6OptionField classes too which
manages all the options available in the MIPv6 header and also performs the necessary padding
to ensure that the whole header remains a multiple of 8 octets. The padding mechanism uses
CalculatePad() method with the help of GetAlignment() method in Mipv6OptionHeader, when
Addoption() method includes an additional new option with a header. The GetAlignment() function
returns information about padding rule (RFC 6275).

Using these classes, it is easy to create a mobility message or, insert an option header in a packet. For
example, to create a BU message, containing an option of AlternateCoA, the Packet class is
instantiated and the mobility header is inserted using the AddHeader() method. The mobility header
contains the main MIPv6BUHeader class instance. To Create an AlternetCoA class instance, the
AddOption() method of the header class is called to insert the option within the BU header. Thus the
header instantiation is completed and the mobility message is formed.

.. _fig-header:
   
.. figure:: figures/mipv6-header.*
   :align: center

   MIPv6 header class diagram


internetstack, netdevice
------------------------
Mipv6Demux class is implemented to handle mobility message classes (MIPv6BU, MIPv6BA etc.) which
are inherited from Mipv6Mobility class. The functionality of Mipv6Demux class is similar as the
IPv6OptionDemux class which de-multiplexes IPv6 options. The recognition and forwarding of mobility
messages with the mobility header is done by the existing lower layer IPv6 class (Ipv6L3Protocol).
In MIPv6 layer, the mobility message specific operation starts after the recognition made by the
Mipv6Demux class. It defines three functions:

* InsertMobility()

* GetMobility() and

* RemoveMobility()

At the time of MIPv6 installation to an entity (MN, HA and CN), Mipv6Demux class inserts all
required mobility types (BU, BA, HoTI, CoTI, HoT and CoT) for that node by calling InsertMobility()
function. It is done in advance such that whenever it has to send/receive such messages, it can
recognize it through GetMobility() function and handle it appropriately. If there is a need to define
new mobility message (e.g., BU, BA), one must create a subclass (e.g. MIPv6BU, MIPv6BA) of Mipv6Mobility
class and insert it into Mipv6Demux class. Alongside, Mipv6Mobility declares two abstract methods which
are implemented by its sub classes:

* GetMobilityNumber() and

* Process()

According to RFC 6275, each mobility message contains a mobility number, which is returned by the
GetMobilityNumber() function. If a mobility message is received from the lower layer, it is processed
by the Process() function. It checks whether all fields of the message is correct or, not. If it finds
all the fields are meaningful to process further, it calls the appropriate Receive() function of the node
(i.e., MN, HA, and CN) to handle it. For example, the Receive() function of MIPv6HA is called to send a BA
message to the MN.

After the implementation of the above header classes, MIPv6 layer specific classes and routing
classes, we implement the agent based classes:

* Mipv6Mn and

* Mipv6Ha/CN

All of them are derived from the base Mipv6Agent class, which implements two main methods:

* Receive() and

* HandleXX()

The Receive() function is called by the Process() function of Mipv6Mobility class, after receiving a
message from the Mipv6L4Protocol class. After receiving such mobility message, it calls the proper
handler function such as HandleBU(), HandleBA() etc. These handler functions actually handle the
mobility messages and trigger the next event. For example, after receiving BU message, HandleBU()
forms a BA message and sends it. However Mipv6Agent class has not defined the handler classes as
its function is specific to the agent classes.

Attributes
**********

* Mipv6Demux::Mobilities->The set of IPv6 Mobilities registered with this demux

Output
******

The following traceSources are used in this model

* Mipv6Agent::AgentTx->Trace source indicating a transmitted mobility handling
  packets (BU, BA etc.) by this agent
* Mipv6Agent::AgentPromiscRx->Trace source indicating a received mobility handling
  packets by this agent. This is a promiscuous trace
* Mipv6Agent::AgentRx->Trace source indicating a received mobility handling packets
  by this agent. This is a non-promiscuous trace