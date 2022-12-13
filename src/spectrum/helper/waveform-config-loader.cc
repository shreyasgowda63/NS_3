/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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

#include "waveform-config-loader.h"

#include "waveform-config-lexer.h"

#include "ns3/advanced-waveform-generator-helper.h"
#include "ns3/attribute.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/spectrum-channel.h"
#include "ns3/spectrum-model.h"
#include "ns3/string.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <limits>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

/**
 * \file
 * \ingroup spectrum
 * ns3::WaveformConfigLoader implementation.
 */

NS_LOG_COMPONENT_DEFINE("WaveformConfigLoader");

namespace
{
/** Shorthand for ns3::WaveformConfigLexer::Token */
using LexerToken = ns3::WaveformConfigLexer::Token;
/** Shorthand for ns3::WaveformConfigLexer::TokenType */
using LexerTokenType = ns3::WaveformConfigLexer::TokenType;

/**
 * List of LexerTokenType that is used as a key in a lookup table.
 */
typedef std::vector<LexerTokenType> TokenKey;

/**
 * Helper function to convert a set of LexerTokenType arguments
 * into a TokenKey.
 *
 * \param types A parameter pack containing 0 or more LexerTokenTypes.
 * \return a TokenKey containing the list of supplied
 * LexerTokenTypes.
 */
template <class... Type>
TokenKey
MakeTokenKey(Type... types)
{
    return TokenKey{types...};
}

/**
 * Helper function to convert a list of LexerToken objects to a TokenKey.
 *
 * \param tokens a list of LexerToken objects.
 * \return a TokenKey containing the LexerTokenType of the supplied
 * LexerToken objects.
 */
TokenKey
MakeTokenKey(const std::vector<LexerToken>& tokens)
{
    TokenKey key;
    key.reserve(tokens.size());

    for (const LexerToken& token : tokens)
    {
        key.push_back(token.type);
    }

    return key;
}

/**
 * Enumeration of states in a state machine responsible for
 * validating the configuration file syntax and extracting
 * waveform parameters.
 */
enum class State
{
    UNKNOWN,         //!< State is unknown
    BEGIN,           //!< Start of a new waveform definition
    NODE,            //!< Parsed the node index
    CONST_INTERVAL,  //!< Parsed a constant interval
    CUSTOM_INTERVAL, //!< Parsed a custom interval
    RAND_INTERVAL,   //!< Parsed a random interval
    BAND,            //!< Parsed a band entry
    TXSLOT,          //!< Parsed a time slot entry
    DBM,             //!< Parsed a time slot value entry
    END,             //!< End of a waveform definition
    ERROR            //!< Error state
};                   // enum class State

/**
 * Stores the transmission power level for a specific frequency
 */
struct DbmValue
{
  public:
    /**
     * Default constructor
     */
    DbmValue()
        : centerFrequency(0),
          value(0)
    {
    }

    /**
     * Constructor
     *
     * \param frequency Transmit frequency
     * \param val Transmit power in dBm
     */
    DbmValue(double frequency, double val)
        : centerFrequency(frequency),
          value(val)
    {
    }

    double centerFrequency; //!< Center frequency of the target band.
    double value;           //!< Transmission power level in dBm.
};                          // struct DbmValue

/**
 * Stores the duration and the list of DbmValues
 * for a specific time slot.
 */
struct TxSlot
{
  public:
    /**
     * Default constructor
     */
    TxSlot()
        : duration(0),
          defaultDbm(0)
    {
    }

    double duration;              //!< Duration of the time slot in milliseconds.
    double defaultDbm;            //!< Default transmission power level in dBm.
    std::vector<DbmValue> values; //!< List of DbmValue entries.
};                                // struct TxSlot

/**
 * Stores all of the parameters defined by a configuration entry.
 */
struct WaveformParameters
{
  public:
    /**
     * Default constructor
     */
    WaveformParameters()
        : nodeIndex(0),
          intervalObject(),
          bands(),
          slots()
    {
    }

    /** Index of the node that the generator will be attached to. */
    std::uint32_t nodeIndex;

