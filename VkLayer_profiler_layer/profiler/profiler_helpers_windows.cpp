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

#ifdef WIN32
#include "profiler_helpers.h"
#include "../profiler_layer_objects/VkDevice_object.h"

#if !defined _DEBUG && !defined NDEBUG
#define NDEBUG // for assert.h
#endif

#include <Windows.h>
#include <assert.h>

#include <d3d12.h>
#include <dxgi1_6.h>

namespace Profiler
{
    struct StablePowerState_T
    {
        HMODULE       m_hDXGIModule;
        HMODULE       m_hD3D12Module;
        ID3D12Device* m_pD3D12Device;
    };

    static HINSTANCE g_hProfilerDllInstance;

    /***********************************************************************************\

    Function:
        GetApplicationPath

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

                GetModuleFileNameA( hCurrentModule, buffer.data(), static_cast<uint32_t>(buffer.size()) );

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

    /***********************************************************************************\

    Function:
        GetLayerPath

    Description:
        Returns full path to the profiler layer shared object file.

    \***********************************************************************************/
    std::filesystem::path ProfilerPlatformFunctions::GetLayerPath()
    {
        static std::filesystem::path layerPath;

        // TODO

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
        // Read registry key
        DWORD enablePreemptionValue = 1;
        DWORD enablePreemptionValueSize = sizeof( DWORD );

        // Currently the only way to disable GPU DMA packet preemption is to set
        // HKLM\SYSTEM\CurrentControlSet\Control\GraphicsDriver\Scheduler\EnablePreemption
        // DWORD value to 0.
        RegGetValueA(
            HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\GraphicsDriver\\Scheduler",
            "EnablePreemption",
            RRF_RT_REG_DWORD,
            nullptr,
            &enablePreemptionValue,
            &enablePreemptionValueSize );

        return static_cast<bool>(enablePreemptionValue);
    }

    /***********************************************************************************\

    Function:
        SetStablePowerState

    Description:
        Forces GPU to run at constant frequency for more reliable measurements.
        Not all systems support this feature.

    \***********************************************************************************/
    bool ProfilerPlatformFunctions::SetStablePowerState( VkDevice_Object* pDevice, void** ppStateHandle )
    {
        HRESULT hr = S_OK;

        typedef decltype(CreateDXGIFactory)* PFN_CREATE_DXGI_FACTORY;

        PFN_CREATE_DXGI_FACTORY pfnCreateDXGIFactory;
        PFN_D3D12_CREATE_DEVICE pfnD3D12CreateDevice;

        // Allocate a structure for the stable power state state.
        StablePowerState_T* pState = static_cast<StablePowerState_T*>( malloc( sizeof( StablePowerState_T ) ) );
        if( pState == nullptr )
        {
            // Failed to allocate stable power state struct.
            *ppStateHandle = nullptr;
            return false;
        }

        memset( pState, 0, sizeof( StablePowerState_T ) );

        // Load required modules.
        pState->m_hDXGIModule = LoadLibraryA( "dxgi.dll" );
        if( pState->m_hDXGIModule == nullptr )
            hr = E_FAIL;

        pState->m_hD3D12Module = LoadLibraryA( "d3d12.dll" );
        if( pState->m_hD3D12Module == nullptr )
            hr = E_FAIL;

        // Get required entry points.
        if( SUCCEEDED( hr ) )
        {
            pfnCreateDXGIFactory = reinterpret_cast<PFN_CREATE_DXGI_FACTORY>( GetProcAddress( pState->m_hDXGIModule, "CreateDXGIFactory" ) );
            if( pfnCreateDXGIFactory == nullptr )
                hr = E_FAIL;
        }
        if( SUCCEEDED( hr ) )
        {
            pfnD3D12CreateDevice = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>( GetProcAddress( pState->m_hD3D12Module, "D3D12CreateDevice" ) );
            if( pfnD3D12CreateDevice == nullptr )
                hr = E_FAIL;
        }

        // Create DXGI factory to enumerate DirectX adapters.
        IDXGIFactory* pDXGIFactory = nullptr;
        if( SUCCEEDED( hr ) )
        {
            hr = pfnCreateDXGIFactory( IID_PPV_ARGS( &pDXGIFactory ) );
        }

        const VkPhysicalDeviceProperties& physicalDeviceProperties = pDevice->pPhysicalDevice->Properties;

        // Find the DXGI adapter for the selected physical device.
        IDXGIAdapter* pDeviceDXGIAdapter = nullptr;
        if( SUCCEEDED( hr ) )
        {
            IDXGIAdapter* pDXGIAdapter = nullptr;
            for(UINT adapterIndex = 0;
                pDXGIFactory->EnumAdapters( adapterIndex, &pDXGIAdapter ) == S_OK;
                ++adapterIndex )
            {
                DXGI_ADAPTER_DESC adapterDesc;
                pDXGIAdapter->GetDesc( &adapterDesc );

                // TODO: Handle multi-gpu systems with the same gpus.
                if( (adapterDesc.VendorId == physicalDeviceProperties.vendorID) &&
                    (adapterDesc.DeviceId == physicalDeviceProperties.deviceID) )
                {
                    pDeviceDXGIAdapter = pDXGIAdapter;
                    break;
                }

                pDXGIAdapter->Release();
            }

            if( pDeviceDXGIAdapter == nullptr )
                hr = E_FAIL;

            pDXGIFactory->Release();
        }

        // Create a D3D12 device on the selected physical device.
        if( SUCCEEDED( hr ) )
        {
            hr = pfnD3D12CreateDevice( pDeviceDXGIAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS( &pState->m_pD3D12Device ) );
            pDeviceDXGIAdapter->Release();
        }

        if( SUCCEEDED( hr ) )
        {
            hr = pState->m_pD3D12Device->SetStablePowerState( true );
        }

        // Free the state if anything went wrong.
        if( FAILED( hr ) )
        {
            // Failed to set stable power state.
            if( pState->m_pD3D12Device != nullptr )
            {
                pState->m_pD3D12Device->Release();
            }

            if( pState->m_hD3D12Module != nullptr )
            {
                FreeLibrary( pState->m_hD3D12Module );
            }

            if( pState->m_hDXGIModule != nullptr )
            {
                FreeLibrary( pState->m_hDXGIModule );
            }

            free( pState );

            pState = nullptr;
        }

        *ppStateHandle = pState;

        return SUCCEEDED( hr );
    }

