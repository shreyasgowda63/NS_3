.. include:: replace.txt
.. highlight:: cpp

.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

Logging
-------

The |ns3| logging facility can be used to monitor or debug the progress
of simulation programs.  Logging output can be enabled by program statements
in your ``main()`` program or by the use of the ``NS_LOG`` environment variable.

Logging statements are not compiled into optimized builds of |ns3|.  To use
logging, one must build the (default) debug build of |ns3|.

The project makes no guarantee about whether logging output will remain 
the same over time.  Users are cautioned against building simulation output
frameworks on top of logging code, as the output and the way the output
is enabled may change over time. 

Overview
********

|ns3| logging statements are typically used to log various program
execution events, such as the occurrence of simulation events or the
use of a particular function.

For example, this code snippet is from ``Ipv4L3Protocol::IsDestinationAddress()``::

  if (address == iaddr.GetBroadcast ())
    {
      NS_LOG_LOGIC ("For me (interface broadcast address)");
      return true;
     }

If logging has been enabled for the ``Ipv4L3Protocol`` component at a severity
of ``LOGIC`` or above (see below about log severity), the statement
will be printed out; otherwise, it will be suppressed.

Enabling Output
===============

There are two ways that users typically control log output.  The
first is by setting the ``NS_LOG`` environment variable; e.g.:

.. sourcecode:: bash

   $ NS_LOG="*" ./waf --run first

will run the ``first`` tutorial program with all logging output.  (The
specifics of the ``NS_LOG`` format will be discussed below.)

This can be made more granular by selecting individual components:

.. sourcecode:: bash

   $ NS_LOG="Ipv4L3Protocol" ./waf --run first

The output can be further tailored with prefix options.
  
