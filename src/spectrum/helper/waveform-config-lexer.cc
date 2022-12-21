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
 *
 * Brief description: Implementation of a lexer for complex
 * waveform config files.
 *
 */

#include "waveform-config-lexer.h"

#include <bitset>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <regex>
#include <set>
#include <type_traits>

/**
 * \file
 * \ingroup spectrum
 * ns3::WaveformConfigLexer::StreamContainer class implementation.
 * ns3::AllocatedFileStream class implementation.
 * ns3::ExternalStream class implementation.
 * ns3::WaveformConfigLexer implementation.
 */

namespace
{
/**
 * Encapsulates the logic for determining if a character
 * is the end of a token.
 */
class TerminatorMatcher
{
  public:
    /**
     *  Generates the set of terminating characters
     *  from a pair of iterators.
     *
     *  \param begin Pointer to the start of the range
     *  \param end Pointer to one past the end of the range
     */
    template <class Iterator>
    TerminatorMatcher(Iterator begin, Iterator end)
        : m_terminators()
    {
        for (Iterator iter = begin; iter != end; ++iter)
        {
            std::size_t value = static_cast<std::size_t>(*iter);

            if (value <= m_terminators.size())
            {
                m_terminators.set(value);
            }
        }
    }

    /**
     *  Generates the set of terminating characters
     *  from a container object.
     *  The container object must implement begin()/end() functions
     *  which return iterators to the start and end of the set.
     *
     *  \param terminators A container object holding a set of characters.
     */
    template <class Container>
    TerminatorMatcher(const Container& terminators)
        : TerminatorMatcher(terminators.begin(), terminators.end())
    {
    }

    /**
     * Checks whether the supplied character is in the
     * set of terminating characters.
     *
     *  \param c A character in the range 0-255
     *
     *  \return true if c is in the set of terminating
     *  characters, false otherwise.
     */
    bool operator()(unsigned char c) const
    {
        return m_terminators.test(c);
    }

  private:
    /** Bitset container large enough to cover 256 characters. */
    typedef std::bitset<256> TerminatorSet;

    TerminatorSet m_terminators; //!< the set of terminating characters
};                               // class TerminatorMatcher

/**
 * Encapsulates the logic for determining if a string
 * matches a token pattern.
 */
class ValueMatcher
{
  public:
    /**
     * Struct used to identify a constructor parameter as a plain string
     */
    struct StringValue
    {
        std::string value; //!< String value used as target of comparisons
    };

    /**
     * Struct used to identify a constructor parameter as a regex pattern
     */
    struct RegexPattern
    {
        std::string pattern; //!< Regex pattern used for comparisons
    };

    /**
     *  Constructs an object which matches strings
     *  based on equality.
     *
     *  \param match_value String used as the basis of the comparison
     */
    ValueMatcher(const StringValue& match_value)
        : m_matcher()
    {
        m_matcher = [match_value](const std::string& input) { return match_value.value == input; };
    }

    /**
     *  Constructs an object which matches strings
     *  based on regular expression pattern.
     *
     *  Throws an exception if pattern does not contain
     *  a valid ECMAScript regular expression.
     *
     *  \param match_value Regular expression in ECMAScript format
     */
    ValueMatcher(const RegexPattern& match_value)
        : m_matcher()
    {
        std::regex expression(match_value.pattern);

        m_matcher = [expression](const std::string& input) {
            return std::regex_match(input, expression);
        };
    }

  public:
    /**
     * Checks whether the supplied string contains a token value.
     *
     *  \param input String containing a potential token value
     *
     *  \return true if input matches the stored value or pattern,
     *  false otherwise.
     */
    bool operator()(const std::string& input) const
    {
        return m_matcher(input);
    }

  private:
    /** Defines the function signature for the internal function object. */
    typedef std::function<bool(const std::string&)> MatcherFunction;

    /** Function object used to match input strings */
    MatcherFunction m_matcher;
}; // class ValueMatcher
} // unnamed namespace

