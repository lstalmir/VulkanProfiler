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

#pragma once
#include "vk_dispatch_tables.h"
#include <assert.h>

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        LayerCreateInfoTypeTraits

    Description:
        Contains metadata for each VkLayer*CreateInfo structure.

    \***********************************************************************************/
    template<typename LayerCreateInfo>
    struct LayerCreateInfoTypeTraits;

    template<>
    struct LayerCreateInfoTypeTraits<VkLayerDeviceCreateInfo>
    {
        static constexpr VkStructureType sType = VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO;
    };

    template<>
    struct LayerCreateInfoTypeTraits<VkLayerInstanceCreateInfo>
    {
        static constexpr VkStructureType sType = VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO;
    };

    /***********************************************************************************\

    Function:
        GetLayerLinkInfo

    Description:
        Iterates through the CreateInfo structure until VK_LAYER_LINK_INFO structure is
        found. Returns pointer to the layer link info structure or nullptr.

    \***********************************************************************************/
    template<typename LayerCreateInfo, typename CreateInfo>
    inline LayerCreateInfo* GetLayerLinkInfo( const CreateInfo* pCreateInfo, VkLayerFunction function )
    {
        auto pLayerCreateInfo = reinterpret_cast<const LayerCreateInfo*>(pCreateInfo->pNext);

        // Step through the chain of pNext until we get to the link info
        while( (pLayerCreateInfo)
            && (pLayerCreateInfo->sType != LayerCreateInfoTypeTraits<LayerCreateInfo>::sType ||
                pLayerCreateInfo->function != function) )
        {
            pLayerCreateInfo = reinterpret_cast<const LayerCreateInfo*>(pLayerCreateInfo->pNext);
        }

        return const_cast<LayerCreateInfo*>(pLayerCreateInfo);
    }

    /***********************************************************************************\

    Function:
        AppendPNext

    Description:

    \***********************************************************************************/
    template<typename StructureType>
    inline void AppendPNext( StructureType& structure, void* pNext )
    {
        VkBaseOutStructure* pStruct = reinterpret_cast<VkBaseOutStructure*>(&structure);

        // Skip pNext pointers until we get to the end of chain
        while( pStruct->pNext ) pStruct = pStruct->pNext;

        pStruct->pNext = reinterpret_cast<VkBaseOutStructure*>(pNext);
    }
}
