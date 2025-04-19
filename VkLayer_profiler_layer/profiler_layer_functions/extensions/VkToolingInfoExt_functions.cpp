// Copyright (c) 2024-2024 Lukasz Stalmirski
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

#include "VkToolingInfoExt_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        GetPhysicalDeviceToolPropertiesEXT

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkToolingInfoExt_Functions::GetPhysicalDeviceToolPropertiesEXT(
        VkPhysicalDevice physicalDevice,
        uint32_t* pToolCount,
        VkPhysicalDeviceToolPropertiesEXT* pToolProperties )
    {
        auto& id = InstanceDispatch.Get( physicalDevice );

        uint32_t toolCount = *pToolCount;
        VkResult result = VK_SUCCESS;

        if( id.Instance.Callbacks.GetPhysicalDeviceToolPropertiesEXT )
        {
            // Report tools from the next layers.
            result = id.Instance.Callbacks.GetPhysicalDeviceToolPropertiesEXT(
                physicalDevice, pToolCount, pToolProperties );
        }
        else
        {
            // This layer is last in chain, start with no tools.
            *pToolCount = 0;
        }

        if( (result == VK_SUCCESS) || (result == VK_INCOMPLETE) )
        {
            VkToolingInfoExt_Functions::AppendProfilerToolInfo(
                result,
                toolCount,
                pToolCount,
                pToolProperties );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        AppendProfilerToolInfo

    Description:
        Helper function that appends profiler tool information to the list of tools.
        Used by vkGetPhysicalDeviceToolPropertiesEXT and promoted equivalent.

    \***********************************************************************************/
    void VkToolingInfoExt_Functions::AppendProfilerToolInfo(
        VkResult& result,
        uint32_t inToolCount,
        uint32_t* pOutToolCount,
        VkPhysicalDeviceToolPropertiesEXT* pToolProperties )
    {
        if( pToolProperties != nullptr )
        {
            if( inToolCount > *pOutToolCount )
            {
                // Fill the buffer with the profiler tool.
                VkPhysicalDeviceToolProperties& toolProperties = pToolProperties[*pOutToolCount];
                memset( &toolProperties, 0, sizeof( VkPhysicalDeviceToolProperties ) );

                toolProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TOOL_PROPERTIES;
                toolProperties.pNext = nullptr;
                toolProperties.purposes =
                    VK_TOOL_PURPOSE_PROFILING_BIT |
                    VK_TOOL_PURPOSE_DEBUG_MARKERS_BIT_EXT;

                ProfilerStringFunctions::CopyString( toolProperties.name, VK_LAYER_profiler_product_name );
                ProfilerStringFunctions::CopyString( toolProperties.version, VK_LAYER_profiler_ver );
                ProfilerStringFunctions::CopyString( toolProperties.description, VK_LAYER_profiler_desc );
                ProfilerStringFunctions::CopyString( toolProperties.layer, VK_LAYER_profiler_name );

                ( *pOutToolCount )++;
            }
            else
            {
                // The buffer is too small.
                result = VK_INCOMPLETE;
            }
        }
        else
        {
            // Include the profiler in the reported number of tools.
            ( *pOutToolCount )++;
        }
    }
}
