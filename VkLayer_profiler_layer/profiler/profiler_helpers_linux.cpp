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
        FindFile

    Description:
        Find file in directory.

    \***********************************************************************************/
    std::filesystem::path ProfilerPlatformFunctions::FindFile(
        const std::filesystem::path& directory,
        const std::filesystem::path& filename,
        const bool recurse )
    {
        if( std::filesystem::exists( directory ) )
        {
            // Enumerate all files in the directory
            for( auto directoryEntry : std::filesystem::directory_iterator( directory ) )
            {
                if( directoryEntry.path().filename() == filename )
                {
                    return directoryEntry;
                }

                // Check in subdirectories
                if( directoryEntry.is_directory() && recurse )
                {
                    std::filesystem::path result = FindFile( directoryEntry, filename, recurse );

                    if( !result.empty() )
                    {
                        return result;
                    }
                }
            }
        }

        return "";
    }

}

#endif // __linux__
