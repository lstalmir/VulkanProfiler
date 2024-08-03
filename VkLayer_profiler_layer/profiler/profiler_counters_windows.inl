// Copyright (c) 2024 Lukasz Stalmirski
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

#include <Windows.h>
#include <assert.h>

namespace Profiler
{
    /***********************************************************************************\

    Function:
        OSGetPreferredTimeDomain

    Description:
        Returns the preferred time domain on this operating system.

    \***********************************************************************************/
    PROFILER_FORCE_INLINE VkTimeDomainEXT OSGetPreferredTimeDomain( uint32_t, const VkTimeDomainEXT* )
    {
        return VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT;
    }

    /***********************************************************************************\

    Function:
        OSGetDefaultTimeDomain

    Description:
        Returns the default time domain on this operating system.

    \***********************************************************************************/
    PROFILER_FORCE_INLINE VkTimeDomainEXT OSGetDefaultTimeDomain()
    {
        return VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT;
    }

    /***********************************************************************************\

    Function:
        OSGetTimestamp

    Description:
        Returns current CPU timestamp.

    \***********************************************************************************/
    PROFILER_FORCE_INLINE uint64_t OSGetTimestamp( VkTimeDomainEXT timeDomain )
    {
        // Only performance counter time domain is supported on Windows.
        assert( timeDomain == VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT );
        UNREFERENCED_PARAMETER( timeDomain );

        LARGE_INTEGER timestamp;

        // It is very unlikely this function will ever fail.
        // According to MSDN, it always returns a valid timestamp since Windows XP.
        QueryPerformanceCounter( &timestamp );

        return timestamp.QuadPart;
    }

    /***********************************************************************************\

    Function:
        OSGetTimestampFrequency

    Description:
        Returns CPU counter frequency.

    \***********************************************************************************/
    PROFILER_FORCE_INLINE uint64_t OSGetTimestampFrequency( VkTimeDomainEXT timeDomain )
    {
        // Only performance counter time domain is supported on Windows.
        assert( timeDomain == VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT );
        UNREFERENCED_PARAMETER( timeDomain );

        // Timestamp frequency is constant and can be cached.
        static LARGE_INTEGER frequency;

        if( frequency.QuadPart == 0 )
        {
            // It is very unlikely this function will ever fail.
            // According to MSDN, it always returns a valid frequency since Windows XP.
            QueryPerformanceFrequency( &frequency );
        }

        return frequency.QuadPart;
    }
}
