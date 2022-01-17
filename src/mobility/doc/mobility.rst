.. include:: replace.txt

.. _Mobility:

Mobility
--------

.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

The mobility support in |ns3| includes:

- a set of mobility models which are used to track and maintain the *current* cartesian position and speed of an object.
- a "course change notifier" trace source which can be used to register listeners to the course changes of a mobility model
- a number of helper classes which are used to place nodes and setup mobility models (including parsers for some mobility definition formats).

Model Description
*****************

The source code for mobility lives in the directory ``src/mobility``.

Design
======

The design includes mobility models, position allocators, and helper
functions.  

In |ns3|, `MobilityModel`` objects track the evolution of position
with respect to a (cartesian) coordinate system.  The mobility model
is typically aggregated to an ``ns3::Node`` object and queried using
``GetObject<MobilityModel> ()``. The base class ``ns3::MobilityModel``
is subclassed for different motion behaviors.

The initial position of objects is typically set with a PositionAllocator.
These types of objects will lay out the position on a notional canvas.
Once the simulation starts, the position allocator may no longer be
used, or it may be used to pick future mobility "waypoints" for such
mobility models.

Most users interact with the mobility system using mobility helper
classes.  The MobilityHelper combines a mobility model and position
allocator, and can be used with a node container to install a similar
mobility capability on a set of nodes.

We first describe the coordinate system and issues 
surrounding multiple coordinate systems.

Coordinate system
#################

There are many possible coordinate systems and possible translations between
them.  |ns3| uses the Cartesian coordinate system only, at present.

The question has arisen as to how to use the mobility models (supporting
Cartesian coordinates) with different coordinate systems.  This is possible
if the user performs conversion between the |ns3| Cartesian and the
other coordinate system.  One possible library to assist is
the `proj4 <https://proj.org>`_ library for projections and reverse 
projections.

If we support converting between coordinate systems, we must adopt a
reference.  It has been suggested to use the geocentric Cartesian coordinate
system as a reference.  Contributions are welcome in this regard.

The question has arisen about adding a new mobility model whose motion
is natively implemented in a different coordinate system (such as an
orbital mobility model implemented using spherical coordinate system).
We advise to create a subclass with the APIs desired
(such as Get/SetSphericalPosition), and new position allocators, and 
implement the motion however desired, but must also support the conversion to 
cartesian (by supporting the cartesian Get/SetPosition). 

Coordinates
###########

The base class for a coordinate is called ``ns3::Vector``.  While
positions are normally described as coordinates and not vectors in
the literature, it is possible to reuse the same data structure to
represent position (x,y,z) and velocity (magnitude and direction
from the current position).  |ns3| uses class Vector for both.  
  
There are also some additional related structures used to support
mobility models.

- Rectangle
- Box
- Waypoint

MobilityModel
#############

Describe base class

- GetPosition ()
- Position and Velocity attributes
- GetDistanceFrom ()
- CourseChangeNotification

MobilityModel Subclasses
########################

- ConstantPosition
- ConstantVelocity
- ConstantAcceleration
- GaussMarkov
- Hierarchical
- RandomDirection2D
- RandomWalk2D
- RandomWaypoint
- SteadyStateRandomWaypoint
- Waypoint
- CircleMobility

PositionAllocator
#################

Position allocators usually used only at beginning, to lay out the nodes
initial position.  However, some mobility models (e.g. RandomWaypoint) will
use a position allocator to pick new waypoints.

- ListPositionAllocator
- GridPositionAllocator
- RandomRectanglePositionAllocator
- RandomBoxPositionAllocator
- RandomDiscPositionAllocator
- UniformDiscPositionAllocator

Helper
######

A special mobility helper is provided that is mainly aimed at supporting
the installation of mobility to a Node container (when using containers
at the helper API level).  The MobilityHelper class encapsulates 
a MobilityModel factory object and a PositionAllocator used for
initial node layout.  

