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

#include "dhcp6-header.h"

#include "ns3/application.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/traced-callback.h"

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

    /**
     * \brief Get the identifier option.
     * \return The client's identifier.
     */
    IdentifierOption GetSelfIdentifier();

    int64_t AssignStreams(int64_t stream) override;

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * State of the DHCPv6 client.
     */
    enum State
    {
        WAIT_ADVERTISE = 1,           // Waiting for an advertise message
        WAIT_REPLY = 2,               // Waiting for a reply message
        RENEW = 3,                    // Renewing the lease
        WAIT_REPLY_AFTER_DECLINE = 4, // Waiting for a reply after sending a decline message
        WAIT_REPLY_AFTER_RELEASE = 5, // Waiting for a reply after sending a release message
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
    void ProcessReply(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress server);

    /**
     * \brief Check lease status after sending a Decline or Release message.
     * \param iDev The net device of the client
     * \param header The DHCPv6 header
     * \param server The address of the server
     */
    void CheckLeaseStatus(Ptr<NetDevice> iDev, Dhcp6Header header, Inet6SocketAddress server);

    /**
     * \brief Accept the DHCPv6 offer.
     * \param offeredAddress The IPv6 address that has been accepted.
     */
    void AcceptedAddress(const Ipv6Address& offeredAddress);

    /**
     * \brief Add a declined address to the list maintained by the client.
     * \param offeredAddress The IPv6 address to be declined.
     */
    void AddDeclinedAddress(const Ipv6Address& offeredAddress);

    /**
     * \brief Send a Decline message to the DHCPv6 server
     */
    void DeclineOffer();

    /**
     * \brief Send a renew message to the DHCPv6 server.
     * \param iaidList The IAIDs whose leases are to be renewed.
     */
    void SendRenew(std::vector<uint32_t> iaidList);

    /**
     * \brief Send a rebind message to the DHCPv6 server.
     * \param iaidList The IAIDs whose leases are to be rebound.
     */
    void SendRebind(std::vector<uint32_t> iaidList);

    /**
     * \brief Send a Release message to the DHCPv6 server.
     * \param address The address whose lease is to be released.
     */
    void SendRelease(Ipv6Address address);

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
     * The socket used for communication.
     */
    Ptr<Socket> m_socket;

    /**
     * Pointer to the net device of the client.
     */
    Ptr<NetDevice> m_device;

    /**
     * The state of the DHCPv6 client.
     */
    uint8_t m_state;

    /**
     * First boot of the client.
     */
    bool m_firstBoot;

    /**
     * Store the starting timestamp for the Elapsed Time option
     */
    Time m_startTime;

    /**
     * Store the transaction ID of the initiated message exchange.
     */
    uint32_t m_clientTransactId;

    /**
     * Store the client identifier option.
     */
    IdentifierOption m_clientIdentifier;

    /**
     * Store the server identifier option.
     */
    IdentifierOption m_serverIdentifier;

    /**
     * Track the IPv6 Address - IAID association.
     */
    std::unordered_map<Ipv6Address, uint32_t, Ipv6AddressHash> m_iaidMap;

    /**
     * List of addresses to be declined by the client.
     */
    std::vector<Ipv6Address> m_declinedAddresses;

    /**
     * Track whether DAD callback on all addresses has been scheduled.
     */
    bool m_addressDadComplete;

    /**
     * The number of addresses offered to the client.
     */
    uint8_t m_offeredAddresses;

    /**
     * The number of addresses accepted by the client.
     */
    uint8_t m_acceptedAddresses;

    /**
     * Time when message exchange starts.
     */
    Time m_msgStartTime;

    /**
     * SOL_MAX_RT value, default = 3600 / 100 = 36 sec
     */
    Time m_solicitInterval;

    /**
     * Event ID for the solicit event
     */
    EventId m_solicitEvent;

    /**
     * Time after which lease should be renewed.
     */
    Time m_renew;

    /**
     * Time after which client should send a Rebind message.
     */
    Time m_rebind;

    /**
     * Preferred lifetime of the address.
     */
    Time m_prefLifetime;

    /**
     * Valid lifetime of the address
     */
    Time m_validLifetime;

    /**
     * Event ID for the renew event
     */
    EventId m_renewEvent;

    /**
     * Event ID for the rebind event
     */
    EventId m_rebindEvent;

    /**
     * Store all the Event IDs for the addresses being Released.
     */
    std::vector<EventId> m_releaseEvent;

    /**
     * Track the latest IANA ID
     */
    uint32_t m_ianaIds;

    /**
     * Random variable to set transaction ID
     */
    Ptr<RandomVariableStream> m_transactionId;

    /**
     * Random jitter before sending the first Solicit.
     */
    Ptr<RandomVariableStream> m_solicitJitter;

    /**
     * Trace the newly obtained lease.
     */
    TracedCallback<const Ipv6Address&> m_newLease;
};

} // namespace ns3

#endif
