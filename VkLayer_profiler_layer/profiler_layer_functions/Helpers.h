#pragma once
#include "vulkan_traits/vulkan_traits.h"
#include <vk_layer.h>
#include <vk_dispatch_table_helper.h>

namespace Profiler
{
    /***********************************************************************************\

    Function:
        GetLayerLinkInfo

    Description:
        Iterates through the CreateInfo structure until VK_LAYER_LINK_INFO structure is
        found. Returns pointer to the layer link info structure or nullptr.

    \***********************************************************************************/
    template<typename LayerCreateInfo, typename CreateInfo>
    inline const LayerCreateInfo* GetLayerLinkInfo( const CreateInfo* pCreateInfo )
    {
        auto pLayerCreateInfo = reinterpret_cast<const LayerCreateInfo*>(pCreateInfo->pNext);

        // Step through the chain of pNext until we get to the link info
        while( (pLayerCreateInfo)
            && (pLayerCreateInfo->sType != VkStructureTypeTraits<LayerCreateInfo>::Type ||
                pLayerCreateInfo->function != VK_LAYER_LINK_INFO) )
        {
            pLayerCreateInfo = reinterpret_cast<const LayerCreateInfo*>(pLayerCreateInfo->pNext);
        }

        return pLayerCreateInfo;
    }

    /***********************************************************************************\

    Function:
        CopyString

    Description:

    \***********************************************************************************/
    static inline void CopyString( char* dst, size_t dstSize, const char* src )
    {
#   ifdef WIN32
        strcpy_s( dst, dstSize, src );
#   else
        strcpy( dst, src );
#   endif
    }

    /***********************************************************************************\

    Function:
        CopyString

    Description:

    \***********************************************************************************/
    template<size_t dstSize>
    static inline void CopyString( char( &dst )[dstSize], const char* src )
    {
        return CopyString( dst, dstSize, src );
    }
}
