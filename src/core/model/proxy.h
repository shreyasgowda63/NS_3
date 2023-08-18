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

    T operator*()
    {
        return (*m_item);
    }

    T* operator->()
    {
        return &(*m_item);
    } // Support drill-down

    Ptr<T> m_item;
};
} // namespace ns3