    /**
     * A RandomVariableStream instance which controls the number of
     * milliseconds between the end of one transmission and the start
     * of the next transmission.
     */
    ns3::Ptr<ns3::RandomVariableStream> intervalObject;
    ns3::Bands bands;          //!< List of transmit frequency bands
    std::vector<TxSlot> slots; //!< List of time slots
};                             // struct WaveformParameters

/**
 *  Attempts to convert a std::string to a double.
 *  If the conversion fails the boolean is false
 *  and the contents of the double are undefined.
 *
 *  \param value string containing a possible double value.
 *  \return a tuple containing a boolean and the converted
 *  value.
 */
std::tuple<bool, double>
parseDouble(const std::string& value)
{
    bool valid = true;
    double result = 0;

    try
    {
        std::size_t pos = 0;
        result = std::stod(value, &pos);

        if (pos < value.size())
        {
            valid = false;
        }
    }
    catch (const std::exception&)
    {
        valid = false;
    }

    return std::make_tuple(valid, result);
}

/**
 *
 * Removes the leading and trailing double quotes from
 * a string token.
 *
 * \param value a string from a String LexerToken
 * \return the input string with the leading and
 * trailing double quotes removed.
 */
std::string
stripQuotes(const std::string& value)
{
    // remove the leading and trailing double quotes
    if (value.empty())
    {
        return value;
    }

    std::size_t front = 0;
    std::size_t back = value.size() - 1;

    if (value.front() == '"')
    {
        ++front;
    }

    if (value.back() == '"')
    {
        --back;
    }

    if (front > back)
    {
        // only one character and it is a double quote.
        // return an empty string
        return "";
    }

    return value.substr(front, back - front + 1);
}

/**
 * Stream operator for converting a State value to a string
 * for logging/debugging.
 *
 * \param stream the output stream.
 * \param state the value to serialize.
 * \return reference to \p stream
 */
std::ostream&
operator<<(std::ostream& stream, State state)
{
    switch (state)
    {
    case State::BEGIN: {
        stream << "BeginWaveform";
    }
    break;
    case State::NODE: {
        stream << "Node";
    }
    break;
    case State::CONST_INTERVAL: {
        stream << "Interval";
    }
    break;
    case State::CUSTOM_INTERVAL: {
        stream << "Interval";
    }
    break;
    case State::RAND_INTERVAL: {
        stream << "Interval";
    }
    break;
    case State::BAND: {
        stream << "Band";
    }
    break;
    case State::TXSLOT: {
        stream << "Txslot";
    }
    break;
    case State::DBM: {
        stream << "Dbm";
    }
    break;
    case State::END: {
        stream << "EndWaveform";
    }
    break;
    case State::ERROR: {
        stream << "Error";
    }
    break;
    case State::UNKNOWN: {
        stream << "Unknown";
    }
    break;
    default: {
        stream << "State:" << static_cast<int>(state);
    }
    break;
    }

    return stream;
}

/**
 * Struct to encapsulate a TokenKey and used for outputting
 * TokenKey to a stream using operator<<.
 *
 * This is necessary to work around some wonky c++ type
 * dependent lookup rules.
 */
struct TokenKeyWrapper
{
    /**
     * Constructs a TokenKeyWrapper from the supplied TokenKey.
     *
     * \param key The TokenKey object to wrap. The TokenKey object
     * must remain valid for the lifetime of the TokenKeyWrapper object.
     */
    TokenKeyWrapper(const TokenKey& key)
        : key(key)
    {
    }

    /**
     * Reference to a valid TokenKey object.
     */
    const TokenKey& key;
};

/**
 * Stream operator for serializing a TokenKey to a string
 * for logging/debugging.
 *
 * \param stream Output stream for writing
 * \param wrapper The TokenKey to serialize
 * \return reference to \p stream
 */
std::ostream&
operator<<(std::ostream& stream, const TokenKeyWrapper& wrapper)
{
    std::string separator = "";

    for (auto elem : wrapper.key)
    {
        stream << separator << elem;

        if (separator.empty())
        {
            separator = ", ";
        }
    }

    return stream;
}

} // unnamed namespace

