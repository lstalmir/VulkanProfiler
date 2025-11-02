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

static VkResult CheckResult( VkResult result )
{
    // Exported interface functions must never fail.
    // All error codes are negative.
    assert( result >= VK_SUCCESS );
    return result;
}

/***************************************************************************************\

Function:
    vkGetInstanceProcAddr

Description:
    Entrypoint to the VkInstance.
    Required by layer interface version 0 and 1.

\***************************************************************************************/
VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(
    VkInstance instance,
    const char* name )
{
    return Profiler::VkInstance_Functions::GetInstanceProcAddr( instance, name );
}

/***************************************************************************************\

Function:
    vkGetDeviceProcAddr

Description:
    Entrypoint to the VkDevice.
    Required by layer interface version 0 and 1.

\***************************************************************************************/
VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(
    VkDevice device,
    const char* name )
{
    return Profiler::VkDevice_Functions::GetDeviceProcAddr( device, name );
}

/***************************************************************************************\

Function:
    vkEnumerateInstanceLayerProperties

Description:
    Entrypoint to the EnumerateInstanceLayerProperties.
    Required by layer interface version 0.

\***************************************************************************************/
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(
    uint32_t* pPropertyCount,
    VkLayerProperties* pProperties )
{
    return CheckResult( Profiler::VkInstance_Functions::EnumerateInstanceLayerProperties(
        pPropertyCount,
        pProperties ) );
}

/***************************************************************************************\

Function:
    vkEnumerateInstanceExtensionProperties

Description:
    Entrypoint to the EnumerateInstanceExtensionProperties.
    Required by layer interface version 0.

\***************************************************************************************/
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(
    const char* pLayerName,
    uint32_t* pPropertyCount,
    VkExtensionProperties* pProperties )
{
    assert( pLayerName && ( strcmp( pLayerName, VK_LAYER_profiler_name ) == 0 ) );
    return CheckResult( Profiler::VkInstance_Functions::EnumerateInstanceExtensionProperties(
        pLayerName,
        pPropertyCount,
        pProperties ) );
}

/***************************************************************************************\

Function:
    vkEnumerateDeviceLayerProperties

Description:
    Entrypoint to the EnumerateDeviceLayerProperties.
    Required by layer interface version 0.

\***************************************************************************************/
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(
    VkPhysicalDevice physicalDevice,
    uint32_t* pPropertyCount,
    VkLayerProperties* pProperties )
{
    assert( physicalDevice == VK_NULL_HANDLE );
    return CheckResult( Profiler::VkInstance_Functions::EnumerateDeviceLayerProperties(
        VK_NULL_HANDLE,
        pPropertyCount,
        pProperties ) );
}

/***************************************************************************************\

Function:
    vkEnumerateDeviceExtensionProperties

Description:
    Entrypoint to the EnumerateDeviceExtensionProperties.
    Required by layer interface version 0.

\***************************************************************************************/
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(
    VkPhysicalDevice physicalDevice,
    const char* pLayerName,
    uint32_t* pPropertyCount,
    VkExtensionProperties* pProperties )
{
    assert( physicalDevice == VK_NULL_HANDLE );
    assert( pLayerName && ( strcmp( pLayerName, VK_LAYER_profiler_name ) == 0 ) );
    return CheckResult( Profiler::VkInstance_Functions::EnumerateDeviceExtensionProperties(
        VK_NULL_HANDLE,
        pLayerName,
        pPropertyCount,
        pProperties ) );
}

/***************************************************************************************\

Function:
    vkNegotiateLoaderLayerInterfaceVersion

Description:
    Entrypoint to the layer interface version negotiation.
    Required by layer interface version 2.

\***************************************************************************************/
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkNegotiateLoaderLayerInterfaceVersion(
    VkNegotiateLayerInterface* pVersionStruct )
{
    if( pVersionStruct == nullptr ||
        pVersionStruct->sType != LAYER_NEGOTIATE_INTERFACE_STRUCT )
    {
        // Invalid or unsupported structure.
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // The layer currently supports version 2 of the interface.
    // According to the spec no loader should report a version lower than 2 (as this is the version that introduced this function).
    assert( pVersionStruct->loaderLayerInterfaceVersion >= 2 );
    pVersionStruct->loaderLayerInterfaceVersion = 2;

    // Fill function pointers.
    pVersionStruct->pfnGetInstanceProcAddr = Profiler::VkInstance_Functions::GetInstanceProcAddr;
    pVersionStruct->pfnGetDeviceProcAddr = Profiler::VkDevice_Functions::GetDeviceProcAddr;
    pVersionStruct->pfnGetPhysicalDeviceProcAddr = nullptr;

    return VK_SUCCESS;
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
