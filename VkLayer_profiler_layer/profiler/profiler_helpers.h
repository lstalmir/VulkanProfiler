#pragma once
#include "vulkan_traits/vulkan_traits.h"
#include <vulkan/vk_layer.h>
#include <vector>

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

    Function:
        GetImageCreateInfoForResource

    Description:
        Get CreateInfo structure for given embedded resource.

    \***********************************************************************************/
    template<typename ImageResource>
    VkImageCreateInfo GetImageCreateInfoForResource()
    {
        VkImageType imageType = VK_IMAGE_TYPE_3D;

        if( ImageResource::depth == 0 )
        {
            imageType = VK_IMAGE_TYPE_2D;

            if( ImageResource::height == 0 )
            {
                imageType = VK_IMAGE_TYPE_1D;
            }
        }

        VkStructure<VkImageCreateInfo> createInfo;
        createInfo.imageType = imageType;
        createInfo.extent.width = std::max<uint32_t>( 1, ImageResource::width );
        createInfo.extent.height = std::max<uint32_t>( 1, ImageResource::height );
        createInfo.extent.depth = std::max<uint32_t>( 1, ImageResource::depth );
        createInfo.format = ImageResource::format;
        createInfo.mipLevels = ImageResource::mipCount;
        createInfo.arrayLayers = ImageResource::arraySize;
        createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        return createInfo;
    }

    /***********************************************************************************\

    Function:
        GetBufferImageCopyForResource

    Description:
        Get list of VkBufferImageCopy structures for image initialization.

    \***********************************************************************************/
    template<typename ImageResource>
    std::vector<VkBufferImageCopy> GetBufferImageCopyForResource()
    {
        // One copy for each mip in each layer
        std::vector<VkBufferImageCopy> copyRegions;

        uint32_t currentBufferOffset = 0;

        // Iterate over array layers
        for( uint32_t arrayLayer = 0; arrayLayer < ImageResource::arraySize; ++arrayLayer )
        {
            // Iterate over mip levels in layer
            for( uint32_t mipLevel = 0; mipLevel < ImageResource::mipCount; ++mipLevel )
            {
                // Recalculate image size for current mip level
                VkExtent3D mipLevelExtent;
                mipLevelExtent.width = std::max<uint32_t>( 1, ImageResource::width >> mipLevel );
                mipLevelExtent.height = std::max<uint32_t>( 1, ImageResource::height >> mipLevel );
                mipLevelExtent.depth = std::max<uint32_t>( 1, ImageResource::depth >> mipLevel );

                // Prepare copy descriptor
                VkStructure<VkBufferImageCopy> copyRegion;
                copyRegion.bufferOffset = currentBufferOffset;
                copyRegion.imageExtent = mipLevelExtent;
                copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegion.imageSubresource.mipLevel = mipLevel;
                copyRegion.imageSubresource.baseArrayLayer = arrayLayer;
                copyRegion.imageSubresource.layerCount = 1;

                copyRegions.push_back( copyRegion );

                currentBufferOffset += ImageResource::mipSizes[mipLevel];
            }
        }

        return copyRegions;
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
