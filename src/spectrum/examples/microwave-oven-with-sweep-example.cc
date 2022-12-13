/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Mathew Bielejeski <bielejeski1@llnl.gov>
 */

#include "ns3/core-module.h"
#include "ns3/gnuplot.h"
#include "ns3/mobility-module.h"
#include "ns3/net-device.h"
#include "ns3/non-communicating-net-device.h"
#include "ns3/spectrum-module.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <ostream>
#include <regex>
#include <tuple>
#include <vector>

/**
 * \file
 *
 * This example creates a complex waveform generator that transmits the waveform
 * of a microwave oven (including the frequency sweep) as described in the paper
 * Microwave Oven Signal Modeling (WCNC 2008), Taher et. al
 *
 * A spectrum analyzer is used to measure the transmitted spectra from the
 * waveform generator. The file "spectrum-analyzer-microwave-1-0.tr" contains its
 * output post simulation (and can be plotted with Gnuplot or MATLAB).
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MicrowaveOvenWithSweepExample");

namespace
{

/**
 * Helper class responsible for collecting power density data
 * which is used to generate a plot in gnuplot.
 */
class SpectrumDataCollector
{
  public:
    /**
     * Default Constructor
     */
    SpectrumDataCollector()
    {
    }

    /**
     * Destructor
     */
    ~SpectrumDataCollector()
    {
    }

    /**
     * Extracts the power density per band data from the supplied
     * SpectrumValue object and stores it in an internal container.
     *
     * \param avgPowerDensity Pointer to a SpectrumValue object
     */
    void HandleCallback(Ptr<const SpectrumValue> avgPowerDensity)
    {
        auto bandBegin = avgPowerDensity->ConstBandsBegin();
        auto bandEnd = avgPowerDensity->ConstBandsEnd();
        auto valuesBegin = avgPowerDensity->ConstValuesBegin();
        auto valuesEnd = avgPowerDensity->ConstValuesEnd();

        for (; bandBegin != bandEnd; ++bandBegin)
        {
            if (valuesBegin == valuesEnd)
            {
                // no more data
                return;
            }

            m_points.emplace_back(Simulator::Now(), *bandBegin, *valuesBegin);

            ++valuesBegin;
        }
    }

    /**
     * Reports the number of data points collected so far.
     *
     * \return The number of collected data points.
     */
    std::size_t Size() const
    {
        return m_points.size();
    }

    /**
     * Retrieves the data point stored at the supplied index.
     *
     * \param index The location of the data
     *
     * \return A tuple containing the Simulator time when the data
     * was collected, information about the band, and the power density
     * for the band.
     */
    std::tuple<Time, BandInfo, double> Get(std::size_t index) const
    {
        return m_points.at(index);
    }

  private:
    /**
     * Tuple containing data for a data point.
     *
     * \tparam Time Simulator time when the data was collected.
     * \tparam BandInfo Frequency information about the band
     * \tparam double Power density for the band at the simulator time.
     */
    typedef std::tuple<Time, BandInfo, double> DataPoint;

    /**
     * A collection of DataPoint tuples.
     */
    typedef std::vector<DataPoint> PointList;

    /**
     * A collection of DataPoint tuples collected during the simulation.
     */
    PointList m_points;

}; // class SpectrumDataCollector

} // unnamed namespace

/**
 * Helper function to create one or more waveform generators from the
 * supplied arguments.  The waveform generators that are created generate
 * waveforms that simulate a microwave oven using power densities taken from the
 * paper Microwave Oven Signal Modeling (WCNC 2008), Taher et. al
 *
 * \param nodes A container of nodes which will hold the waveform generators
 * created by this function.
 * \param channel Defines the properties of the channel that the waveform generator
 * will transmit over.
 * \param useMicrowave2 Boolean value indicating whether the waveform generators
 * created should use the power spectral density from microwave model #2 instead
 * of model #1 from the paper.
 * \param generateSweep Boolean value indicating whether the sweep portion of
 * the waveform should be added to the waveform generator.
 *
 * \return a container of NetDevices which contain the newly created waveform
 * generators.
 */
