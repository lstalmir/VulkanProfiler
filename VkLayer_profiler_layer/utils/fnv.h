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
#include <string_view>

#define FNV_64_OFFSET_BASIS 0xcbf29ce484222325ull
#define FNV_64_PRIME 0x100000001b3ull

#define FNV_32_OFFSET_BASIS 0x811c9dc5ul
#define FNV_32_PRIME 0x1000193ul

inline constexpr uint64_t FNV( const char* str, size_t length )
{
    if( !str || length == 0 )
    {
        return 0;
    }

    uint64_t hash = FNV_64_OFFSET_BASIS;
    for( size_t i = 0; i < length; ++i )
    {
        hash ^= static_cast<uint64_t>( str[i] );
        hash *= FNV_64_PRIME;
    }
    return hash;
}

inline constexpr uint64_t FNV( const char* str )
{
    if( !str || !*str )
    {
        return 0;
    }

    uint64_t hash = FNV_64_OFFSET_BASIS;
    while( *str )
    {
        hash ^= static_cast<uint64_t>( *str );
        hash *= FNV_64_PRIME;
        ++str;
    }
    return hash;
}

inline constexpr uint64_t FNV( const std::string_view& str )
{
    return FNV( str.data(), str.length() );
}
