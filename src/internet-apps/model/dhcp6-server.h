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

class InetSocketAddress;
class Socket;
class Packet;

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
     * \param from Address of the DHCPv6 client
     */
    void SendAdvertise(Ptr<NetDevice> iDev, Dhcp6Header header, InetSocketAddress from);

    /**
     * \brief Sends Reply after receiving Request
     * \param iDev incoming NetDevice
     * \param header DHCPv6 header of the received message
     * \param from Address of the DHCP client
     */
    void SendReply(Ptr<NetDevice> iDev, Dhcp6Header header, InetSocketAddress from);

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
    Ptr<Socket> m_socket;

    /**
     * \brief Pointer to the net device used by the server.
     */
    Ptr<NetDevice> m_device;

    /**
     * \brief Store the address pool and range.
     * Address pool + Min address / Max address
     */
    typedef std::map<Ipv6Address, std::pair<Ipv6Address, Ipv6Address>> AddressPool;

    /**
     * \brief Leased address container
     * Client DUID + Ipv6Address / Lease time
     */
    typedef std::map<Address, std::pair<Ipv6Address, uint32_t>> LeasedAddresses;

    /**
     * \brief Leased address container iterator
     */
    typedef std::map<Address, std::pair<Ipv6Address, uint32_t>>::iterator LeasedAddressesIterator;

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
     * \brief List of available address pools.
     * The first pool corresponds to the subnet that the DHCPv6 server belongs
     * to.
     */
    AddressPool availablePools;

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
};

} // namespace ns3

#endif
