/*
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
 */

#ifndef DHCP6_SERVER_H
#define DHCP6_SERVER_H

#include "dhcp6-header.h"

#include "ns3/application.h"
#include "ns3/ipv6-address.h"

namespace ns3
{

class Inet6SocketAddress;
class Socket;
class Packet;

/**
 * \ingroup dhcp6
 *
 * \class LeaseInfo
 * \brief Includes information about available subnets and corresponding leases.
 */
class LeaseInfo
{
  public:
    /**
     * Constructor.
     * \param addressPool Address pool
     * \param prefix Prefix of the address pool
     * \param minAddress Minimum address in the pool
     * \param maxAddress Maximum address in the pool
     */
    LeaseInfo(Ipv6Address addressPool,
              Ipv6Prefix prefix,
              Ipv6Address minAddress,
              Ipv6Address maxAddress);

    /**
     * \brief Get the address pool.
     * \return The address pool
     */
    Ipv6Address GetAddressPool();

    /**
     * \brief Get the prefix of the address pool.
     * \return The prefix of the address pool
     */
    Ipv6Prefix GetPrefix();

    /**
     * \brief Get the minimum address in the pool.
     * \return The minimum address in the pool
     */
    Ipv6Address GetMinAddress();

    /**
     * \brief Get the maximum address in the pool.
     * \return The maximum address in the pool
     */
    Ipv6Address GetMaxAddress();

    /**
     * \brief Get the number of addresses leased.
     * \return The number of addresses leased
     */
    uint32_t GetNumAddresses();

    /**
     * \brief Expired Addresses (Section 6.2 of RFC 8415)
     * Expired time / Ipv6Address
     */
    typedef std::map<uint32_t, Ipv6Address> ExpiredAddresses;

    /**
     * \brief Leased Addresses
     * Client DUID + Ipv6Address / Lease time
     */
    typedef std::map<Address, std::pair<Ipv6Address, uint32_t>> LeasedAddresses;

    /**
     * \brief Declined Addresses
     * Client DUID + Ipv6Address
     */
    typedef std::map<Address, Ipv6Address> DeclinedAddresses;

    LeasedAddresses m_leasedAddresses;     //!< Leased addresses
    ExpiredAddresses m_expiredAddresses;   //!< Expired addresses
    DeclinedAddresses m_declinedAddresses; //!< Declined addresses

  private:
    Ipv6Address m_addressPool; //!< Address pool
    Ipv6Prefix m_prefix;       //!< Prefix of the address pool
    Ipv6Address m_minAddress;  //!< Minimum address in the pool
    Ipv6Address m_maxAddress;  //!< Maximum address in the pool
    uint32_t m_numAddresses;   //!< Number of addresses leased.
};

/**
 * \ingroup dhcp6
 *
 * \class Dhcp6Server
 * \brief Implements the DHCPv6 server.
 */
class Dhcp6Server : public Application
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * \brief Default constructor.
     */
    Dhcp6Server();

    /**
     * \brief Set the net device that the DHCPv6 server will use.
     * \param netDevice The net device that the server will use
     */
    void SetDhcp6ServerNetDevice(Ptr<NetDevice> netDevice);

    /**
     * \brief Add a managed address pool.
     * \param pool The address pool to be managed by the server.
     * \param prefix The prefix of the address pool.
     * \param minAddress The minimum address in the pool.
     * \param maxAddress The maximum address in the pool.
     */
    void AddSubnet(Ipv6Address pool,
                   Ipv6Prefix prefix,
                   Ipv6Address minAddress,
                   Ipv6Address maxAddress);

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * \brief Handles incoming packets from the network
     * \param socket Socket bound to port 547 of the DHCP server
     */
    void NetHandler(Ptr<Socket> socket);