Group mobility is also configurable via a GroupMobilityHelper object.
Group mobility reuses the HierarchicalMobilityModel allowing one to
define a reference (parent) mobility model and child (member) mobility
models, with the position being the vector sum of the two mobility
model positions (i.e., the child position is defined as an offset to
the parent position).  In the GroupMobilityHelper, the parent mobility
model is not associated with any node, and is used as the parent mobility
model for all (distinct) child mobility models.  The reference point group
mobility model [Camp2002]_ is the basis for this |ns3| model.

ns-2 MobilityHelper
###################

The |ns2| mobility format is a widely used mobility trace format.  The
documentation is available at: http://www.isi.edu/nsnam/ns/doc/node172.html

Valid trace files use the following |ns2| statements:

.. sourcecode:: bash

   $node set X_ x1
   $node set Y_ y1
   $node set Z_ z1
   $ns at $time $node setdest x2 y2 speed
   $ns at $time $node set X_ x1
   $ns at $time $node set Y_ Y1
   $ns at $time $node set Z_ Z1

In the above, the initial positions are set using the ``set`` statements.
Also, this ``set`` can be specified for a future time, such as in the
last three statements above.  

The command ``setdest`` instructs the simulation to start moving the 
specified node towards the coordinate (x2, y2) at the specified time.
Note that the node may never get to the destination, but will
proceed towards the destination at the specified speed until it
either reaches the destination (where it will pause), is set to 
a new position (via ``set``), or sent on another course change
(via ``setdest``).

Note that in |ns3|, movement along the Z dimension is not supported.

Some examples of external tools that can export in this format include:

- `BonnMotion <http://net.cs.uni-bonn.de/wg/cs/applications/bonnmotion/>`_

  - `Installation instructions <https://www.nsnam.org/wiki/HOWTO_use_ns-3_with_BonnMotion_mobility_generator_and_analysis_tool>`_ and
  - `Documentation <https://sys.cs.uos.de/bonnmotion/doc/README.pdf>`_ for using BonnMotion with |ns3|

- `SUMO <https://sourceforge.net/apps/mediawiki/sumo/index.php?title=Main_Page>`_
- `TraNS <http://trans.epfl.ch/>`_
- |ns2| `setdest <http://www.winlab.rutgers.edu/~zhibinwu/html/ns2_wireless_scene.htm>`_ utility

A special Ns2MobilityHelper object can be used to parse these files
and convert the statements into |ns3| mobility events.  The underlying
ConstantVelocityMobilityModel is used to model these movements.

See below for additional usage instructions on this helper.

Scope and Limitations
=====================

- only cartesian coordinates are presently supported

CircleMobilityModel
###################

The inspired by ConstantPositionMobilityModel of ns-3 by Mathieu Lacage and circle function logic of 
circle mobility model of Omnet++ by Andras Varga.

The position and velocity calculations are inspired from and circle function Logic of 
circle mobility model of Omnet++. Omnet++ used radian-degree conversions in its implementation. 
But here we are doing the angle math in degrees only.

Scope and Limitations:
=====================================

The movement of the object will be controlled by parameters  `Origin, Radius, StartAngle, Speed` 
and `Direction`. This mobility model enforces no bounding box by itself. 

No seperate helper function is implemented to handle this model in a special way. 
The standatd MobilityHelper of |ns3| is used to configure this model.
No special PositionAllocator is needed to configure the initial positions of the nodes. 
The user can use any of the existing PositionAllocator of |ns3|.

The mobility model parameters/attributes can be set during initialization of the mobility model.
During initialization itself, we can configure different  Mobility Parameter. 

After initialization, if the user want to change the mobility parameter of one particular node,
or group of nodes, that can be only done through a custom SetAttributes method of the model.

Changing of parameters using  SetAttributes method will cause the node to change position instantaneously
in a discrete jump such as a instant change in altitude and radius. This is a maojor limitaion/caveat of this model.
Taking this in consideration, a forced position override by calling SetPosition() after the model has been initialized 
is also allowed. Because, in that case also, with respect to the other initial parameters, 
the path of the node will get configured by with respect to the new position input.

The implementation of this model is not 2d-specific. i.e. if you provide z-value greater than 0, 
then you may use it in 3d scenarios. It is possible to use this model as  child in a 
hierarchical/group mobility and create more practical 3d mobility scenarios.

