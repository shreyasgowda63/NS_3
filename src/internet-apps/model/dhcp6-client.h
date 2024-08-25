/*
 * Copyright (c) 2024 NITK Surathkal
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
 * Author: Kavya Bhat <kavyabhat@gmail.com>
 *
 */

#ifndef DHCP6_CLIENT_H
#define DHCP6_CLIENT_H

#include "dhcp6-duid.h"
#include "dhcp6-header.h"

#include "ns3/application.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/traced-callback.h"
#include "ns3/trickle-timer.h"

namespace ns3
{
namespace internetapps
{

/**
 * @ingroup dhcp6
 *
 * @class Dhcp6Client
 * @brief Implements the DHCPv6 client.
 */
class Dhcp6Client : public Application
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Default constructor.
     */
    Dhcp6Client();

    /**
     * @brief Set the net device that the DHCPv6 client will use.
     * @param netDevice The net device that the client will use
     */
    void SetDhcp6ClientNetDevice(Ptr<NetDevice> netDevice);

    /**
     * @brief Get the DUID.
     * @return The DUID-LL which identifies the client.
     */
    Duid GetSelfDuid();

    /// State of the DHCPv6 client.
    enum State
    {
        WAIT_ADVERTISE = 1,           // Waiting for an advertise message
        WAIT_REPLY = 2,               // Waiting for a reply message
        RENEW = 3,                    // Renewing the lease
        WAIT_REPLY_AFTER_DECLINE = 4, // Waiting for a reply after sending a decline message
        WAIT_REPLY_AFTER_RELEASE = 5, // Waiting for a reply after sending a release message
    };

    /**
     * \brief Used to print the state of the client.
     * \param os The output stream.
     */
    void Print(std::ostream& os) const;

    int64_t AssignStreams(int64_t stream) override;

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * @brief Verify the incoming advertise message.
     * @param header The DHCPv6 header received.
     */
    void ValidateAdvertise(Dhcp6Header header);

    /**
     * @brief Send a request to the DHCPv6 server.
     * @param iDev The net device of the client
     * @param header The DHCPv6 header
     * @param server The address of the server
     */
    void SendRequest(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress server);

    /**
     * @brief Send a request to the DHCPv6 server.
     * @param iDev The net device of the client
     * @param header The DHCPv6 header
     * @param server The address of the server
     */
    void ProcessReply(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress server);

    /**
     * @brief Check lease status after sending a Decline or Release message.
     * @param iDev The net device of the client
     * @param header The DHCPv6 header
     * @param server The address of the server
     */
    void CheckLeaseStatus(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress server) const;

    /**
     * @brief Accept the DHCPv6 offer.
     * @param offeredAddress The IPv6 address that has been accepted.
     */
    void AcceptedAddress(const Ipv6Address& offeredAddress);

    /**
     * @brief Add a declined address to the list maintained by the client.
     * @param offeredAddress The IPv6 address to be declined.
     */
    void AddDeclinedAddress(const Ipv6Address& offeredAddress);

    /**
     * @brief Send a Decline message to the DHCPv6 server
     */
    void DeclineOffer();

    /**
     * @brief Send a renew message to the DHCPv6 server.
     * @param iaidList The IAIDs whose leases are to be renewed.
     */
    void SendRenew(std::vector<uint32_t> iaidList);

    /**
     * @brief Send a rebind message to the DHCPv6 server.
     * @param iaidList The IAIDs whose leases are to be rebound.
     */
    void SendRebind(std::vector<uint32_t> iaidList);

    /**
     * @brief Send a Release message to the DHCPv6 server.
     * @param address The address whose lease is to be released.
     */
    void SendRelease(Ipv6Address address);

    /**
     * @brief Handles incoming packets from the network
     * @param socket Socket bound to port 546 of the DHCP client
     */
    void NetHandler(Ptr<Socket> socket);

    /**
     * @brief Handle changes in the link state.
     */
    void LinkStateHandler();

    /**
     * @brief Callback for when an M flag is received.
     * @param recvInterface The interface on which the M flag was received.
     */
    void ReceiveMflag(uint32_t recvInterface);

    /**
     * @brief Used to send the Solicit message and start the DHCPv6 client.
     */
    void Boot();

    /**
     * @brief Retrieve all existing IAIDs.
     * @return A list of all IAIDs.
     */
    std::vector<uint32_t> GetIaids();

    Ptr<Socket> m_socket;         //!< Socket used for communication.
    Ptr<NetDevice> m_device;      //!< Pointer to the net device of the client.
    Duid m_clientDuid;            //!< Store client DUID.
    Duid m_serverDuid;            //!< Store server DUID.
    uint8_t m_state;              //!< State of the DHCPv6 client.
    uint32_t m_clientTransactId;  //!< Transaction ID of the client-initiated message.
    uint8_t m_nOfferedAddresses;  //!< Number of addresses offered to client.
    uint8_t m_nAcceptedAddresses; //!< Number of addresses accepted by client.
    TrickleTimer m_solicitTimer;  //!< TrickleTimer to schedule Solicit messages.
    Time m_msgStartTime;          //!< Time when message exchange starts.
    Time m_solicitInterval;       //!< SOL_MAX_RT, default = 36 secs.
    Time m_renew;                 //!< Time after which lease should be renewed.
    Time m_rebind;                //!< Time after which client should send a Rebind message.
    Time m_prefLifetime;          //!< Preferred lifetime of the address.
    Time m_validLifetime;         //!< Valid lifetime of the address.
    EventId m_solicitEvent;       //!< Event ID for the Solicit event.
    EventId m_renewEvent;         //!< Event ID for the Renew event.
    EventId m_rebindEvent;        //!< Event ID for the rebind event
    TracedCallback<const Ipv6Address&> m_newLease; //!< Trace the new lease.

    /// Track the IPv6 Address - IAID association.
    std::unordered_map<Ipv6Address, uint32_t, Ipv6AddressHash> m_iaidMap;

    /// List of addresses to be declined by the client.
    std::vector<Ipv6Address> m_declinedAddresses;

    /// Track whether DAD callback on all addresses has been scheduled.
    bool m_addressDadComplete;

    /// Store all the Event IDs for the addresses being Released.
    std::vector<EventId> m_releaseEvent;

    /**
     * Track the IA_NA IDs being used.
     * These are initialized before sending the Solicit message.
     */
    std::vector<uint32_t> m_iaNaIds;

    /// Random variable to set transaction ID
    Ptr<RandomVariableStream> m_transactionId;

    /// Random jitter before sending the first Solicit.
    Ptr<RandomVariableStream> m_solicitJitter;

    /// Random variable used to create the IAID.
    Ptr<RandomVariableStream> m_iaidStream;
};

/**
 * \brief Stream output operator
 * \param os output stream
 * \param h the Dhcp6Client
 * \return updated stream
 */
std::ostream& operator<<(std::ostream& os, const Dhcp6Client& h);

} // namespace internetapps
} // namespace ns3

#endif