    /**
     * \brief Sends DHCPv6 Advertise after receiving DHCPv6 Solicit.
     * \param iDev incoming NetDevice
     * \param header DHCPv6 header of the received message
     * \param client Address of the DHCPv6 client
     */
    void ProcessSolicit(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress client);

    /**
     * \brief Sends DHCPv6 Advertise after receiving DHCPv6 Solicit.
     * \param iDev incoming NetDevice
     * \param header DHCPv6 header of the received message
     * \param client Address of the DHCPv6 client
     */
    void SendAdvertise(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress client);

    /**
     * \brief Sends Reply after receiving Request
     * \param iDev incoming NetDevice
     * \param header DHCPv6 header of the received message
     * \param client Address of the DHCP client
     */
    void SendReply(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress client);

    /**
     * \brief Modifies the remaining lease time of addresses
     */
    void TimerHandler();

    /**
     * \brief The port number of the DHCPv6 server.
     */
    static const int PORT = 547;

    /**
     * \brief The socket bound to port 547.
     */
    Ptr<Socket> m_recvSocket;

    /**
     * \brief The socket used to send packets.
     */
    Ptr<Socket> m_sendSocket;

    /**
     * \brief Pointer to the net device used by the server.
     */
    Ptr<NetDevice> m_device;

    /**
     * \brief The server identifier.
     */
    IdentifierOption m_serverIdentifier;

    /**
     * \brief Store the address pool and range.
     * Address pool + Min address / Max address
     */
    typedef std::map<Ipv6Address, std::pair<Ipv6Address, Ipv6Address>> AddressPool;

    /**
     * \brief Store the prefixes of the address pool.
     * Address pool + Prefix
     */
    typedef std::map<Ipv6Address, Ipv6Prefix> Prefixes;

    /**
     * \brief Leased address container
     * Client DUID + Ipv6Address / Lease time
     */
    typedef std::map<Address, std::pair<Ipv6Address, uint32_t>> LeasedAddresses;

    /**
     * \brief Leased address container iterator
     */
    typedef std::map<Address, std::pair<Ipv6Address, uint32_t>>::iterator LeasedAddressesIterator;

    // TODO: track expired addresses

    /**
     * \brief Declined addresses
     */
    typedef std::list<Ipv6Address> DeclinedAddresses;

    /**
     * \brief Declined address iterator.
     */
    typedef std::list<Ipv6Address>::iterator DeclinedAddressIterator;

    /// Available address container - IP addr
    typedef std::list<Ipv4Address> AvailableAddress;

    /**
     * \brief Map of available address pools.
     */
    AddressPool availablePools;

    /**
     * \brief Map of prefixes corresponding to the address pools.
     */
    Prefixes availablePrefixes;

    /**
     * \brief Store IA bindings.
     * DUID + IA Type / IAID
     */
    std::map<Address, std::pair<uint8_t, uint32_t>> iaBindings;

    /**
     * \brief Addresses that have been leased to clients.
     */
    LeasedAddresses m_leasedAddresses;

    /**
     * \brief Keeps track of addresses that have been declined by clients.
     */
    DeclinedAddresses m_declinedAddresses;

    /**
     * \brief Default preferred lifetime for an address.
     * According to ISC's Kea guide, the default preferred lifetime is 3000
     * seconds.
     */
    Time m_prefLifetime;

    /**
     * \brief Default valid lifetime.
     * According to ISC's Kea guide, the default valid lifetime is 4000 seconds.
     */
    Time m_validLifetime;

    /**
     * \brief The default renew timer.
     * This defines the T1 timer. According to ISC's Kea guide, the default
     * renew timer is 1000 seconds.
     */
    Time m_renew;

    /**
     * \brief The default rebind timer.
     * This defines the T2 timer. According to ISC's Kea guide, the default
     * rebind timer is 2000 seconds.
     */
    Time m_rebind;

    /**
     * \brief List of all managed subnets.
     */
    std::vector<LeaseInfo> m_subnets;
};

} // namespace ns3

#endif
