// Copyright (c) 2019-2021 Lukasz Stalmirski
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

#include <dlfcn.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>

namespace Profiler
{
    /***********************************************************************************\

    Function:
        GetApplicationPath

    Description:
        Returns full path to the current application executable file.

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
        GetLayerPath

    Description:
        Returns full path to the profiler layer shared object file.

    \***********************************************************************************/
    std::filesystem::path ProfilerPlatformFunctions::GetLayerPath()
    {
        static std::filesystem::path layerPath;

        if( layerPath.empty() )
        {
            Dl_info info;

            if( dladdr( (const void*)&ProfilerPlatformFunctions::GetLayerPath, &info ) )
            {
                layerPath = info.dli_fname;
            }
        }

        return layerPath;
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
        SetStablePowerState

    Description:
        Forces GPU to run at constant frequency for more reliable measurements.
        Not all systems support this feature.

    \***********************************************************************************/
    bool ProfilerPlatformFunctions::SetStablePowerState( VkDevice_Object*, void** )
    {
        return false;
    }

    /***********************************************************************************\

    Function:
        ResetStablePowerState

    Description:
        Restores the default (dynamic) GPU frequency.

    \***********************************************************************************/
    void ProfilerPlatformFunctions::ResetStablePowerState( void* )
    {
    }

    /***********************************************************************************\

    Function:
        SetLibraryInstanceHandle

    Description:
        No-op.

    \***********************************************************************************/
    void ProfilerPlatformFunctions::SetLibraryInstanceHandle( void* )
    {
    }

    /***********************************************************************************\

    Function:
        GetLibraryInstanceHandle

    Description:
        No-op.

    \***********************************************************************************/
    void* ProfilerPlatformFunctions::GetLibraryInstanceHandle()
    {
        return nullptr;
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
    
    /***********************************************************************************\

    Function:
        GetLocalTime

    Description:
        Returns local time.

    \***********************************************************************************/
    void ProfilerPlatformFunctions::GetLocalTime( tm* localTime, const time_t& time )
    {
        *localTime = *localtime( &time );
    }

    /***********************************************************************************\

    Function:
        GetEnvironmentVar

    Description:
        Get environment variable.

    \***********************************************************************************/
    std::optional<std::string> ProfilerPlatformFunctions::GetEnvironmentVar( const char* pVariableName )
    {
        const char* pVariable = std::getenv( pVariableName );

        if( !pVariable )
        {
            return std::nullopt;
        }

        return std::string( pVariable );
    }

    /***********************************************************************************\

    Function:
        OpenLibrary

    Description:
        Opens a dynamic library.

    \***********************************************************************************/
    void* ProfilerPlatformFunctions::OpenLibrary( const char* pLibraryName )
    {
        return dlopen( pLibraryName, RTLD_NOW | RTLD_GLOBAL );
    }

    /***********************************************************************************\

    Function:
        CloseLibrary

    Description:
        Closes the dynamic library.

    \***********************************************************************************/
    void ProfilerPlatformFunctions::CloseLibrary( void* pLibrary )
    {
        dlclose( pLibrary );
    }

    /***********************************************************************************\

    Function:
        GetProcAddress

    Description:
        Returns address of the specified function in the dynamic library.

    \***********************************************************************************/
    VoidFunction ProfilerPlatformFunctions::GetProcAddress( void* pLibrary, const char* pProcName )
    {
        return reinterpret_cast<VoidFunction>( dlsym( pLibrary, pProcName ) );
    }
}

#endif // __linux__