    /***********************************************************************************\

    Function:
        ResetStablePowerState

    Description:
        Restores the default (dynamic) GPU frequency.

    \***********************************************************************************/
    void ProfilerPlatformFunctions::ResetStablePowerState( void* pStateHandle )
    {
        StablePowerState_T* pState = static_cast<StablePowerState_T*>( pStateHandle );

        if( pState != nullptr )
        {
            pState->m_pD3D12Device->SetStablePowerState( false );
            pState->m_pD3D12Device->Release();

            FreeLibrary( pState->m_hD3D12Module );
            FreeLibrary( pState->m_hDXGIModule );

            free( pState );
        }
    }

    /***********************************************************************************\

    Function:
        SetLibraryInstanceHandle

    Description:
        Saves HINSTANCE handle to the loaded layer's DLL.
        The handle is used by win32 imgui implementation to hook on incoming messages.

    \***********************************************************************************/
    void ProfilerPlatformFunctions::SetLibraryInstanceHandle( void* hLibraryInstance )
    {
        static_assert( sizeof( HINSTANCE ) == sizeof( void* ),
            "Sizeof HINSTANCE must be equal to size of opaque void* for the function "
            "ProfilerPlatformFunctions::SetLibraryInstanceHandle to work correctly." );

        g_hProfilerDllInstance = static_cast<HINSTANCE>(hLibraryInstance);
    }

    /***********************************************************************************\

    Function:
        GetLibraryInstanceHandle

    Description:
        Returns the saved HINSTANCE handle to the loaded layer's DLL.

    \***********************************************************************************/
    void* ProfilerPlatformFunctions::GetLibraryInstanceHandle()
    {
        return g_hProfilerDllInstance;
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

        OutputDebugStringA( str );
    }

    /***********************************************************************************\

    Function:
        GetCurrentThreadId

    Description:
        Get unique identifier of the currently running thread.

    \***********************************************************************************/
    uint32_t ProfilerPlatformFunctions::GetCurrentThreadId()
    {
        return ::GetCurrentThreadId();
    }

    /***********************************************************************************\

    Function:
        GetCurrentProcessId

    Description:
        Get unique identifier of the currently running process.

    \***********************************************************************************/
    uint32_t ProfilerPlatformFunctions::GetCurrentProcessId()
    {
        return ::GetCurrentProcessId();
    }

    /***********************************************************************************\

    Function:
        GetLocalTime

    Description:
        Returns local time.

    \***********************************************************************************/
    void ProfilerPlatformFunctions::GetLocalTime( tm* localTime, const time_t& time )
    {
        ::localtime_s( localTime, &time );
    }

    /***********************************************************************************\

    Function:
        GetEnvironmentVar

    Description:
        Get environment variable.

    \***********************************************************************************/
    std::optional<std::string> ProfilerPlatformFunctions::GetEnvironmentVar( const char* pVariableName )
    {
        DWORD variableLength = GetEnvironmentVariableA( pVariableName, nullptr, 0 );

        if( variableLength == 0 )
        {
            assert( GetLastError() == ERROR_ENVVAR_NOT_FOUND );
            return std::nullopt;
        }

        std::string variable( variableLength + 1, '\0' );
        GetEnvironmentVariableA( pVariableName, variable.data(), variableLength + 1 );

        return variable;
    }

}

#endif // WIN32