NetDeviceContainer
CreateFromHelper(NodeContainer& nodes,
                 Ptr<SpectrumChannel> channel,
                 bool useMicrowave2 = false,
                 bool generateSweep = false)
{
    // Get the power spectral density for a microwave oven.
    double defaultDbm = -67;
    std::size_t sweepOffset = 13;
    Ptr<SpectrumValue> mwoPsd;

    // Helper function to convert decibals to watts
    auto decibalToWatt = [](double& db) {
        // convert dBm to dBW
        double temp = (db - 30.0) / 10.0;

        // convert to Watt
        db = std::pow(10, temp);
    };

    if (useMicrowave2)
    {
        defaultDbm = -68;
        sweepOffset = 11;
        mwoPsd = MicrowaveOvenSpectrumValueHelper::CreatePowerSpectralDensityMwo2();
    }
    else
    {
        mwoPsd = MicrowaveOvenSpectrumValueHelper::CreatePowerSpectralDensityMwo1();
    }

    // The power spectral density for the microwave oven contains the
    // transient values but does not include the frequency sweep.
    // Create the frequency sweep portion of the microwave oven output,
    // using values based on graphs in the paper:
    // Microwave Oven Signal Modeling (WCNC 2008), Taher et. al
    std::vector<double> transientValues(mwoPsd->ConstValuesBegin(), mwoPsd->ConstValuesEnd());

    AdvancedWaveformGeneratorHelper waveformGeneratorHelper;

    // Set the interval of the waveform generator to 20 milliseconds
    StringValue intervalAttribute("ns3::ConstantRandomVariable[Constant=20]");
    waveformGeneratorHelper.SetPhyAttribute("Interval", intervalAttribute);

    waveformGeneratorHelper.SetChannel(channel);
    waveformGeneratorHelper.SetBands(mwoPsd->ConstBandsBegin(), mwoPsd->ConstBandsEnd());

    // The complex waveform of the microwave is created by adding a series of
    // power spectral density arrays.
    waveformGeneratorHelper.AddTxPowerSpectralDensity(MilliSeconds(1), transientValues);

    if (generateSweep)
    {
        // Create a vector representing the beginning and end of the frequency sweep
        std::vector<double> sweepEndsValues(transientValues.size(), defaultDbm);
        // Create a vector representing the middle of the frequency sweep
        std::vector<double> sweepMiddleValues(transientValues.size(), defaultDbm);

        // Values are based on the graph in Figure 2 for MWO #1
        // Each entry in the vector represents the power at a particular frequency.
        // The frequency for a specific index can be accessed using
        // mwoPsd->ConstBandsBegin() + index.
        sweepEndsValues[sweepOffset] = -44;
        sweepEndsValues[sweepOffset + 1] = -35;
        sweepEndsValues[sweepOffset + 2] = -44;

        sweepMiddleValues[sweepOffset + 1] = -44;
        sweepMiddleValues[sweepOffset + 2] = -35;
        sweepMiddleValues[sweepOffset + 3] = -44;

        // The waveform generator expects power spectral density values to be
        // in watts, convert decibals to watts here.
        std::for_each(sweepEndsValues.begin(), sweepEndsValues.end(), decibalToWatt);
        std::for_each(sweepMiddleValues.begin(), sweepMiddleValues.end(), decibalToWatt);

        waveformGeneratorHelper.AddTxPowerSpectralDensity(MilliSeconds(2), sweepEndsValues);
        waveformGeneratorHelper.AddTxPowerSpectralDensity(MilliSeconds(2), sweepMiddleValues);
        waveformGeneratorHelper.AddTxPowerSpectralDensity(MilliSeconds(2), sweepEndsValues);
    }
    else
    {
        // If not generating the sweep, fill the gap with dead air
        double defaultWatt = defaultDbm;

        decibalToWatt(defaultWatt);

        std::vector<double> middleValues(transientValues.size(), defaultWatt);

        waveformGeneratorHelper.AddTxPowerSpectralDensity(MilliSeconds(6), middleValues);
    }

    waveformGeneratorHelper.AddTxPowerSpectralDensity(MilliSeconds(1), transientValues);

    return waveformGeneratorHelper.Install(nodes);
}

/**
 * Helper function which creates one or more waveform generators using data
 * from the supplied file.
 *
 * \param confFile File containing data describing one or more waveform
 * generators.
 * \param nodes A container of nodes which will hold the created waveform
 * generators.
 * \param channel Defines the properties of the channel that the waveform generator
 * will transmit over.
 *
 * \return a container of NetDevices which contain the newly created waveform
 * generators.
 */
NetDeviceContainer
CreateFromLoader(const std::string& confFile, NodeContainer& nodes, Ptr<SpectrumChannel> channel)
{
    WaveformConfigLoader loader;

    return loader.Load(confFile, channel, nodes);
}

/**
 * Helper function which takes a SpectrumDataCollector object
 * and generates a gnuplot file which can be used to create a plot
 * of the power density per frequency during the simulation.
 *
 * \param collector a SpectrumDataCollector which collected power density
 * data during the simulation.
 * \param outFile base name for the plot and image files.
 */
