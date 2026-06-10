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
#include <filesystem>
#include <string_view>
#include <string>
#include <optional>

#include <simdjson.h>

namespace Profiler
{
    class DeviceProfilerJsonValue;
    class DeviceProfilerJsonObject;
    class DeviceProfilerJsonArray;

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

        DeviceProfilerJsonValue GetParsedDocument();

    private:
        struct ParserData;
        std::unique_ptr<ParserData> m_pData;

        bool EnsureParserData();
    };

    /*************************************************************************\

    Class:
        DeviceProfilerJsonValue

    Description:
        Helper class to represent a JSON value.

    \*************************************************************************/
    class DeviceProfilerJsonValue
    {
    public:
        DeviceProfilerJsonValue();
        DeviceProfilerJsonValue( simdjson::ondemand::value&& value );

        bool IsValid() const { return m_Value.has_value(); }

        std::string_view ToStringView() const { return CastTo<std::string_view>(); }
        std::string ToString() const { return std::string( ToStringView() ); }
        int64_t ToInt64() const { return CastTo<int64_t>(); }
        uint64_t ToUint64() const { return CastTo<uint64_t>(); }
        int32_t ToInt32() const { return CastTo<int32_t>(); }
        uint32_t ToUint32() const { return CastTo<uint32_t>(); }
        double ToDouble() const { return CastTo<double>(); }
        float ToFloat() const { return static_cast<float>( ToDouble() ); }
        bool ToBool() const { return CastTo<bool>(); }

        DeviceProfilerJsonObject ToObject() const;
        DeviceProfilerJsonArray ToArray() const;

        // Object-like accessor.
        DeviceProfilerJsonValue Get( std::string_view key ) const;

        // Array-like accessor.
        DeviceProfilerJsonValue Get( size_t index ) const;

    private:
        std::optional<simdjson::ondemand::value> mutable m_Value;

        template<typename T>
        T CastTo() const;
    };

    /*************************************************************************\

    Class:
        DeviceProfilerJsonObject

    Description:
        Helper class to represent a JSON object.

    \*************************************************************************/
    class DeviceProfilerJsonObject
    {
    public:
        class Iterator;

        DeviceProfilerJsonObject();
        DeviceProfilerJsonObject( simdjson::ondemand::object&& object );

        bool IsValid() const { return m_Object.has_value(); }

        size_t GetSize() const;

        DeviceProfilerJsonValue Get( std::string_view key ) const;

        Iterator begin() const;
        Iterator end() const;

    private:
        std::optional<simdjson::ondemand::object> mutable m_Object;
    };

    /*************************************************************************\

    Class:
        DeviceProfilerJsonObject::Iterator

    Description:
        Helper class to represent a JSON object iterator.

    \*************************************************************************/
    class DeviceProfilerJsonObject::Iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::pair<std::string_view, DeviceProfilerJsonValue>;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = value_type;

        Iterator( simdjson::ondemand::object_iterator&& iterator = simdjson::ondemand::object_iterator() )
            : m_Iterator( std::move( iterator ) )
        {
        }

        value_type operator*()
        {
            auto result = *m_Iterator;
            if( result.error() )
                return { std::string_view(), DeviceProfilerJsonValue() };
            auto& field = result.value_unsafe();
            auto key = field.unescaped_key();
            if( key.error() )
                return { std::string_view(), DeviceProfilerJsonValue() };
            return { key.value(), std::move( field.value() ) };
        }

        Iterator& operator++() { return ++m_Iterator, *this; }
        bool operator!=( const Iterator& other ) const { return m_Iterator != other.m_Iterator; }
        bool operator==( const Iterator& other ) const { return m_Iterator == other.m_Iterator; }

    private:
        simdjson::ondemand::object_iterator m_Iterator;
    };

    /*************************************************************************\

    Class:
        DeviceProfilerJsonArray

    Description:
        Helper class to represent a JSON array.

    \*************************************************************************/
    class DeviceProfilerJsonArray
    {
    public:
        class Iterator;

        DeviceProfilerJsonArray();
        DeviceProfilerJsonArray( simdjson::ondemand::array&& array );

        bool IsValid() const { return m_Array.has_value(); }

        size_t GetSize() const;

        DeviceProfilerJsonValue Get( size_t index ) const;

        Iterator begin() const;
        Iterator end() const;

    private:
        std::optional<simdjson::ondemand::array> mutable m_Array;
    };

    /*************************************************************************\

    Class:
        DeviceProfilerJsonArray::Iterator

    Description:
        Helper class to represent a JSON array iterator.

    \*************************************************************************/
    class DeviceProfilerJsonArray::Iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = DeviceProfilerJsonValue;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = value_type;

        Iterator( simdjson::ondemand::array_iterator&& iterator = simdjson::ondemand::array_iterator() )
            : m_Iterator( std::move( iterator ) )
        {
        }

        value_type operator*()
        {
            auto result = *m_Iterator;
            if( result.error() )
                return DeviceProfilerJsonValue();
            return std::move( result.value_unsafe() );
        }

        Iterator& operator++() { return ++m_Iterator, *this; }
        bool operator!=( const Iterator& other ) const { return m_Iterator != other.m_Iterator; }
        bool operator==( const Iterator& other ) const { return m_Iterator == other.m_Iterator; }

    private:
        simdjson::ondemand::array_iterator m_Iterator;
    };

    /*************************************************************************\

    Function:
        CastTo

    Description:
        Cast the JSON value to a specific type.

    \*************************************************************************/
    template<typename T>
    inline T DeviceProfilerJsonValue::CastTo() const
    {
        if( !m_Value.has_value() )
        {
            return T();
        }

        auto result = m_Value->template get<T>();
        if( result.error() )
        {
            return T();
        }

        return result.value_unsafe();
    }
}
