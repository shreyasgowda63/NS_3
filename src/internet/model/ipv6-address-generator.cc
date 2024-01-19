/*
 * Copyright (c) 2008 University of Washington
 * Copyright (c) 2011 Atishay Jain
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
 */

#include "ipv6-address-generator.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/simulation-singleton.h"

#include <list>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv6AddressGenerator");

/**
 * \ingroup address
 *
 * \brief Implementation class of Ipv6AddressGenerator
 * This generator assigns addresses sequentially from a provided
 * network address; used in topology code. It also keeps track of all
 * addresses assigned to perform duplicate detection.
 *
 */
class Ipv6AddressGeneratorImpl
{
  public:
    Ipv6AddressGeneratorImpl();
    virtual ~Ipv6AddressGeneratorImpl();

    /**
     * \brief Initialise the base network and interfaceId for the generator
     *
     * The first call to NextAddress() or GetAddress() will return the
     * value passed in.
     *
     * \param net The network for the base Ipv6Address
     * \param prefix The prefix of the base Ipv6Address
     * \param interfaceId The base interface ID used for initialization
     */
    void Init(const Ipv6Address net, const Ipv6Prefix prefix, const Ipv6Address interfaceId);

    /**
     * \brief Get the next network according to the given Ipv6Prefix
     *
     * This operation is a pre-increment, meaning that the internal state
     * is changed before returning the new network address.
     *
     * This also resets the interface ID to the base interface ID that was
     * used for initialization.
     *
     * \param prefix The Ipv6Prefix used to set the next network
     * \returns the IPv6 address of the next network
     */
    Ipv6Address NextNetwork(const Ipv6Prefix prefix);

    /**
     * \brief Get the current network of the given Ipv6Prefix
     *
     * Does not change the internal state; this just peeks at the current
     * network
     *
     * \param prefix The Ipv6Prefix for the current network
     * \returns the IPv6 address of the current network
     */
    Ipv6Address GetNetwork(const Ipv6Prefix prefix) const;

    /**
     * \brief Set the interfaceId for the given Ipv6Prefix
     *
     * \param interfaceId The interfaceId to set for the current Ipv6Prefix
     * \param prefix The Ipv6Prefix whose address is to be set
     */
    void InitAddress(const Ipv6Address interfaceId, const Ipv6Prefix prefix);

    /**
     * \brief Get the Ipv6Address that will be allocated upon NextAddress ()
     *
     * Does not change the internal state; just is used to peek the next
     * address that will be allocated upon NextAddress ()
     *
     * \param prefix The Ipv6Prefix for the current network
     * \returns the IPv6 address
     */
    Ipv6Address GetAddress(const Ipv6Prefix prefix) const;

    /**
     * \brief Allocate the next Ipv6Address for the configured network and prefix
     *
     * This operation is a post-increment, meaning that the first address
     * allocated will be the one that was initially configured.
     *
     * \param prefix The Ipv6Prefix for the current network
     * \returns the IPv6 address
     */
    Ipv6Address NextAddress(const Ipv6Prefix prefix);

    /**
     * \brief Reset the networks and Ipv6Address to zero
     */
    void Reset();

    /**
     * \brief Add the Ipv6Address to the list of IPv6 entries
     *
     * Typically, this is used by external address allocators that want
     * to make use of this class's ability to track duplicates.  AddAllocated
     * is always called internally for any address generated by NextAddress ()
     *
     * \param addr The Ipv6Address to be added to the list of Ipv6 entries
     * \returns true on success
     */
    bool AddAllocated(const Ipv6Address addr);

    /**
     * \brief Check the Ipv6Address allocation in the list of IPv6 entries
     *
     * \param addr The Ipv6Address to be checked in the list of Ipv4 entries
     * \returns true if the network is already allocated
     */
    bool IsAddressAllocated(const Ipv6Address addr);

    /**
     * \brief Check if a network has already allocated addresses
     *
     * \param addr The Ipv6 network to be checked
     * \param prefix The Ipv6 network prefix
     * \returns true if the network is already allocated
     */
    bool IsNetworkAllocated(const Ipv6Address addr, const Ipv6Prefix prefix);

