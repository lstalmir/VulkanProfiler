// Copyright (c) 2026 Lukasz Stalmirski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "profiler_json_parser.h"

namespace Profiler
{
    /*************************************************************************\

    Structure:
        DeviceProfilerJsonParser::ParserData

    Description:
        Holds the parser and documents for the JSON parser.

    \*************************************************************************/
    struct DeviceProfilerJsonParser::ParserData
    {
        simdjson::ondemand::parser m_Parser;
        simdjson::padded_string m_DataBuffer;

        // Single document mode.
        std::optional<simdjson::ondemand::document> m_Document;

        // Multi document mode.
        std::optional<simdjson::ondemand::document_stream> m_DocumentStream;
        std::optional<simdjson::ondemand::document_stream::iterator> m_DocumentStreamIterator;
    };

    /*************************************************************************\

    Function:
        DeviceProfilerJsonParser

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerJsonParser::DeviceProfilerJsonParser()
        : m_pData( nullptr )
    {
    }

    /*************************************************************************\

    Function:
        ~DeviceProfilerJsonParser

    Description:
        Destructor.

    \*************************************************************************/
    DeviceProfilerJsonParser::~DeviceProfilerJsonParser()
    {
    }

    /*************************************************************************\

    Function:
        ParseFile

    Description:
        Parses JSON document from a file.

    \*************************************************************************/
    bool DeviceProfilerJsonParser::ParseFile( const std::filesystem::path& filename )
    {
        if( !EnsureParserData() )
        {
            return false;
        }

        // Clear previous document(s).
        Reset();

        // Load data from the file.
        auto loadResult = simdjson::padded_string::load( filename.string() );
        if( loadResult.error() )
        {
            return false;
        }

        m_pData->m_DataBuffer = std::move( loadResult.value() );

        if( !m_pData->m_DataBuffer.data() )
        {
            return false;
        }

        // Start parsing the document.
        auto iterateResult = m_pData->m_Parser.iterate( m_pData->m_DataBuffer );
        if( iterateResult.error() )
        {
            m_pData->m_DataBuffer = simdjson::padded_string();
            return false;
        }

        m_pData->m_Document = std::move( iterateResult.value_unsafe() );

        return true;
    }

    /*************************************************************************\

    Function:
        ParseString

    Description:
        Parses JSON document from a string.

    \*************************************************************************/
    bool DeviceProfilerJsonParser::ParseString( const std::string_view& json )
    {
        if( !EnsureParserData() )
        {
            return false;
        }

        // Clear previous document(s).
        Reset();

        // Copy the input string into the padded string buffer.
        m_pData->m_DataBuffer = simdjson::padded_string( json );

        if( !m_pData->m_DataBuffer.data() )
        {
            return false;
        }

        // Start parsing the document.
        auto iterateResult = m_pData->m_Parser.iterate( m_pData->m_DataBuffer );
        if( iterateResult.error() )
        {
            m_pData->m_DataBuffer = simdjson::padded_string();
            return false;
        }

        m_pData->m_Document = std::move( iterateResult.value_unsafe() );

        return true;
    }

    /*************************************************************************\

    Function:
        ParseLines

    Description:
        Parses multiple JSON documents from a string.

    \*************************************************************************/
    bool DeviceProfilerJsonParser::ParseLines( const std::string_view& json )
    {
        if( !EnsureParserData() )
        {
            return false;
        }

        // Clear previous document(s).
        Reset();

        // Copy the input string into the padded string buffer.
        m_pData->m_DataBuffer = simdjson::padded_string( json );

        if( !m_pData->m_DataBuffer.data() )
        {
            return false;
        }

        // Start parsing the documents.
        auto iterateResult = m_pData->m_Parser.iterate_many(
            m_pData->m_DataBuffer,
            simdjson::ondemand::DEFAULT_BATCH_SIZE,
            true /*allowCommaSeparated*/ );

        if( iterateResult.error() )
        {
            m_pData->m_DataBuffer = simdjson::padded_string();
            return false;
        }

        m_pData->m_DocumentStream = std::move( iterateResult.value_unsafe() );
        m_pData->m_DocumentStreamIterator = m_pData->m_DocumentStream->begin();

        return true;
    }

