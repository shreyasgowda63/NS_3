
#ifndef NETDEVICEANIM_H
#define NETDEVICEANIM_H
#include "ns3/animation-interface.h"
#include "ns3/object.h"

namespace ns3
{
class NetDeviceAnim : public Object
{
  public:
    /**
     * \brief Get the type identificator.
     * \return type identificator
     */
    static TypeId GetTypeId();
    // inherited from Object
    void DoInitialize() override;
};
} // namespace ns3

#endif
