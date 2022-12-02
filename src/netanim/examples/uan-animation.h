/*
 * Copyright (c) 2010 Andrea Sacco
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
 * Author: Andrea Sacco <andrea.sacco85@gmail.com>
 */

#ifndef UAN_CW_EXAMPLE_H
#define UAN_CW_EXAMPLE_H

#include "ns3/network-module.h"
#include "ns3/uan-module.h"

using namespace ns3;

/**
 * \class NetAnimExperiment
 * \brief Helper class for UAN CW MAC example
 *
 */
class NetAnimExperiment
{
  public:
    /**
     * Run function
     * \param uan the UAN helper
     */
    void Run(UanHelper& uan);
    /**
     * Receive packet function
     * \param socket the socket to receive from
     */
    void ReceivePacket(Ptr<Socket> socket);
    /**
     * Update positions function
     * \param nodes the collection of nodes
     */
    void UpdatePositions(NodeContainer& nodes);
    /// Reset data function
    void ResetData();
    /**
     * Increment CW function
     * \param cw the CW
     */
    void IncrementCw(uint32_t cw);
    uint32_t m_numNodes{15};   ///< number of nodes
    uint32_t m_dataRate{80};   ///< data rate
    double m_depth{70};        ///< depth
    double m_boundary{500};    ///< boundary
    uint32_t m_packetSize{32}; ///< packet size
    uint32_t m_bytesTotal{0};  ///< bytes total
    uint32_t m_cwMin{10};      ///< CW minimum
    uint32_t m_cwMax{400};     ///< CW maximum
    uint32_t m_cwStep{10};     ///< CW step
    uint32_t m_avgs{3};        ///< averages

    Time m_slotTime; ///< slot time
    Time m_simTime;  ///< simulation time

    std::vector<double> m_throughputs; ///< throughputs

    /// the experiment
    NetAnimExperiment();
};

#endif /* UAN_CW_EXAMPLE_H */
