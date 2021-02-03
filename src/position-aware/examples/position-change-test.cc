#include "ns3/position-aware-helper.h"
#include "ns3/position-aware.h"
#include "ns3/mobility-model.h"
#include "ns3/mobility-helper.h"
#include "ns3/string.h"
#include "ns3/config.h"
#include <iostream>
using std::cout;
using std::endl;
using namespace ns3;

class PositionChangeNotifyTest
{
 protected:
  typedef PositionChangeNotifyTest  ThisType;
 public:
  PositionChangeNotifyTest()
  {
  }
  ~PositionChangeNotifyTest()
  {
  }
  void PositionChangeCallback(std::string _path,
                              Ptr<const PositionAware> _position_aware){
    Ptr<Node> node = _position_aware->GetObject<Node>();
    Ptr<MobilityModel> mobility = _position_aware->GetObject<MobilityModel>();
    std::cout << "[Node " << node->GetId() << "]"
        << " Position Change: " << mobility->GetPosition()
        << std::endl;
  }
  void TimeoutCallback(std::string _path,
                       Ptr<const PositionAware> _position_aware){
    Ptr<Node> node = _position_aware->GetObject<Node>();
    Ptr<MobilityModel> mobility = _position_aware->GetObject<MobilityModel>();
    std::cout << "[Node " << node->GetId() << "]"
        << " Timeout, new position: " << mobility->GetPosition()
        << std::endl;
  }
  void CreateNodes(){
    std::cout<<"Creating Nodes"<<endl;
    nodes_.Create(2);
  }

  void InstallMobility(){
    std::cout<<"Installing Mobility"<<endl;
    MobilityHelper mobility;
    mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                   "X", StringValue ("100.0"),
                                   "Y", StringValue ("100.0"),
                                   "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));
   mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                              "Mode", StringValue ("Time"),
                              "Time", StringValue ("5s"),
                              "Speed", StringValue ("ns3::UniformRandomVariable[Min=10|Max=25]"),
                              "Bounds", StringValue ("0|200|0|200"));
    mobility.Install (nodes_);
  }

  void InstallPositionAware(){
    std::cout<<"Install Position Aware"<<endl;
    PositionAwareHelper position_aware(Seconds(4),50.0);
    position_aware.Install(nodes_);
  }

  void ConnectCallbacks(){
    Config::Connect ("/NodeList/*/$ns3::PositionAware/PositionChangeNotify",
                   MakeCallback (&ThisType::PositionChangeCallback, this));
  Config::Connect ("/NodeList/*/$ns3::PositionAware/TimeoutNotify",
                   MakeCallback (&ThisType::TimeoutCallback, this));
  }

  void Run(){
    CreateNodes();
    InstallMobility();
    InstallPositionAware();
    ConnectCallbacks();
    Simulator::Stop(Seconds(100));
    Simulator::Run();
    Simulator::Destroy();
  }

  NodeContainer nodes_;
};

int main (int argc, char **argv)
{
PositionChangeNotifyTest test;
test.Run();
return 0;

}