    /**
     * \brief Used to turn off fatal errors and assertions, for testing
     */
    void TestMode();

  private:
    static const uint32_t N_BITS = 128;                //!< the number of bits in the address
    static const uint32_t MOST_SIGNIFICANT_BIT = 0x80; //!< MSB set to 1

    /**
     * \brief Create an index number for the prefix
     * \param prefix the prefix to index
     * \returns an index
     */
    uint32_t PrefixToIndex(Ipv6Prefix prefix) const;

    /**
     * \brief This class holds the state for a given network
     */
    class NetworkState
    {
      public:
        uint8_t prefix[16];  //!< the network prefix
        uint32_t shift;      //!< a shift
        uint8_t network[16]; //!< the network
        uint8_t addr[16];    //!< the address
        uint8_t addrMax[16]; //!< the maximum address
    };

    NetworkState m_netTable[N_BITS]; //!< the available networks

    /**
     * \brief This class holds the allocated addresses
     */
    class Entry
    {
      public:
        uint8_t addrLow[16];  //!< the lowest allocated address
        uint8_t addrHigh[16]; //!< the highest allocated address
    };

    std::list<Entry> m_entries; //!< contained of allocated addresses
    Ipv6Address m_base;         //!< base address
    bool m_test;                //!< test mode (if true)
};

Ipv6AddressGeneratorImpl::Ipv6AddressGeneratorImpl()
    : m_entries(),
      m_base("::1"),
      m_test(false)
{
    NS_LOG_FUNCTION(this);
    Reset();
}

void
Ipv6AddressGeneratorImpl::Reset()
{
    NS_LOG_FUNCTION(this);

    uint8_t prefix[16] = {0};

    for (uint32_t i = 0; i < N_BITS; ++i)
    {
        for (uint32_t j = 0; j < 16; ++j)
        {
            m_netTable[i].prefix[j] = prefix[j];
        }
        for (uint32_t j = 0; j < 15; ++j)
        {
            prefix[15 - j] >>= 1;
            prefix[15 - j] |= (prefix[15 - j - 1] & 1);
        }
        prefix[0] |= MOST_SIGNIFICANT_BIT;
        for (uint32_t j = 0; j < 15; ++j)
        {
            m_netTable[i].network[j] = 0;
        }
        m_netTable[i].network[15] = 1;
        for (uint32_t j = 0; j < 15; ++j)
        {
            m_netTable[i].addr[j] = 0;
        }
        m_netTable[i].addr[15] = 1;
        for (uint32_t j = 0; j < 16; ++j)
        {
            m_netTable[i].addrMax[j] = ~prefix[j];
        }
        m_netTable[i].shift = N_BITS - i;
    }
    m_entries.clear();
    m_base = Ipv6Address("::1");
    m_test = false;
}

Ipv6AddressGeneratorImpl::~Ipv6AddressGeneratorImpl()
{
    NS_LOG_FUNCTION(this);
}

void
Ipv6AddressGeneratorImpl::Init(const Ipv6Address net,
                               const Ipv6Prefix prefix,
                               const Ipv6Address interfaceId)
{
    NS_LOG_FUNCTION(this << net << prefix << interfaceId);

    m_base = interfaceId;
    //
    // We're going to be playing with the actual bits in the network and prefix so
    // pull them out into ints.
    //
    uint8_t prefixBits[16];
    prefix.GetBytes(prefixBits);
    uint8_t netBits[16];
    net.GetBytes(netBits);
    uint8_t interfaceIdBits[16];
    interfaceId.GetBytes(interfaceIdBits);
    //
    // Some quick reasonableness testing.
    //
    // Convert the network prefix into an index into the network number table.
    // The network number comes in to us properly aligned for the prefix and so
    // needs to be shifted right into the normalized position (lowest bit of the
    // network number at bit zero of the int that holds it).
    //
    uint32_t index = PrefixToIndex(prefix);
    NS_LOG_DEBUG("Index " << index);
    uint32_t a = m_netTable[index].shift / 8;
    uint32_t b = m_netTable[index].shift % 8;
    for (int32_t j = 15 - a; j >= 0; j--)
    {
        m_netTable[index].network[j + a] = netBits[j];
    }
    for (uint32_t j = 0; j < a; j++)
    {
        m_netTable[index].network[j] = 0;
    }
    for (uint32_t j = 15; j >= a; j--)
    {
        m_netTable[index].network[j] = m_netTable[index].network[j] >> b;
        m_netTable[index].network[j] |= m_netTable[index].network[j - 1] << (8 - b);
    }
    for (int32_t j = 0; j < 16; j++)
    {
        m_netTable[index].addr[j] = interfaceIdBits[j];
    }
}