This implementation tries to keep this CircleMobilityModel as simple as possible and make it compatible with hierarchical mobility model to make complex movements. For example, if we use WaypointMobilityModel as parent and CircleMobilityModel as child, then we will have different kinds of spiral like movements that will mimic realistic movements of circulating UAV/Aircraft.

While using this model in a hierarchical/group mobility scenario, maximum speed of the node being its "nominal" speed plus the speed of the parent mobility model. So, due to the changes of parent mobility, the speed of (UAV) node may exceed the UAV specifications.
So that, the user should be aware and configure the speed parent mobility and child mobility(node) accordingly. 
All this kind of group mobility related issues are not addressed in this simple circle mobiity model.


References
==========

.. [Camp2002] T. Camp, J. Boleng, V. Davies.  "A survey of mobility models for ad hoc network research",
   in Wireless Communications and Mobile Computing, 2002: vol. 2, pp. 2483-2502.

Usage
*****

Most |ns3| program authors typically interact with the mobility system
only at configuration time.  However, various |ns3| objects interact
with mobility objects repeatedly during runtime, such as a propagation
model trying to determine the path loss between two mobile nodes.

Helper
======

A typical usage pattern can be found in the ``third.cc`` program in the
tutorial.

First, the user instantiates a ``MobilityHelper`` object and sets some
``Attributes`` controlling the "position allocator" functionality.

.. sourcecode:: cpp

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
    "MinX", DoubleValue (0.0),
    "MinY", DoubleValue (0.0),
    "DeltaX", DoubleValue (5.0),
    "DeltaY", DoubleValue (10.0),
    "GridWidth", UintegerValue (3),
    "LayoutType", StringValue ("RowFirst"));

This code tells the mobility helper to use a two-dimensional grid to initially
place the nodes.  The first argument is an |ns3| TypeId specifying the
type of mobility model; the remaining attribute/value pairs configure
this position allocator.

Next, the user typically sets the MobilityModel subclass; e.g.:

.. sourcecode:: cpp

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
    "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));

Once the helper is configured, it is typically passed a container, such as:

.. sourcecode:: cpp

  mobility.Install (wifiStaNodes);

A MobilityHelper object may be reconfigured and reused for different
NodeContainers during the configuration of an |ns3| scenario.

Ns2MobilityHelper
=================

Two example programs are provided demonstrating the use of the
|ns2| mobility helper:

- ns2-mobility-trace.cc
- bonnmotion-ns2-example.cc

ns2-mobility-trace
##################

The ``ns2-mobility-trace.cc`` program is an example of loading an
|ns2| trace file that specifies the movements of two nodes over 100
seconds of simulation time.  It is paired with the file 
``default.ns_movements``.

The program behaves as follows:

- a Ns2MobilityHelper object is created, with the specified trace file. 
- A log file is created, using the log file name argument.
- A node container is created with the number of nodes specified in the command line.  For this particular trace file, specify the value 2 for this argument.
- the Install() method of Ns2MobilityHelper to set mobility to nodes. At this moment, the file is read line by line, and the movement is scheduled in the simulator.
- A callback is configured, so each time a node changes its course a log message is printed.

The example prints out messages generated by each read line from the ns2 movement trace file.   For each line, it shows if the line is correct, or of it has errors and in this case it will be ignored.

Example usage:

.. sourcecode:: bash

  $ ./ns3 --run "ns2-mobility-trace \
  --traceFile=src/mobility/examples/default.ns_movements \
  --nodeNum=2 \
  --duration=100.0 \
  --logFile=ns2-mob.log"

Sample log file output:

.. sourcecode:: text

  +0.0ns POS: x=150, y=93.986, z=0; VEL:0, y=50.4038, z=0
  +0.0ns POS: x=195.418, y=150, z=0; VEL:50.1186, y=0, z=0
  +104727357.0ns POS: x=200.667, y=150, z=0; VEL:50.1239, y=0, z=0
  +204480076.0ns POS: x=205.667, y=150, z=0; VEL:0, y=0, z=0

bonnmotion-ns2-example
######################

The ``bonnmotion-ns2-example.cc`` program, which models the movement of
a single mobile node for 1000 seconds of simulation time, has a few 
associated files:

