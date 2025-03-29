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

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        Vector2

    Description:
        Simple wrapper over a pair of values.
        Used to avoid including third-party libraries just to store the data.

    \***********************************************************************************/
    template<typename T>
    struct Vector2
    {
        T x, y;

        inline constexpr Vector2()
            : x( T() )
            , y( T() )
        {
        }

        inline constexpr Vector2( T x, T y )
            : x( x )
            , y( y )
        {
        }

        inline constexpr T& operator[]( int index )
        {
            return ( &x )[index];
        }

        inline constexpr T operator[]( int index ) const
        {
            return ( &x )[index];
        }
    };

    using Int2 = Vector2<int>;
    using Float2 = Vector2<float>;
}

// Constructor and conversion operator for ImVec2
#define IM_VEC2_CLASS_EXTRA                  \
    template<typename T>                     \
    ImVec2( const Profiler::Vector2<T>& v )  \
    {                                        \
        x = (float)v.x;                      \
        y = (float)v.y;                      \
    }                                        \
                                             \
    template<typename T>                     \
    operator Profiler::Vector2<T>() const    \
    {                                        \
        return Profiler::Vector2<T>( x, y ); \
    }
