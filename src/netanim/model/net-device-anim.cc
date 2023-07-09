#include "net-device-anim.h"

#include "ns3/object.h"

namespace ns3
{
TypeId
NetDeviceAnim::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::NetDeviceAnim").SetParent<Object>().SetGroupName("NetDeviceAnim");

    return tid;
}

TypeId
NetDeviceAnim::GetInstanceTypeId() const
{
    return GetTypeId();
}
} // namespace ns3