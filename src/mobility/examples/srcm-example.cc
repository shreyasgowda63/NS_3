#include <ns3/core-module.h>
#include <ns3/mobility-module.h>
#include <ns3/network-module.h>

using namespace ns3;

static void 
CourseChange (std::string foo, Ptr<const MobilityModel> mobility)
{
  Vector pos = mobility->GetPosition ();
  Vector vel = mobility->GetVelocity ();
  std::cout << Simulator::Now () << ", model=" << mobility << ", POS: x=" << pos.x << ", y=" << pos.y
            << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
            << ", z=" << vel.z << std::endl;
}

int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  NodeContainer c;
  c.Create (1);
 
  ObjectFactory pos;
  pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
  pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=-200.0|Max=200.0]"));
  pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=-200.0|Max=200.0]")); 
  Ptr<PositionAllocator> pa = pos.Create ()->GetObject<PositionAllocator> ();

  MobilityHelper mobility;
  mobility.SetPositionAllocator (pa);
  mobility.SetMobilityModel ("ns3::SemiRandomCircularMobilityModel",
		     "Angle",StringValue("ns3::UniformRandomVariable[Min=0.0|Max=180.0]"),
             "TuringRadius",StringValue("ns3::UniformRandomVariable[Min=0.1|Max=200.0]"),
             "Pause",StringValue("ns3::ConstantRandomVariable[Constant=0.25]"),
             "FlyingHeight",StringValue("ns3::UniformRandomVariable[Min=150.0|Max=200.0]"),
		     "Speed",StringValue("ns3::UniformRandomVariable[Min=200.0|Max=200.0]"));
  mobility.InstallAll ();
  Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange",
                   MakeCallback (&CourseChange));

  AsciiTraceHelper ascii;
  MobilityHelper::EnableAsciiAll (ascii.CreateFileStream ("SRCM.mob"));

  Simulator::Stop (Seconds (100.0));

  Simulator::Run ();

  Simulator::Destroy ();
  return 0;
}