namespace ns3
{

/**
 *  Encapuslates all of the logic for
 *  parsing a complex waveform configuration file
 *  and creating complex waveform objects.
 */
class Parser
{
  public:
    /**
     * Default constructor
     */
    Parser()
        : m_stateLookup(),
          m_transitionTable(),
          m_currentState(State::UNKNOWN),
          m_context()
    {
        m_stateLookup = CreateLookupTable();
        m_transitionTable = CreateTransitionTable();
    }

    /**
     * Destructor
     */
    ~Parser()
    {
    }

    /**
     * Parses the waveform configuration data from the input stream,
     * creates WaveformGenerator objects and installs them
     * on the nodes located in the nodeStore.
     *
     * Returns a container of NetDevice objects where each NetDevice object
     * has a reference to one of the WaveformGenerator objects.
     *
     * \param stream input stream containing waveform configuration data.
     * \param channel the channel the waveform generators will use
     * for transmission.
     * \param nodeStore object storing the list of nodes.
     * \return container of newly created NetDevice objects.
     */
    NetDeviceContainer Load(std::istream& stream,
                            Ptr<SpectrumChannel> channel,
                            NodeContainer& nodeStore)
    {
        WaveformConfigLexer lexer(stream);

        m_context.reset(new Context());

        m_context->channel = channel;
        m_context->nodeStore = &nodeStore;

        while (!lexer.Eof())
        {
            std::vector<LexerToken> tokens = TokenizeLine(lexer);

            if (tokens.empty())
            {
                // ignore empty lines
                continue;
            }

            ParseLine(tokens);
        }

        return m_context->devices;
    }

  private:
    /// Lookup table for mapping TokenKey to State values.
    typedef std::map<TokenKey, State> KeyStateTable;

    /// Type defining a link between two states.
    typedef std::pair<State, State> Edge;

    /// Function signature for a function that parses a list of LexerTokens.
    typedef std::function<bool(const std::vector<LexerToken>&)> TokenParser;

    /// Lookup table for mapping State transitions to parser functions.
    typedef std::multimap<Edge, TokenParser> TransitionTable;

    /**
     *  Caches all of the input and output data needed when
     *  parsing a configuration file.
     */
    struct Context
    {
        /// Channel used by the created generators
        Ptr<SpectrumChannel> channel;

        /// List of created NetDevice objects
        NetDeviceContainer devices;

        /// Stores parameters of the current waveform.
        WaveformParameters waveformParams;

        /// Pointer to node store
        NodeContainer* nodeStore;
    };

    /**
     * Generates a lookup table which maps TokenKey values
     * to State values.
     *
     * \return table with one to one mapping of TokenKey to State
     */
    KeyStateTable CreateLookupTable() const
    {
        return KeyStateTable{
            {MakeTokenKey(LexerTokenType::Begin, LexerTokenType::Waveform), State::BEGIN},
            {MakeTokenKey(LexerTokenType::Node, LexerTokenType::Number), State::NODE},
            {MakeTokenKey(LexerTokenType::Interval,
                          LexerTokenType::Constant,
                          LexerTokenType::Number),
             State::CONST_INTERVAL},
            {MakeTokenKey(LexerTokenType::Interval, LexerTokenType::Custom, LexerTokenType::String),
             State::CUSTOM_INTERVAL},
            {MakeTokenKey(LexerTokenType::Interval,
                          LexerTokenType::Random,
                          LexerTokenType::Number,
                          LexerTokenType::Number),
             State::RAND_INTERVAL},
            {MakeTokenKey(LexerTokenType::Band, LexerTokenType::Number, LexerTokenType::Number),
             State::BAND},
            {MakeTokenKey(LexerTokenType::Txslot, LexerTokenType::Number, LexerTokenType::Number),
             State::TXSLOT},
            {MakeTokenKey(LexerTokenType::Dbm, LexerTokenType::Number, LexerTokenType::Number),
             State::DBM},
            {MakeTokenKey(LexerTokenType::End, LexerTokenType::Waveform), State::END}};
    }

