#include "spatial-index.h"

#include "ns3/log.h"
#include "ns3/mobility-model.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("SpatialIndexing");
void
SpatialIndexing::add (Ptr<const Node> _node, const Vector &position) {
  auto p = _node->GetObject<PositionAware>();
  if(nullptr == p) {
    NS_LOG_WARN("Using Spatial Indexing when Position Aware is not installed");
  } else {
  p->TraceConnectWithoutContext (
      "PositionChange", MakeCallback (&SpatialIndexing::HandlePositionChange, this));
  p->TraceConnectWithoutContext (
      "Timeout", MakeCallback (&SpatialIndexing::HandlePositionChange, this));
  }
  doAdd(_node,position);
}

void SpatialIndexing::AddIfInRange (
    std::pair<ns3::Ptr<const ns3::Node>, ns3::Vector> nodeVec,
    const Vector &                                    position,
    double                                            range_squared,
    std::vector<Ptr<const Node>> &                    nodes)
{
  if(CalculateDistanceSquared(nodeVec.second, position) <= range_squared)
    {
    nodes.push_back(nodeVec.first);
    }
}

}
