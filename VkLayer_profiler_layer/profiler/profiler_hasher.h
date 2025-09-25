// Copyright (c) 2025 Lukasz Stalmirski
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
#include <stdint.h>
#include <string>
#include <vector>
#include <type_traits>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        Hasher

    Description:
        Helper class that allows to compute hash of given data.

    \***********************************************************************************/
    class HashInput
    {
        template<typename T>
        using Iterable =
            std::void_t<
                std::enable_if_t<std::is_same_v<
                    decltype( std::begin( std::declval<const T&>() ) ),
                    decltype( std::end( std::declval<const T&>() ) )>>,
                decltype( *std::begin( std::declval<const T&>() ) )>;

        template<typename T, typename = void>
        struct IsIterable : std::false_type
        {
        };

        template<typename T>
        struct IsIterable<T, Iterable<T>> : std::true_type
        {
        };

    public:
        void Reset()
        {
            m_Input.clear();
        }

        void Add( const void* pData, size_t dataSize )
        {
            const char* pCharData = reinterpret_cast<const char*>( pData );
            m_Input.insert( m_Input.end(), pCharData, pCharData + dataSize );
        }

        void Add( const std::string& str )
        {
            Add( str.c_str(), str.length() );
        }

        template<typename T>
        std::enable_if_t<IsIterable<T>::value> Add( const T& iterable, bool sort = false )
        {
            Add( std::begin( iterable ), std::end( iterable ), sort );
        }

        template<typename T>
        std::enable_if_t<!IsIterable<T>::value> Add( const T& value )
        {
            Add( &value, sizeof( T ) );
        }

        template<typename IterT>
        void Add( const IterT& begin, const IterT& end, bool sort = false )
        {
            using ValueT = typename std::iterator_traits<IterT>::value_type;

            if( sort )
            {
                std::vector<ValueT> sorted( begin, end );
                std::sort( sorted.begin(), sorted.end() );
                Add( sorted.data(), sorted.size() * sizeof( ValueT ) );
            }
            else
            {
                m_Input.reserve( m_Input.size() + std::distance( begin, end ) * sizeof( ValueT ) );

                for( IterT it = begin; it != end; ++it )
                {
                    Add( *it );
                }
            }
        }

        const char* GetData() const
        {
            return m_Input.data();
        }

        size_t GetSize() const
        {
            return m_Input.size();
        }

    private:
        std::vector<char> m_Input;
    };
}