namespace ns3
{

/**
 * Abstract class defining the interface for
 * derived stream classes.
 */
class WaveformConfigLexer::StreamContainer
{
  public:
    /**
     * Default constructor
     */
    StreamContainer()
    {
    }

    /**
     * Destructor
     */
    virtual ~StreamContainer()
    {
    }

    /**
     * Get a reference to the underlying input stream.
     *
     * \return reference to the underlying input stream
     */
    std::istream& GetStream()
    {
        return DoGetStream();
    }

  private:
    /**
     * Pure virtual function that must be implemented
     * by derived classes to return a reference to
     * the underlying input stream object.
     *
     * \return reference to the underlying input stream
     */
    virtual std::istream& DoGetStream() = 0;
}; // class StreamContainer

/**
 * Class derived from StreamContainer which represents
 * an input stream backed by a file on disk.
 */
class AllocatedFileStream : public WaveformConfigLexer::StreamContainer
{
  public:
    /**
     * Constructor which creates an input stream pointing to the file
     * located at \p filepath.
     *
     * \param filepath Location of the file to open.
     *
     * \throw std::ios_base::failure if the file could not be opened.
     */
    AllocatedFileStream(const std::string& filepath)
        : m_stream(new std::ifstream())
    {
        OpenFile(filepath);
    }

    /**
     * Destructor
     */
    virtual ~AllocatedFileStream()
    {
    }

  private:
    /**
     * Attempts to open the file located at \p filepath.
     *
     * \param filepath Location of the file to open.
     *
     * \throw std::ios_base::failure if the file could not be opened.
     */
    void OpenFile(const std::string& filepath)
    {
        // turn on exceptions so that stream->open will throw
        // an exception if it fails
        m_stream->exceptions(std::ios::badbit | std::ios::failbit);

        m_stream->open(filepath);

        // file is open, turn off exceptions
        m_stream->exceptions(std::ios::goodbit);
    }

    /**
     * Returns a reference to the input stream.
     *
     * \return reference to the underlying stream object.
     */
    virtual std::istream& DoGetStream()
    {
        return *m_stream;
    }

    /**
     * Managed pointer to an input file stream object.
     */
    std::unique_ptr<std::ifstream> m_stream;
}; // class AllocatedFileStream

/**
 * Class derived from StreamContainer which represents
 * an input stream created by some external method.
 *
 * This class does not provide any functionality,
 * instead it acts as a wrapper around the input
 * stream and provides a consistent interface
 * for the WaveformConfigLexer class.
 */
class ExternalStream : public WaveformConfigLexer::StreamContainer
{
  public:
    /**
     * Construct which Sets the input stream stored in this instance
     * to point to \p stream
     *
     * \param stream a valid input stream object
     */
    ExternalStream(std::istream& stream)
        : m_stream(&stream)
    {
    }

    /**
     * Destructor
     */
    virtual ~ExternalStream()
    {
        m_stream = nullptr;
    }

  private:
    /**
     * Returns a reference to the input stream.
     * \return reference to the underlying stream object
     */
    virtual std::istream& DoGetStream()
    {
        return *m_stream;
    }

    /**
     * Pointer to an input stream
     */
    std::istream* m_stream;
}; // class ExternalStream

/**
 * Encapsulates the traits for a specific token type.
 */
class WaveformConfigLexer::TokenTraits
{
  public:
    /**
     * Constructor
     *
     *  \param type Token type that the traits apply to
     *  \param matcher Object used to identify strings that
     *  the traits apply to
     *  \param checker Object used to identify the set of
     *  characters that represent the end of the token
     */
    TokenTraits(TokenType type, ValueMatcher matcher, TerminatorMatcher checker)
        : m_type(type),
          m_isMatch(matcher),
          m_isTerminator(checker)
    {
    }

    /**
     *  Destructor
     */
    virtual ~TokenTraits()
    {
    }

    /**
     * Returns the TokenType that the traits are associated with
     *  \return TokenType associated with this object
     */
    TokenType Type() const
    {
        return m_type;
    }

    /**
     * Function to check whether a supplied string represents
     * a token type.
     *
     *  \param value Input string
     *  \return true if the input string matches the
     *  stored token pattern, false otherwise
     */
    bool IsMatch(const std::string& value) const
    {
        return m_isMatch(value);
    }

