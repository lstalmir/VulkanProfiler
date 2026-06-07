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
#include <assert.h>
#include <string_view>
#include <type_traits>

#include <simdjson.h>

namespace Profiler
{
    class DeviceProfilerJsonValueBuilder;
    class DeviceProfilerJsonObjectBuilder;
    class DeviceProfilerJsonArrayBuilder;

    /*************************************************************************\

    Class:
        DeviceProfilerJsonBuilder

    Description:
        Hide the simdjson builder implementation to avoid coupling the
        rest of the code.

    \*************************************************************************/
    typedef simdjson::builder::string_builder DeviceProfilerJsonBuilder;

    /*************************************************************************\

    Class:
        DeviceProfilerJsonValueBuilder

    Description:
        Helper class to build JSON values. Can be used only once per instance
        to ensure that the JSON is well-formed.

    \*************************************************************************/
    class DeviceProfilerJsonValueBuilder
    {
    public:
        DeviceProfilerJsonValueBuilder( DeviceProfilerJsonBuilder& builder );
        ~DeviceProfilerJsonValueBuilder();

        DeviceProfilerJsonValueBuilder( DeviceProfilerJsonValueBuilder&& ) = delete;
        DeviceProfilerJsonValueBuilder& operator=( DeviceProfilerJsonValueBuilder&& ) = delete;

        DeviceProfilerJsonValueBuilder( const DeviceProfilerJsonValueBuilder& ) = delete;
        DeviceProfilerJsonValueBuilder& operator=( const DeviceProfilerJsonValueBuilder& ) = delete;

        template<typename T>
        void MakeValue( const T& value );
        void MakeNull();
        DeviceProfilerJsonArrayBuilder MakeArray();
        DeviceProfilerJsonObjectBuilder MakeObject();

    private:
        DeviceProfilerJsonBuilder& m_Builder;

        bool m_IsEmpty;
    };

    /*************************************************************************\

    Class:
        DeviceProfilerJsonArrayBuilder

    Description:
        Helper class to build JSON arrays.
        It provides a higher-level interface to simdjson builder.

    \*************************************************************************/
    class DeviceProfilerJsonArrayBuilder
    {
    public:
        DeviceProfilerJsonArrayBuilder( DeviceProfilerJsonBuilder& builder );
        DeviceProfilerJsonArrayBuilder( DeviceProfilerJsonBuilder& builder, std::nullptr_t );
        ~DeviceProfilerJsonArrayBuilder();

        DeviceProfilerJsonArrayBuilder( DeviceProfilerJsonArrayBuilder&& ) = delete;
        DeviceProfilerJsonArrayBuilder& operator=( DeviceProfilerJsonArrayBuilder&& ) = delete;

        DeviceProfilerJsonArrayBuilder( const DeviceProfilerJsonArrayBuilder& ) = delete;
        DeviceProfilerJsonArrayBuilder& operator=( const DeviceProfilerJsonArrayBuilder& ) = delete;

        template<typename T>
        DeviceProfilerJsonArrayBuilder& Add( const T& value );
        DeviceProfilerJsonValueBuilder Add();
        DeviceProfilerJsonArrayBuilder AddArray();
        DeviceProfilerJsonObjectBuilder AddObject();
        DeviceProfilerJsonArrayBuilder AddArrayOrNull( bool condition );
        DeviceProfilerJsonObjectBuilder AddObjectOrNull( bool condition );

        void End();

        bool IsEmpty() const { return m_IsEmpty; }
        bool IsEnded() const { return m_IsEnded; }
        bool IsNull() const { return m_IsNull; }

        operator bool() const { return !m_IsEnded; }

    private:
        DeviceProfilerJsonBuilder& m_Builder;

        bool m_IsEmpty;
        bool m_IsEnded;
        bool m_IsNull;
    };

