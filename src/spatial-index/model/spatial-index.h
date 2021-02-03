#ifndef SPATIAL_INDEX_H
#define SPATIAL_INDEX_H

#include "ns3/vector.h"
#include "ns3/object.h"
#include "ns3/node.h"
#include "ns3/position-aware.h"

#include <vector>

namespace ns3 {

class Node;
class MobilityModel;

class SpatialIndexing : public Object
{
public:
  typedef std::pair<Ptr<const Node>,double> RangeEntry_t;
  typedef std::vector<RangeEntry_t>                  RangeList_t;

  void AddIfInRange (std::pair<ns3::Ptr<const ns3::Node>, ns3::Vector> nodeVec, const Vector& position, double range_squared, std::vector<Ptr<const Node> >& nodes);
  void add (Ptr<const Node> _node, const Vector &position);
  virtual void doAdd (Ptr<const Node> _node, const Vector &position) = 0;
  virtual //size_t
  void remove (Ptr<const Node> _node)              = 0;
  virtual void update (Ptr<const Node> _node, const Vector& position)      = 0;
  virtual std::vector<Ptr<const Node> > getNodesInRange (double _range, const Vector& position, const Ptr<const Node> sourceNode) = 0; //todo make generic return type?
  virtual void HandlePositionChange (Ptr<const PositionAware> position_aware) = 0;
};

}//namespace ns3
#endif//SPATIAL_INDEX_H

