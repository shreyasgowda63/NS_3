/*
 * Copyright (c) 2009 CTTC
 * Copyright (c) 2017 Lawrence Livermore National Laboratory
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
 * Modified By: Mathew Bielejeski <bielejeski1@llnl.gov> (Multiple time slots)
 */

#ifndef WAVEFORM_GENERATOR_H
#define WAVEFORM_GENERATOR_H

#include <ns3/event-id.h>
#include <ns3/mobility-model.h>
#include <ns3/net-device.h>
#include <ns3/nstime.h>
#include <ns3/packet.h>
#include <ns3/spectrum-channel.h>
#include <ns3/spectrum-phy.h>
#include <ns3/spectrum-value.h>
#include <ns3/trace-source-accessor.h>

#include <utility>
#include <vector>

/**
 * \file
 * \ingroup spectrum
 * ns3::WaveformGenerator class declaration.
 */

namespace ns3
{

// forward declaration
class AntennaModel;
class RandomVariableStream;
class SpectrumModel;
class SpectrumValue;

/**
 * \ingroup spectrum
 *
 * Simple SpectrumPhy implementation which transmits customizable waveforms.
 * The generated waveform is composed of multiple time slots where
 * each slot has a specific duration and power spectrum density.
 *
 * The WaveformGenerator can be thought of as a table
 * where the row index is a frequency band and the column index is time.
 * Each cell of the table is the power (in Watts) of the transmission
 * for that band at that time. The total number of rows is equal to
 * the number of bands that the generator transmits over and the
 * number of columns adds up to the total duration of the transmission.
 *
 * This PHY model supports a single antenna model instance which is
 * used for both transmission and reception (though received signals
 * are discarded by this PHY).
 */
class WaveformGenerator : public SpectrumPhy
{
  public:
    /**
     * Default Constructor
     */
    WaveformGenerator();

    /**
     * Destructor
     */
    ~WaveformGenerator() override;

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    // inherited from SpectrumPhy
    void SetChannel(Ptr<SpectrumChannel> c) override;
    void SetMobility(Ptr<MobilityModel> m) override;
    void SetDevice(Ptr<NetDevice> d) override;
    Ptr<MobilityModel> GetMobility() const override;
    Ptr<NetDevice> GetDevice() const override;
    Ptr<const SpectrumModel> GetRxSpectrumModel() const override;
    Ptr<Object> GetAntenna() const override;
    void StartRx(Ptr<SpectrumSignalParameters> params) override;

    /**
     * Append a new time slot with the specified duration
     * and Power Spectral Density to the list of time slots.
     *
     * \param duration the length of time this time slot will transmit
     * \param txs the Power Spectral Density transmitted during this time slot.
     */
    void AddTimeSlot(Time duration, Ptr<SpectrumValue> txs);

    /**
     * Remove all time slots.
     */
    void ClearTimeSlots();

    /**
     * Return the number of time slots.
     *
     * \return the number of time slots.
     */
    std::size_t TimeSlotCount() const;

    /**
     * Return the duration of the time slot at the specified index.
     *
     * \param index index of the time slot to access
     * \return the duration of the requested time slot
     */
    Time GetTimeSlotDuration(std::size_t index) const;

    /**
     * Return a pointer to the SpectrumModel stored at the specified index.
     *
     * \param index index of the time slot to access
     *
     * \return the SpectrumModel associated with the requested time slot
     * or a null pointer if the index is invalid.
     */
    Ptr<const SpectrumModel> GetTimeSlotSpectrumModel(std::size_t index) const;

    /**
     * Return a pointer to the SpectrumValue stored at the specified index.
     *
     * \param index index of the time slot to access
     *
     * \return the spectrum value associated with the requested time slot
     * or a null pointer if the index is invalid.
     */
    Ptr<const SpectrumValue> GetTimeSlotSpectrumValue(std::size_t index) const;

    /**
     * Set the AntennaModel which will be used for transmissions
     *
     * \param a the Antenna Model
     */
    void SetAntenna(Ptr<AntennaModel> a);

    /**
     * Set a fixed interval between the end of one waveform and start of the next
     *
     * This is a convenience function to easily set a constant interval
     * between waveforms. It is equivalent to creating a new ConstantRandomVariable
     * with the specified duration and calling SetInterval();
     *
     * \param duration Amount of time between the end of one waveform and the start of
     * the next waveform
     */
    void SetFixedInterval(Time duration);

    /**
     * Set the RandomVariableStream to use for generating intervals between waveforms
     *
     * \param rand A RandomVariableStream instance which will be used to calculate the
     * amount of time to wait between the end of one waveform transmission and
     * the start of the next waveform transmission
     */
    void SetInterval(Ptr<RandomVariableStream> rand);

    /**
     * Get the RandomVariableStream currently used to calculate intervals
     *
     * \return The current interval instance
     */
    Ptr<RandomVariableStream> GetInterval() const;

    /**
     * Start the waveform generator
     */
    virtual void Start();

    /**
     * Stop the waveform generator
     */
    virtual void Stop();

  private:
    /**
     * Generate a waveform according to the configured parameters
     */
    void GenerateWaveform();

    /**
     * Transmit the specified power spectrum density for the
     * specified duration of time.
     *
     * \param slotIndex the location of the time slot to transmit
     */
    void TransmitSlot(std::size_t slotIndex);

  private:
    /**
     * Stored the time slot duration and the power spectrum density values
     * for a particular time slot.
     */
    using TimeSlot = std::pair<Time, Ptr<SpectrumValue>>;

    /**
     * Stores a collection of \p TimeSlot objects representing the complete waveform.
     */
    using TimeSlots = std::vector<TimeSlot>;

  private:
    // Inherited functions
    void DoDispose() override;

  private:
    Ptr<MobilityModel> m_mobility;  //!< Mobility model
    Ptr<AntennaModel> m_antenna;    //!< Antenna model
    Ptr<NetDevice> m_netDevice;     //!< Owning NetDevice
    Ptr<SpectrumChannel> m_channel; //!< Channel

    /**
     * RNG controlling the number of seconds between the end of one
     * transmission and the start of the next transmission.
     */
    Ptr<RandomVariableStream> m_interval;

    TimeSlots m_timeSlots;  //!< Tx PSD
    std::size_t m_nextSlot; //!< Index of the next time slot to transmit
    Time m_startTime;       //!< Start time
    EventId m_nextEvent;    //!< Next waveform generation event

    /// Callback triggered at the start of the waveform transmission
    TracedCallback<Ptr<const Packet>> m_phyTxStartTrace;

    /// Callback triggered at the end of the waveform transmission
    TracedCallback<Ptr<const Packet>> m_phyTxEndTrace;
}; // class WaveformGenerator

} // namespace ns3

#endif /* WAVEFORM_GENERATOR_H */
