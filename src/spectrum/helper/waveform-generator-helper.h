/*
 * Copyright (c) 2010 CTTC
 * Copyright (c) 2020 Lawrence Livermore National Laboratory
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 * Modified By: Mathew Bielejeski <bielejeski1@llnl.gov> (Refactoring)
 */

#ifndef WAVEFORM_GENERATOR_HELPER_H
#define WAVEFORM_GENERATOR_HELPER_H

#include "ns3/attribute.h"
#include "ns3/deprecated.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/queue.h"

#include <string>

/**
 * \file
 * \ingroup spectrum
 * ns3::WaveformGeneratorHelper class declaration.
 */

namespace ns3
{

// forward declarations
class SpectrumValue;
class SpectrumChannel;

/**
 * \ingroup spectrum
 *
 * \brief Create a Waveform generator using the old waveform generator API.
 *
 * This class is deprecated and will be removed in a future release.  Users
 * should switch to the AdvancedWaveformGeneratorHelper.
 */
// Deprecating the entire class confuses doxygen and generates a bunch of
// doxygen warnings
class /* NS_DEPRECATED_3_37("Use the AdvancedWaveformGeneratorHelper") */ WaveformGeneratorHelper
{
  public:
    /**
     * Default constructor
     */
    NS_DEPRECATED_3_37("Use the AdvancedWaveformGeneratorHelper")
    WaveformGeneratorHelper();

    /**
     * Destructor
     */
    ~WaveformGeneratorHelper();

    /**
     * set the SpectrumChannel that will be used by SpectrumPhy instances created by this helper
     *
     * @param channel
     */
    void SetChannel(Ptr<SpectrumChannel> channel);

    /**
     * set the SpectrumChannel that will be used by SpectrumPhy instances created by this helper
     *
     * @param channelName
     */
    void SetChannel(std::string channelName);

    /**
     * Set the length of time from the start of one waveform to the start of the next
     * waveform
     *
     * \param duration The period of the waveform
     */
    void SetPeriod(Time duration);

    /**
     * Set the percentage of time that the wave is "on"
     *
     * The duty cycle is the percentage of the period where the waveform is transmitting.
     *
     * The duration of the "on" time is calculated as: Period * DutyCycle
     * The duration of the "off" time is calculated as: Period * (1-DutyCycle)
     *
     * \param percentage A value between 0.0 and 1.0
     */
    void SetDutyCycle(double percentage);

    /**
     *
     * @param txPsd the Power Spectral Density to be used for transmission by all created PHY
     * instances
     *
     */
    void SetTxPowerSpectralDensity(Ptr<SpectrumValue> txPsd);

    /**
     * Set attribute on each SpectrumPhy created
     *
     * \param name the name of the attribute to set
     * \param v the value of the attribute
     *
     */
    void SetPhyAttribute(std::string name, const AttributeValue& v);

    /**
     * Set multiple attributes on each SpectrumPhy created
     *
     * \tparam Ts \deduced A collection of alternating std::string, AttributeValue types
     *
     * \param name the name of the attribute to set
     * \param v the value of the attribute
     * \param args Additional name/value pairs
     *
     */
    template <class... Ts>
    void SetPhyAttribute(std::string name, const AttributeValue& v, Ts&&... args);

    /**
     * Set multiple attribute on each NetDevice created
     *
     * \tparam Ts \deduced A collection of alternating std::string, AttributeValue types
     *
     * \param name the name of the attribute to set
     * \param v the value of the attribute to set
     * \param args Additional name/value pairs
     *
     *
     */
    template <class... Ts>
    void SetDeviceAttribute(std::string name, const AttributeValue& v, Ts&&... args);

    /**
     * Configure the AntennaModel instance for each new device to be created
     *
     * \param type the type of the model to set
     *
     */
    void SetAntenna(std::string type);

    /**
     * Configure the AntennaModel instance for each new device created
     *
     * \tparam Ts \deduced Argument types
     * \param type the type of the model to set
     * \param [in] args Name and AttributeValue pairs to set.
     */
    template <typename... Ts>
    void SetAntenna(std::string type, Ts&&... args);

    /**
     * @param c the set of nodes on which a device must be created
     * @return a device container which contains all the devices created by this method.
     */
    NetDeviceContainer Install(NodeContainer c) const;
    /**
     * @param node the node on which a device must be created
     * \returns a device container which contains all the devices created by this method.
     */
    Ptr<NetDevice> Install(Ptr<Node> node) const;
    /**
     * @param nodeName the name of node on which a device must be created
     * @return a device container which contains all the devices created by this method.
     */
    Ptr<NetDevice> Install(std::string nodeName) const;

  protected:
    bool m_periodSet; //!< Flag indicating that SetPeriod has been called
    // old api specific fields
    Time m_period; //!< Length of time from the start of one waveform to the next

    bool m_dutyCycleSet; //!< Flag indicating that SetDutyCycle has been called
    /**
     * Percentage of m_period that the waveform is "on"
     * The value must be between 0.0 and 1.0
     */
    double m_dutyCycle;

    ObjectFactory m_phy;            //!< Object factory for the phy objects
    ObjectFactory m_device;         //!< Object factory for the NetDevice objects
    ObjectFactory m_antenna;        //!< Object factory for the Antenna objects
    Ptr<SpectrumChannel> m_channel; //!< Channel
    Ptr<SpectrumValue> m_txPsd;     //!< Tx power spectral density
};

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/
template <class... Ts>
void
WaveformGeneratorHelper::SetPhyAttribute(std::string attribName,
                                         const AttributeValue& v,
                                         Ts&&... args)
{
    SetPhyAttribute(attribName, v);
    SetPhyAttribute(std::forward<Ts>(args)...);
}

template <class... Ts>
void
WaveformGeneratorHelper::SetDeviceAttribute(std::string name, const AttributeValue& v, Ts&&... args)
{
    m_device.Set(name, v, std::forward<Ts>(args)...);
}

template <typename... Ts>
void
WaveformGeneratorHelper::SetAntenna(std::string type, Ts&&... args)
{
    m_antenna = ObjectFactory(type, std::forward<Ts>(args)...);
}

} // namespace ns3

#endif /* WAVEFORM_GENERATOR_HELPER_H */
