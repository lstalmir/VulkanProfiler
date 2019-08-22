#pragma once
#include "vulkan_traits/vulkan_traits.h"
#include <vulkan/vulkan.h>

// Helper macro for rolling-back to valid state
#define DESTROYANDRETURNONFAIL( VKRESULT )  \
    {                                       \
        VkResult result = (VKRESULT);       \
        if( result != VK_SUCCESS )          \
        {                                   \
            /* Destroy() must be defined */ \
            Destroy();                      \
            return result;                  \
        }                                   \
    }

namespace Profiler
{
    /***********************************************************************************\

    Function:
        ZeroMemory

    Description:
        Fill memory region with zeros.

    \***********************************************************************************/
    template<typename T>
    void ClearMemory( T* pMemory )
    {
        memset( pMemory, 0, sizeof( T ) );
    }

    /***********************************************************************************\

    Structure:
        VkStructure

    Description:
        Wrapper for Vulkan structures with default initialization and sType setup.

    \***********************************************************************************/
    template<typename StructureType>
    struct VkStructure : StructureType
    {
        // Clear the structure and set optional sType field
        VkStructure()
        {
            // Clear contents
            memset( this, 0, sizeof( StructureType ) );

            SetStructureType();
        }

    private:
        template<bool sTypeDefined = VkStructureTypeTraits<StructureType>::Defined>
        constexpr void SetStructureType()
        {
            // Structure does not have sType field
        }

        template<>
        constexpr void SetStructureType<true>()
        {
            // Structure has sType field
            this->sType = VkStructureTypeTraits<StructureType>::Type;
        }
    };
}