    /*************************************************************************\

    Function:
        Reset

    Description:
        Frees parsed document(s) and storage buffer.

    \*************************************************************************/
    void DeviceProfilerJsonParser::Reset()
    {
        if( m_pData )
        {
            m_pData->m_Document = std::nullopt;

            m_pData->m_DocumentStream = std::nullopt;
            m_pData->m_DocumentStreamIterator = simdjson::ondemand::document_stream::iterator();

            m_pData->m_DataBuffer = simdjson::padded_string();
        }
    }

    /*************************************************************************\

    Function:
        Reset

    Description:
        Frees parsed document(s) and storage buffer.

    \*************************************************************************/
    void DeviceProfilerJsonParser::Rewind()
    {
        if( !m_pData )
        {
            return;
        }

        if( m_pData->m_Document.has_value() )
        {
            m_pData->m_Document->rewind();
            return;
        }

        if( m_pData->m_DocumentStream.has_value() )
        {
            m_pData->m_DocumentStreamIterator = m_pData->m_DocumentStream->begin();
            return;
        }
    }

    /*************************************************************************\

    Function:
        GetParsedDocument

    Description:
        Returns the next document's root.

    \*************************************************************************/
    DeviceProfilerJsonValueReader DeviceProfilerJsonParser::GetParsedDocument()
    {
        if( !m_pData )
        {
            return DeviceProfilerJsonValueReader();
        }

        if( m_pData->m_Document.has_value() )
        {
            return simdjson::ondemand::value( *m_pData->m_Document );
        }

        if( m_pData->m_DocumentStream.has_value() )
        {
            if( !m_pData->m_DocumentStreamIterator.has_value() )
            {
                // Start iterating the document stream.
                m_pData->m_DocumentStreamIterator = m_pData->m_DocumentStream->begin();
            }
            else
            {
                // Advance to the next document in the stream.
                ++( *m_pData->m_DocumentStreamIterator );
            }

            if( !m_pData->m_DocumentStreamIterator->at_end() )
            {
                return simdjson::ondemand::value( **m_pData->m_DocumentStreamIterator );
            }
        }

        return DeviceProfilerJsonValueReader();
    }

    /*************************************************************************\

    Function:
        EnsureParserData

    Description:
        Allocates the parser data if it hasn't been allocated yet.

    \*************************************************************************/
    bool DeviceProfilerJsonParser::EnsureParserData()
    {
        if( !m_pData )
        {
            m_pData.reset( new( std::nothrow ) ParserData() );
        }

        return m_pData != nullptr;
    }

    /*************************************************************************\

    Function:
        DeviceProfilerJsonValue

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerJsonValueReader::DeviceProfilerJsonValueReader()
        : m_Value( std::nullopt )
    {
    }

    /*************************************************************************\

    Function:
        DeviceProfilerJsonValue

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerJsonValueReader::DeviceProfilerJsonValueReader( simdjson::ondemand::value&& value )
        : m_Value( value )
    {
    }

    /*************************************************************************\

    Function:
        ReadObject

    Description:
        Start reading the JSON value as an object.

    \*************************************************************************/
    DeviceProfilerJsonObjectReader DeviceProfilerJsonValueReader::ReadObject()
    {
        try
        {
            return m_Value.value().get_object().take_value();
        }
        catch( ... )
        {
            SetError();
        }

        return DeviceProfilerJsonObjectReader();
    }

    /*************************************************************************\

    Function:
        ReadArray

    Description:
        Start reading the JSON value as an array.

    \*************************************************************************/
    DeviceProfilerJsonArrayReader DeviceProfilerJsonValueReader::ReadArray()
    {
        try
        {
            return m_Value.value().get_array().take_value();
        }
        catch( ... )
        {
            SetError();
        }

        return DeviceProfilerJsonArrayReader();
    }

