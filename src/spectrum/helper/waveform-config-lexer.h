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

#ifndef WAVEFORM_CONFIG_LEXER_H_
#define WAVEFORM_CONFIG_LEXER_H_

#include "ns3/simple-ref-count.h"

#include <cstddef>
#include <istream>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

/**
 * \file
 * \ingroup spectrum
 * ns3::WaveformConfigLexer class declaration.
 */

namespace ns3
{

/**
 * \ingroup spectrum
 * \brief Class which reads the input stream and converts sequences
 * of characters to tokens.
 *
 * Valid config files contain the following statements:
 \verbatim
   #this is a comment line
   begin waveform
   node <nodeIndex>
   interval <type> <arguments>
   band <centerFreq> <width>
   begin txslot <duration> <defaultDbm>
   dbm <centerFreq> <value> \endverbatim
 *
 */
class WaveformConfigLexer : public SimpleRefCount<WaveformConfigLexer>
{
  public:
    /// opaque class which abstracts access to the data stream
    class StreamContainer;

    /**
     * Enumeration specifying the supported token types.
     */
    enum class TokenType
    {
        Unknown,    //!< token is unknown or not set
        Band,       //!< token is the band keyword
        Begin,      //!< token is the begin keyword
        Constant,   //!< token is the constant keyword
        Custom,     //!< token is the custom keyword
        Dbm,        //!< token is the dbm keyword
        End,        //!< token is the end keyword
        Interval,   //!< token is the interval keyword
        Node,       //!< token is the node keyword
        Random,     //!< token is the random keyword
        Txslot,     //!< token is the txslot keyword
        Waveform,   //!< token is the waveform keyword
        Newline,    //!< token is a newline
        Whitespace, //!< token is a whitespace
        Comment,    //!< token is the start of a comment
        Number,     //!< token is an integer or floating point number
        String,     //!< token is a string value
        EndofFile   //!< token represents the end of a file stream
    };              // enum class TokenType

    /**
     * Encapsulates all of the data associated with a token.
     */
    struct Token
    {
      public:
        /**
         * Default Constructor
         *
         * Equivalent to Token(TokenType::Unknown, 0, 0, "")
         */
        Token()
            : type(TokenType::Unknown),
              lineNumber(0),
              column(0),
              value()
        {
        }

        /**
         * Creates a token with the specified parameters.
         *
         * \param type Type of the token
         * \param line Line number in the input stream where the token
         * is located
         * \param col  Column number in the input stream where the token
         * is located
         * \param val  Token string extracted from the input stream
         */
        Token(TokenType type, std::size_t line, std::size_t col, std::string val)
            : type(type),
              lineNumber(line),
              column(col),
              value(val)
        {
        }

        TokenType type; //!< Type that this token represents

        /**
         * Line number in the input stream where this token is located.
         * First line starts at 1.
         */
        std::size_t lineNumber;

        /**
         * Column number in the input stream where this token is located.
         * First column starts at 1.
         */
        std::size_t column;

        std::string value; //!< token string extracted from the input stream
    };                     // struct Token

    /** Opaque class containing logic for parsing tokens. */
    class TokenTraits;

    /**
     * Initializes the object using the provided file path as the
     * location where the configuration data is stored.
     * Opens the file located at \p filepath and reads the first
     * block of data into memory.
     *
     * \param filepath Path to a valid waveform file
     *
     * \throw std::ios_base::failure if the file cannot be opened
     */
    WaveformConfigLexer(const std::string& filepath);

    /**
     * Initializes the object using the provided stream as the input source.
     * Reads the first block of data from \p stream into memory.
     *
     * \p stream must remain valid for the lifetime of this object instance.
     *
     * \param stream reference to an input stream containing waveform
     * config data
     */
    WaveformConfigLexer(std::istream& stream);

    /**
     * Destructor
     */
    ~WaveformConfigLexer();

    /**
     * Specifies if there is more data available for processing or not.
     *
     * \return true when all data in the input stream has been processed
     *          or if the stream encountered an error, false otherwise.
     */
    bool Eof() const;

    /**
     * Pulls characters from the stream until a token match occurs.
     *
     * If the input stream encounters an error and is no longer
     * readable, the stream is marked eof and a token with
     * the type TokenType::EndofFile is returned.
     *
     * \return The next token in the input stream.
     */
    Token GetNextToken();

  private:
    /** Container for holding character data.  */
    typedef std::vector<char> StreamBuffer;

    /** Memory managed TokenTraits object. */
    typedef std::unique_ptr<TokenTraits> UniqueTokenTraits;

    /** Vector for holding a collection of token traits. */
    typedef std::vector<UniqueTokenTraits> TokenTraitsTable;

    /** Maximum size of the internal stream buffer. */
    static const std::size_t BLOCK_SIZE = 1 << 20;

    /**
     * Default constructor
     *
     * This constructor creates an empty stream object and should only
     * be used by other constructors to set all of the member variables
     * to default values.
     */
    WaveformConfigLexer();

    /**
     * Searches for a token traits object that matches the supplied
     * string.
     *
     * This function always returns a pointer to a valid TokenTraits
     * object. If no match is found, the token traits object for the
     * unknown token is returned.
     *
     * \param value token string extracted from the input stream
     *
     * \return the token traits that match the input string
     */
    TokenTraits* FindTokenTraits(const std::string& value) const;

    /**
     * Generates the token traits table which is used to look up
     * the token traits for a particular input string.
     *
     * \return data structure which stores TokenTraits
     */
    TokenTraitsTable GenerateTraitsTable() const;

    /**
     * Fetch the character at the current position in the stream.
     *
     * \return the current character in the input stream
     */
    unsigned char GetChar() const;

    /**
     * Moves the input stream forward one character
     * and returns the new character.
     *
     * \return the next character in the input stream
     * or 0 if there are no more characters.
     */
    unsigned char GetNextChar();

    /**
     * Moves the input stream forward one character
     */
    void StepOnce();

    /**
     * Loads a block of data from the input stream
     * into an internal buffer.
     *
     * If the input stream encounters an error,
     * the eof flag is set and the internal buffer
     * is cleared.
     */
    void ReadBlock();

    /** Memory managed pointer to the stream container. */
    std::unique_ptr<StreamContainer> m_stream;

    /** Lookup table mapping strings to token types. */
    TokenTraitsTable m_traitsTable;

    /** Internal buffer holding data from the input stream. */
    StreamBuffer m_buffer;

    /** Current read position in the internal buffer. */
    StreamBuffer::const_iterator m_position;

    /** One past the end of valid data in the internal buffer. */
    StreamBuffer::const_iterator m_end;

    std::size_t m_line;   //!< Current line, first line is 1
    std::size_t m_column; //!< Current column, first column is 1
};                        // class WaveformConfigLexer

/**
 * Stream operator for converting a LexerTokenType value to a string
 * for logging/debugging.
 *
 * \param stream the output stream.
 * \param type the value to serialize.
 * \return reference to \p stream
 */
std::ostream& operator<<(std::ostream& stream, WaveformConfigLexer::TokenType type);

} // namespace ns3

#endif
