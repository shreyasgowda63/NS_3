#ifndef BRUTEFORCE_HPP
#define BRUTEFORCE_HPP

#include "spatial-index.h"

#include <map>

namespace ns3 {

class YansWifiPhy;

class BruteForceSpatialIndexing: public ns3::SpatialIndexing {
 public:
  virtual void doAdd(Ptr<const Node> _node, const Vector& position)                                    override;
  virtual void remove(Ptr<const Node> _node)                                 override;
  virtual void update(Ptr<const Node> _node, const Vector& position)                                 override;
  virtual std::vector<Ptr<const Node> > getNodesInRange(double _range, const Vector& position, const Ptr<const Node> sourceNode) override; //Ptr<const MobilityModel> _node, double _range)  override;

  virtual void HandlePositionChange(Ptr<const PositionAware> position_aware) override;

private:
  std::map<Ptr<const Node>, Vector> m_map;
};

}//namespace ns3
#endif//BRUTEFORCE_HPP