- ``bonnmotion.ns_movements`` is the |ns2|-formatted mobility trace
- ``bonnmotion.params`` is a BonnMotion-generated file with some metadata about the mobility trace
- ``bonnmotion.ns_params`` is another BonnMotion-generated file with ns-2-related metadata.  

Neither of the latter two files is used by |ns3|, although they are generated
as part of the BonnMotion process to output ns-2-compatible traces.

The program ``bonnmotion-ns2-example.cc`` will output the following to stdout:

.. sourcecode:: text

  At 0.00 node 0: Position(329.82, 66.06, 0.00);   Speed(0.53, -0.22, 0.00)
  At 100.00 node 0: Position(378.38, 45.59, 0.00);   Speed(0.00, 0.00, 0.00)
  At 200.00 node 0: Position(304.52, 123.66, 0.00);   Speed(-0.92, 0.97, 0.00)
  At 300.00 node 0: Position(274.16, 131.67, 0.00);   Speed(-0.53, -0.46, 0.00)
  At 400.00 node 0: Position(202.11, 123.60, 0.00);   Speed(-0.98, 0.35, 0.00)
  At 500.00 node 0: Position(104.60, 158.95, 0.00);   Speed(-0.98, 0.35, 0.00)
  At 600.00 node 0: Position(31.92, 183.87, 0.00);   Speed(0.76, -0.51, 0.00)
  At 700.00 node 0: Position(107.99, 132.43, 0.00);   Speed(0.76, -0.51, 0.00)
  At 800.00 node 0: Position(184.06, 80.98, 0.00);   Speed(0.76, -0.51, 0.00)
  At 900.00 node 0: Position(250.08, 41.76, 0.00);   Speed(0.60, -0.05, 0.00)

The motion of the mobile node is sampled every 100 seconds, and its position
and speed are printed out.  This output may be compared to the output of
a similar |ns2| program (found in the |ns2| ``tcl/ex/`` directory of |ns2|)
running from the same mobility trace. 

The next file is generated from |ns2| (users will have to download and
install |ns2| and run this Tcl program to see this output).
The output of the |ns2| ``bonnmotion-example.tcl`` program is shown below
for comparison (file ``bonnmotion-example.tr``):

.. sourcecode:: text

  M 0.00000 0 (329.82, 66.06, 0.00), (378.38, 45.59), 0.57
  M 100.00000 0 (378.38, 45.59, 0.00), (378.38, 45.59), 0.57
  M 119.37150 0 (378.38, 45.59, 0.00), (286.69, 142.52), 1.33
  M 200.00000 0 (304.52, 123.66, 0.00), (286.69, 142.52), 1.33
  M 276.35353 0 (286.69, 142.52, 0.00), (246.32, 107.57), 0.70
  M 300.00000 0 (274.16, 131.67, 0.00), (246.32, 107.57), 0.70
  M 354.65589 0 (246.32, 107.57, 0.00), (27.38, 186.94), 1.04
  M 400.00000 0 (202.11, 123.60, 0.00), (27.38, 186.94), 1.04
  M 500.00000 0 (104.60, 158.95, 0.00), (27.38, 186.94), 1.04
  M 594.03719 0 (27.38, 186.94, 0.00), (241.02, 42.45), 0.92
  M 600.00000 0 (31.92, 183.87, 0.00), (241.02, 42.45), 0.92
  M 700.00000 0 (107.99, 132.43, 0.00), (241.02, 42.45), 0.92
  M 800.00000 0 (184.06, 80.98, 0.00), (241.02, 42.45), 0.92
  M 884.77399 0 (241.02, 42.45, 0.00), (309.59, 37.22), 0.60
  M 900.00000 0 (250.08, 41.76, 0.00), (309.59, 37.22), 0.60

The output formatting is slightly different, and the course change
times are additionally plotted, but it can be seen that the position 
vectors are the same between the two traces at intervals of 100 seconds.

The mobility computations performed on the |ns2| trace file are slightly
different in |ns2| and |ns3|, and floating-point arithmetic is used,
so there is a chance that the position in |ns2| may be slightly 
different than the respective position when using the trace file
in |ns3|.  

