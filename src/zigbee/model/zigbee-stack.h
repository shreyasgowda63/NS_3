/*
 * Copyright (c) 2024 Tokushima University, Japan
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
 * Author:
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#ifndef ZIGBEE_STACK_H
#define ZIGBEE_STACK_H

#include "zigbee-nwk.h"

#include <ns3/lr-wpan-mac-base.h>
#include <ns3/lr-wpan-net-device.h>
#include <ns3/traced-callback.h>

#include <stdint.h>
#include <string>

namespace ns3
{

class Node;

namespace zigbee
{

class ZigbeeNwk;

/**
 * \ingroup zigbee
 *
 * \brief Zigbee protocol stack to device interface.
 *
 * This class is an encapsulating class representing the protocol stack as described
 * by the Zigbee Specification. In the current implementation only the Zigbee
 * network layer (NWK) is included. However, this class is meant be later
 * be extended to include other layer and sublayers part of the Zigbee Specification.
 * The implementation is analogous to a NetDevice which encapsulates PHY and
 * MAC layers and provide the necessary hooks. Zigbee Stack is meant to encapsulate
 * NWK, APS, ZLC layers (and others if applicable).
 */
class ZigbeeStack : public Object
{
  public:
    /**
     * Get the type ID.
     *
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * Default constructor
     */
    ZigbeeStack();
    ~ZigbeeStack() override;
    /**
     * Get the Channel object of the underlying LrWpanNetDevice
     * \return The LrWpanNetDevice Channel Object
     */
    Ptr<Channel> GetChannel() const;
    /**
     * Get the node currently using this ZigbeeStack.
     * \return The reference to the node object using this ZigbeeStack.
     */
    Ptr<Node> GetNode() const;

    /**
     * Get the NWK layer used by this ZigbeeStack.
     *
     * \return the NWK object
     */
    Ptr<ZigbeeNwk> GetNwk() const;
    /**
     * Set the NWK layer used by this ZigbeeStack.
     *
     * \param nwk The NWK layer object
     */
    void SetNwk(Ptr<ZigbeeNwk> nwk);
    /**
     * \brief Returns a smart pointer to the underlying LrWpanNetDevice.
     *
     * \return A smart pointer to the underlying LrWpanNetDevice.
     */
    Ptr<lrwpan::LrWpanNetDevice> GetLrWpanNetDevice() const;
    /**
     * \brief Setup Zigbee to be the next set of higher layers for the specified LrWpanNetDevice.
     * All the packets incoming and outgoing from the LrWpanNetDevice will be
     * processed ZigbeeNetDevice.
     *
     * \param [in] lrwpanDevice A smart pointer to the LrWpanNetDevice used by Zigbee.
     */
    void SetLrWpanNetDevice(Ptr<lrwpan::LrWpanNetDevice> lrwpanDevice);

  protected:
    /**
     * Dispose of the Objects used by the ZigbeeStack
     */
    void DoDispose() override;

    /**
     * Initialize of the Objects used by the ZigbeeStack
     */
    void DoInitialize() override;

  private:
    /**
     * Configure NWK, APS layers, connect to the underlying MAC layer.
     */
    void CompleteConfig();

    Ptr<lrwpan::LrWpanMacBase> m_mac; //!< The underlying LrWpan MAC connected to this Zigbee Stack.
    Ptr<ZigbeeNwk> m_nwk;             //!< The Zigbee Network layer.
    Ptr<Node> m_node;                 //!< The node associated with this NetDevice.
    Ptr<lrwpan::LrWpanNetDevice>
        m_lrwpanNetDevice; //!< Smart pointer to the underlying LrWpanNetDevice.
};

} // namespace zigbee
} // namespace ns3

#endif /* ZIGBEE_STACK_H */