void
GeneratePlotFile(const SpectrumDataCollector& collector, const std::string outFile)
{
    std::string baseFileName = outFile;
    if (baseFileName.empty())
    {
        baseFileName = "microwave-spectrum-interference";
    }
    const std::string imageFileName = baseFileName + ".png";
    const std::string plotFileName = baseFileName + ".plt";

    std::cout << "Generating plot file: " << plotFileName << "\n";

    Gnuplot plot(imageFileName);
    plot.SetTitle("Configurable Interference Example");

    plot.SetTerminal("png");
    plot.SetLegend("time (ms)", "frequency (MHz)");
    plot.AppendExtra(R"___(set zlabel "value (dBm/Hz)")___");
    plot.AppendExtra("unset surface");
    plot.AppendExtra("set pm3d at s");
    plot.AppendExtra("set palette");

    Gnuplot3dDataset dataset;
    std::string title = "Interference source: '" + outFile + "'";
    // Avoid interpreting `_` as a subscript, by escaping it
    std::regex underscore("_");
    title = std::regex_replace(title, underscore, "\\\\_");
    dataset.SetTitle(title);

    Time prevTimestamp;
    Time timestamp;
    BandInfo band;
    double value;

    for (std::size_t i = 0; i < collector.Size(); ++i)
    {
        std::tie(timestamp, band, value) = collector.Get(i);

        if ((i > 0) && (timestamp != prevTimestamp))
        {
            dataset.AddEmptyLine();
        }

        double scaledValue = -150;
        if (value > 0)
        {
            scaledValue = 10.0 * std::log10(value);
        }

        dataset.Add(timestamp.GetMilliSeconds(), band.fc / 1e6, scaledValue);

        prevTimestamp = timestamp;
    }

    plot.AddDataset(dataset);

    std::ofstream fileStream(plotFileName);
    plot.GenerateOutput(fileStream);

    std::cout << "\nRun \"gnuplot " << plotFileName << "\"\n"
              << "to create the image file " << imageFileName << "\n\n";
}

/**
 *  Extracts the file name from the supplied file path and returns the filename.
 *
 *  \param path The file path
 *
 *  \return The file name extract from the path or an empty string if the
 *  path does not have a file name.
 */
std::string
ExtractFileName(const std::string& path)
{
    auto pos = path.find_last_of('/');

    if (pos != std::string::npos)
    {
        return path.substr(pos + 1);
    }

    return path;
}

/**
 * Function to stream the configuration of a WaveformGenerator object.
 * This function will print out the power density for each frequency for each
 * slot for the supplied WaveformGenerator object.
 *
 * \param stream Output stream which will receive the data.
 * \param generator The WaveformGenerator instance to be printed.
 *
 * \return A reference to the supplied output stream
 */
std::ostream&
operator<<(std::ostream& stream, const WaveformGenerator& generator)
{
    std::size_t numSlots = generator.TimeSlotCount();

    for (std::size_t i = 0; i < numSlots; ++i)
    {
        stream << "Slot " << i << ":\n";
        Ptr<const SpectrumValue> value = generator.GetTimeSlotSpectrumValue(i);

        auto bandsIter = value->ConstBandsBegin();
        auto bandsEnd = value->ConstBandsEnd();

        auto valueIter = value->ConstValuesBegin();
        auto valueEnd = value->ConstValuesEnd();

        for (; bandsIter != bandsEnd; ++bandsIter, ++valueIter)
        {
            if (valueIter == valueEnd)
            {
                break;
            }

            stream << bandsIter->fc << ", " << *valueIter << '\n';
        }

        stream << '\n';
    }

    return stream;
}

