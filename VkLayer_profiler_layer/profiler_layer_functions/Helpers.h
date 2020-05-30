#pragma once
#include <assert.h>
#include <vulkan/vk_layer.h>
#include <vk_dispatch_table_helper.h>

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

    /***********************************************************************************\

    Function:
        Create

    Description:
        Create new object. If constructor throws an exception, return it as VkResult value.

    \***********************************************************************************/
    template<typename Class, typename... Args>
    inline VkResult Create( Class*& pObject, Args&&... args )
    {
        try
        {
            pObject = new Class( std::forward<Args>( args )... );
        }
        catch( std::bad_alloc )
        { // Thrown from new() operator
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        catch( VkResult result )
        { // Thrown from constructor
            assert( result != VK_SUCCESS );
            return result;
        }
        catch( ... )
        { // Unhandled exception
            assert( false );
            return VK_ERROR_INITIALIZATION_FAILED;
        }
        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Destroy the object.

    \***********************************************************************************/
    template<typename Class>
    inline void Destroy( Class*& pObject )
    {
        delete pObject;
        pObject = nullptr;
    }
}