Ipv6Address
Ipv6AddressGeneratorImpl::GetNetwork(const Ipv6Prefix prefix) const
{
    NS_LOG_FUNCTION(this);
    uint8_t nw[16] = {0};
    uint32_t index = PrefixToIndex(prefix);
    uint32_t a = m_netTable[index].shift / 8;
    uint32_t b = m_netTable[index].shift % 8;
    for (uint32_t j = 0; j < 16 - a; ++j)
    {
        nw[j] = m_netTable[index].network[j + a];
    }
    for (uint32_t j = 0; j < 15; j++)
    {
        nw[j] = nw[j] << b;
        nw[j] |= nw[j + 1] >> (8 - b);
    }
    nw[15] = nw[15] << b;

    return Ipv6Address(nw);
}

Ipv6Address
Ipv6AddressGeneratorImpl::NextNetwork(const Ipv6Prefix prefix)
{
    NS_LOG_FUNCTION(this);

    uint32_t index = PrefixToIndex(prefix);
    // Reset the base to what was initialized
    uint8_t interfaceIdBits[16];
    m_base.GetBytes(interfaceIdBits);
    for (int32_t j = 0; j < 16; j++)
    {
        m_netTable[index].addr[j] = interfaceIdBits[j];
    }

    for (int32_t j = 15; j >= 0; j--)
    {
        if (m_netTable[index].network[j] < 0xff)
        {
            ++m_netTable[index].network[j];
            break;
        }
        else
        {
            ++m_netTable[index].network[j];
        }
    }

    uint8_t nw[16];
    uint32_t a = m_netTable[index].shift / 8;
    uint32_t b = m_netTable[index].shift % 8;
    for (uint32_t j = 0; j < 16 - a; ++j)
    {
        nw[j] = m_netTable[index].network[j + a];
    }
    for (uint32_t j = 16 - a; j < 16; ++j)
    {
        nw[j] = 0;
    }
    for (uint32_t j = 0; j < 15; j++)
    {
        nw[j] = nw[j] << b;
        nw[j] |= nw[j + 1] >> (8 - b);
    }
    nw[15] = nw[15] << b;

    return Ipv6Address(nw);
}

void
Ipv6AddressGeneratorImpl::InitAddress(const Ipv6Address interfaceId, const Ipv6Prefix prefix)
{
    NS_LOG_FUNCTION(this);

    uint32_t index = PrefixToIndex(prefix);
    uint8_t interfaceIdBits[16];
    interfaceId.GetBytes(interfaceIdBits);

    for (uint32_t j = 0; j < 16; ++j)
    {
        m_netTable[index].addr[j] = interfaceIdBits[j];
    }
}

Ipv6Address
Ipv6AddressGeneratorImpl::GetAddress(const Ipv6Prefix prefix) const
{
    NS_LOG_FUNCTION(this);

    uint32_t index = PrefixToIndex(prefix);

    uint8_t nw[16] = {0};
    uint32_t a = m_netTable[index].shift / 8;
    uint32_t b = m_netTable[index].shift % 8;
    for (uint32_t j = 0; j < 16 - a; ++j)
    {
        nw[j] = m_netTable[index].network[j + a];
    }
    for (uint32_t j = 0; j < 15; j++)
    {
        nw[j] = nw[j] << b;
        nw[j] |= nw[j + 1] >> (8 - b);
    }
    nw[15] = nw[15] << b;
    for (uint32_t j = 0; j < 16; j++)
    {
        nw[j] |= m_netTable[index].addr[j];
    }

    return Ipv6Address(nw);
}

