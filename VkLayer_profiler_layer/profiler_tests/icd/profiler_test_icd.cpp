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

#include "profiler_test_icd.h"
#include "profiler_test_icd_helpers.h"
#include "profiler_test_instance.h"

PFN_vkVoidFunction vk_icdGetInstanceProcAddr(
    VkInstance instance,
    const char* pName )
{
    return vkGetInstanceProcAddr( instance, pName );
}

VkResult vk_icdNegotiateLoaderICDInterfaceVersion(
    uint32_t* pSupportedVersion )
{
    if( *pSupportedVersion >= 5 )
    {
        *pSupportedVersion = 5;
        return VK_SUCCESS;
    }

    if( *pSupportedVersion >= 2 )
    {
        *pSupportedVersion = 2;
        return VK_SUCCESS;
    }

    return VK_ERROR_INCOMPATIBLE_DRIVER;
}

VkResult vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance )
{
    return vk_new<Profiler::ICD::Instance>( pInstance, *pCreateInfo );
}

VkResult vkEnumerateInstanceVersion(
    uint32_t* pApiVersion )
{
    *pApiVersion = VK_API_VERSION_1_3;
    return VK_SUCCESS;
}

VkResult vkEnumerateInstanceLayerProperties(
    uint32_t* pPropertyCount,
    VkLayerProperties* pProperties )
{
    *pPropertyCount = 0;
    return VK_SUCCESS;
}

VkResult vkEnumerateInstanceExtensionProperties(
    const char* pLayerName,
    uint32_t* pPropertyCount,
    VkExtensionProperties* pProperties )
{
    const VkExtensionProperties properties[] = {
#ifdef VK_KHR_surface
        { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_SURFACE_SPEC_VERSION },
#endif
#ifdef VK_KHR_win32_surface
        { VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_SPEC_VERSION },
#endif
    };

    const uint32_t propertyCount = std::size( properties );

    if( !pProperties )
    {
        *pPropertyCount = propertyCount;
        return VK_SUCCESS;
    }

    const uint32_t count = std::min( *pPropertyCount, propertyCount );
    for( uint32_t i = 0; i < count; ++i )
    {
        pProperties[ i ] = properties[ i ];
    }

    *pPropertyCount = count;

    if( count < propertyCount )
    {
        return VK_INCOMPLETE;
    }

    return VK_SUCCESS;
}
