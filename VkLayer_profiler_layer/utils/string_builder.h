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
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        FlagsStringBuilder

    Description:
        Helper class that allows to concatenate flag names into a single string.

    \***********************************************************************************/
    struct FlagsStringBuilder
    {
        static constexpr const char* DefaultFlagsSeparator = " | ";

        std::stringstream m_Stream;
        const char* m_pSeparator;
        bool m_Empty;

        FlagsStringBuilder( const char* pSeparator = DefaultFlagsSeparator )
            : m_pSeparator( pSeparator )
            , m_Empty( true )
        {
        }

        void AddFlag( const std::string_view& flag )
        {
            if( !m_Empty )
            {
                m_Stream << m_pSeparator;
            }

            m_Stream << flag;
            m_Empty = false;
        }

        void AddUnknownFlags( uint64_t flags, uint32_t startIndex )
        {
            for( uint32_t i = startIndex; i < sizeof( uint64_t ) * 8; ++i )
            {
                const uint64_t unkownFlag = ( 1ULL << i );
                if( flags & unkownFlag )
                {
                    if( !m_Empty )
                    {
                        m_Stream << m_pSeparator;
                    }

                    m_Stream << "Unknown flag (" << unkownFlag << ")";
                    m_Empty = false;
                }
            }
        }

        std::string BuildString() const
        {
            return m_Stream.str();
        }
    };
}