Ipv6Address
Ipv6AddressGeneratorImpl::NextAddress(const Ipv6Prefix prefix)
{
    NS_LOG_FUNCTION(this);

    uint32_t index = PrefixToIndex(prefix);

    uint8_t ad[16] = {0};
    uint32_t a = m_netTable[index].shift / 8;
    uint32_t b = m_netTable[index].shift % 8;
    for (uint32_t j = 0; j < 16 - a; ++j)
    {
        ad[j] = m_netTable[index].network[j + a];
    }
    for (uint32_t j = 0; j < 15; j++)
    {
        ad[j] = ad[j] << b;
        ad[j] |= ad[j + 1] >> (8 - b);
    }
    ad[15] = ad[15] << b;
    for (uint32_t j = 0; j < 16; j++)
    {
        ad[j] |= m_netTable[index].addr[j];
    }
    Ipv6Address addr = Ipv6Address(ad);

    for (int32_t j = 15; j >= 0; j--)
    {
        if (m_netTable[index].addr[j] < 0xff)
        {
            ++m_netTable[index].addr[j];
            break;
        }
        else
        {
            ++m_netTable[index].addr[j];
        }
    }

    //
    // Make a note that we've allocated this address -- used for address collision
    // detection.
    //
    AddAllocated(addr);
    return addr;
}

bool
Ipv6AddressGeneratorImpl::AddAllocated(const Ipv6Address address)
{
    NS_LOG_FUNCTION(this << address);

    uint8_t addr[16];
    address.GetBytes(addr);

    std::list<Entry>::iterator i;

    for (i = m_entries.begin(); i != m_entries.end(); ++i)
    {
        NS_LOG_LOGIC("examine entry: " << Ipv6Address((*i).addrLow) << " to "
                                       << Ipv6Address((*i).addrHigh));
        //
        // First things first.  Is there an address collision -- that is, does the
        // new address fall in a previously allocated block of addresses.
        //
        if (!(Ipv6Address(addr) < Ipv6Address((*i).addrLow)) &&
            ((Ipv6Address(addr) < Ipv6Address((*i).addrHigh)) ||
             (Ipv6Address(addr) == Ipv6Address((*i).addrHigh))))
        {
            NS_LOG_LOGIC(
                "Ipv6AddressGeneratorImpl::Add(): Address Collision: " << Ipv6Address(addr));
            if (!m_test)
            {
                NS_FATAL_ERROR(
                    "Ipv6AddressGeneratorImpl::Add(): Address Collision: " << Ipv6Address(addr));
            }
            return false;
        }
        //
        // If the new address is less than the lowest address in the current
        // block and can't be merged into to the current block, then insert it
        // as a new block before the current block.
        //
        uint8_t taddr[16];
        for (uint32_t j = 0; j < 16; j++)
        {
            taddr[j] = (*i).addrLow[j];
        }
        taddr[15] -= 1;
        if (Ipv6Address(addr) < Ipv6Address(taddr))
        {
            break;
        }
        //
        // If the new address fits at the end of the block, look ahead to the next
        // block and make sure it's not a collision there.  If we won't overlap,
        // then just extend the current block by one address.  We expect that
        // completely filled network ranges will be a fairly rare occurrence,
        // so we don't worry about collapsing address range blocks.
        //
        for (uint32_t j = 0; j < 16; j++)
        {
            taddr[j] = (*i).addrLow[j];
        }
        taddr[15] += 1;
        if (Ipv6Address(addr) == Ipv6Address(taddr))
        {
            auto j = i;
            ++j;

            if (j != m_entries.end())
            {
                if (Ipv6Address(addr) == Ipv6Address((*j).addrLow))
                {
                    NS_LOG_LOGIC("Ipv6AddressGeneratorImpl::Add(): "
                                 "Address Collision: "
                                 << Ipv6Address(addr));
                    if (!m_test)
                    {
                        NS_FATAL_ERROR("Ipv6AddressGeneratorImpl::Add(): Address Collision: "
                                       << Ipv6Address(addr));
                    }
                    return false;
                }
            }

            NS_LOG_LOGIC("New addrHigh = " << Ipv6Address(addr));
            for (uint32_t j = 0; j < 16; j++)
            {
                (*i).addrHigh[j] = addr[j];
            }
            return true;
        }
        //
        // If we get here, we know that the next lower block of addresses
        // couldn't have been extended to include this new address since the
        // code immediately above would have been executed and that next lower
        // block extended upward.  So we know it's safe to extend the current
        // block down to include the new address.
        //
        for (uint32_t j = 0; j < 16; j++)
        {
            taddr[j] = (*i).addrLow[j];
        }
        taddr[15] -= 1;
        if (Ipv6Address(addr) == Ipv6Address(taddr))
        {
            NS_LOG_LOGIC("New addrLow = " << Ipv6Address(addr));
            for (uint32_t j = 0; j < 16; j++)
            {
                (*i).addrLow[j] = addr[j];
            }
            return true;
        }
    }

    Entry entry;
    for (uint32_t j = 0; j < 16; j++)
    {
        entry.addrLow[j] = entry.addrHigh[j] = addr[j];
    }
    m_entries.insert(i, entry);
    return true;
}