    /**
     * Generates the lookup table which maps a pair of State values
     * to a parser function.
     *
     * This table is used to determine whether a transition between two
     * State values is possible; and if so, the parser function that should
     * be used to parse the LexerTokens in the new State.
     *
     * \return table with one to one mapping of Edge to TokenParser
     */
    TransitionTable CreateTransitionTable()
    {
        return TransitionTable{
            {std::make_pair(State::END, State::BEGIN),
             [this](const std::vector<LexerToken>& tokens) { return this->StartWaveform(); }},
            {std::make_pair(State::ERROR, State::BEGIN),
             [this](const std::vector<LexerToken>& tokens) { return this->StartWaveform(); }},
            {std::make_pair(State::UNKNOWN, State::BEGIN),
             [this](const std::vector<LexerToken>& tokens) { return this->StartWaveform(); }},
            {std::make_pair(State::BEGIN, State::NODE),
             [this](const std::vector<LexerToken>& tokens) { return this->SetNodeId(tokens[1]); }},
            {std::make_pair(State::NODE, State::CONST_INTERVAL),
             [this](const std::vector<LexerToken>& tokens) {
                 return this->CreateConstantInterval(tokens[2]);
             }},
            {std::make_pair(State::NODE, State::CUSTOM_INTERVAL),
             [this](const std::vector<LexerToken>& tokens) {
                 return this->CreateCustomInterval(tokens[2]);
             }},
            {std::make_pair(State::NODE, State::RAND_INTERVAL),
             [this](const std::vector<LexerToken>& tokens) {
                 return this->CreateRandomInterval(tokens[2], tokens[3]);
             }},
            {std::make_pair(State::CONST_INTERVAL, State::BAND),
             [this](const std::vector<LexerToken>& tokens) {
                 return this->CreateBand(tokens[1], tokens[2]);
             }},
            {std::make_pair(State::CUSTOM_INTERVAL, State::BAND),
             [this](const std::vector<LexerToken>& tokens) {
                 return this->CreateBand(tokens[1], tokens[2]);
             }},
            {std::make_pair(State::RAND_INTERVAL, State::BAND),
             [this](const std::vector<LexerToken>& tokens) {
                 return this->CreateBand(tokens[1], tokens[2]);
             }},
            {std::make_pair(State::BAND, State::BAND),
             [this](const std::vector<LexerToken>& tokens) {
                 return this->CreateBand(tokens[1], tokens[2]);
             }},
            {std::make_pair(State::BAND, State::TXSLOT),
             [this](const std::vector<LexerToken>& tokens) {
                 return this->CreateTransmitSlot(tokens[1], tokens[2]);
             }},
            {std::make_pair(State::TXSLOT, State::DBM),
             [this](const std::vector<LexerToken>& tokens) {
                 return this->SetDbmValue(tokens[1], tokens[2]);
             }},
            {std::make_pair(State::TXSLOT, State::TXSLOT),
             [this](const std::vector<LexerToken>& tokens) {
                 return this->CreateTransmitSlot(tokens[1], tokens[2]);
             }},
            {std::make_pair(State::TXSLOT, State::END),
             [this](const std::vector<LexerToken>& tokens) { return this->EndWaveform(); }},
            {std::make_pair(State::DBM, State::DBM),
             [this](const std::vector<LexerToken>& tokens) {
                 return this->SetDbmValue(tokens[1], tokens[2]);
             }},
            {std::make_pair(State::DBM, State::END),
             [this](const std::vector<LexerToken>& tokens) { return this->EndWaveform(); }},
            {std::make_pair(State::DBM, State::TXSLOT),
             [this](const std::vector<LexerToken>& tokens) {
                 return this->CreateTransmitSlot(tokens[1], tokens[2]);
             }}};
    }

    /**
     * Repeatedly extracts LexerToken objects from the input stream until a
     * newline or end of file token is encountered.
     *
     *  \param lexer object responsible for extracting LexerTokens from the
     *  input stream.
     *  \return list of LexerToken objects extracted from the input stream.
     */
    std::vector<LexerToken> TokenizeLine(WaveformConfigLexer& lexer) const
    {
        bool done = false;
        std::vector<LexerToken> tokens;

        while (!done)
        {
            LexerToken token = lexer.GetNextToken();

            if (token.type == LexerTokenType::Comment || token.type == LexerTokenType::Whitespace)
            {
                // these tokens are discarded
                continue;
            }

            if (token.type == LexerTokenType::Newline || token.type == LexerTokenType::EndofFile)
            {
                // these tokens mark the end of a line
                done = true;
                continue;
            }

            NS_LOG_DEBUG("Token: value='" << token.value << "', type=" << token.type);

            tokens.emplace_back(std::move(token));
        }

        return tokens;
    }

