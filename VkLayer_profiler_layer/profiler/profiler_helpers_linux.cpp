#ifdef __linux__
#include "profiler_helpers.h"

#include <assert.h>

#include <unistd.h>
#include <limits.h>

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
}

#endif // __linux__
