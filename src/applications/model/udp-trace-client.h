/*
 *  Copyright (c) 2007,2008, 2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                              <amine.ismail@udcast.com>
 */

#ifndef UDP_TRACE_CLIENT_H
#define UDP_TRACE_CLIENT_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ipv4-address.h"
#include "ns3/ptr.h"

#include <optional>
#include <vector>

namespace ns3
{

class Socket;
class Packet;

/**
 * \ingroup udpclientserver
 *
 * \brief A trace based streamer
 *
 * Sends UDP packets based on a trace file of a MPEG4 stream.
 * Trace files can be downloaded from:
 * https://web.archive.org/web/20210113211420/http://trace.eas.asu.edu/mpeg4/index.html
 * (the 2 first lines of the file should be removed) A valid trace file is a file with 4 columns:
 * \li -1- the first one represents the frame index
 * \li -2- the second one indicates the type of the frame: I, P or B
 * \li -3- the third one indicates the time on which the frame was generated by the encoder
 * (integer, milliseconds) \li -4- the fourth one indicates the frame size in byte
 *
 * Additional trace files can be generated from MPEG4 files using the tool
 * available in https://pypi.org/project/trace-extractor/
 *
 * If no valid MPEG4 trace file is provided to the application the trace from
 * g_defaultEntries array will be loaded.
 *
 * Also note that:
 * \li -1- consecutive 'B' frames are sent together,
 * \li -2- any trace file is (by default) read again once finished (loop).
 *
 * The latter behavior can be changed through the "TraceLoop" attribute.
 */
class UdpTraceClient : public Application
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    UdpTraceClient();
    ~UdpTraceClient() override;

    static constexpr uint16_t DEFAULT_PORT{100}; //!< default port

    /**
     * \brief set the remote address and port
     * \param ip remote IP address
     * \param port remote port
     * \deprecated Deprecated since ns-3.43. Use SetRemote without port parameter instead.
     */
    NS_DEPRECATED_3_43("Use SetRemote without port parameter instead")
    void SetRemote(const Address& ip, uint16_t port);

    /**
     * \brief set the remote address
     * \param addr remote address
     */
    void SetRemote(const Address& addr);

    /**
     * \brief Set the trace file to be used by the application
     * \param filename a path to an MPEG4 trace file formatted as follows:
     *  Frame No Frametype   Time[ms]    Length [byte]
     *  Frame No Frametype   Time[ms]    Length [byte]
     *  ...
     */
    void SetTraceFile(const std::string& filename);

    /**
     * \brief Return the maximum packet size
     * \return the maximum packet size
     */
    uint16_t GetMaxPacketSize();

    /**
     * \brief Set the maximum packet size
     * \param maxPacketSize The maximum packet size
     */
    void SetMaxPacketSize(uint16_t maxPacketSize);

    /**
     * \brief Set the trace loop flag
     * \param traceLoop true if the trace should be re-used
     */
    void SetTraceLoop(bool traceLoop);

  private:
    void StartApplication() override;
    void StopApplication() override;

    /**
     * \brief Set the remote port (temporary function until deprecated attributes are removed)
     * \param port remote port
     */
    void SetPort(uint16_t port);

    /**
     * \brief Get the remote port (temporary function until deprecated attributes are removed)
     * \return the remote port
     */
    uint16_t GetPort() const;

    /**
     * \brief Get the remote address (temporary function until deprecated attributes are removed)
     * \return the remote address
     */
    Address GetRemote() const;

    /**
     * \brief Load a trace file
     * \param filename the trace file path
     */
    void LoadTrace(const std::string& filename);

    /**
     * \brief Load the default trace
     */
    void LoadDefaultTrace();

    /**
     * \brief Send a packet
     */
    void Send();

    /**
     * \brief Send a packet of a given size
     * \param size the packet size
     */
    void SendPacket(uint32_t size);

    /**
     * \brief Entry to send.
     *
     * Each entry represents an MPEG frame
     */
    struct TraceEntry
    {
        uint32_t timeToSend; //!< Time to send the frame
        uint32_t packetSize; //!< Size of the frame
        char frameType;      //!< Frame type (I, P or B)
    };

    uint32_t m_sent;                    //!< Counter for sent packets
    Ptr<Socket> m_socket;               //!< Socket
    Address m_peer;                     //!< Peer address
    std::optional<uint16_t> m_peerPort; //!< Remote peer port (deprecated) // NS_DEPRECATED_3_43
    Address m_local;                    //!< Local address to bind to
    uint8_t m_tos;                      //!< The packets Type of Service
    EventId m_sendEvent;                //!< Event to send the next packet

    std::vector<TraceEntry> m_entries;    //!< Entries in the trace to send
    uint32_t m_currentEntry;              //!< Current entry index
    static TraceEntry g_defaultEntries[]; //!< Default trace to send
    uint16_t m_maxPacketSize; //!< Maximum packet size to send (including the SeqTsHeader)
    bool m_traceLoop;         //!< Loop through the trace file
};

} // namespace ns3

#endif /* UDP_TRACE_CLIENT_H */
