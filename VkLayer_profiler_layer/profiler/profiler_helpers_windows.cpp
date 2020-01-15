#ifdef WIN32
#include "profiler_helpers.h"

#include <Windows.h>

namespace Profiler
{
    /***********************************************************************************\

    Function:
        GetApplicationDir_Windows

    Description:
        Returns directory in which current exe is located.

    \***********************************************************************************/
    std::filesystem::path ProfilerPlatformFunctions::GetApplicationPath()
    {
        static std::filesystem::path applicationPath;

        if( applicationPath.empty() )
        {
            // Grab handle to the application module
            const HMODULE hCurrentModule = GetModuleHandleA( nullptr );

            std::string buffer;

            DWORD lastError = ERROR_INSUFFICIENT_BUFFER;
            while( lastError == ERROR_INSUFFICIENT_BUFFER )
            {
                // Increase size of the buffer a bit
                buffer.resize( buffer.size() + MAX_PATH );

                GetModuleFileNameA( hCurrentModule, buffer.data(), buffer.size() );

                // Update last error value
                lastError = GetLastError();
            }

            if( lastError == ERROR_SUCCESS )
            {
                applicationPath = buffer;
            }
            else
            {
                // Failed to get exe path
                applicationPath = "";
            }
        }

        return applicationPath;
    }
}

#endif // WIN32
