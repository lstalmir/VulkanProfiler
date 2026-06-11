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
#include <simdjson.h>

#include <string>
#include <filesystem>

namespace Profiler::simdjson_helpers
{
    /*************************************************************************\

    Function:
        GetStringLike

    Description:
        Helper function to get a string-like values
        (std::string, std::filesystem::path, etc.) from JSON.

    \*************************************************************************/
    template<typename T>
    inline simdjson::error_code GetStringLike( simdjson::ondemand::value& value, T& out ) noexcept
    {
        std::string_view stringView;

        auto error = value.get_string( stringView );
        if( !error )
        {
            try
            {
                out.assign( stringView );
            }
            catch( ... )
            {
                error = simdjson::error_code::MEMALLOC;
            }
        }

        return error;
    }
}

/*****************************************************************************\

Function:
    get

Description:
    Specialization for std::string.

\*****************************************************************************/
template<>
inline simdjson::error_code simdjson::ondemand::value::get( std::string& out ) noexcept
{
    return Profiler::simdjson_helpers::GetStringLike( *this, out );
}

/*****************************************************************************\

Function:
    get

Description:
    Specialization for std::filesystem::path.

\*****************************************************************************/
template<>
inline simdjson::error_code simdjson::ondemand::value::get( std::filesystem::path& out ) noexcept
{
    return Profiler::simdjson_helpers::GetStringLike( *this, out );
}
