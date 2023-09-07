/*
 * Copyright (c) 2023 NITK Surathkal
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Raghuram Kannan <raghuramkannan400@gmail.com>
 *          Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#ifndef PROXY_H
#define PROXY_H

#include "ns3/object.h"

namespace ns3
{
template <typename T>
/**
 * Proxy Object class
 */
class Proxy : public Object
{
  public:
    /**
     * Creates a proxy of the object passed as a parameter.
     * \param [in] item smart pointer of the object to be proxied.
     *
     */
    Proxy(Ptr<T> item)
        : m_item(item)
    {
    }

    /**
     * \brief Get the type ID.
     * \return The object TypeId.
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::Proxy").SetParent<Object>().SetGroupName("Core");
        return tid;
    }

    /**
     * A dereference.
     * \returns The proxied Object.
     */
    T operator*()
    {
        return (*m_item);
    }

    /**
     * An lvalue member access.
     * \returns Address of the proxied object.
     */
    T* operator->()
    {
        return &(*m_item);
    } // Support drill-down

    /**
     * Pointer to proxied object
     * \returns A pointer to the proxied object.
     */
    Ptr<T> PeekPointer()
    {
        return (m_item);
    }

    /** The pointer to the proxied object */
    Ptr<T> m_item;
};

/**
 * Template specialization for casting a Proxy<Ptr<T>>
 * into the underlying Prt<T>.
 *
 * \tparam T \deduced The desired type to cast to.
 * \param [in] p The original Ptr.
 * \return The result of the cast.
 */
/** @{ */

template <typename T>
Ptr<T>
ConstCast(const Ptr<Proxy<T>>& p)
{
    return p->PeekPointer();
}

template <typename T>
Ptr<T>
DynamicCast(const Ptr<Proxy<T>>& p)
{
    return p->PeekPointer();
}

template <typename T>
Ptr<T>
StaticCast(const Ptr<Proxy<T>>& p)
{
    return p->PeekPointer();
}

/** @} */

} // namespace ns3

#endif /* PROXY_H */
