
#ifndef NETDEVICEANIM_H
#define NETDEVICEANIM_H
#include "ns3/animation-interface.h"
#include "ns3/object.h"

namespace ns3
{
class NetDeviceAnim : public Object
{
  public:
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
};
} // namespace ns3

#endif