    /**
     * Checks that the supplied list of LexerToken objects represents a
     * valid combination.
     * If the combination represents a valid state,
     * looks in the TransitionTable to see if there is a parser
     * associated with the transition from the current state to the new
     * state.
     * If a parser exists, it is used to extract waveform parameters from
     * the LexerTokens.
     * If there is no transition from the current state to the new state,
     * an error is logged and the waveform is marked as invalid.
     *
     * \param tokens list of tokens extracted from the input stream.
     */
    void ParseLine(const std::vector<LexerToken>& tokens)
    {
        TokenKey key = MakeTokenKey(tokens);

        auto stateIter = m_stateLookup.find(key);

        if (stateIter == m_stateLookup.end())
        {
            // not a valid combination of tokens
            NS_LOG_ERROR("Syntax error on line " << tokens[0].lineNumber << ": Invalid token key ("
                                                 << TokenKeyWrapper(key) << ')');

            m_currentState = State::ERROR;

            return;
        }

        State nextState = stateIter->second;

        auto transitionIter = m_transitionTable.find(std::make_pair(m_currentState, nextState));

        if (transitionIter == m_transitionTable.end())
        {
            // not a valid transition
            if (m_currentState != State::ERROR)
            {
                NS_LOG_ERROR("Syntax error on line "
                             << tokens[0].lineNumber << ": cannot transition from state "
                             << m_currentState << " to state " << nextState);
            }

            m_currentState = State::ERROR;
            return;
        }

        bool success = transitionIter->second(tokens);

        if (!success)
        {
            nextState = State::ERROR;
        }

        NS_LOG_DEBUG("Transitioning from state " << m_currentState << " to " << nextState);

        m_currentState = nextState;
    }

    /**
     * Parser function associated with the BEGIN state.
     *
     * \return always returns true.
     */
    bool StartWaveform()
    {
        // clear the waveform parameters
        m_context->waveformParams = WaveformParameters();

        return true;
    }

    /**
     * Parser function associated with the END state.
     *
     * \return true if a new WaveformGenerator object
     * was created from the waveform parameters.
     */
    bool EndWaveform()
    {
        // maps the center frequency for a band to the index
        // of the band in the bands vector.
        std::map<double, std::size_t> centerFrequencies;
        WaveformParameters& params = m_context->waveformParams;
        AdvancedWaveformGeneratorHelper generatorHelper;

        // function that converts dBm to Watts
        auto decibalToWatt = [](double db) {
            // convert dBm to dBW
            double temp = (db - 30.0) / 10.0;

            // convert to Watt
            return std::pow(10, temp);
        };

        bool valid = ValidateParameters(params);

        if (!valid)
        {
            return false;
        }

        NS_LOG_DEBUG("Waveform: "
                     << " nodeIndex=" << params.nodeIndex << ", bands=" << params.bands.size()
                     << ", slots=" << params.slots.size());

        generatorHelper.SetChannel(m_context->channel);

        generatorHelper.SetPhyAttribute("Interval", PointerValue(params.intervalObject));

        for (std::size_t i = 0; i < params.bands.size(); ++i)
        {
            BandInfo& band = params.bands[i];

            centerFrequencies[band.fc] = i;
        }

        generatorHelper.SetBands(params.bands);

        for (const TxSlot& slot : params.slots)
        {
            double defaultPowerDensity = decibalToWatt(slot.defaultDbm);

            // create a vector equal in size to the number of
            // bands and fill it with the default value for this
            // time slot.
            std::vector<double> powerDensities(params.bands.size(), defaultPowerDensity);

            // Iterate through the custom values for this time slot
            // and update the corresponding entries in the power density
            // vector.
            NS_LOG_DEBUG("slot has " << slot.values.size() << " custom values");

            for (const DbmValue& dbm : slot.values)
            {
                auto iter = centerFrequencies.find(dbm.centerFrequency);

                if (iter == centerFrequencies.end())
                {
                    // invalid frequency, this should have been caught by
                    // the validation code
                    NS_LOG_ERROR("processed a dBm entry with an invalid "
                                 << "center frequency (" << dbm.centerFrequency
                                 << "), this should have been caught during validation");

                    continue;
                }

                powerDensities[iter->second] = decibalToWatt(dbm.value);

                NS_LOG_DEBUG("setting power density for frequency "
                             << iter->first << " ( index=" << iter->second << ")"
                             << " to value " << powerDensities[iter->second] << " (" << dbm.value
                             << " dBm)");
            }

            generatorHelper.AddTxPowerSpectralDensity(MilliSeconds(slot.duration), powerDensities);
        }

        Ptr<Node> node = m_context->nodeStore->Get(params.nodeIndex);
        Ptr<NetDevice> device = generatorHelper.Install(node);

        m_context->devices.Add(device);

        return true;
    }