int
main(int argc, char** argv)
{
    bool useGnuplot = false;
    bool useMicrowave2 = false;
    bool generateSweep = false;
    std::string confFile;

    const std::string sweepFile = "src/spectrum/examples/5MHz_microwave_spectrum.conf";

    CommandLine cmd;

    cmd.AddValue("waveform",
                 "Input file describing a waveform.  If not specified "
                 "then the builtin Microwave Oven #1 from the paper is used. "
                 "An example input file is located at " +
                     sweepFile,
                 confFile);
    cmd.AddValue("mwo2",
                 "Use the builtin Microwave Oven #2 from the paper "
                 "instead of Microwave Oven #1",
                 useMicrowave2);
    cmd.AddValue("generate-sweep",
                 "When using the builtin microwave waveforms, "
                 "also generate the sweep between the two transient areas. "
                 "The plot from --Mwo2=true --GenerateSweep=true will match "
                 "the plot from " +
                     sweepFile,
                 generateSweep);
    cmd.AddValue("plot",
                 "Generate a gnuplot file containing the output from the "
                 "spectrum analyzer",
                 useGnuplot);

    cmd.Parse(argc, argv);

    std::string plotFile;

    if (!confFile.empty())
    {
        plotFile = ExtractFileName(confFile);
    }
    else
    {
        if (useMicrowave2)
        {
            plotFile = "microwave-oven-2";
        }
        else
        {
            plotFile = "microwave-oven-1";
        }

        if (generateSweep)
        {
            plotFile += "-with-sweep";
        }
    } // ! confFile
    const std::string traceFile = plotFile;

    // Report what we're doing
    std::cout << "\n" << cmd.GetName() << ":\n";
    if (!confFile.empty())
    {
        std::cout << "Reading waveform from " << confFile << "\n";
    }
    else
    {
        std::cout << "Using builtin microwave oven #" << (useMicrowave2 ? 2 : 1);

        if (generateSweep)
        {
            std::cout << "and generating sweep between transients";
        }

        std::cout << '\n';
    }
    std::cout << std::endl;

    // nodes and positions
    std::cout << "Creating radiating node\n";
    NodeContainer microwaveNodes;
    microwaveNodes.Create(1);

    std::cout << "Creating spectrum analyzer node\n";
    NodeContainer spectrumAnalyzerNodes;
    spectrumAnalyzerNodes.Create(1);

    NodeContainer allNodes(microwaveNodes, spectrumAnalyzerNodes);

    std::cout << "Positioning analyzer 30 m from radiator.\n";
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> nodePositionList = CreateObject<ListPositionAllocator>();
    nodePositionList->Add(
        Vector(30.0, 0.0, 0.0));                  // Microwave Oven; 30m away from spectrum analyzer
    nodePositionList->Add(Vector(0.0, 0.0, 0.0)); // Spectrum Analyzer
    mobility.SetPositionAllocator(nodePositionList);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(allNodes);

    // channel
    std::cout << "Creating spectrum channel.\n";
    SpectrumChannelHelper channelHelper = SpectrumChannelHelper::Default();
    channelHelper.SetChannel("ns3::MultiModelSpectrumChannel");
    Ptr<SpectrumChannel> channel = channelHelper.Create();

    /********************************/
    /* Configure waveform generator */
    /********************************/
    std::cout << "Creating waveform generator.\n";
    NetDeviceContainer devices;

    if (confFile.empty())
    {
        devices = CreateFromHelper(microwaveNodes, channel, useMicrowave2, generateSweep);
    }
    else
    {
        devices = CreateFromLoader(confFile, microwaveNodes, channel);
    }

    NS_ASSERT_MSG(devices.GetN() > 0, "no complex waveform generators were created");

    Ptr<NetDevice> microwaveDevice = devices.Get(0);

    Ptr<WaveformGenerator> generator = microwaveDevice->GetObject<NonCommunicatingNetDevice>()
                                           ->GetPhy()
                                           ->GetObject<WaveformGenerator>();

    NS_LOG_DEBUG("Generator Configuration:\n" << *generator);

    Simulator::Schedule(MilliSeconds(10), &WaveformGenerator::Start, generator);

    /********************************/
    /* Configure spectrum analyzer  */
    /********************************/
    std::cout << "Configuring analyzer.\n";

    SpectrumAnalyzerHelper spectrumAnalyzerHelper;
    spectrumAnalyzerHelper.SetChannel(channel);
    spectrumAnalyzerHelper.SetRxSpectrumModel(SpectrumModelIsm2400MhzRes1Mhz);
    spectrumAnalyzerHelper.SetPhyAttribute("Resolution", TimeValue(MilliSeconds(1)));
    spectrumAnalyzerHelper.SetPhyAttribute("NoisePowerSpectralDensity",
                                           DoubleValue(1e-15)); // -120 dBm/Hz
    std::cout << "Configuring ascii trace file, basename: " << traceFile << "\n";
    spectrumAnalyzerHelper.EnableAsciiAll(traceFile);
    NetDeviceContainer spectrumAnalyzerDevices =
        spectrumAnalyzerHelper.Install(spectrumAnalyzerNodes);

    SpectrumDataCollector dataCollector;

    if (useGnuplot)
    {
        std::cout << "Configuring plot data collector.\n";
        Ptr<Node> analyzerNode = spectrumAnalyzerNodes.Get(0);
        Ptr<NetDevice> analyzerDevice = spectrumAnalyzerDevices.Get(0);

        std::ostringstream stream;
        stream << "/NodeList/" << analyzerNode->GetId() << "/DeviceList/"
               << analyzerDevice->GetIfIndex()
               << "/$ns3::NonCommunicatingNetDevice/Phy/AveragePowerSpectralDensityReport";

        Config::ConnectWithoutContext(
            stream.str(),
            MakeCallback(&SpectrumDataCollector::HandleCallback, &dataCollector));
    } // if useGnuplot

    Simulator::Stop(MilliSeconds(30));

    std::cout << "Running simulation";
    Simulator::Run();
    std::cout << "...done.\n";

    Simulator::Destroy();

    if (useGnuplot)
    {
        GeneratePlotFile(dataCollector, plotFile);
    }

    std::cout << "Simulation done!" << std::endl;

    return 0;
}
