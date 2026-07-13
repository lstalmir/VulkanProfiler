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

#include "profiler_json_builder.h"

namespace Profiler
{
    /*************************************************************************\

    Function:
        DeviceProfilerJsonValueBuilder

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerJsonValueBuilder::DeviceProfilerJsonValueBuilder( DeviceProfilerJsonBuilder& builder )
        : m_Builder( builder )
        , m_IsEmpty( true )
    {
    }

    /*************************************************************************\

    Function:
        ~DeviceProfilerJsonValueBuilder

    Description:
        Destructor.

    \*************************************************************************/
    DeviceProfilerJsonValueBuilder::~DeviceProfilerJsonValueBuilder()
    {
        if( m_IsEmpty )
        {
            m_Builder.append_null();
        }
    }

    /*************************************************************************\

    Function:
        MakeNull

    Description:
        Sets the JSON value to null.

    \*************************************************************************/
    void DeviceProfilerJsonValueBuilder::MakeNull()
    {
        assert( m_IsEmpty && "JSON value has already been set" );
        m_IsEmpty = false;

        m_Builder.append_null();
    }

    /*************************************************************************\

    Function:
        MakeArray

    Description:
        Sets the JSON value to an array and returns a builder object.

    \*************************************************************************/
    DeviceProfilerJsonArrayBuilder DeviceProfilerJsonValueBuilder::MakeArray()
    {
        assert( m_IsEmpty && "JSON value has already been set" );
        m_IsEmpty = false;

        return DeviceProfilerJsonArrayBuilder( m_Builder );
    }

    /*************************************************************************\

    Function:
        MakeObject

    Description:
        Sets the JSON value to an object and returns a builder object.

    \*************************************************************************/
    DeviceProfilerJsonObjectBuilder DeviceProfilerJsonValueBuilder::MakeObject()
    {
        assert( m_IsEmpty && "JSON value has already been set" );
        m_IsEmpty = false;

        return DeviceProfilerJsonObjectBuilder( m_Builder );
    }

    /*************************************************************************\

    Function:
        DeviceProfilerJsonArrayBuilder

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerJsonArrayBuilder::DeviceProfilerJsonArrayBuilder( DeviceProfilerJsonBuilder& builder )
        : m_Builder( builder )
        , m_IsEmpty( true )
        , m_IsEnded( false )
        , m_IsNull( false )
    {
        m_Builder.start_array();
    }

    /*************************************************************************\

    Function:
        DeviceProfilerJsonArrayBuilder

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerJsonArrayBuilder::DeviceProfilerJsonArrayBuilder( DeviceProfilerJsonBuilder& builder, std::nullptr_t )
        : m_Builder( builder )
        , m_IsEmpty( true )
        , m_IsEnded( true )
        , m_IsNull( true )
    {
        m_Builder.append_null();
    }

    /*************************************************************************\

    Function:
        ~DeviceProfilerJsonArrayBuilder

    Description:
        Destructor.

    \*************************************************************************/
    DeviceProfilerJsonArrayBuilder::~DeviceProfilerJsonArrayBuilder()
    {
        if( !m_IsEnded )
        {
            m_Builder.end_array();
        }
    }

    /*************************************************************************\

    Function:
        Add

    Description:
        Adds a value to the JSON array and returns a builder object.

    \*************************************************************************/
    DeviceProfilerJsonValueBuilder DeviceProfilerJsonArrayBuilder::Add()
    {
        if( !m_IsEmpty )
        {
            m_Builder.append_comma();
        }

        m_IsEmpty = false;

        return DeviceProfilerJsonValueBuilder( m_Builder );
    }

    /*************************************************************************\

    Function:
        AddArray

    Description:
        Adds an array to the JSON array and returns a builder object.

    \*************************************************************************/
    DeviceProfilerJsonArrayBuilder DeviceProfilerJsonArrayBuilder::AddArray()
    {
        return AddArrayOrNull( true );
    }

    /*************************************************************************\

    Function:
        AddObject

    Description:
        Adds an object to the JSON array and returns a builder object.

    \*************************************************************************/
    DeviceProfilerJsonObjectBuilder DeviceProfilerJsonArrayBuilder::AddObject()
    {
        return AddObjectOrNull( true );
    }

    /*************************************************************************\

    Function:
        AddArrayOrNull

    Description:
        Adds an array to the JSON array and returns a builder object if the
        condition is true, otherwise adds null.

    \*************************************************************************/
    DeviceProfilerJsonArrayBuilder DeviceProfilerJsonArrayBuilder::AddArrayOrNull( bool condition )
    {
        if( !m_IsEmpty )
        {
            m_Builder.append_comma();
        }

        m_IsEmpty = false;

        if( condition )
        {
            return DeviceProfilerJsonArrayBuilder( m_Builder );
        }
        else
        {
            return DeviceProfilerJsonArrayBuilder( m_Builder, nullptr );
        }
    }