    /**
     * Parser function associated with the BAND state.
     *
     * \param frequencyToken LexerToken containing the center frequency
     * argument.
     * \param widthToken LexerToken containing the width of the band.
     * \return true if the LexerToken objects contains valid data.
     */
    bool CreateBand(const LexerToken& frequencyToken, const LexerToken& widthToken)
    {
        bool valid = false;
        double frequency = 0;
        double width = 0;

        std::tie(valid, frequency) = parseDouble(frequencyToken.value);

        if (!valid || frequency <= 0)
        {
            NS_LOG_ERROR("Line " << frequencyToken.lineNumber << ", Col " << frequencyToken.column
                                 << ": Invalid band frequency");

            return false;
        }

        std::tie(valid, width) = parseDouble(widthToken.value);

        if (!valid || width < 0 || width > frequency)
        {
            NS_LOG_ERROR("Line " << widthToken.lineNumber << "," << widthToken.column
                                 << ": Invalid band width");

            return false;
        }

        double halfWidth = width / 2.0;
        BandInfo band;
        band.fc = frequency;
        band.fl = frequency - halfWidth;
        band.fh = frequency + halfWidth;

        m_context->waveformParams.bands.push_back(band);

        return true;
    }

    /**
     * Parser function associated with the CONST_INTERVAL state.
     *
     * \param intervalToken LexerToken containing the number of milliseconds
     * between the end of one transmission and the start of the next
     * transmission.
     *
     * \return true if \p intervalToken contains valid data.
     */
    bool CreateConstantInterval(const LexerToken& intervalToken)
    {
        bool valid = false;
        double interval = 0;

        std::tie(valid, interval) = parseDouble(intervalToken.value);

        if (!valid || interval <= 0)
        {
            NS_LOG_ERROR("Line " << intervalToken.lineNumber << "," << intervalToken.column
                                 << ": Invalid waveform interval");

            return false;
        }

        Ptr<ConstantRandomVariable> intervalVariable =
            CreateObjectWithAttributes<ConstantRandomVariable>("Constant", DoubleValue(interval));

        m_context->waveformParams.intervalObject = intervalVariable;

        return true;
    }

    /**
     * Parser function associated with the CUSTOM_INTERVAL state.
     *
     * \param customToken LexerToken containing a serialized ns3 object
     *
     * \return true if \p customToken contains valid data.
     */
    bool CreateCustomInterval(const LexerToken& customToken)
    {
        std::string serializedObject = stripQuotes(customToken.value);

        PointerValue ptrValue;

        bool valid = ptrValue.DeserializeFromString(serializedObject, 0);

        if (!valid)
        {
            NS_LOG_ERROR("Line " << customToken.lineNumber << "," << customToken.column
                                 << ": Invalid custom interval '" << customToken.value
                                 << "', could not create object from parameters");

            return false;
        }

        Ptr<RandomVariableStream> rngObject = ptrValue.Get<RandomVariableStream>();

        if (!rngObject)
        {
            NS_LOG_ERROR("Line " << customToken.lineNumber << "," << customToken.column
                                 << ": Invalid custom interval, object does not "
                                 << "implement the RandomVariableStream interface");

            return false;
        }

        m_context->waveformParams.intervalObject = rngObject;

        return true;
    }

