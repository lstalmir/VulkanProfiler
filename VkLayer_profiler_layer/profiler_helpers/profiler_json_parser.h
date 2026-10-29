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

#pragma once
#include "profiler_json_common.h"

#include <filesystem>
#include <string_view>
#include <string>
#include <optional>

namespace Profiler
{
    class DeviceProfilerJsonValueReader;
    class DeviceProfilerJsonObjectReader;
    class DeviceProfilerJsonArrayReader;

    /*************************************************************************\

    Class:
        DeviceProfilerJsonParser

    Description:
        Helper class to parse JSON files.

    \*************************************************************************/
    class DeviceProfilerJsonParser
    {
    public:
        DeviceProfilerJsonParser();
        ~DeviceProfilerJsonParser();

        bool ParseFile( const std::filesystem::path& filename );
        bool ParseString( const std::string_view& json );
        bool ParseLines( const std::string_view& jsons );

        void Reset();
        void Rewind();

        DeviceProfilerJsonValueReader GetParsedDocument();

    private:
        struct ParserData;
        std::unique_ptr<ParserData> m_pData;

        bool EnsureParserData();
    };

    /*************************************************************************\

    Class:
        DeviceProfilerJsonReaderBase

    Description:
        Base class for all JSON readers.
        Provides common functionality to track read errors.

    \*************************************************************************/
    class DeviceProfilerJsonReaderBase
    {
    public:
        DeviceProfilerJsonReaderBase()
            : m_HasReadError( false )
        {
        }

        void SetError() { m_HasReadError = true; }
        void ClearErrors() { m_HasReadError = false; }
        bool ReadErrors() { return std::exchange( m_HasReadError, false ); }

        operator bool() & { return !m_HasReadError; }

    private:
        bool m_HasReadError;
    };

    /*************************************************************************\

    Class:
        DeviceProfilerJsonValueReader

    Description:
        Helper class to read a JSON value.

    \*************************************************************************/
    class DeviceProfilerJsonValueReader
        : public DeviceProfilerJsonReaderBase
    {
    public:
        DeviceProfilerJsonValueReader();
        DeviceProfilerJsonValueReader( simdjson::ondemand::value&& value );

        bool IsValid() const { return m_Value.has_value(); }

        std::string_view ToStringView() { return CastTo<std::string_view>(); }
        std::string ToString() { return std::string( ToStringView() ); }
        int64_t ToInt64() { return CastTo<int64_t>(); }
        uint64_t ToUint64() { return CastTo<uint64_t>(); }
        int32_t ToInt32() { return CastTo<int32_t>(); }
        uint32_t ToUint32() { return CastTo<uint32_t>(); }
        double ToDouble() { return CastTo<double>(); }
        float ToFloat() { return static_cast<float>( ToDouble() ); }
        bool ToBool() { return CastTo<bool>(); }

        DeviceProfilerJsonObjectReader ReadObject();
        DeviceProfilerJsonArrayReader ReadArray();

    private:
        std::optional<simdjson::ondemand::value> m_Value;

        template<typename T>
        T CastTo();
    };

    /*************************************************************************\

    Class:
        DeviceProfilerJsonObjectReader

    Description:
        Helper class to read a JSON object.

    \*************************************************************************/
    class DeviceProfilerJsonObjectReader
        : public DeviceProfilerJsonReaderBase
    {
    public:
        class Iterator;

        DeviceProfilerJsonObjectReader();
        DeviceProfilerJsonObjectReader( simdjson::ondemand::object&& object );

        DeviceProfilerJsonObjectReader( DeviceProfilerJsonObjectReader&& ) = delete;
        DeviceProfilerJsonObjectReader& operator=( DeviceProfilerJsonObjectReader&& ) = delete;

        DeviceProfilerJsonObjectReader( const DeviceProfilerJsonObjectReader& ) = delete;
        DeviceProfilerJsonObjectReader& operator=( const DeviceProfilerJsonObjectReader& ) = delete;

        bool IsValid() const { return m_Object.has_value(); }

        template<typename T>
        DeviceProfilerJsonObjectReader& Read( std::string_view key, T& target );

        template<typename T>
        DeviceProfilerJsonObjectReader& ReadArray( std::string_view key, T& target );

        DeviceProfilerJsonValueReader Read( std::string_view key );
        DeviceProfilerJsonObjectReader ReadObject( std::string_view key );
        DeviceProfilerJsonArrayReader ReadArray( std::string_view key );

        Iterator begin();
        Iterator end();

    private:
        std::optional<simdjson::ondemand::object> m_Object;
    };

