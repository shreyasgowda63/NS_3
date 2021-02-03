#ifndef POSITION_AWARE_HELPER_H
#define POSITION_AWARE_HELPER_H

#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/object-factory.h"

namespace ns3{
class PositionAware;

class PositionAwareHelper
{
 public:
  PositionAwareHelper();
  PositionAwareHelper(const Time& _t,const double& _d);
  ~PositionAwareHelper();
  void SetTimeout(const Time& _t);
  Time GetTimeout() const;
  void SetDistance(const double& _d);
  double GetDistance() const;
  void Install(Ptr<Node> _node) const;
  void Install(std::string& _node_name) const;
  void Install(NodeContainer _container) const;
  void InstallAll(void);
 private:
//  ObjectFactory position_aware_factory_;
  Time timeout_;
  double distance_;
};

}//namespace ns3


#endif//POSITION_AWARE_HELPER_H

