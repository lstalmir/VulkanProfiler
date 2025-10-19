// Copyright (c) 2019-2025 Lukasz Stalmirski
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

#define VK_NO_PROTOTYPES
#include "profiler_layer_functions/core/VkInstance_functions.h"
#include "profiler_layer_functions/core/VkDevice_functions.h"
#include <vulkan/vk_layer.h>

// clang-format off
#undef VK_LAYER_EXPORT
#if (defined(__GNUC__) && (__GNUC__ >= 4)) || \
    (defined(__clang__)) || \
    (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))
#  define VK_LAYER_EXPORT extern "C" __attribute__((visibility("default")))
#else
// MSVC compiler uses def file for export definitions.
#  define VK_LAYER_EXPORT extern "C"
#endif
// clang-format on

/***************************************************************************************\

Function:
    vkGetInstanceProcAddr

Description:
    Entrypoint to the VkInstance

\***************************************************************************************/
VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(
    VkInstance instance,
    const char* name )
{
    return Profiler::VkInstance_Functions::GetInstanceProcAddr( instance, name );
}

/***************************************************************************************\

Function:
    vkGetDeviceProcAddr

Description:
    Entrypoint to the VkDevice

\***************************************************************************************/
VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(
    VkDevice device,
    const char* name )
{
    return Profiler::VkDevice_Functions::GetDeviceProcAddr( device, name );
}

/***************************************************************************************\

Function:
    vkEnumerateInstanceLayerProperties

Description:
    Entrypoint to the vkEnumerateInstanceLayerProperties.
    Loader on Android requires this function to be exported directly from the SO.

\***************************************************************************************/
VK_LAYER_EXPORT VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(
    uint32_t* pPropertyCount,
    VkLayerProperties* pProperties )
{
    return Profiler::VkInstance_Functions::EnumerateInstanceLayerProperties( pPropertyCount, pProperties );
}

/***************************************************************************************\

Function:
    vkEnumerateInstanceExtensionProperties

Description:
    Entrypoint to the vkEnumerateInstanceExtensionProperties.
    Loader on Android requires this function to be exported directly from the SO.

\***************************************************************************************/
VK_LAYER_EXPORT VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(
    const char* pLayerName,
    uint32_t* pPropertyCount,
    VkExtensionProperties* pProperties )
{
    return Profiler::VkInstance_Functions::EnumerateInstanceExtensionProperties( pLayerName, pPropertyCount, pProperties );
}

#ifdef WIN32
/***************************************************************************************\

Function:
    DllMain

Description:
    Called when the DLL is (un)loaded.

\***************************************************************************************/
BOOL APIENTRY DllMain(
    HINSTANCE hDllInstance,
    DWORD dwReason,
    LPVOID lpReserved )
{
    if( dwReason == DLL_PROCESS_ATTACH )
    {
        // Save the profiler's DLL instance handle for window message hooking.
        Profiler::ProfilerPlatformFunctions::SetLibraryInstanceHandle( hDllInstance );
    }

    return TRUE;
}
#endif