    /*************************************************************************\

    Class:
        DeviceProfilerJsonObjectBuilder

    Description:
        Helper class to build JSON objects.
        It provides a higher-level interface to simdjson builder.

    \*************************************************************************/
    class DeviceProfilerJsonObjectBuilder
    {
    public:
        DeviceProfilerJsonObjectBuilder( DeviceProfilerJsonBuilder& builder );
        DeviceProfilerJsonObjectBuilder( DeviceProfilerJsonBuilder& builder, std::nullptr_t );
        ~DeviceProfilerJsonObjectBuilder();

        DeviceProfilerJsonObjectBuilder( DeviceProfilerJsonObjectBuilder&& ) = delete;
        DeviceProfilerJsonObjectBuilder& operator=( DeviceProfilerJsonObjectBuilder&& ) = delete;

        DeviceProfilerJsonObjectBuilder( const DeviceProfilerJsonObjectBuilder& ) = delete;
        DeviceProfilerJsonObjectBuilder& operator=( const DeviceProfilerJsonObjectBuilder& ) = delete;

        template<typename T>
        DeviceProfilerJsonObjectBuilder& Add( std::string_view key, const T& value );
        DeviceProfilerJsonValueBuilder Add( std::string_view key );
        DeviceProfilerJsonArrayBuilder AddArray( std::string_view key );
        DeviceProfilerJsonObjectBuilder AddObject( std::string_view key );
        DeviceProfilerJsonArrayBuilder AddArrayOrNull( std::string_view key, bool condition );
        DeviceProfilerJsonObjectBuilder AddObjectOrNull( std::string_view key, bool condition );

        void End();

        bool IsEmpty() const { return m_IsEmpty; }
        bool IsEnded() const { return m_IsEnded; }
        bool IsNull() const { return m_IsNull; }

        operator bool() const { return !m_IsEnded; }

    private:
        DeviceProfilerJsonBuilder& m_Builder;

        bool m_IsEmpty;
        bool m_IsEnded;
        bool m_IsNull;
    };

    /*************************************************************************\

    Function:
        MakeValue

    Description:
        Creates a JSON value.

    \*************************************************************************/
    template<typename T>
    inline void DeviceProfilerJsonValueBuilder::MakeValue( const T& value )
    {
        assert( m_IsEmpty && "JSON value has already been set" );
        m_IsEmpty = false;

        if constexpr( std::is_same_v<T, std::nullptr_t> )
        {
            m_Builder.append_null();
        }
        else if constexpr( std::is_convertible_v<T, std::string_view> )
        {
            m_Builder.escape_and_append_with_quotes(
                std::string_view( value ) );
        }
        else
        {
            m_Builder.append( value );
        }
    }

    /*************************************************************************\

    Function:
        Add

    Description:
        Adds a value to the JSON array.

    \*************************************************************************/
    template<typename T>
    inline DeviceProfilerJsonArrayBuilder& DeviceProfilerJsonArrayBuilder::Add( const T& value )
    {
        if( !m_IsEmpty )
        {
            m_Builder.append_comma();
        }

        m_IsEmpty = false;

        if constexpr( std::is_same_v<T, std::nullptr_t> )
        {
            m_Builder.append_null();
        }
        else if constexpr( std::is_convertible_v<T, std::string_view> )
        {
            m_Builder.escape_and_append_with_quotes(
                std::string_view( value ) );
        }
        else
        {
            m_Builder.append( value );
        }

        return *this;
    }

    /*************************************************************************\

    Function:
        Add

    Description:
        Adds a key-value pair to the JSON object.

    \*************************************************************************/
    template<typename T>
    inline DeviceProfilerJsonObjectBuilder& DeviceProfilerJsonObjectBuilder::Add( std::string_view key, const T& value )
    {
        if( !m_IsEmpty )
        {
            m_Builder.append_comma();
        }

        m_IsEmpty = false;

        m_Builder.append_key_value( key, value );

        return *this;
    }
}
