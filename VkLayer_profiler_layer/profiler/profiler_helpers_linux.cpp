// Copyright (c) 2020 Lukasz Stalmirski
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

#ifdef __linux__
#include "profiler_helpers.h"

#include <assert.h>

#include <unistd.h>
#include <limits.h>
#include <sys/types.h>

namespace Profiler
{
    /***********************************************************************************\

    Function:
        GetApplicationPath

    Description:
        Returns directory in which current process is located.

    \***********************************************************************************/
    std::filesystem::path ProfilerPlatformFunctions::GetApplicationPath()
    {
        static std::filesystem::path applicationPath;

        if( applicationPath.empty() )
        {
            char path[ PATH_MAX ];

            // Read /proc/self/exe symlink
            ssize_t result = readlink( "/proc/self/exe", path, std::extent_v<decltype(path)> );
            assert( result > 0 );

            applicationPath = path;
        }

        return applicationPath;
    }

    /***********************************************************************************\

    Function:
        IsPreemptionEnabled

    Description:
        Checks if the scheduler allows preemption of DMA packets sent to the GPU.

    \***********************************************************************************/
    bool ProfilerPlatformFunctions::IsPreemptionEnabled()
    {
        return false;
    }

    /***********************************************************************************\

    Function:
        WriteDebugUnformatted

    Description:
        Write string to debug output.

    \***********************************************************************************/
    void ProfilerPlatformFunctions::WriteDebugUnformatted( const char* str )
    {
        [[maybe_unused]]
        const size_t messageLength = std::strlen( str );
        // Output strings must end with newline
        assert( str[ messageLength - 1 ] == '\n' );

        // Use standard output
        printf( "%s", str );
    }

    /***********************************************************************************\

    Function:
        GetCurrentThreadId

    Description:
        Get unique identifier of the currently running thread.

    \***********************************************************************************/
    uint32_t ProfilerPlatformFunctions::GetCurrentThreadId()
    {
        return static_cast<uint32_t>(gettid());
    }

    /***********************************************************************************\

    Function:
        GetCurrentProcessId

    Description:
        Get unique identifier of the currently running process.

    \***********************************************************************************/
    uint32_t ProfilerPlatformFunctions::GetCurrentProcessId()
    {
        return static_cast<uint32_t>(getpid());
    }

}

#endif // __linux__
