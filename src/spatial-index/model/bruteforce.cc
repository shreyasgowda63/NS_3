#include "bruteforce.h"

#include "ns3/mobility-model.h"
#include <cfloat>
#include <cmath>

namespace ns3 {
void
BruteForceSpatialIndexing::doAdd(Ptr<const Node> _node, const Vector3D& position)
{
  m_map[_node] = position;
}

void
BruteForceSpatialIndexing::remove(Ptr<const Node> _node)
{
  //auto node=std::find(m_list.begin(),m_list.end(),_node);
  //m_list.erase(node);
  m_map.erase(_node);
}

void
BruteForceSpatialIndexing::update(Ptr<const Node> _node, const Vector3D& position)
{
  add(_node, position); //update is just the same as add in this case
}

std::vector<Ptr<const Node> > BruteForceSpatialIndexing::getNodesInRange( double _range, const Vector& position, const Ptr<const Node> sourceNode) //MobilityModel& _node, double _range)
{
  static std::vector<Ptr<const Node> > nodes;
  nodes.clear();
  double range_squared = _range*_range;

  for(auto it = m_map.begin(); it != m_map.end(); ++it)
    {
    AddIfInRange(*it, position, range_squared, nodes);
    }
  return nodes;
}

void
BruteForceSpatialIndexing::HandlePositionChange(Ptr<const PositionAware> position_aware)
{
  //nothing for now.  need to add later?
}

}//namespace ns3
