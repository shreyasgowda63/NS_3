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

#ifndef WAVEFORM_CONFIG_LOADER_H_
#define WAVEFORM_CONFIG_LOADER_H_

#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/ptr.h"

#include <memory>

/**
 * \file
 * \ingroup spectrum
 * ns3::WaveformConfigLoader class declaration.
 */

namespace ns3
{

// forward declaration
class SpectrumChannel;

/**
 * Parses complex waveform descriptions, creates WaveformGenerator
 * objects from the descriptions, and adds the generator objects to the
 * supplied nodes.
 *
 * The complex waveform generator configuration file
 * has the following syntax:
 *
 * \verbatim
  #Comment line
  begin waveform
  node <nodePosition>
  interval constant <value>
  interval custom <value>
  interval random <min> <max>
  band <centerFrequency> <width>
  txslot <duration> <defaultValue>
  dbm <centerFrequency> <value>
  end waveform \endverbatim
 *
 * The following rules must be followed when creating a configuration file.
 * - Only one command per line
 * - The order of commands is important.
 * - Comments can appear on their own line or at the end of a line.
 * - Commands are case sensitive.
 * - Multiple waveforms can be defined in a single file.
 * - String arguments must be surrounded by double quotes.
 *
 * Commands:
<table style="border-collapse:collapse;table-layout:fixed;width:100%;border:1px solid black">
    <tr>
        <th style="border:1px solid black;width:10%">Command</th>
        <th style="border:1px solid black;width:10%">Follows</th>
        <th style="border:1px solid black;width:7%">Min Occurs</th>
        <th style="border:1px solid black;width:7%">Max Occurs</th>
        <th style="border:1px solid black;width:10%">Argument Name</th>
        <th style="border:1px solid black;width:10%">Argument Type</th>
        <th style="border:1px solid black">Description</th>
    </tr>
    <tr>
        <td style="border:1px solid black">begin waveform</td>
        <td style="border:1px solid black">
            <ul style="list-style-type:none;padding:0;margin:0">
                <li>comment</li>
                <li>blank line</li>
                <li>end waveform</li>
            </ul>
        </td>
        <td style="border:1px solid black;text-align:center">0</td>
        <td style="border:1px solid black;text-align:center">Unbounded</td>
        <td style="border:1px solid black;text-align:center"></td>
        <td style="border:1px solid black;text-align:center"></td>
        <td style="border:1px solid black">Indicates the start of a new waveform.</td>
    </tr>
    <tr>
        <td style="border:1px solid black">node</td>
        <td style="border:1px solid black">begin waveform</td>
        <td style="border:1px solid black;text-align:center">1</td>
        <td style="border:1px solid black;text-align:center">1</td>
        <td style="border:1px solid black;text-align:center">nodePosition</td>
        <td style="border:1px solid black;text-align:center">Positive Integer</td>
        <td style="border:1px solid black">
            <p>
            Specifies the location of the node in the supplied node list that
            will store the waveform generator
            </p>
        </td>
    </tr>
    <tr>
        <td style="border:1px solid black">interval constant</td>
        <td style="border:1px solid black">node</td>
        <td style="border:1px solid black;text-align:center">0</td>
        <td style="border:1px solid black;text-align:center">1</td>
        <td style="border:1px solid black;text-align:center">value</td>
        <td style="border:1px solid black;text-align:center">Positive Integer</td>
        <td style="border:1px solid black">
            <p>
            Specifies a constant amount of simulator time (in seconds) between
            the end of one waveform transmission and the start of the next
            transmission.
            </p>
            <p>This command is shorthand for:</p>
            <code>
            interval custom "ns3::ConstantRandomVariable[Constant=value]"
            </code>
            <p>
            Only one interval type can be specified in a waveform description,
            if this interval type is included, the other two types must be omitted.
            </p>
        </td>
    </tr>
    <tr>
        <td style="border:1px solid black">interval custom</td>
        <td style="border:1px solid black">node</td>
        <td style="border:1px solid black;text-align:center">0</td>
        <td style="border:1px solid black;text-align:center">1</td>
        <td style="border:1px solid black;text-align:center">value</td>
        <td style="border:1px solid black;text-align:center">String</td>
        <td style="border:1px solid black">
            <p>
            This interval type is used to specify a custom ns3 class that is
            used to control the amount of simulator time (in seconds) between
            the end of one waveform transmission and the start of the next
            transmission.
            </p>
            <p>
            The argument must be a string that contains the serialized form of
            an ns3 class that inherits from ns3::RandomVariableStream.
            </p>
            <p>
            Only one interval type can be specified in a waveform description,
            if this interval type is included, the other two types must be
            omitted.
            </p>
        </td>
    </tr>
    <tr>
        <td style="border:1px solid black">interval random</td>
        <td style="border:1px solid black">node</td>
        <td style="border:1px solid black;text-align:center">0</td>
        <td style="border:1px solid black;text-align:center">1</td>
        <td style="border:1px solid black;text-align:center;padding:1px 0">
            <ul style="list-style-type:none;padding:0;margin:0">
                <li style="padding:3em 0">min</li>
                <li style="border-top:1px solid black;padding:3em 0">max</li>
            </ul>
        </td>
        <td style="border:1px solid black;text-align:center;padding:1px 0">
        </td>
            <ul style="list-style-type:none;padding:0;margin:0">
                <li style="padding:3em 0">Positive Integer</li>
                <li style="border-top:1px solid black;padding:3em 0">Positive Integer</li>
            </ul>
        <td style="border:1px solid black">
            <p>
            Specifies a random amount of simulator time (in seconds) between
            the end of one waveform transmission and the start
            of the next transmission.
            </p>
            <p>
            The min and max arguments are used to control the minimum and
            maximum amount of time between transmissions.
            </p>
            <p>
            This command is shorthand for:
            </p>
            <code>
            interval custom "ns3::UniformRandomVariable[Min=min|Max=max]"
            </code>
            <p>
            Only one interval type can be specified in a waveform description,
            if this interval type is included, the other two types must be
            omitted.
            </p>
        </td>
    </tr>
    <tr>
        <td style="border:1px solid black">band</td>
        <td style="border:1px solid black">
            <ul style="list-style-type:none;padding:0;margin:0">
                <li>band</li>
                <li>interval</li>
            </ul>
        </td>
        <td style="border:1px solid black;text-align:center">1</td>
        <td style="border:1px solid black;text-align:center">Unbounded</td>
        <td style="border:1px solid black;text-align:center;padding:1px 0">
            <ul style="list-style-type:none;padding:0;margin:0">
                <li style="padding:3em 0">centerFrequency</li>
                <li style="border-top:1px solid black;padding:3em 0">width</li>
            </ul>
        </td>
        <td style="border:1px solid black;text-align:center;padding:1px 0">
            <ul style="list-style-type:none;padding:0;margin:0">
                <li style="padding:3em 0">Positive Double</li>
                <li style="border-top:1px solid black;padding:3em 0">Positive Double</li>
            </ul>
        </td>
        <td style="border:1px solid black">
            <p>Defines a frequency range over which noise will be transmitted.</p>
            <p>
            The first argument specifies the center frequency of the band,
            in Hz
            </p>
            <p>
            The second argument specifies the width of the band, in Hz, which is
            used to set the lower and upper frequency bounds.
            </p>
            <p>The range of the frequency is calculated as:</p>
            <pre>[centerFrequency-floor(width/2), centerFrequency+floor(width/2)]</pre>
        </td>
    </tr>
    <tr>
        <td style="border:1px solid black">txslot</td>
        <td style="border:1px solid black">
            <ul style="list-style-type:none;padding:0;margin:0">
                <li>band</li>
                <li>dbm</li>
                <li>txslot</li>
            </ul>
        </td>
        <td style="border:1px solid black;text-align:center">1</td>
        <td style="border:1px solid black;text-align:center">Unbounded</td>
        <td style="border:1px solid black;text-align:center;padding:1px 0">
            <ul style="list-style-type:none;padding:0;margin:0">
                <li style="padding:3em 0">duration</li>
                <li style="border-top:1px solid black;padding:3em 0">defaultValue</li>
            </ul>
        </td>
        <td style="border:1px solid black;text-align:center;padding:1px 0">
            <ul style="list-style-type:none;padding:0;margin:0">
                <li style="padding:3em 0">Positive Double</li>
                <li style="border-top:1px solid black;padding:3em 0">Double</li>
            </ul>
        </td>
        <td style="border:1px solid black">
            <p>Adds a new transmission slot to the waveform generator.</p>
            <p>
            The first argument specifies the duration of the slot, in
            milliseconds.
            </p>
            <p>
            The second argument specifies the default power value (in dBm)
            that will be used for all bands during the transmission.
            </p>
            <p>
            The power value for specific bands can be modified from the
            default by using the dbm command.
            </p>
        </td>
    </tr>
    <tr>
        <td style="border:1px solid black">dbm</td>
        <td style="border:1px solid black">
            <ul style="list-style-type:none;padding:0;margin:0">
                <li>dbm</li>
                <li>txslot</li>
            </ul>
        </td>
        <td style="border:1px solid black;text-align:center">0</td>
        <td style="border:1px solid black;text-align:center">Number of band entries</td>
        <td style="border:1px solid black;text-align:center;padding:1px 0">
            <ul style="list-style-type:none;padding:0;margin:0">
                <li style="padding:3em 0">centerFrequency</li>
                <li style="border-top:1px solid black;padding:3em 0">value</li>
            </ul>
        </td>
        <td style="border:1px solid black;text-align:center;padding:1px 0">
            <ul style="list-style-type:none;padding:0;margin:0">
                <li style="padding:3em 0">Positive Double</li>
                <li style="border-top:1px solid black;padding:3em 0">Double</li>
            </ul>
        </td>
        <td style="border:1px solid black">
            <p>
            Changes the power value for a particular band on the current
            transmission slot.
            </p>
            <p>
            The first argument specifies the centerFrequency of the band that
            will be modified.  This value must match the center frequency of
            a band defined for the current waveform.
            </p>
            <p>
            The second argument is the new value (in dBm) that will be used
            for the band.
            </p>
        </td>
    </tr>
    <tr>
        <td style="border:1px solid black">end waveform</td>
        <td style="border:1px solid black">
            <ul style="list-style-type:none;padding:0;margin:0">
                <li>dbm</li>
                <li>txslot</li>
            </ul>
        </td>
        <td style="border:1px solid black;text-align:center">0</td>
        <td style="border:1px solid black;text-align:center">Number of begin waveforms</td>
        <td style="border:1px solid black;text-align:center">None</td>
        <td style="border:1px solid black;text-align:center">N/A</td>
        <td style="border:1px solid black">
            <p>
            Signals the end of the current waveform. All parameters collected
            between the begin waveform/end waveform commands will be used
            to generate a new complex waveform generator object.
            </p>
        </td>
    </tr>
</table>
 *
 * Examples:
 * \verbatim
   #Waveform 1, constant interval
   begin waveform
   node 0
   interval constant 100
   band 2.412e9 2.0e7  #Frequency: 2412Mhz, Width: 20Mhz
   band 2.437e9 2.0e7
   band 2.462e9 2.0e7
   txslot 20 -70   #First slot, 20ms duration, default values only
   txslot 40 -70   #Second slot, 40ms duration
   dbm 2.412e9 -32 #Change default value in second slot for band 2412Mhz
   dbm 2.437e9 -47 #Change default value in second slot for band 2437Mhz
   end waveform

   #Waveform 2, random interval
   begin waveform
   node 2
   interval random 10 200
   band 2.422e9 4.0e7
   txslot 30 -40
   end waveform

   #Waveform 3, custom interval
   begin waveform
   node 1
   interval custom "ns3::SequentialRandomVariable[Min=20|Max=500|Increment=10|Consecutive=5]"
   band 5.180e9 2.0e7  #802.11 5Ghz Channel 36
   band 5.210e9 8.0e7  #802.11 5Ghz Channel 42
   txslot 10 -70
   dbm 5.180e9 -32
   dbm 5.210e9 -44
   txslot 35 -65
   end waveform
  \endverbatim
 *
 */
class WaveformConfigLoader
{
  public:
    /** Default Constructor */
    WaveformConfigLoader();