Use of Random Variables
=======================

A typical use case is to evaluate protocols on a mobile topology that
involves some randomness in the motion or initial position allocation.
To obtain random motion and positioning that is not affected by
the configuration of the rest of the scenario, it is recommended to
use the "AssignStreams" facility of the random number system.

Class ``MobilityModel`` and class ``PositionAllocator`` both have public
API to assign streams to underlying random variables:

.. sourcecode:: cpp

  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model. Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream first stream index to use
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams (int64_t stream);

The class ``MobilityHelper`` also provides this API.  The typical usage 
pattern when using the helper is:

.. sourcecode:: cpp

  int64_t streamIndex = /*some positive integer */  
  MobilityHelper mobility;
  ... (configure mobility)
  mobility.Install (wifiStaNodes);
  int64_t streamsUsed = mobility.AssignStreams (wifiStaNodes, streamIndex);

If AssignStreams is called before Install, it will not have any effect.

Advanced Usage
==============

A number of external tools can be used to generate traces read by
the Ns2MobilityHelper.

ns-2 scengen
############

TBD

BonnMotion
##########

http://net.cs.uni-bonn.de/wg/cs/applications/bonnmotion/

SUMO
####

http://sourceforge.net/apps/mediawiki/sumo/index.php?title=Main_Page

TraNS
#####

http://trans.epfl.ch/

Examples
========

- main-random-topology.cc
- main-random-walk.cc
- main-grid-topology.cc
- ns2-mobility-trace.cc
- ns2-bonnmotion.cc
- simple-3d-circle-mobility-example1.cc

reference-point-group-mobility-example.cc
#########################################

The reference point group mobility model ([Camp2002]_) is demonstrated
in the example program `reference-point-group-mobility-example.cc`.
This example runs a short simulation that illustrates a parent
WaypointMobilityModel traversing a rectangular course within a bounding
box, and three member nodes independently execute a two-dimensional
random walk around the parent position, within a small bounding box.
The example illustrates configuration using the GroupMobilityHelper
and manual configuration without a helper; the configuration option
is selectable by command-line argument.

The example outputs two mobility trace files, a course change trace and
a time-series trace of node position.  The latter trace file can be
parsed by a Bash script (`reference-point-group-mobility-animate.sh`)
to create PNG images at one-second intervals, which can then be combined
using an image processing program such as ImageMagick to form a
basic animated gif of the mobility.  The example and animation program
files have further instructions on how to run them.

simple-3d-circle-mobility-example1.cc
######################################################

This Example `Simple3DCircleMobilityExample1.cc` will generate a 5 UAV node topology 
and simulate CircleMobilityModel in them.
This simulation will create a NetAnim Trace file as an output.

The name of the NetAnim trace file will depend on the selected example scenario.

You can run the example script as follows:

.. sourcecode:: bash

    $./waf "simple-3d-circle-mobility-example1 --example=7"

This will run the simulation for the 7th example scenario presented below.

This simulation and will create the file `Simple3DCircleMobilityExample-7.xml`
We can visualize the scenario using NetAnim using this xml file.

**Initializing and using the model in different ways**


The following are different ways in which we can initialize and use the model:
All the example codes will set the CircleMobilityModel in all the nodes in the 
NodeContainer but move them differently according to settings

**Example 1:**

In this all the nodes start the movement at `(0,0,0)` but will have different 
origins derived from the default random value of radius, start angle 
and will have random speed and direction. So, all the nodes will circulate 
in different circular paths but the nodes will pass the point `(0,0,0)`

.. sourcecode:: cpp

    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::CircleMobilityModel");
    mobility.Install (UAVs);
  
 
**Example 2:**

In this, all the nodes will start the movement at initial position provided by the PositionAllocator
and calculate origins with respect to the positions and with respect to the default random value
of radius, start angle and will have random speed and direction.
So, all the nodes will circulate in different circles but will pass the initial point provided by PositionAllocator

.. code-block:: cpp

    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::CircleMobilityModel");
    mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
                               "X", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"),
                               "Y", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"),
                               "Z", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"));
    mobility.Install (UAVs);
  
 
