#ifndef IPV4_RAW_SOCKET_IMPL_H
#define IPV4_RAW_SOCKET_IMPL_H

#include "ipv4-header.h"
#include "ipv4-interface.h"
#include "ipv4-route.h"

#include "ns3/socket.h"

#include <list>
#include <unordered_set>

namespace ns3
{

class NetDevice;
class Node;

/**
 * \ingroup socket
 * \ingroup ipv4
 *
 * \brief IPv4 raw socket.
 *
 * A RAW Socket typically is used to access specific IP layers not usually
 * available through L4 sockets, e.g., ICMP. The implementer should take
 * particular care to define the Ipv4RawSocketImpl Attributes, and in
 * particular the Protocol attribute.
 */
class Ipv4RawSocketImpl : public Socket
{
  public:
    /**
     * \brief Get the type ID of this class.
     * \return type ID
     */
    static TypeId GetTypeId();

    Ipv4RawSocketImpl();

    /**
     * \brief Set the node associated with this socket.
     * \param node node to set
     */
    void SetNode(Ptr<Node> node);

    Socket::SocketErrno GetErrno() const override;

    /**
     * \brief Get socket type (NS3_SOCK_RAW)
     * \return socket type
     */
    Socket::SocketType GetSocketType() const override;

    Ptr<Node> GetNode() const override;
    int Bind(const Address& address) override;
    int Bind() override;
    int Bind6() override;
    int GetSockName(Address& address) const override;
    int GetPeerName(Address& address) const override;
    int Close() override;
    int ShutdownSend() override;
    int ShutdownRecv() override;
    int Connect(const Address& address) override;
    int Listen() override;
    uint32_t GetTxAvailable() const override;
    int Send(Ptr<Packet> p, uint32_t flags) override;
    int SendTo(Ptr<Packet> p, uint32_t flags, const Address& toAddress) override;
    uint32_t GetRxAvailable() const override;
    Ptr<Packet> Recv(uint32_t maxSize, uint32_t flags) override;
    Ptr<Packet> RecvFrom(uint32_t maxSize, uint32_t flags, Address& fromAddress) override;
    int MulticastJoinGroup(uint32_t interface, const Address& groupAddress) override;
    int MulticastLeaveGroup(uint32_t interface, const Address& groupAddress) override;

    /**
     * \brief Set protocol field.
     * \param protocol protocol to set
     */
    void SetProtocol(uint8_t protocol);

    /**
     * \brief Forward up to receive method.
     * \param p packet
     * \param ipHeader IPv4 header
     * \param incomingInterface incoming interface
     * \return true if forwarded, false otherwise
     */
    bool ForwardUp(Ptr<const Packet> p, Ipv4Header ipHeader, Ptr<Ipv4Interface> incomingInterface);
    bool SetAllowBroadcast(bool allowBroadcast) override;
    bool GetAllowBroadcast() const override;

  private:
    void DoDispose() override;

    /**
     * \struct Data
     * \brief IPv4 raw data and additional information.
     */
    struct Data
    {
        Ptr<Packet> packet;    /**< Packet data */
        Ipv4Address fromIp;    /**< Source address */
        uint16_t fromProtocol; /**< Protocol used */
    };

    mutable Socket::SocketErrno m_err; //!< Last error number.
    Ptr<Node> m_node;                  //!< Node
    Ipv4Address m_src;                 //!< Source address.
    Ipv4Address m_dst;                 //!< Destination address.
    uint8_t m_protocol;                //!< Protocol.
    std::list<Data> m_recv;            //!< Packet waiting to be processed.
    bool m_shutdownSend;               //!< Flag to shutdown send capability.
    bool m_shutdownRecv;               //!< Flag to shutdown receive capability.
    uint32_t m_icmpFilter;             //!< ICMPv4 filter specification
    bool m_iphdrincl; //!< Include IP Header information (a.k.a setsockopt (IP_HDRINCL))

    /**
     * Hash of an Ipv4Address, uint32_t pair
     */
    struct Ipv4InterfacePairHash
    {
      public:
        /**
         * \brief Returns the hash of an IPv4 address / interface pair.
         * \param x the IPv4 address / interface pair
         * \return the hash
         *
         * This method uses Ipv4AddressHash and std::hash, as speed is
         * more important than perfection.
         */
        std::size_t operator()(const std::pair<Ipv4Address, uint32_t>& x) const
        {
            return Ipv4AddressHash()(x.first) ^ std::hash<uint32_t>()(x.second);
        }
    };

    /**
     * IPv4 multicast addresses this endpoint is receiving
     */
    std::unordered_set<std::pair<Ipv4Address, uint32_t>, Ipv4InterfacePairHash>
        m_multicastAddresses;
};

} // namespace ns3

#endif /* IPV4_RAW_SOCKET_IMPL_H */