bool
Ipv6AddressGeneratorImpl::IsAddressAllocated(const Ipv6Address address)
{
    NS_LOG_FUNCTION(this << address);

    uint8_t addr[16];
    address.GetBytes(addr);

    for (auto i = m_entries.begin(); i != m_entries.end(); ++i)
    {
        NS_LOG_LOGIC("examine entry: " << Ipv6Address((*i).addrLow) << " to "
                                       << Ipv6Address((*i).addrHigh));

        if (!(Ipv6Address(addr) < Ipv6Address((*i).addrLow)) &&
            ((Ipv6Address(addr) < Ipv6Address((*i).addrHigh)) ||
             (Ipv6Address(addr) == Ipv6Address((*i).addrHigh))))
        {
            NS_LOG_LOGIC("Ipv6AddressGeneratorImpl::IsAddressAllocated(): Address Collision: "
                         << Ipv6Address(addr));
            return true;
        }
    }
    return false;
}

bool
Ipv6AddressGeneratorImpl::IsNetworkAllocated(const Ipv6Address address, const Ipv6Prefix prefix)
{
    NS_LOG_FUNCTION(this << address << prefix);

    NS_ABORT_MSG_UNLESS(
        address == address.CombinePrefix(prefix),
        "Ipv6AddressGeneratorImpl::IsNetworkAllocated(): network address and mask don't match "
            << address << " " << prefix);

    for (auto i = m_entries.begin(); i != m_entries.end(); ++i)
    {
        NS_LOG_LOGIC("examine entry: " << Ipv6Address((*i).addrLow) << " to "
                                       << Ipv6Address((*i).addrHigh));
        Ipv6Address low = Ipv6Address((*i).addrLow);
        Ipv6Address high = Ipv6Address((*i).addrHigh);

        if (address == low.CombinePrefix(prefix) || address == high.CombinePrefix(prefix))
        {
            NS_LOG_LOGIC(
                "Ipv6AddressGeneratorImpl::IsNetworkAllocated(): Network already allocated: "
                << address << " " << low << "-" << high);
            return false;
        }
    }
    return true;
}

void
Ipv6AddressGeneratorImpl::TestMode()
{
    NS_LOG_FUNCTION(this);
    m_test = true;
}

