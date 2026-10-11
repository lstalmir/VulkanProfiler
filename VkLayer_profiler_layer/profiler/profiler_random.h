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
#include <mutex>
#include <random>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        DeviceProfilerRandom

    Description:
        Thread-safe random number generator for the profiler.

    \***********************************************************************************/
    class DeviceProfilerRandom
    {
    public:
        explicit DeviceProfilerRandom( uint64_t seed = std::mt19937_64::default_seed )
            : m_pEngine( std::make_unique<std::mt19937_64>( seed ) )
        {
        }

        template<typename T = uint64_t>
        inline std::enable_if_t<std::is_integral_v<T>, T> GetInteger()
        {
            std::scoped_lock lock( m_Mutex );
            std::uniform_int_distribution<T> distribution( std::numeric_limits<T>::min(), std::numeric_limits<T>::max() );
            return distribution( *m_pEngine );
        }

        template<typename T = uint64_t>
        inline std::enable_if_t<std::is_integral_v<T>, T> GetInteger( T max )
        {
            std::scoped_lock lock( m_Mutex );
            std::uniform_int_distribution<T> distribution( std::numeric_limits<T>::min(), max );
            return distribution( *m_pEngine );
        }

        template<typename T = uint64_t>
        inline std::enable_if_t<std::is_integral_v<T>, T> GetInteger( T min, T max )
        {
            std::scoped_lock lock( m_Mutex );
            std::uniform_int_distribution<T> distribution( min, max );
            return distribution( *m_pEngine );
        }

        template<typename T = float>
        inline std::enable_if_t<std::is_floating_point_v<T>, T> GetFloat()
        {
            std::scoped_lock lock( m_Mutex );
            std::uniform_real_distribution<T> distribution( 0.0, 1.0 );
            return distribution( *m_pEngine );
        }

        template<typename T = float>
        inline std::enable_if_t<std::is_floating_point_v<T>, T> GetFloat( T max )
        {
            std::scoped_lock lock( m_Mutex );
            std::uniform_real_distribution<T> distribution( 0.0, max );
            return distribution( *m_pEngine );
        }

        template<typename T = float>
        inline std::enable_if_t<std::is_floating_point_v<T>, T> GetFloat( T min, T max )
        {
            std::scoped_lock lock( m_Mutex );
            std::uniform_real_distribution<T> distribution( min, max );
            return distribution( *m_pEngine );
        }

    private:
        std::mutex m_Mutex;
        std::unique_ptr<std::mt19937_64> m_pEngine;
    };
}