The second way to enable logging is to use explicit statements in your
program, such as in the ``first`` tutorial program::
   
   int
   main (int argc, char *argv[])
   {
     LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
     LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
     ...

(The meaning of ``LOG_LEVEL_INFO``, and other possible values,
will be discussed below.)

``NS_LOG`` Syntax
=================

The ``NS_LOG`` environment variable contains a list of log components
and options.  Log components are separated by \`:' characters:

.. sourcecode:: bash

   $ NS_LOG="<log-component>:<log-component>..."

Options for each log component are given as flags after
each log component:

.. sourcecode:: bash

   $ NS_LOG="<log-component>=<option>|<option>...:<log-component>..."

Options control the severity and level for that component,
and whether optional information should be included, such as the
simulation time, simulation node, function name, and the symbolic severity.

Log Components
==============

Generally a log component refers to a single source code ``.cc`` file,
and encompasses the entire file.

Some helpers have special methods to enable the logging of all components
in a module, spanning different compilation units, but logically grouped
together, such as the |ns3| wifi code::

   WifiHelper wifiHelper;
   wifiHelper.EnableLogComponents (); 

The ``NS_LOG`` log component wildcard \`*' will enable all components.

To see what log components are defined, any of these will work:

.. sourcecode:: bash

   $ NS_LOG="print-list" ./waf --run ...

   $ NS_LOG="foo"  # a token not matching any log-component

The first form will print the name and enabled flags for all log components
which are linked in; try it with ``scratch-simulator``.
The second form prints all registered log components,
then exit with an error.


Severity and Level Options
==========================

Individual messages belong to a single "severity class," set by the macro
creating the message.  In the example above,
``NS_LOG_LOGIC(..)`` creates the message in the ``LOG_LOGIC`` severity class.

The following severity classes are defined as ``enum`` constants:

================  =====================================
Severity Class    Meaning
================  =====================================
``LOG_NONE``      The default, no logging
``LOG_ERROR``     Serious error messages only
``LOG_WARN``      Warning messages
``LOG_DEBUG``     For use in debugging
``LOG_INFO``      Informational
``LOG_FUNCTION``  Function tracing
``LOG_LOGIC``     Control flow tracing within functions
================  =====================================

Typically one wants to see messages at a given severity class *and higher*.
This is done by defining inclusive logging "levels":

======================  ===========================================
Level                   Meaning
======================  ===========================================
``LOG_LEVEL_ERROR``     Only ``LOG_ERROR`` severity class messages.
``LOG_LEVEL_WARN``      ``LOG_WARN`` and above.
``LOG_LEVEL_DEBUG``     ``LOG_DEBUG`` and above.
``LOG_LEVEL_INFO``      ``LOG_INFO`` and above.
``LOG_LEVEL_FUNCTION``  ``LOG_FUNCTION`` and above.
``LOG_LEVEL_LOGIC``     ``LOG_LOGIC`` and above.
``LOG_LEVEL_ALL``       All severity classes.
``LOG_ALL``             Synonym for ``LOG_LEVEL_ALL``
======================  ===========================================

The severity class and level options can be given in the ``NS_LOG``
environment variable by these tokens:

============  =================
Class         Level
============  =================
``error``     ``level_error``
``warn``      ``level_warn``
``debug``     ``level_debug``
``info``      ``level_info``
``function``  ``level_function``
``logic``     ``level_logic``
..            | ``level_all``
              | ``all``
              | ``*``
============  =================

Using a severity class token enables log messages at that severity only.
For example, ``NS_LOG="*=warn"`` won't output messages with severity ``error``.
``NS_LOG="*=level_debug"`` will output messages at severity levels
``debug`` and above.

Severity classes and levels can be combined with the \`|' operator:
``NS_LOG="*=level_warn|logic"`` will output messages at severity levels
``error``, ``warn`` and ``logic``.

The ``NS_LOG`` severity level wildcard \`*' and ``all``
are synonyms for ``level_all``.

For log components merely mentioned in ``NS_LOG``

.. sourcecode:: bash

   $ NS_LOG="<log-component>:..."

the default severity is ``LOG_LEVEL_ALL``.


Prefix Options
==============

A number of prefixes can help identify
where and when a message originated, and at what severity.

The available prefix options (as ``enum`` constants) are

======================  ===========================================
Prefix Symbol           Meaning
======================  ===========================================
``LOG_PREFIX_FUNC``     Prefix the name of the calling function.
``LOG_PREFIX_TIME``     Prefix the simulation time.
``LOG_PREFIX_NODE``     Prefix the node id.
``LOG_PREFIX_LEVEL``    Prefix the severity level.
``LOG_PREFIX_ALL``      Enable all prefixes.
======================  ===========================================

The prefix options are described briefly below.

The options can be given in the ``NS_LOG``
environment variable by these tokens:

================  =========
Token             Alternate
================  =========
``prefix_func``   ``func``
``prefix_time``   ``time``
``prefix_node``   ``node``
``prefix_level``  ``level``
``prefix_all``    | ``all``
                  | ``*``
================  =========

For log components merely mentioned in ``NS_LOG``

.. sourcecode:: bash

   $ NS_LOG="<log-component>:..."

the default prefix options are ``LOG_PREFIX_ALL``.

Severity Prefix
###############

The severity class of a message can be included with the options
``prefix_level`` or ``level``.  For example, this value of ``NS_LOG``
enables logging for all log components (\`*') and all severity
classes (``=all``), and prefixes the message with the severity
class (``|prefix_level``).

.. sourcecode:: bash

   $ NS_LOG="*=all|prefix_level" ./waf --run scratch-simulator
   Scratch Simulator
   level=ERROR error message
   level=WARN warn message
   level=DEBUG debug message
   level=INFO info message
   level=FUNCT function message
   level=LOGIC logic message

Time Prefix
###########

The simulation time can be included with the options
``prefix_time`` or ``time``.  This prints the simulation time in seconds.

Node Prefix
###########

The simulation node id can be included with the options
``prefix_node`` or ``node``.

Function Prefix
###############

The name of the calling function can be included with the options
``prefix_func`` or ``func``.


``NS_LOG`` Wildcards
####################

The log component wildcard \`*' will enable all components.  To
enable all components at a specific severity level
use ``*=<severity>``.

The severity level option wildcard \`*' is a synonym for ``all``.
This must occur before any \`|' characters separating options.
To enable all severity classes, use ``<log-component>=*``,
or ``<log-component>=*|<options>``.

The option wildcard \`*' or token ``all`` enables all prefix options,
but must occur *after* a \`|' character.  To enable a specific
severity class or level, and all prefixes, use
``<log-component>=<severity>|*``.

The combined option wildcard ``**`` enables all severities and all prefixes;
for example, ``<log-component>=**``.

The uber-wildcard ``***`` enables all severities and all prefixes
for all log components.  These are all equivalent:

.. sourcecode:: bash

   $ NS_LOG="***" ...      $ NS_LOG="*=all|*" ...        $ NS_LOG="*=*|all" ...	      
   $ NS_LOG="*=**" ...     $ NS_LOG="*=level_all|*" ...  $ NS_LOG="*=*|prefix_all" ...
   $ NS_LOG="*=*|*" ...
                       
Be advised:  even the trivial ``scratch-simulator`` produces over
46K lines of output with ``NS_LOG="***"``!

Log message format
******************

|ns3| is in the process of transition to structured logging. For that,
logfmt_ format is used. Prefixes are always printed in logfmt style
style, while the message itself may be in logfmt or free form.

A typical logfmt message looks like this::

   time_s=0.527784687 node=1 component=WifiPhy func=StartRx level=DEBUG tag=sync_to_signal msg="Sync to signal with power {power_w}" power_w=6.8767e-09

Here ``time_s`` key is the time prefix ("s" stands for "seconds"
unit), ``node`` key is the node prefix, ``component`` and ``func`` are
parts of function prefix and ``level`` is the level prefix. The rest
is the message itself.

By covention, messages contain ``tag`` key to help filter out a
particular message within a function and a ``msg`` key with a
human-readable description. Other keys, such as ``power_w``, are
specific to a particular message type.

However, legacy messages may not follow logfmt style::

   time_s=0.547681921 node=0 component=FqCoDelQueueDisc func=DoEnqueue level=DEBUG Packet enqueued into flow 686; flow index 1

This message can still be parsed as logfmt message, but flow number
cannot be easily extracted.

Parsing logs
============

Logs may change over time without notice, and no backward
compatibility is guaranteed. However, it may still be useful to
automatically process logs for debugging purposes.

For example, it is possible to run

.. sourcecode:: bash

   $ NS_LOG="WifiPhy=level|func|debug|prefix_node|prefix_time" ./waf --run "wifi-rate-adaptation-distance --steps=5" 2>out.log

Collected log can then be processed with agrind_ to select interesting
messages and convert them to `JSON Lines <http://jsonlines.org/>`_ format,

.. sourcecode:: bash

 $ agrind <out.log '* | logfmt | where tag=sync_to_signal' -o json >out.jsonl

Pandas can load JSON lines format, so it is possible to plot values
with the following script:

.. sourcecode:: python

  import numpy as np
  import pandas as pd
  from matplotlib.backends.backend_agg import FigureCanvasAgg as FigureCanvas
  from matplotlib.figure import Figure


  data_frame = pd.read_json("out.jsonl", lines=True)

  figure = Figure()
  ax = figure.add_subplot()

  for node, df in data_frame.query('tag == "sync_to_signal"').groupby("node"):
      ax.plot(
          df["time_s"],
          30.0 + np.log10(0.1 * df["power_w"]),
          ["x", "+"][node],
          label=f"Node {node}",
      )

  ax.set_xlabel("Time, s")
  ax.set_ylabel("Power, dBm")
  ax.legend()
  ax.grid()

  canvas = FigureCanvas(figure)
  canvas.print_figure("out.png")

.. _logfmt: https://www.brandur.org/logfmt

.. _agrind: https://github.com/rcoh/angle-grinder

How to add logging to your code
*******************************

Adding logging to your code is very simple:

1. Invoke the ``NS_LOG_COMPONENT_DEFINE (...);`` macro
   inside of ``namespace ns3``.

  Create a unique string identifier (usually based on the name of the file
  and/or class defined within the file) and register it with a macro call
  such as follows:

  ::

    namespace ns3 {
    
    NS_LOG_COMPONENT_DEFINE ("Ipv4L3Protocol");
    ...

  This registers ``Ipv4L3Protocol`` as a log component.

  (The macro was carefully written to permit inclusion either within or
  outside of namespace ``ns3``, and usage will vary across the codebase, but
  the original intent was to register this *outside* of namespace ``ns3``
  at file global scope.)

2. Add logging statements (macro calls) to your functions and function bodies.

In case you want to add logging statements to the methods of your template class
(which are defined in an header file):

1. Invoke the ``NS_LOG_TEMPLATE_DECLARE;`` macro in the private section of
   your class declaration. For instance:

  ::

    template <typename Item>
    class Queue : public QueueBase
    {
    ...
    private:
      std::list<Ptr<Item> > m_packets;          //!< the items in the queue
      NS_LOG_TEMPLATE_DECLARE;                  //!< the log component
    };

  This requires you to perform these steps for all the subclasses of your class.

2. Invoke the ``NS_LOG_TEMPLATE_DEFINE (...);`` macro in the constructor of
   your class by providing the name of a log component registered by calling
   the ``NS_LOG_COMPONENT_DEFINE (...);`` macro in some module. For instance:

  ::

    template <typename Item>
    Queue<Item>::Queue ()
      : NS_LOG_TEMPLATE_DEFINE ("Queue")
    {
    }

3. Add logging statements (macro calls) to the methods of your class.

In case you want to add logging statements to a static member template
(which is defined in an header file):

1. Invoke the ``NS_LOG_STATIC_TEMPLATE_DEFINE (...);`` macro in your static
   method by providing the name of a log component registered by calling
   the ``NS_LOG_COMPONENT_DEFINE (...);`` macro in some module. For instance:

  ::

    template <typename Item>
    void
    NetDeviceQueue::PacketEnqueued (Ptr<Queue<Item> > queue,
                                    Ptr<NetDeviceQueueInterface> ndqi,
                                    uint8_t txq, Ptr<const Item> item)
    {

      NS_LOG_STATIC_TEMPLATE_DEFINE ("NetDeviceQueueInterface");
    ...

2. Add logging statements (macro calls) to your static method.

Controlling timestamp precision
*******************************

Timestamps are printed out in units of seconds.  When used with the default
|ns3| time resolution of nanoseconds, the default timestamp precision is 9 
digits, with fixed format, to allow for 9 digits to be consistently printed 
to the right of the decimal point.  Example:

::

  +0.000123456s RandomVariableStream:SetAntithetic(0x805040, 0)

When the |ns3| simulation uses higher time resolution such as picoseconds
or femtoseconds, the precision is expanded accordingly; e.g. for picosecond:

::

  +0.000123456789s RandomVariableStream:SetAntithetic(0x805040, 0)

When the |ns3| simulation uses a time resolution lower than microseconds,
the default C++ precision is used.
 
An example program at ``src\core\examples\sample-log-time-format.cc``
demonstrates how to change the timestamp formatting.

The maximum useful precision is 20 decimal digits, since Time is signed 64 
bits.

Logging Macros
==============

  The logging macros and associated severity levels are

  ================  ==========================
  Severity Class    Macro
  ================  ==========================
  ``LOG_NONE``      (none needed)
  ``LOG_ERROR``     ``NS_LOG_ERROR (...);``
  ``LOG_WARN``      ``NS_LOG_WARN (...);``
  ``LOG_DEBUG``     ``NS_LOG_DEBUG (...);``
  ``LOG_INFO``      ``NS_LOG_INFO (...);``
  ``LOG_FUNCTION``  ``NS_LOG_FUNCTION (...);``
  ``LOG_LOGIC``     ``NS_LOG_LOGIC (...);``
  ================  ==========================

  The macros function as output streamers, so anything you can send to
  ``std::cout``, joined by ``<<`` operators, is allowed::

    void MyClass::Check (int value, char * item)
    {
      NS_LOG_FUNCTION (this << arg << item);
      if (arg > 10)
        {
          NS_LOG_ERROR ("encountered bad value " << value <<
                        " while checking " << name << "!");
        }
      ...
    }

  Note that ``NS_LOG_FUNCTION`` automatically inserts a \`\ :literal:`,\ `'
  (comma-space) separator between each of its arguments.
  This simplifies logging of function arguments;
  just concatenate them with ``<<`` as in the example above.

Unconditional Logging
=====================

As a convenience, the ``NS_LOG_UNCOND (...);`` macro will always log its
arguments, even if the associated log-component is not enabled at any
severity.  This macro does not use any of the prefix options.  Note that
logging is only enabled in debug builds; this macro won't produce
output in optimized builds.


Guidelines
==========

* Start every class method with ``NS_LOG_FUNCTION (this << args...);``
  This enables easy function call tracing.

  * Except:  don't log operators or explicit copy constructors, 
    since these will cause infinite recursion and stack overflow.

  * For methods without arguments use the same form:
    ``NS_LOG_FUNCTION (this);``

  * For static functions:

    * With arguments use ``NS_LOG_FUNCTION (...);`` as normal.
    * Without arguments use ``NS_LOG_FUNCTION_NOARGS ();``

* Use ``NS_LOG_ERROR`` for serious error conditions that probably
  invalidate the simulation execution.

* Use ``NS_LOG_WARN`` for unusual conditions that may be correctable.
  Please give some hints as to the nature of the problem and how
  it might be corrected.

* ``NS_LOG_DEBUG`` is usually used in an *ad hoc* way to understand
  the execution of a model.

* Use ``NS_LOG_INFO`` for additional information about the execution,
  such as the size of a data structure when adding/removing from it.

* Use ``NS_LOG_LOGIC`` to trace important logic branches within a function.

* Test that your logging changes do not break the code.  
  Run some example programs with all log components turned on (e.g. 
  ``NS_LOG="***"``).

* Use an explicit cast for any variable of type uint8_t or int8_t, 
  e.g., ``NS_LOG_LOGIC ("Variable i is " << static_cast<int> (i));``.
  Without the cast, the integer is interpreted as a char, and the result
  will be most likely not in line with the expectations.
  This is a well documented C++ 'feature'.