    /*************************************************************************\

    Function:
        DeviceProfilerJsonObject

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerJsonObjectReader::DeviceProfilerJsonObjectReader()
        : m_Object( std::nullopt )
    {
    }

    /*************************************************************************\

    Function:
        DeviceProfilerJsonObject

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerJsonObjectReader::DeviceProfilerJsonObjectReader( simdjson::ondemand::object&& object )
        : m_Object( object )
    {
    }

    /*************************************************************************\

    Function:
        Read

    Description:
        Read a JSON field with a specific key as a value.

    \*************************************************************************/
    DeviceProfilerJsonValueReader DeviceProfilerJsonObjectReader::Read( std::string_view key )
    {
        try
        {
            return m_Object.value().find_field_unordered( key ).take_value();
        }
        catch( ... )
        {
            SetError();
        }

        return DeviceProfilerJsonValueReader();
    }

    /*************************************************************************\

    Function:
        ReadObject

    Description:
        Begin reading a JSON field with a specific key as an object.

    \*************************************************************************/
    DeviceProfilerJsonObjectReader DeviceProfilerJsonObjectReader::ReadObject( std::string_view key )
    {
        simdjson::ondemand::object object;

        if( Read( key, object ) )
        {
            return std::move( object );
        }

        return DeviceProfilerJsonObjectReader();
    }

    /*************************************************************************\

    Function:
        ReadArray

    Description:
        Begin reading a JSON field with a specific key as an array.

    \*************************************************************************/
    DeviceProfilerJsonArrayReader DeviceProfilerJsonObjectReader::ReadArray( std::string_view key )
    {
        simdjson::ondemand::array array;

        if( Read( key, array ) )
        {
            return std::move( array );
        }

        return DeviceProfilerJsonArrayReader();
    }

    /*************************************************************************\

    Function:
        begin

    Description:
        Returns an iterator to the beginning of the JSON object.

    \*************************************************************************/
    DeviceProfilerJsonObjectReader::Iterator DeviceProfilerJsonObjectReader::begin()
    {
        try
        {
            return Iterator( *this, m_Object.value().begin().take_value() );
        }
        catch( ... )
        {
            SetError();
        }

        return Iterator( *this );
    }

    /*************************************************************************\

    Function:
        end

    Description:
        Returns an iterator to the end of the JSON object.

    \*************************************************************************/
    DeviceProfilerJsonObjectReader::Iterator DeviceProfilerJsonObjectReader::end()
    {
        try
        {
            return Iterator( *this, m_Object.value().end().take_value() );
        }
        catch( ... )
        {
            SetError();
        }

        return Iterator( *this );
    }

    /*************************************************************************\

    Function:
        DeviceProfilerJsonArrayReader

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerJsonArrayReader::DeviceProfilerJsonArrayReader()
        : m_Array( std::nullopt )
    {
    }

    /*************************************************************************\

    Function:
        DeviceProfilerJsonArrayReader

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerJsonArrayReader::DeviceProfilerJsonArrayReader( simdjson::ondemand::array&& array )
        : m_Array( array )
    {
    }

    /*************************************************************************\

    Function:
        begin

    Description:
        Returns an iterator to the beginning of the JSON array.

    \*************************************************************************/
    DeviceProfilerJsonArrayReader::Iterator DeviceProfilerJsonArrayReader::begin()
    {
        try
        {
            return Iterator( *this, m_Array.value().begin().take_value() );
        }
        catch( ... )
        {
            SetError();
        }

        return Iterator( *this );
    }

    /*************************************************************************\

    Function:
        end

    Description:
        Returns an iterator to the end of the JSON array.

    \*************************************************************************/
    DeviceProfilerJsonArrayReader::Iterator DeviceProfilerJsonArrayReader::end()
    {
        try
        {
            return Iterator( *this, m_Array.value().end().take_value() );
        }
        catch( ... )
        {
            SetError();
        }

        return Iterator( *this );
    }
}
