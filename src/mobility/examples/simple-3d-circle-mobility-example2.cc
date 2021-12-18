#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h" 

using namespace ns3;

int main (int argc, char *argv[])
{
  //Create 50 nodes and make them move in random 3D directions using GaussMarkovMobilityModel	
  NodeContainer RandomNodes;	
  RandomNodes.Create (50);
  MobilityHelper Randmobility;
  Randmobility.SetMobilityModel ("ns3::GaussMarkovMobilityModel",
  "Bounds", BoxValue (Box (0, 300, 0, 300, 20, 200)),
  "TimeStep", TimeValue (Seconds (0.5)),
  "Alpha", DoubleValue (0.85),
  "MeanVelocity", StringValue ("ns3::UniformRandomVariable[Min=0|Max=10]"),
  "MeanDirection", StringValue ("ns3::UniformRandomVariable[Min=0|Max=6.283185307]"),
  "MeanPitch", StringValue ("ns3::UniformRandomVariable[Min=0.05|Max=0.5]"),
  "NormalVelocity", StringValue ("ns3::NormalRandomVariable[Mean=10.0|Variance=5.0|Bound=10.0]"),
  "NormalDirection", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.2|Bound=0.4]"),
  "NormalPitch", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.02|Bound=0.04]"));
  Randmobility.Install (RandomNodes);

   //Create 1 UAV nodes and make it move in circle on top of all the other nodes  - origin=(150,150,250), radius=150, Height=250		
  NodeContainer CirclesUAV;
  CirclesUAV.Create (1);
	
  MobilityHelper CircleMobility;
	
  CircleMobility.SetMobilityModel ("ns3::CircleMobilityModel");
  CircleMobility.Install (CirclesUAV);
  //SetParameters(const Vector &Origin, const double Radius, const double StartAngle, const double Speed)	
  CirclesUAV.Get (0)->GetObject<CircleMobilityModel> ()->SetParameters(Vector (150, 150, 250), 150, 0, 20); 

  AnimationInterface anim ("Simple3DCircleMobilityExample2.xml"); 
  anim.UpdateNodeSize (50, 10, 10);
  
  Simulator::Stop (Seconds (200.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