    /**
     * Function to check whether the supplied character represents
     * the end of a token.
     *
     *  \param c Input character
     *  \return true if the input character is
     *  in the set of terminating characters for this token
     */
    bool IsTerminator(unsigned char c) const
    {
        return m_isTerminator(c);
    }

  private:
    TokenType m_type;                 //!< Type of token these traits apply to.
    ValueMatcher m_isMatch;           //!< Rule for matching this token type.
    TerminatorMatcher m_isTerminator; //!< Rule for ending this token type.
};                                    // class WaveformConfigLexer::TokenTraits

/**
 * Helper function to create a new TokenTraits object which is wrapped
 * in a unique_ptr.
 *
 * \param type TokenType to store in the traits object
 * \param matcher ValueMatcher for the TokenType
 * \param checker TerminatorMatcher for the TokenType
 *
 * \return a new TokenTraits object containing the supplied arguments.
 */
std::unique_ptr<WaveformConfigLexer::TokenTraits>
make_unique_traits(WaveformConfigLexer::TokenType type,
                   ValueMatcher matcher,
                   TerminatorMatcher checker)
{
    return std::unique_ptr<WaveformConfigLexer::TokenTraits>(
        new WaveformConfigLexer::TokenTraits(type, std::move(matcher), std::move(checker)));
}

/**
 * Helper function to create a new TokenTraits object which is wrapped
 * in a unique_ptr
 *
 * \param params A 3-tuple containing the arguments for the
 * TokenTraits constructor
 *
 * \return a new TokenTraits object containing the arguments stored
 * in the tuple
 */
std::unique_ptr<WaveformConfigLexer::TokenTraits>
make_unique_traits(
    std::tuple<WaveformConfigLexer::TokenType, ValueMatcher, TerminatorMatcher> params)
{
    return make_unique_traits(std::get<0>(params), std::get<1>(params), std::get<2>(params));
}

WaveformConfigLexer::WaveformConfigLexer(const std::string& filepath)
    : WaveformConfigLexer()
{
    m_stream.reset(new AllocatedFileStream(filepath));
    m_traitsTable = GenerateTraitsTable();

    ReadBlock();
}

WaveformConfigLexer::WaveformConfigLexer(std::istream& stream)
    : WaveformConfigLexer()

{
    m_stream.reset(new ExternalStream(stream));
    m_traitsTable = GenerateTraitsTable();

    ReadBlock();
}

WaveformConfigLexer::WaveformConfigLexer()
    : m_stream(),
      m_traitsTable(),
      m_buffer(BLOCK_SIZE, 0),
      m_position(m_buffer.end()),
      m_end(m_buffer.end()),
      m_line(1),
      m_column(0)
{
}

WaveformConfigLexer::~WaveformConfigLexer()
{
}

/**
 * The lexer is at the end of the file
 * if m_stream->eof() is true and
 * all of the data in the internal buffer
 * has been processed.
 */
bool
WaveformConfigLexer::Eof() const
{
    return m_stream->GetStream().eof() && (m_position == m_end);
}

WaveformConfigLexer::Token
WaveformConfigLexer::GetNextToken()
{
    unsigned char c = GetChar();
    Token token;

    token.lineNumber = m_line;
    token.column = m_column;

    if (c == 0)
    {
        // end of file
        token.type = TokenType::EndofFile;
        return token;
    }

    token.value.push_back(c);

    while (true)
    {
        TokenTraits* traits = FindTokenTraits(token.value);

        c = GetNextChar();

        if (traits->IsTerminator(c))
        {
            token.type = traits->Type();
            break;
        }

        token.value.push_back(c);
    }

    if (token.type == TokenType::Newline)
    {
        ++m_line;
        m_column = 0;
    }

    return token;
}

WaveformConfigLexer::TokenTraits*
WaveformConfigLexer::FindTokenTraits(const std::string& value) const
{
    TokenTraits* unknownTrait = 0;

    for (const UniqueTokenTraits& traits : m_traitsTable)
    {
        if (traits->IsMatch(value))
        {
            return traits.get();
        }

        if (traits->Type() == TokenType::Unknown)
        {
            unknownTrait = traits.get();
        }
    }

    // should never get here because the unknown trait
    // matches everything
    return unknownTrait;
}

WaveformConfigLexer::TokenTraitsTable
WaveformConfigLexer::GenerateTraitsTable() const
{
    const unsigned char NEWLINE = '\n';

    const std::string END_OF_LINE("\0\n", 2);

    // regex matching integer and floating point numbers
    const std::string NUMBER_PATTERN =
        R"PAT([+-]?(0|[1-9][0-9]*)(.[0-9]+)?([eE][+-]?[0-9]{1,3})?)PAT";

    std::string allCharacters;
    std::string whitespace;
    std::string notWhitespace;

    for (int i = 0; i < 256; ++i)
    {
        allCharacters.push_back(static_cast<unsigned char>(i));

        if (i != NEWLINE && std::isspace(i))
        {
            whitespace.push_back(static_cast<unsigned char>(i));
        }
        else
        {
            notWhitespace.push_back(static_cast<unsigned char>(i));
        }
    }

    using StringMatch = ValueMatcher::StringValue;
    using RegexMatch = ValueMatcher::RegexPattern;
    // initializer_lists are always passed by value which means that they
    // cannot contain unique_ptr since unique_ptr cannot be copied.
    // This requires building the traits table in two steps:
    // First, create an initializer list that contains the arguments
    // for each Token.
    // Second, iterate over the entries in the initializer list
    // creating a new TokenTraits object from the entry and passing
    // the new object to the vector.
    const std::initializer_list<std::tuple<TokenType, ValueMatcher, TerminatorMatcher>>
        TRAITS_PARAMS = {std::make_tuple(TokenType::Band,
                                         ValueMatcher(StringMatch{"band"}),
                                         TerminatorMatcher(whitespace)),
                         std::make_tuple(TokenType::Begin,
                                         ValueMatcher(StringMatch{"begin"}),
                                         TerminatorMatcher(whitespace)),
                         std::make_tuple(TokenType::Comment,
                                         ValueMatcher(RegexMatch{"^#.*"}),
                                         TerminatorMatcher(END_OF_LINE)),
                         std::make_tuple(TokenType::Constant,
                                         ValueMatcher(StringMatch{"constant"}),
                                         TerminatorMatcher(whitespace)),
                         std::make_tuple(TokenType::Custom,
                                         ValueMatcher(StringMatch{"custom"}),
                                         TerminatorMatcher(whitespace)),
                         std::make_tuple(TokenType::Dbm,
                                         ValueMatcher(StringMatch{"dbm"}),
                                         TerminatorMatcher(whitespace)),
                         std::make_tuple(TokenType::End,
                                         ValueMatcher(StringMatch{"end"}),
                                         TerminatorMatcher(whitespace)),
                         std::make_tuple(TokenType::Interval,
                                         ValueMatcher(StringMatch{"interval"}),
                                         TerminatorMatcher(whitespace)),
                         std::make_tuple(TokenType::Node,
                                         ValueMatcher(StringMatch{"node"}),
                                         TerminatorMatcher(whitespace)),
                         std::make_tuple(TokenType::Newline,
                                         ValueMatcher(StringMatch{"\n"}),
                                         TerminatorMatcher(allCharacters)),
                         std::make_tuple(TokenType::Number,
                                         ValueMatcher(RegexMatch{NUMBER_PATTERN}),
                                         TerminatorMatcher(whitespace + END_OF_LINE)),
                         std::make_tuple(TokenType::Random,
                                         ValueMatcher(StringMatch{"random"}),
                                         TerminatorMatcher(whitespace)),
                         std::make_tuple(TokenType::String,
                                         ValueMatcher(RegexMatch{R"PAT("[\S ]*")PAT"}),
                                         TerminatorMatcher(whitespace + END_OF_LINE)),
                         std::make_tuple(TokenType::Txslot,
                                         ValueMatcher(StringMatch{"txslot"}),
                                         TerminatorMatcher(whitespace + END_OF_LINE)),
                         std::make_tuple(TokenType::Waveform,
                                         ValueMatcher(StringMatch{"waveform"}),
                                         TerminatorMatcher(whitespace + END_OF_LINE)),
                         std::make_tuple(TokenType::Whitespace,
                                         ValueMatcher(RegexMatch{R"PAT([ \f\r\t\v]+)PAT"}),
                                         TerminatorMatcher(notWhitespace)),
                         std::make_tuple(TokenType::Unknown,
                                         ValueMatcher(RegexMatch{R"(.+)"}),
                                         TerminatorMatcher(END_OF_LINE))};

    TokenTraitsTable table;

    // create the table from the arguments in the initializer_list
    for (const auto& params : TRAITS_PARAMS)
    {
        table.emplace_back(make_unique_traits(params));
    }

    return table;
}

unsigned char
WaveformConfigLexer::GetChar() const
{
    unsigned char c = 0;

    if (!Eof())
    {
        c = static_cast<unsigned char>(*m_position);
    }

    return c;
}

unsigned char
WaveformConfigLexer::GetNextChar()
{
    StepOnce();

    return GetChar();
}

void
WaveformConfigLexer::StepOnce()
{
    // is there any more data in the buffer?
    auto diff = std::distance(m_position, m_end);

    if (diff <= 1)
    {
        // no more data, try and read another block
        ReadBlock();
    }
    else
    {
        // more data, advance the iterator
        ++m_position;
        ++m_column;
    }
}

void
WaveformConfigLexer::ReadBlock()
{
    std::istream& inputStream = m_stream->GetStream();

    if (Eof())
    {
        // no more data
        return;
    }
    else if (!inputStream.good())
    {
        // stream is in a bad state
        // set the eof bit and clear the buffer
        inputStream.setstate(std::ios::eofbit);
        m_position = m_end;

        return;
    }

    inputStream.read(m_buffer.data(), m_buffer.size());

    // get the number of bytes read
    std::streamsize count = inputStream.gcount();

    if (count > 0)
    {
        m_position = m_buffer.begin();
        m_end = m_position + count;
        ++m_column;
    }
    else
    {
        m_position = m_end;
    }
}

std::ostream&
operator<<(std::ostream& stream, WaveformConfigLexer::TokenType type)
{
    using LexerTokenType = WaveformConfigLexer::TokenType;

    switch (type)
    {
    case LexerTokenType::Band: {
        stream << "Band";
    }
    break;
    case LexerTokenType::Begin: {
        stream << "Begin";
    }
    break;
    case LexerTokenType::Constant: {
        stream << "Constant";
    }
    break;
    case LexerTokenType::Custom: {
        stream << "Custom";
    }
    break;
    case LexerTokenType::Dbm: {
        stream << "Dbm";
    }
    break;
    case LexerTokenType::End: {
        stream << "End";
    }
    break;
    case LexerTokenType::Interval: {
        stream << "Interval";
    }
    break;
    case LexerTokenType::Node: {
        stream << "Node";
    }
    break;
    case LexerTokenType::Random: {
        stream << "Random";
    }
    break;
    case LexerTokenType::Txslot: {
        stream << "TxSlot";
    }
    break;
    case LexerTokenType::Waveform: {
        stream << "Waveform";
    }
    break;
    case LexerTokenType::Newline: {
        stream << "Newline";
    }
    break;
    case LexerTokenType::Whitespace: {
        stream << "Whitespace";
    }
    break;
    case LexerTokenType::Comment: {
        stream << "Comment";
    }
    break;
    case LexerTokenType::Number: {
        stream << "Number";
    }
    break;
    case LexerTokenType::String: {
        stream << "String";
    }
    break;
    case LexerTokenType::EndofFile: {
        stream << "EndofFile";
    }
    break;
    case LexerTokenType::Unknown: {
        stream << "Unknown";
    }
    break;
    default: {
        stream << "TokenType:" << static_cast<int>(type);
    }
    break;
    }

    return stream;
}

} // namespace ns3
