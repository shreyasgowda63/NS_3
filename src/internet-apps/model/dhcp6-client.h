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

#ifndef DHCP6_CLIENT_H
#define DHCP6_CLIENT_H

#include "dhcp6-header.h"

#include "ns3/application.h"
#include "ns3/socket.h"

namespace ns3
{

/**
 * \ingroup dhcp6
 *
 * \class Dhcp6Client
 * \brief Implements the DHCPv6 client.
 */
class Dhcp6Client : public Application
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
    Dhcp6Client();

    /**
     * \brief Set the net device that the DHCPv6 client will use.
     * \param netDevice The net device that the client will use
     */
    void SetDhcp6ClientNetDevice(Ptr<NetDevice> netDevice);

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * \brief State of the DHCPv6 client.
     */
    enum State
    {
        WAIT_ADVERTISE = 1, // Waiting for an advertise message
        WAIT_REPLY = 2,     // Waiting for a reply message
        RENEW = 3,          // Renewing the lease
    };

    /**
     * \brief Verify the incoming advertise message.
     * \param header The DHCPv6 header received.
     */
    void ValidateAdvertise(Dhcp6Header header);

    /**
     * \brief Send a request to the DHCPv6 server.
     * \param iDev The net device of the client
     * \param header The DHCPv6 header
     * \param server The address of the server
     */
    void SendRequest(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress server);

    /**
     * \brief Send a request to the DHCPv6 server.
     * \param iDev The net device of the client
     * \param header The DHCPv6 header
     * \param server The address of the server
     */
    void AcceptReply(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress server);

    /**
     * \brief Handles incoming packets from the network
     * \param socket Socket bound to port 546 of the DHCP client
     */
    void NetHandler(Ptr<Socket> socket);

    /**
     * \brief Handle changes in the link state.
     */
    void LinkStateHandler();

    /**
     * \brief Used to send the Solicit message and start the DHCPv6 client.
     */
    void Boot();

    /**
     * \brief The port number of the DHCPv6 client.
     */
    static const int DHCP_CLIENT_PORT = 546;

    /**
     * \brief The port number of the DHCPv6 server.
     */
    static const int DHCP_PEER_PORT = 547;

    /**
     * \brief The initial address of the client. It is set to the
     */
    Ipv6Address m_clientAddress;

    /**
     * \brief The socket used for communication.
     */
    Ptr<Socket> m_socket;

    /**
     * \brief Pointer to the net device of the client.
     */
    Ptr<NetDevice> m_device;

    /**
     * \brief The state of the DHCPv6 client.
     */
    uint8_t m_state;

    /**
     * \brief First boot of the client.
     */
    bool m_firstBoot;

    /**
     * \brief Store the starting timestamp for the Elapsed Time option
     */
    Time m_startTime;

    /**
     * \brief Store the transaction ID of the initiated message exchange.
     */
    uint32_t m_clientTransactId;

    /**
     * \brief Store the client identifier option.
     */
    IdentifierOption m_clientIdentifier;

    Time m_solicitInterval; //!< SOL_MAX_RT value, default = 3600 / 100 = 36 sec
    EventId m_solicitEvent; //!< Event ID for the solicit event
};

} // namespace ns3

#endif
