# /*
#  * This program is free software; you can redistribute it and/or modify
#  * it under the terms of the GNU General Public License version 2 as
#  * published by the Free Software Foundation;
#  *
#  * This program is distributed in the hope that it will be useful,
#  * but WITHOUT ANY WARRANTY; without even the implied warranty of
#  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  * GNU General Public License for more details.
#  *
#  * You should have received a copy of the GNU General Public License
#  * along with this program; if not, write to the Free Software
#  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#  */

# importing of libraries

import ns.applications
import ns.core
import ns.internet
import ns.network
import ns.point_to_point

#script are used to enable two logging components that are built into the Echo Client and Echo Server applications

ns.core.LogComponentEnable("UdpEchoClientApplication", ns.core.LOG_LEVEL_INFO)
ns.core.LogComponentEnable("UdpEchoServerApplication", ns.core.LOG_LEVEL_INFO)

#The next two lines of code in our script will actually create the ns-3 Node objects that will represent the computers in the simulation.

nodes = ns.network.NodeContainer()
nodes.Create(2)

# these three lines will use a single PointToPointHelper to configure and connect ns-3 PointToPointNetDevice and PointToPointChannel objects in this script.

#instantiates a PointToPointHelper object on the stack
pointToPoint = ns.point_to_point.PointToPointHelper()
#tells the PointToPointHelper object to use the value “5Mbps” (five megabits per second) as the “DataRate” when it creates a PointToPointNetDevice object.
pointToPoint.SetDeviceAttribute("DataRate", ns.core.StringValue("5Mbps"))
#tells the PointToPointHelper to use the value “2ms” (two milliseconds) as the value of the transmission delay of every point to point channel it subsequently creates.
pointToPoint.SetChannelAttribute("Delay", ns.core.StringValue("2ms"))

#pointToPoint.Install (nodes) call we will have two nodes, each with an installed point-to-point net device and a single point-to-point channel between them.
devices = pointToPoint.Install(nodes)
#These two lines will install an Internet Stack (TCP, UDP, IP, etc.) on each of the nodes in the node container.
stack = ns.internet.InternetStackHelper()
stack.Install(nodes)
#declare an address helper object and tell it that it should begin allocating IP addresses from the network 10.1.1.0 using the mask 255.255.255.0 to define the allocatable bits.
address = ns.internet.Ipv4AddressHelper()
address.SetBase(ns.network.Ipv4Address("10.1.1.0"),
                ns.network.Ipv4Mask("255.255.255.0"))
#performs the actual address assignment
interfaces = address.Assign(devices)
#The following lines of code in our example script, first.cc, are used to set up a UDP echo server application on one of the nodes
echoServer = ns.applications.UdpEchoServerHelper(9)

serverApps = echoServer.Install(nodes.Get(1))
serverApps.Start(ns.core.Seconds(1.0))
serverApps.Stop(ns.core.Seconds(10.0))

echoClient = ns.applications.UdpEchoClientHelper(interfaces.GetAddress(1), 9)
echoClient.SetAttribute("MaxPackets", ns.core.UintegerValue(1))
echoClient.SetAttribute("Interval", ns.core.TimeValue(ns.core.Seconds(1.0)))
echoClient.SetAttribute("PacketSize", ns.core.UintegerValue(1024))

clientApps = echoClient.Install(nodes.Get(0))
#these lines will start and stop the applications
clientApps.Start(ns.core.Seconds(2.0))
clientApps.Stop(ns.core.Seconds(10.0))

ns.core.Simulator.Run()
#to clean up 
ns.core.Simulator.Destroy()