**Example 3:**

In this, all the nodes will start the movement at position with respect to different 
origins derived from the default random value of radius, start angle 
and will have random speed and direction.
So, all the nodes will circulate in different circular planes perpendicular to the z-axis

.. code-block:: cpp

    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::CircleMobilityModel", 
                               "UseConfiguredOrigin",BooleanValue(true));
    mobility.Install (UAVs);
  
 
**Example 4:**

In this, all the nodes will start the movement with respect to different 
origins derived from the user provided range of random value of radius, start angle 
and will have random speed and direction.
So, all the nodes will circulate in different circular x-y planes perpendicular to the z-axis

.. code-block:: cpp

    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::CircleMobilityModel", 
                               "UseConfiguredOrigin",BooleanValue(true),
                               "MinOrigin",Vector3DValue(Vector3D(0,0,0)),"MaxOrigin",Vector3DValue(Vector3D(500,500,500)),
                               "MinMaxRadius",Vector2DValue(Vector2D(500,500)),
                               "MinMaxStartAngle",Vector2DValue(Vector2D(0,0)),
                               "MinMaxSpeed",Vector2DValue(Vector2D(30,60)),
                               "RandomizeDirection",BooleanValue(false),
                               "Clockwise",BooleanValue(true)
    mobility.Install (UAVs);
  
What ever may be the way in which we initialize the mobility model, 
we can customize the path of any single node by 
class CircleMobilityModel : public MobilityModel
using the CircleMobilityModel::SetParameters function at any time.

**Example 5:**

.. code-block:: cpp

   //void SetParameters(const Vector &Origin, const double Radius, const double StartAngle, const bool Clockwise, const double Speed);
   mobility.Get (0)->GetObject<CircleMobilityModel> ()->SetParameters(Vector (150, 150, 250), 150, 0, true, 20); 


**Example 6:**

If the user choose to use the initial position of the node (provided by PositionAllocator) as origin,
they can do it as follows:
 
.. code-block:: cpp

           mobility.SetMobilityModel ("ns3::CircleMobilityModel",
                                    "UseInitialPositionAsOrigin", BooleanValue(true),
                                    "MinMaxSpeed",Vector2DValue(Vector2D(10,10)),
                                    "RandomizeDirection",BooleanValue(false),
                                    "MinMaxRadius",Vector2DValue(Vector2D(300,300)));
          mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
                                    "X", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"),
                                    "Y", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"),
                                    "Z", StringValue ("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"));

**Example 7:**

The CircleMobilityModel can be used in group mobility as shown below:

.. code-block:: cpp

        Ptr<WaypointMobilityModel> waypointMm = CreateObject<WaypointMobilityModel> ();
        waypointMm->AddWaypoint (Waypoint (Seconds (0), Vector (0, 0, 0)));
        waypointMm->AddWaypoint (Waypoint (Seconds (1000), Vector (5000, 0, 0)));
        waypointMm->AddWaypoint (Waypoint (Seconds (2000), Vector (0, 5000, 0)));
        GroupMobilityHelper group;
        group.SetReferenceMobilityModel (waypointMm);
        group.SetMemberMobilityModel ("ns3::CircleMobilityModel","UseConfiguredOrigin",BooleanValue(true));
        group.Install (UAVs);

Validation
**********

TBD

CircleMobilityModel
###################

The CircleMobilityModel test suite performs a simple test to check the case in which there are no runtime parameter changes. 
It first considers the origin to be (0m,0m,0m), radius to be 1m, and speed to be 2*pi m/s, and initial angle to be 0 degre, 
and then check the position at time 0.5 second.

Since PI is irrational, we need to check these x-coordinates with some tolerance value.
So it will test the position provided by the mobility model with calculated value of position (-1m, 0m, 0m) with a tolerance value of 0.1m.
The test will succeed if the values are equal within that tolerance

With the same initial condition, the calculations are repeated at 1.0 second .
And it will test the position provided by the mobility model with calculated value of position (1m, 0m, 0m) with a tolerance value of 0.1m.
The test will succeed if the values are equal within that tolerance



