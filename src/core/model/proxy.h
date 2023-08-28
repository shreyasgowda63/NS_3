#include "ns3/object.h"

namespace ns3
{

template <typename T>
class Proxy : public Object
{
  public:
    Proxy(Ptr<T> item)
        : m_item(item)
    {
    }

    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::Proxy").SetParent<Object>().SetGroupName("Core");
        return tid;
    }

    T operator*()
    {
        return (*m_item);
    }

    T* operator->()
    {
        return &(*m_item);
    } // Support drill-down

    Ptr<T> GetProxied()
    {
        return (m_item);
    }

    Ptr<T> m_item;
};
} // namespace ns3
