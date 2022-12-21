/*
 * Copyright (c) 2018 Lawrence Livermore National Laboratory
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
 * Author: Mathew Bielejeski <bielejeski1@llnl.gov>
 */

#ifndef ADVANCED_WAVEFORM_GENERATOR_HELPER_H
#define ADVANCED_WAVEFORM_GENERATOR_HELPER_H

#include "ns3/attribute.h"
#include "ns3/net-device-container.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/queue.h"
#include "ns3/spectrum-model.h"

#include <string>

/**
 * \file
 * \ingroup spectrum
 * ns3::TransmitSlice struct declaration.
 * ns3::AdvancedWaveformGeneratorHelper class declaration.
 */

namespace ns3
{

// forward declarations
class SpectrumChannel;
class SpectrumValue;

/**
 * \ingroup spectrum
 *
 * Helper class for creating complex waveform generators.
 */
class AdvancedWaveformGeneratorHelper
{
  public:
    /**
     * Default Constructor
     */
    AdvancedWaveformGeneratorHelper();

    /**
     * Destructor
     */
    ~AdvancedWaveformGeneratorHelper();

    /**
     * Set the SpectrumChannel that will be used by SpectrumPhy instances
     * created by this helper
     *
     * \param channel the channel that the waveform generator
     * will use for transmission
     */
    void SetChannel(Ptr<SpectrumChannel> channel);

    /**
     * Set the SpectrumChannel that will be used by SpectrumPhy instances
     * created by this helper
     *
     * \param channelName the global name of a channel
     */
    void SetChannel(std::string channelName);

    /**
     *
     * Create a new SpectrumModel using the specified bands.
     *
     * \param bands list of frequencies that will be transmitted
     */
    void SetBands(const Bands& bands);

    /**
     *
     * Create a new SpectrumModel using the pair of iterators to specify
     * the BandInfo for the model.
     *
     * This is a convenience function to set the bands using a pair of
     * iterators without needing to create an intermediate Bands container.
     *
     * \param begin Iterator pointing to the first band in the collection.
     * \param end Iterator pointing to one past the end of the collection.
     */
    void SetBands(Bands::const_iterator begin, Bands::const_iterator end);

    /**
     *
     * Use the specified SpectrumModel when generating SpectrumValue
     * objects.
     * The SpectrumModel specifies the set of frequency bands that the
     * waveform generator will operate on.
     *
     * \param model spectrum model containing the list of frequency bands
     */
    void SetModel(Ptr<const SpectrumModel> model);

    /**
     * Append a vector of power spectral density values with a specific duration
     * to the list of power spectral density vectors.
     *
     * The number of entries in the power spectral density vector must equal
     * the number of bands in the SpectrumModel.
     *
     * \param duration amount of time this power spectral density
     * will be transmitted
     * \param psd vector of power spectral density values
     * (values must be in Watts).
     * The number of entries in the vector must be equal to the number of bands
     * specified in SetBands or SetSpectrumModel.
     *
     */
    void AddTxPowerSpectralDensity(Time duration, const std::vector<double>& psd);

    /**
     * Append a SpectrumValue with a specific duration to the list of power
     * spectral density vectors.
     *
     * \pre \p value.GetSpectrumModel() must equal the model set in SetModel(). If
     * SetModel() has not been called, the value returned by GetSpectrumModel() will
     * be passed to SetModel()
     * \pre The number of entries in the power spectral density vector must equal
     * the number of bands in the SpectrumModel.
     *
     * \param duration amount of time this power spectral density
     * will be transmitted
     * \param value Spectrum values
     */
    void AddTxPowerSpectralDensity(Time duration, Ptr<const SpectrumValue> value);

    /**
     * Set the interval between the end of one transmission and the start of the next
     *
     * \param interval The amount of time to wait after completing one transmission
     * before starting the next transmission
     */
    void SetInterval(Time interval);

