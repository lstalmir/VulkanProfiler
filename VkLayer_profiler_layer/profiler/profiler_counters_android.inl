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

#include <time.h>
#include <assert.h>

namespace Profiler
{
    /***********************************************************************************\

    Function:
        OSGetPreferredTimeDomain

    Description:
        Returns the preferred time domain on this operating system.

    \***********************************************************************************/
    PROFILER_FORCE_INLINE VkTimeDomainEXT OSGetPreferredTimeDomain( uint32_t timeDomainCount, const VkTimeDomainEXT* pTimeDomains )
    {
        if( timeDomainCount > 0 )
        {
            // Check if raw monotonic clock is supported.
            struct timespec tp;
            if( clock_gettime( CLOCK_MONOTONIC_RAW, &tp ) == 0 )
            {
                for( uint32_t i = 0; i < timeDomainCount; ++i )
                {
                    if( pTimeDomains[ i ] == VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT )
                    {
                        return VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT;
                    }
                }
            }
        }

        return VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT;
    }

    /***********************************************************************************\

    Function:
        OSGetDefaultTimeDomain

    Description:
        Returns the default time domain on this operating system.

    \***********************************************************************************/
    PROFILER_FORCE_INLINE VkTimeDomainEXT OSGetDefaultTimeDomain()
    {
        return OSGetPreferredTimeDomain( 0, nullptr );
    }

    /***********************************************************************************\

    Function:
        OSGetTimestamp

    Description:
        Returns current CPU timestamp.

    \***********************************************************************************/
    PROFILER_FORCE_INLINE uint64_t OSGetTimestamp( VkTimeDomainEXT timeDomain )
    {
        assert( (timeDomain == VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT) ||
            (timeDomain == VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT) );

        clockid_t clkid = (timeDomain == VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT)
            ? CLOCK_MONOTONIC
            : CLOCK_MONOTONIC_RAW;

        struct timespec tp;
        if( clock_gettime( clkid, &tp ) == 0 )
        {
            // Convert tp to nanoseconds.
            return (static_cast<uint64_t>(tp.tv_sec) * 1'000'000'000) + tp.tv_nsec;
        }

        return 0;
    }

    /***********************************************************************************\

    Function:
        OSGetTimestampFrequency

    Description:
        Returns CPU counter frequency.

    \***********************************************************************************/
    PROFILER_FORCE_INLINE uint64_t OSGetTimestampFrequency( VkTimeDomainEXT timeDomain )
    {
        // All timestamps are already premultiplied to nanoseconds, so the frequency is 1GHz.
        return 1'000'000'000;
    }
}