    /*************************************************************************\

    Function:
        AddObjectOrNull

    Description:
        Adds an object to the JSON array and returns a builder object if the
        condition is true, otherwise adds null.

    \*************************************************************************/
    DeviceProfilerJsonObjectBuilder DeviceProfilerJsonArrayBuilder::AddObjectOrNull( bool condition )
    {
        if( !m_IsEmpty )
        {
            m_Builder.append_comma();
        }

        m_IsEmpty = false;

        if( condition )
        {
            return DeviceProfilerJsonObjectBuilder( m_Builder );
        }
        else
        {
            return DeviceProfilerJsonObjectBuilder( m_Builder, nullptr );
        }
    }

    /*************************************************************************\

    Function:
        End

    Description:
        Ends the JSON array.

    \*************************************************************************/
    void DeviceProfilerJsonArrayBuilder::End()
    {
        if( !m_IsEnded )
        {
            m_Builder.end_array();
            m_IsEnded = true;
        }
    }

    /*************************************************************************\

    Function:
        DeviceProfilerJsonObjectBuilder

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerJsonObjectBuilder::DeviceProfilerJsonObjectBuilder( DeviceProfilerJsonBuilder& builder )
        : m_Builder( builder )
        , m_IsEmpty( true )
        , m_IsEnded( false )
        , m_IsNull( false )
    {
        m_Builder.start_object();
    }

    /*************************************************************************\

    Function:
        DeviceProfilerJsonObjectBuilder

    Description:
        Constructor.

    \*************************************************************************/
    DeviceProfilerJsonObjectBuilder::DeviceProfilerJsonObjectBuilder( DeviceProfilerJsonBuilder& builder, std::nullptr_t )
        : m_Builder( builder )
        , m_IsEmpty( true )
        , m_IsEnded( true )
        , m_IsNull( true )
    {
        m_Builder.append_null();
    }

    /*************************************************************************\

    Function:
        ~DeviceProfilerJsonObjectBuilder

    Description:
        Destructor.

    \*************************************************************************/
    DeviceProfilerJsonObjectBuilder::~DeviceProfilerJsonObjectBuilder()
    {
        if( !m_IsEnded )
        {
            m_Builder.end_object();
        }
    }

    /*************************************************************************\

    Function:
        AddValue

    Description:
        Adds a value to the JSON object and returns a builder object.

    \*************************************************************************/
    DeviceProfilerJsonValueBuilder DeviceProfilerJsonObjectBuilder::Add( std::string_view key )
    {
        if( !m_IsEmpty )
        {
            m_Builder.append_comma();
        }

        m_IsEmpty = false;

        m_Builder.escape_and_append_with_quotes( key );
        m_Builder.append_colon();

        return DeviceProfilerJsonValueBuilder( m_Builder );
    }

    /*************************************************************************\

    Function:
        AddArray

    Description:
        Adds an array to the JSON object and returns a builder object.

    \*************************************************************************/
    DeviceProfilerJsonArrayBuilder DeviceProfilerJsonObjectBuilder::AddArray( std::string_view key )
    {
        return AddArrayOrNull( key, true );
    }

    /*************************************************************************\

    Function:
        AddObject

    Description:
        Adds an object to the JSON object and returns a builder object.

    \*************************************************************************/
    DeviceProfilerJsonObjectBuilder DeviceProfilerJsonObjectBuilder::AddObject( std::string_view key )
    {
        return AddObjectOrNull( key, true );
    }

    /*************************************************************************\

    Function:
        AddArrayOrNull

    Description:
        Adds an array to the JSON object and returns a builder object if the
        condition is true, otherwise adds null.

    \*************************************************************************/
    DeviceProfilerJsonArrayBuilder DeviceProfilerJsonObjectBuilder::AddArrayOrNull( std::string_view key, bool condition )
    {
        if( !m_IsEmpty )
        {
            m_Builder.append_comma();
        }

        m_IsEmpty = false;

        m_Builder.escape_and_append_with_quotes( key );
        m_Builder.append_colon();

        if( condition )
        {
            return DeviceProfilerJsonArrayBuilder( m_Builder );
        }
        else
        {
            return DeviceProfilerJsonArrayBuilder( m_Builder, nullptr );
        }
    }

    /*************************************************************************\

    Function:
        AddObjectOrNull

    Description:
        Adds an object to the JSON object and returns a builder object if the
        condition is true, otherwise adds null.

    \*************************************************************************/
    DeviceProfilerJsonObjectBuilder DeviceProfilerJsonObjectBuilder::AddObjectOrNull( std::string_view key, bool condition )
    {
        if( !m_IsEmpty )
        {
            m_Builder.append_comma();
        }

        m_IsEmpty = false;

        m_Builder.escape_and_append_with_quotes( key );
        m_Builder.append_colon();

        if( condition )
        {
            return DeviceProfilerJsonObjectBuilder( m_Builder );
        }
        else
        {
            return DeviceProfilerJsonObjectBuilder( m_Builder, nullptr );
        }
    }

    /*************************************************************************\

    Function:
        End

    Description:
        Ends the JSON object.

    \*************************************************************************/
    void DeviceProfilerJsonObjectBuilder::End()
    {
        if( !m_IsEnded )
        {
            m_Builder.end_object();
            m_IsEnded = true;
        }
    }
}