    /**
     * Parser function associated with the RAND_INTERVAL state.
     *
     * \param minToken LexerToken containing the lower bound of the range.
     * \param maxToken LexerToken containing the upper bound of the range.
     *
     * \return true if \p minToken and \p maxToken contains valid data.
     */
    bool CreateRandomInterval(const LexerToken& minToken, const LexerToken& maxToken)
    {
        bool valid = false;
        double minValue = 0;
        double maxValue = 0;

        std::tie(valid, minValue) = parseDouble(minToken.value);

        if (!valid || minValue <= 0)
        {
            NS_LOG_ERROR("Line " << minToken.lineNumber << "," << minToken.column
                                 << ": Invalid minimum value for waveform interval");

            return false;
        }

        std::tie(valid, maxValue) = parseDouble(maxToken.value);

        if (!valid || maxValue <= 0 || maxValue <= minValue)
        {
            NS_LOG_ERROR("Line " << maxToken.lineNumber << "," << maxToken.column
                                 << ": Invalid maximum value for waveform interval");

            return false;
        }

        Ptr<UniformRandomVariable> intervalVariable =
            CreateObjectWithAttributes<UniformRandomVariable>("Min",
                                                              DoubleValue(minValue),
                                                              "Max",
                                                              DoubleValue(maxValue));

        m_context->waveformParams.intervalObject = intervalVariable;

        return true;
    }

    /**
     * Parser function associated with the TXSLOT state.
     *
     * \param durationToken LexerToken containing the duration of the
     * transmit slot.
     * \param valueToken LexerToken containing the default value of the
     * transmit slot.
     *
     * \return true if \p durationToken and \p valueToken contain
     * valid data.
     */
    bool CreateTransmitSlot(const LexerToken& durationToken, const LexerToken& valueToken)
    {
        bool valid = false;
        TxSlot slot;

        std::tie(valid, slot.duration) = parseDouble(durationToken.value);

        if (!valid || slot.duration <= 0)
        {
            NS_LOG_ERROR("Line " << durationToken.lineNumber << "," << durationToken.column
                                 << ": Invalid value for transmit slot duration");

            return false;
        }

        std::tie(valid, slot.defaultDbm) = parseDouble(valueToken.value);

        if (!valid || slot.defaultDbm > 0)
        {
            NS_LOG_ERROR("Line " << valueToken.lineNumber << "," << valueToken.column
                                 << ": Invalid value for default dBm");

            return false;
        }

        m_context->waveformParams.slots.push_back(slot);

        return true;
    }

    /**
     * Parser function associated with the DBM state.
     *
     * \param frequencyToken LexerToken containing the center frequency of
     * the target band.
     * \param valueToken LexerToken containing the new power value for the
     * band.
     *
     * \return true if \p frequencyToken and \p valueToken contain
     * valid data.
     */
    bool SetDbmValue(const LexerToken& frequencyToken, const LexerToken& valueToken)
    {
        bool valid = false;
        DbmValue value;

        std::tie(valid, value.centerFrequency) = parseDouble(frequencyToken.value);

        if (!valid || value.centerFrequency <= 0)
        {
            NS_LOG_ERROR("Line " << frequencyToken.lineNumber << "," << frequencyToken.column
                                 << ": Invalid value for center frequency");

            return false;
        }

        std::tie(valid, value.value) = parseDouble(valueToken.value);

        if (!valid || value.value > 0)
        {
            NS_LOG_ERROR("Line " << valueToken.lineNumber << "," << valueToken.column
                                 << ": Invalid value for dBm");

            return false;
        }

        if (m_context->waveformParams.slots.empty())
        {
            NS_LOG_ERROR("Line " << valueToken.lineNumber
                                 << ",0: dBm value specified before txslot");

            return false;
        }

        m_context->waveformParams.slots.rbegin()->values.push_back(value);

        return true;
    }