    /** Destructor */
    ~WaveformConfigLoader();

    /**
     * Creates WaveformGenerator objects using the configuration
     * data stored in the file at \p filepath and attaches the objects to
     * the nodes stored in \p nodes.
     *
     * Each net device returned by this function contains a
     * WaveformGenerator object which can be accessed using
     * the following function calls:
     * \code
     * netDevice->GetObject<NonCommunicatingNetDevice> ()->GetPhy ()->GetObject<WaveformGenerator>
     * (); \endcode
     *
     * \param filepath location where the configuration file can be found.
     * \param channel
     * \param nodes The list of nodes that will receive the complex waveform
     * generators created by the loader.
     *
     * \return a container of net devices that were created from the data
     * in the configuration file.
     */
    NetDeviceContainer Load(const std::string& filepath,
                            Ptr<SpectrumChannel> channel,
                            NodeContainer& nodes);

    /**
     * Creates WaveformGenerator objects using the configuration
     * data stored in \p stream and attaches the objects to the nodes stored
     * in \p nodes.
     *
     * Each net device returned by this function contains a
     * WaveformGenerator object which can be accessed using
     * the following function calls:
     * \code
     * netDevice->GetObject<NonCommunicatingNetDevice> ()->GetPhy ()->GetObject<WaveformGenerator>
     * (); \endcode
     *
     * \param stream input stream containing configuration data.
     * \param channel
     * \param nodes The list of nodes that will receive the complex waveform
     * generators created by the loader.
     *
     * \return a container of net devices that were created from the data
     * in the configuration file.
     */
    NetDeviceContainer Load(std::istream& stream,
                            Ptr<SpectrumChannel> channel,
                            NodeContainer& nodes);

}; // class WaveformConfigLoader

} // namespace ns3

#endif