    /*************************************************************************\

    Class:
        DeviceProfilerJsonArrayReader

    Description:
        Helper class to represent a JSON array.

    \*************************************************************************/
    class DeviceProfilerJsonArrayReader
        : public DeviceProfilerJsonReaderBase
    {
    public:
        class Iterator;

        DeviceProfilerJsonArrayReader();
        DeviceProfilerJsonArrayReader( simdjson::ondemand::array&& array );

        DeviceProfilerJsonArrayReader( DeviceProfilerJsonArrayReader&& ) = delete;
        DeviceProfilerJsonArrayReader& operator=( DeviceProfilerJsonArrayReader&& ) = delete;

        DeviceProfilerJsonArrayReader( const DeviceProfilerJsonArrayReader& ) = delete;
        DeviceProfilerJsonArrayReader& operator=( const DeviceProfilerJsonArrayReader& ) = delete;

        bool IsValid() const { return m_Array.has_value(); }

        Iterator begin();
        Iterator end();

    private:
        std::optional<simdjson::ondemand::array> m_Array;
    };

    /*************************************************************************\

    Class:
        DeviceProfilerJsonObject::Iterator

    Description:
        Helper class to represent a JSON object iterator.

    \*************************************************************************/
    class DeviceProfilerJsonObjectReader::Iterator
    {
    public:
        using iterator_impl = simdjson::ondemand::object_iterator;

        using iterator_category = std::input_iterator_tag;
        using value_type = std::pair<std::string_view, DeviceProfilerJsonValueReader>;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = value_type;

        value_type operator*()
        {
            try
            {
                simdjson::ondemand::field field = ( *m_Impl ).take_value();
                return value_type(
                    field.unescaped_key().take_value(),
                    simdjson::ondemand::value( field.value() ) );
            }
            catch( ... )
            {
                m_Reader.SetError();
            }

            return value_type();
        }

        Iterator& operator++() { return ++m_Impl, *this; }
        bool operator!=( const Iterator& other ) const { return m_Impl != other.m_Impl; }
        bool operator==( const Iterator& other ) const { return m_Impl == other.m_Impl; }

    private:
        DeviceProfilerJsonObjectReader& m_Reader;
        iterator_impl m_Impl;

        Iterator( DeviceProfilerJsonObjectReader& reader, iterator_impl&& impl = iterator_impl() )
            : m_Reader( reader )
            , m_Impl( std::move( impl ) )
        {
        }

        friend class DeviceProfilerJsonObjectReader;
    };

    /*************************************************************************\

    Class:
        DeviceProfilerJsonArray::Iterator

    Description:
        Helper class to represent a JSON array iterator.

    \*************************************************************************/
    class DeviceProfilerJsonArrayReader::Iterator
    {
    public:
        using iterator_impl = simdjson::ondemand::array_iterator;

        using iterator_category = std::input_iterator_tag;
        using value_type = DeviceProfilerJsonValueReader;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = value_type;

        value_type operator*()
        {
            try
            {
                return ( *m_Impl ).take_value();
            }
            catch( ... )
            {
                m_Reader.SetError();
            }

            return value_type();
        }

        Iterator& operator++() { return ++m_Impl, *this; }
        bool operator!=( const Iterator& other ) const { return m_Impl != other.m_Impl; }
        bool operator==( const Iterator& other ) const { return m_Impl == other.m_Impl; }

    private:
        DeviceProfilerJsonArrayReader& m_Reader;
        iterator_impl m_Impl;

        Iterator( DeviceProfilerJsonArrayReader& reader, iterator_impl&& impl = iterator_impl() )
            : m_Reader( reader )
            , m_Impl( std::move( impl ) )
        {
        }

        friend class DeviceProfilerJsonArrayReader;
    };

    /*************************************************************************\

    Function:
        CastTo

    Description:
        Cast the JSON value to a specific type.

    \*************************************************************************/
    template<typename T>
    inline T DeviceProfilerJsonValueReader::CastTo()
    {
        try
        {
            return m_Value.value().template get<T>().take_value();
        }
        catch( ... )
        {
            SetError();
        }

        return T();
    }

    /*************************************************************************\

    Function:
        Read

    Description:
        Read a JSON field with a specific key and type from the JSON object.

    \*************************************************************************/
    template<typename T>
    DeviceProfilerJsonObjectReader& DeviceProfilerJsonObjectReader::Read( std::string_view key, T& target )
    {
        try
        {
            if( m_Object.value().find_field_unordered( key ).take_value().template get<T>( target ) != simdjson::SUCCESS )
            {
                SetError();
            }
        }
        catch( ... )
        {
            SetError();
        }

        return *this;
    }

    /*************************************************************************\

    Function:
        ReadArray

    Description:
        Read a JSON array with a specific key and type from the JSON object.

    \*************************************************************************/
    template<typename T>
    DeviceProfilerJsonObjectReader& DeviceProfilerJsonObjectReader::ReadArray( std::string_view key, T& target )
    {
        simdjson::ondemand::array array;

        if( Read( key, array ) )
        {
            for( auto item : array )
            {
                if( item.template get<typename T::value_type>( target.emplace_back() ) != simdjson::SUCCESS )
                {
                    target.pop_back();
                    SetError();
                    break;
                }
            }
        }

        return *this;
    }
}
