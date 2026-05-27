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

#define FNV32_BASIS 0x811c9dc5
#define FNV32_PRIME 0x1000193

#define FNV64_BASIS 0xcbf29ce484222325
#define FNV64_PRIME 0x100000001b3

template<typename T, T Basis, T Prime>
inline constexpr T FNV_Impl( const char* pData, size_t length )
{
    T hash = Basis;

    for( size_t i = 0; i < length; ++i )
    {
        hash *= Prime;
        hash ^= static_cast<T>( pData[i] );
    }

    return hash;
}

inline constexpr size_t FNV_Strlen( const char* pData )
{
    size_t length = 0;

    while( pData[length] != '\0' )
    {
        ++length;
    }

    return length;
}

inline constexpr uint32_t FNV32( const char* pData, size_t length )
{
    return FNV_Impl<uint32_t, FNV32_BASIS, FNV32_PRIME>( pData, length );
}

inline constexpr uint32_t FNV32( const char* pData )
{
    return FNV32( pData, FNV_Strlen( pData ) );
}

inline constexpr uint32_t FNV32( const std::string_view& str )
{
    return FNV32( str.data(), str.length() );
}

inline constexpr uint64_t FNV64( const char* pData, size_t length )
{
    return FNV_Impl<uint64_t, FNV64_BASIS, FNV64_PRIME>( pData, length );
}

inline constexpr uint64_t FNV64( const char* pData )
{
    return FNV64( pData, FNV_Strlen( pData ) );
}

inline constexpr uint64_t FNV64( const std::string_view& str )
{
    return FNV64( str.data(), str.length() );
}

#if( SIZE_MAX == UINT32_MAX )
#define FNV FNV32
#else
#define FNV FNV64
#endif