uint32_t
Ipv6AddressGeneratorImpl::PrefixToIndex(Ipv6Prefix prefix) const
{
    //
    // We've been given a prefix that has a higher order bit set for each bit of
    // the network number.  In order to translate this prefix into an index,
    // we just need to count the number of zero bits in the prefix.  We do this
    // in a loop in which we shift the prefix right until we find the first
    // nonzero bit.  This tells us the number of zero bits, and from this we
    // infer the number of nonzero bits which is the number of bits in the prefix.
    //
    // We use the number of bits in the prefix as the number of bits in the
    // network number and as the index into the network number state table.
    //
    uint8_t prefixBits[16];
    prefix.GetBytes(prefixBits);

    for (int32_t i = 15; i >= 0; --i)
    {
        for (uint32_t j = 0; j < 8; ++j)
        {
            if (prefixBits[i] & 1)
            {
                uint32_t index = N_BITS - (15 - i) * 8 - j;
                NS_ABORT_MSG_UNLESS(index > 0 && index < N_BITS,
                                    "Ip64AddressGenerator::PrefixToIndex(): Illegal Prefix");
                return index;
            }
            prefixBits[i] >>= 1;
        }
    }
    NS_FATAL_ERROR("Ipv6AddressGenerator::PrefixToIndex(): Impossible");
    return 0;
}

void
Ipv6AddressGenerator::Init(const Ipv6Address net,
                           const Ipv6Prefix prefix,
                           const Ipv6Address interfaceId)
{
    NS_LOG_FUNCTION(net << prefix << interfaceId);

    SimulationSingleton<Ipv6AddressGeneratorImpl>::Get()->Init(net, prefix, interfaceId);
}

Ipv6Address
Ipv6AddressGenerator::NextNetwork(const Ipv6Prefix prefix)
{
    NS_LOG_FUNCTION(prefix);

    return SimulationSingleton<Ipv6AddressGeneratorImpl>::Get()->NextNetwork(prefix);
}

Ipv6Address
Ipv6AddressGenerator::GetNetwork(const Ipv6Prefix prefix)
{
    NS_LOG_FUNCTION(prefix);

    return SimulationSingleton<Ipv6AddressGeneratorImpl>::Get()->GetNetwork(prefix);
}

void
Ipv6AddressGenerator::InitAddress(const Ipv6Address interfaceId, const Ipv6Prefix prefix)
{
    NS_LOG_FUNCTION(interfaceId << prefix);

    SimulationSingleton<Ipv6AddressGeneratorImpl>::Get()->InitAddress(interfaceId, prefix);
}

Ipv6Address
Ipv6AddressGenerator::GetAddress(const Ipv6Prefix prefix)
{
    NS_LOG_FUNCTION(prefix);

    return SimulationSingleton<Ipv6AddressGeneratorImpl>::Get()->GetAddress(prefix);
}

Ipv6Address
Ipv6AddressGenerator::NextAddress(const Ipv6Prefix prefix)
{
    NS_LOG_FUNCTION(prefix);

    return SimulationSingleton<Ipv6AddressGeneratorImpl>::Get()->NextAddress(prefix);
}

void
Ipv6AddressGenerator::Reset()
{
    NS_LOG_FUNCTION_NOARGS();

    return SimulationSingleton<Ipv6AddressGeneratorImpl>::Get()->Reset();
}

bool
Ipv6AddressGenerator::AddAllocated(const Ipv6Address addr)
{
    NS_LOG_FUNCTION(addr);

    return SimulationSingleton<Ipv6AddressGeneratorImpl>::Get()->AddAllocated(addr);
}

bool
Ipv6AddressGenerator::IsAddressAllocated(const Ipv6Address addr)
{
    NS_LOG_FUNCTION(addr);

    return SimulationSingleton<Ipv6AddressGeneratorImpl>::Get()->IsAddressAllocated(addr);
}

bool
Ipv6AddressGenerator::IsNetworkAllocated(const Ipv6Address addr, const Ipv6Prefix prefix)
{
    NS_LOG_FUNCTION(addr << prefix);

    return SimulationSingleton<Ipv6AddressGeneratorImpl>::Get()->IsNetworkAllocated(addr, prefix);
}

void
Ipv6AddressGenerator::TestMode()
{
    NS_LOG_FUNCTION_NOARGS();

    SimulationSingleton<Ipv6AddressGeneratorImpl>::Get()->TestMode();
}

} // namespace ns3