    /**
     * Set multiple attributes on each SpectrumPhy created
     *
     * \tparam Ts A collection of alternating std::string, AttributeValue types
     *
     * \param name the name of the attribute to set
     * \param v the value of the attribute
     * \param args Additional name/value pairs
     *
     */
    template <class... Ts>
    void SetPhyAttribute(std::string name, const AttributeValue& v, Ts&&... args);

    /**
     * Set attribute on each NetDevice created
     *
     * \param name the name of the attribute to set
     * \param v the value of the attribute to set
     *
     */
    void SetDeviceAttribute(std::string name, const AttributeValue& v);

    /**
     * Set multiple attribute on each NetDevice created
     *
     * \tparam Ts A collection of alternating std::string, AttributeValue types
     *
     * \param name the name of the attribute to set
     * \param v the value of the attribute to set
     * \param args Additional name/value pairs
     *
     */
    template <class... Ts>
    void SetDeviceAttribute(std::string name, const AttributeValue& v, Ts&&... args);

    /**
     * Configure the AntennaModel instance for each new device created
     *
     * \tparam Ts A collection of alternating std::string, AttributeValue types
     *
     * \param type Type of the antenna model to use
     * \param args Additional name/value attribute pairs
     *
     */
    template <class... Ts>
    void SetAntenna(std::string type, Ts&&... args);

    /**
     * Set multiple attributes for the antenna model
     *
     * \tparam Ts A collection of alternating std::string, AttributeValue types
     *
     * \param name Name of an antenna model attribute
     * \param v Value of the attribute for \p name
     * \param args Additional name/value attribute pairs
     *
     */
    template <class... Ts>
    void SetAntennaAttribute(std::string name, const AttributeValue& v, Ts&&... args);

    /**
     * \param c the set of nodes on which a device must be created
     * \return a device container which contains all the devices created
     * by this method.
     */
    NetDeviceContainer Install(NodeContainer c) const;

    /**
     * \param node the node on which a device must be created
     * \return a pointer to the NetDevice that was
     * added to the supplied node.
     */
    Ptr<NetDevice> Install(Ptr<Node> node) const;
    /**
     * \param nodeName the name of node on which a device must be created
     * \return a pointer to the NetDevice that was
     * added to the supplied node.
     */
    Ptr<NetDevice> Install(std::string nodeName) const;

  protected:
    /**
     * Plain object to temporarily store data related to a time slot.
     */
    struct TransmitSlice
    {
        Time duration; //!< transmit duration

        /// vector of power spectral density values ( in Watts )
        std::vector<double> psd;
    };

    ObjectFactory m_phy;            //!< Object factory for the phy objects
    ObjectFactory m_device;         //!< Object factory for the NetDevice objects
    ObjectFactory m_antenna;        //!< Object factory for the Antenna objects
    Ptr<SpectrumChannel> m_channel; //!< Transmission channel

    Time m_interval; //!< Time between transmissions

    /// Model used when creating SpectrumValue objects
    Ptr<const SpectrumModel> m_model;
    std::vector<TransmitSlice> m_slices; //!< vector of transmit slices
};

template <class... Ts>
void
AdvancedWaveformGeneratorHelper::SetPhyAttribute(std::string name,
                                                 const AttributeValue& val,
                                                 Ts&&... args)
{
    m_phy.Set(name, val, std::forward<Ts>(args)...);
}

template <class... Ts>
void
AdvancedWaveformGeneratorHelper::SetDeviceAttribute(std::string name,
                                                    const AttributeValue& val,
                                                    Ts&&... args)
{
    m_device.Set(name, val, std::forward<Ts>(args)...);
}

template <class... Ts>
void
AdvancedWaveformGeneratorHelper::SetAntenna(std::string type, Ts&&... args)
{
    m_antenna = ObjectFactory(type, std::forward<Ts>(args)...);
}

template <class... Ts>
void
AdvancedWaveformGeneratorHelper::SetAntennaAttribute(std::string name,
                                                     const AttributeValue& val,
                                                     Ts&&... args)
{
    m_antenna.Set(name, val, std::forward<Ts>(args)...);
}

} // namespace ns3

#endif /* WAVEFORM_GENERATOR_HELPER_H */
