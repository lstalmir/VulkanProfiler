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
    DeviceProfilerJsonValue DeviceProfilerJsonParser::GetParsedDocument()
    {
        if( !m_pData )
        {
            return DeviceProfilerJsonValue();
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

        return DeviceProfilerJsonValue();
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
    DeviceProfilerJsonValue::DeviceProfilerJsonValue()
        : m_Value( std::nullopt )
    {
    }

    /*************************************************************************\

    Function:
        DeviceProfilerJsonValue

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerJsonValue::DeviceProfilerJsonValue( simdjson::ondemand::value&& value )
        : m_Value( value )
    {
    }

    /*************************************************************************\

    Function:
        ToObject

    Description:
        Casts the JSON value to an object and returns it.

    \*************************************************************************/
    DeviceProfilerJsonObject DeviceProfilerJsonValue::ToObject() const
    {
        if( !m_Value.has_value() )
        {
            return DeviceProfilerJsonObject();
        }

        auto result = m_Value->get_object();
        if( result.error() )
        {
            return DeviceProfilerJsonObject();
        }

        return std::move( result.value_unsafe() );
    }

    /*************************************************************************\

    Function:
        ToArray

    Description:
        Casts the JSON value to an array and returns it.

    \*************************************************************************/
    DeviceProfilerJsonArray DeviceProfilerJsonValue::ToArray() const
    {
        if( !m_Value.has_value() )
        {
            return DeviceProfilerJsonArray();
        }

        auto result = m_Value->get_array();
        if( result.error() )
        {
            return DeviceProfilerJsonArray();
        }

        return std::move( result.value_unsafe() );
    }

    /*************************************************************************\

    Function:
        Get

    Description:
        Object-like access to the JSON value.

    \*************************************************************************/
    DeviceProfilerJsonValue DeviceProfilerJsonValue::Get( std::string_view key ) const
    {
        if( !m_Value.has_value() )
        {
            return DeviceProfilerJsonValue();
        }

        auto result = m_Value->find_field_unordered( key );
        if( result.error() )
        {
            return DeviceProfilerJsonValue();
        }

        return std::move( result.value_unsafe() );
    }

    /*************************************************************************\

    Function:
        Get

    Description:
        Array-like access to the JSON value.

    \*************************************************************************/
    DeviceProfilerJsonValue DeviceProfilerJsonValue::Get( size_t index ) const
    {
        if( !m_Value.has_value() )
        {
            return DeviceProfilerJsonValue();
        }

        auto result = m_Value->at( index );
        if( result.error() )
        {
            return DeviceProfilerJsonValue();
        }

        return std::move( result.value_unsafe() );
    }

    /*************************************************************************\

    Function:
        DeviceProfilerJsonObject

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerJsonObject::DeviceProfilerJsonObject()
        : m_Object( std::nullopt )
    {
    }

    /*************************************************************************\

    Function:
        DeviceProfilerJsonObject

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerJsonObject::DeviceProfilerJsonObject( simdjson::ondemand::object&& object )
        : m_Object( object )
    {
    }

    /*************************************************************************\

    Function:
        GetSize

    Description:
        Returns the number of key-value pairs in the JSON object.

    \*************************************************************************/
    size_t DeviceProfilerJsonObject::GetSize() const
    {
        if( !m_Object.has_value() )
        {
            return 0;
        }

        return m_Object->count_fields();
    }

    /*************************************************************************\

    Function:
        Get

    Description:
        Returns the value of the specified field in the JSON object.

    \*************************************************************************/
    DeviceProfilerJsonValue DeviceProfilerJsonObject::Get( std::string_view key ) const
    {
        if( !m_Object.has_value() )
        {
            return DeviceProfilerJsonValue();
        }

        auto result = m_Object->find_field_unordered( key );
        if( result.error() )
        {
            return DeviceProfilerJsonValue();
        }

        return std::move( result.value_unsafe() );
    }

    /*************************************************************************\

    Function:
        begin

    Description:
        Returns an iterator to the beginning of the JSON object.

    \*************************************************************************/
    DeviceProfilerJsonObject::Iterator DeviceProfilerJsonObject::begin() const
    {
        if( !m_Object.has_value() )
        {
            return Iterator();
        }

        auto result = m_Object->begin();
        if( result.error() )
        {
            return Iterator();
        }

        return std::move( result.value_unsafe() );
    }

    /*************************************************************************\

    Function:
        end

    Description:
        Returns an iterator to the end of the JSON object.

    \*************************************************************************/
    DeviceProfilerJsonObject::Iterator DeviceProfilerJsonObject::end() const
    {
        if( !m_Object.has_value() )
        {
            return Iterator();
        }

        auto result = m_Object->end();
        if( result.error() )
        {
            return Iterator();
        }

        return std::move( result.value_unsafe() );
    }

    /*************************************************************************\

    Function:
        DeviceProfilerJsonArray

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerJsonArray::DeviceProfilerJsonArray()
        : m_Array( std::nullopt )
    {
    }

    /*************************************************************************\

    Function:
        DeviceProfilerJsonArray

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerJsonArray::DeviceProfilerJsonArray( simdjson::ondemand::array&& array )
        : m_Array( array )
    {
    }

    /*************************************************************************\

    Function:
        GetSize

    Description:
        Returns the number of elements in the JSON array.

    \*************************************************************************/
    size_t DeviceProfilerJsonArray::GetSize() const
    {
        if( !m_Array.has_value() )
        {
            return 0;
        }

        return m_Array->count_elements();
    }

    /*************************************************************************\

    Function:
        Get

    Description:
        Returns the value at the specified index in the JSON array.

    \*************************************************************************/
    DeviceProfilerJsonValue DeviceProfilerJsonArray::Get( size_t index ) const
    {
        if( !m_Array.has_value() )
        {
            return DeviceProfilerJsonValue();
        }

        auto result = m_Array->at( index );
        if( result.error() )
        {
            return DeviceProfilerJsonValue();
        }

        return std::move( result.value_unsafe() );
    }

    /*************************************************************************\

    Function:
        begin

    Description:
        Returns an iterator to the beginning of the JSON array.

    \*************************************************************************/
    DeviceProfilerJsonArray::Iterator DeviceProfilerJsonArray::begin() const
    {
        if( !m_Array.has_value() )
        {
            return Iterator();
        }

        auto result = m_Array->begin();
        if( result.error() )
        {
            return Iterator();
        }

        return std::move( result.value_unsafe() );
    }

    /*************************************************************************\

    Function:
        end

    Description:
        Returns an iterator to the end of the JSON array.

    \*************************************************************************/
    DeviceProfilerJsonArray::Iterator DeviceProfilerJsonArray::end() const
    {
        if( !m_Array.has_value() )
        {
            return Iterator();
        }

        auto result = m_Array->end();
        if( result.error() )
        {
            return Iterator();
        }

        return std::move( result.value_unsafe() );
    }
}
