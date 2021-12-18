#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h" 

using namespace ns3;

int main (int argc, char *argv[])
{


   //Create 1 UAV nodes and make it move in circle on top - origin=(150,150,250), radius=150, Height=250	
  NodeContainer CirclesUAV;
  CirclesUAV.Create (1);
	
  MobilityHelper CircleMobility;
	
  CircleMobility.SetMobilityModel ("ns3::CircleMobilityModel");
  CircleMobility.Install (CirclesUAV);
  //SetParameters(const Vector &Origin, const double Radius, const double StartAngle, const double Speed)	
  CirclesUAV.Get (0)->GetObject<CircleMobilityModel> ()->SetParameters(Vector (150, 150, 250), 150, 0, 20); 

  AnimationInterface anim ("Simple3DCircleMobilityExample1.xml"); 
  anim.UpdateNodeSize (0, 10, 10);
  
  Simulator::Stop (Seconds (200.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