    /**
     * Parser function associated with the NODE state.
     *
     * \param idToken LexerToken containing the index of a node in the
     * node store.
     *
     * \return true if \p idToken contains valid data.
     */
    bool SetNodeId(const LexerToken& idToken)
    {
        unsigned long id = 0;
        std::size_t pos = 0;

        try
        {
            id = std::stoul(idToken.value, &pos);

            if (pos != idToken.value.size())
            {
                NS_LOG_ERROR("Line " << idToken.lineNumber << "," << idToken.column
                                     << ": Invalid value for node id ( value=" << idToken.value
                                     << ")");

                return false;
            }
        }
        catch (const std::out_of_range&)
        {
            NS_LOG_ERROR("Line " << idToken.lineNumber << "," << idToken.column
                                 << ": Value too large for node id");

            return false;
        }
        catch (const std::exception&)
        {
            NS_LOG_ERROR("Line " << idToken.lineNumber << "," << idToken.column
                                 << ": Invalid value for node id ( value=" << idToken.value << ")");

            return false;
        }

        if (id > std::numeric_limits<std::uint32_t>::max())
        {
            NS_LOG_ERROR("Line " << idToken.lineNumber << "," << idToken.column
                                 << ": Value too large for node id ( max="
                                 << std::numeric_limits<std::uint32_t>::max() << ")");

            return false;
        }

        m_context->waveformParams.nodeIndex = static_cast<std::uint32_t>(id);

        return true;
    }

    /**
     * Checks that all of the parameters stored in the \p params object
     * are valid.
     *
     * \param params holds all of the parameters needed
     * to create a WaveformGenerator object.
     *
     * \return true if all of the values in \p params are valid.
     *
     */
    bool ValidateParameters(const WaveformParameters& params)
    {
        if (m_context->nodeStore->Get(params.nodeIndex) == 0)
        {
            NS_LOG_ERROR("Validation error: nodeIndex " << params.nodeIndex
                                                        << " does not reference a valid node");

            return false;
        }

        if (!params.intervalObject)
        {
            NS_LOG_ERROR("Validation error ( nodeIndex=" << params.nodeIndex
                                                         << "): no interval specified");

            return false;
        }

        if (params.bands.empty())
        {
            NS_LOG_ERROR("Validation error ( nodeIndex=" << params.nodeIndex
                                                         << "): no bands specified");

            return false;
        }

        if (params.slots.empty())
        {
            NS_LOG_ERROR("Validation error ( nodeIndex="
                         << params.nodeIndex << "): no transmit slots (txslot) specified");

            return false;
        }

        for (const TxSlot& slot : params.slots)
        {
            for (const DbmValue& dbm : slot.values)
            {
                auto comparator = [&dbm](const BandInfo& band) {
                    return band.fc == dbm.centerFrequency;
                };

                auto iter = std::find_if(params.bands.begin(), params.bands.end(), comparator);

                if (iter == params.bands.end())
                {
                    NS_LOG_ERROR("Validation error ( nodeIndex="
                                 << params.nodeIndex
                                 << "): dbm value does not map to any of the bands "
                                 << "in this waveform.");

                    return false;
                }
            }
        }

        return true;
    }

  private:
    KeyStateTable m_stateLookup; //!< Maps token keys to states.

    /** Maps state transitions to token parsers. */
    TransitionTable m_transitionTable;

    State m_currentState; //!< The current state of the state machine.

    /**
     * Pointer to a Context object.
     * A new context is created each time the Load function is called.
     */
    std::unique_ptr<Context> m_context;
}; // class Parser

////////////////////////////////////////////////////
//
//  WaveformConfigLoader
//
///////////////////////////////////////////////////
WaveformConfigLoader::WaveformConfigLoader()
{
}

WaveformConfigLoader::~WaveformConfigLoader()
{
}

NetDeviceContainer
WaveformConfigLoader::Load(const std::string& filepath,
                           Ptr<SpectrumChannel> channel,
                           NodeContainer& nodes)
{
    std::ifstream stream(filepath);

    if (!stream.is_open())
    {
        NS_FATAL_ERROR("Configuraton file '" << filepath
                                             << "' does not exist or is not readable, aborting\n");
    }

    return Load(stream, channel, nodes);
}

NetDeviceContainer
WaveformConfigLoader::Load(std::istream& stream, Ptr<SpectrumChannel> channel, NodeContainer& nodes)

{
    Parser parser;

    NS_ABORT_MSG_IF(channel == nullptr,
                    "Expected a valid SpectrumChannel object but "
                        << "received a null pointer");

    return parser.Load(stream, channel, nodes);
}

} //  namespace ns3
