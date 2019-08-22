#include "profiler_overlay_font.h"
#include "profiler_overlay.h"
#include "profiler_buffer.h"
#include "profiler_helpers.h"
#include "profiler_resources.generated.h"
#include <algorithm>

namespace Profiler
{
    /***********************************************************************************\

    Function:
        ProfilerOverlayFont

    Description:
        Constructor

    \***********************************************************************************/
    ProfilerOverlayFont::ProfilerOverlayFont()
    {
        ClearMemory( this );
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes profiler overlay font resources.

    \***********************************************************************************/
    VkResult ProfilerOverlayFont::Initialize( VkDevice_Object* pDevice, ProfilerOverlay* pOverlay, ProfilerCallbacks callbacks )
    {
        m_pProfilerOverlay = pOverlay;
        m_Callbacks = callbacks;
        m_Device = pDevice->Device;

        // Find the transfer queue
        uint32_t queueFamilyCount = 0;

        m_Callbacks.pfnGetPhysicalDeviceQueueFamilyProperties(
            pDevice->PhysicalDevice, &queueFamilyCount, nullptr );

        std::vector<VkQueueFamilyProperties> queueFamilyProperties( queueFamilyCount );

        m_Callbacks.pfnGetPhysicalDeviceQueueFamilyProperties(
            pDevice->PhysicalDevice, &queueFamilyCount, queueFamilyProperties.data() );

        // Select queue with highest priority
        float currentQueuePriority = 0.f;

        VkQueue_Object* pTransferQueue = nullptr;

        for( auto& queuePair : pDevice->Queues )
        {
            VkQueue_Object* pQueue = &queuePair.second;

            // Check if queue is in graphics family
            VkQueueFamilyProperties familyProperties = queueFamilyProperties[pQueue->FamilyIndex];

            if( (familyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                (pQueue->Priority > currentQueuePriority) )
            {
                pTransferQueue = pQueue;
                currentQueuePriority = pQueue->Priority;
            }
        }

        if( !pTransferQueue )
        {
            // Transfer queue not found
            DESTROYANDRETURNONFAIL( VK_ERROR_INITIALIZATION_FAILED );
        }

        const uint32_t fontDataByteSize = profiler_font_glyphs::arraySize * profiler_font_glyphs::arraySliceSize;

        // Create the image
        VkStructure<VkImageCreateInfo> imageCreateInfo;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = profiler_font_glyphs::width;
        imageCreateInfo.extent.height = profiler_font_glyphs::height;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.format = profiler_font_glyphs::format;
        imageCreateInfo.mipLevels = profiler_font_glyphs::mipCount;
        imageCreateInfo.arrayLayers = profiler_font_glyphs::arraySize;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        DESTROYANDRETURNONFAIL( ProfilerImage::Initialize(
            pDevice,
            &imageCreateInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            callbacks ) );

        // Prepare staging buffer with image data
        VkStructure<VkBufferCreateInfo> fontDataStagingBufferCreateInfo;
        fontDataStagingBufferCreateInfo.size = fontDataByteSize;
        fontDataStagingBufferCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
        fontDataStagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        ProfilerBuffer fontDataStagingBuffer;
        
        DESTROYANDRETURNONFAIL( fontDataStagingBuffer.Initialize(
            pDevice,
            &fontDataStagingBufferCreateInfo,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            callbacks ) );

        // Map the buffer and copy font data to the GPU
        void* pMappedFontDataStaginBufferMemory;

        VkResult mapResult = fontDataStagingBuffer.Map( &pMappedFontDataStaginBufferMemory );

        if( mapResult != VK_SUCCESS )
        {
            // Map failed. fontDataStagingBuffer must be destroyed explicitly here because
            // it is temporary object, not associated with the font.
            fontDataStagingBuffer.Destroy();

            DESTROYANDRETURNONFAIL( mapResult );
        }

        memcpy( pMappedFontDataStaginBufferMemory, profiler_font_glyphs::data, fontDataByteSize );

        fontDataStagingBuffer.Unmap();

        // Copy the data from staging buffer to the image
        VkStructure<VkCommandBufferAllocateInfo> uploadCommandBufferAllocateInfo;
        uploadCommandBufferAllocateInfo.commandBufferCount = 1;
        uploadCommandBufferAllocateInfo.commandPool = m_pProfilerOverlay->GetCommandPool();
        uploadCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VkCommandBuffer uploadCommandBuffer;

        DESTROYANDRETURNONFAIL( m_Callbacks.pfnAllocateCommandBuffers(
            pDevice->Device, &uploadCommandBufferAllocateInfo, &uploadCommandBuffer ) );

        std::vector<VkBufferImageCopy> copyRegions; // One copy for each mip in each layer
        {
            // Offset to the buffer with data
            uint32_t currentBufferOffset = 0;

            // Iterate over array layers
            for( uint32_t arrayLayer = 0; arrayLayer < imageCreateInfo.arrayLayers; ++arrayLayer )
            {
                // Iterate over mip levels in layer
                for( uint32_t mipLevel = 0; mipLevel < imageCreateInfo.mipLevels; ++mipLevel )
                {
                    // Recalculate image size for current mip level
                    VkExtent3D mipLevelExtent;
                    mipLevelExtent.width = std::max<uint32_t>( 1, imageCreateInfo.extent.width >> mipLevel );
                    mipLevelExtent.height = std::max<uint32_t>( 1, imageCreateInfo.extent.height >> mipLevel );
                    mipLevelExtent.depth = std::max<uint32_t>( 1, imageCreateInfo.extent.depth >> mipLevel );

                    // Prepare copy descriptor
                    VkStructure<VkBufferImageCopy> copyRegion;
                    copyRegion.bufferOffset = currentBufferOffset;
                    copyRegion.imageExtent = mipLevelExtent;
                    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    copyRegion.imageSubresource.mipLevel = mipLevel;
                    copyRegion.imageSubresource.baseArrayLayer = arrayLayer;
                    copyRegion.imageSubresource.layerCount = 1;

                    copyRegions.push_back( copyRegion );

                    currentBufferOffset += profiler_font_glyphs::mipSizes[mipLevel];
                }
            }

            if( currentBufferOffset != fontDataByteSize )
            {
                // TODO invalid final offset
            }
        }

        m_Callbacks.pfnCmdCopyBufferToImage(
            uploadCommandBuffer,
            fontDataStagingBuffer.Buffer,
            Image,
            Layout,
            static_cast<uint32_t>(copyRegions.size()), copyRegions.data() );

        // Submit the upload command buffer to the transfer queue
        VkStructure<VkSubmitInfo> uploadSubmit;
        uploadSubmit.commandBufferCount = 1;
        uploadSubmit.pCommandBuffers = &uploadCommandBuffer;

        VkResult submitResult = m_Callbacks.pfnQueueSubmit(
            pTransferQueue->Queue, 1, &uploadSubmit, VK_NULL_HANDLE );

        if( submitResult != VK_SUCCESS )
        {
            // Submit failed. fontDataStagingBuffer must be destroyed explicitly here because
            // it is temporary object, not associated with the font.
            fontDataStagingBuffer.Destroy();

            DESTROYANDRETURNONFAIL( submitResult );
        }

        // Wait until the data is ready
        VkResult waitResult = m_Callbacks.pfnQueueWaitIdle( pTransferQueue->Queue );

        if( waitResult != VK_SUCCESS )
        {
            // Wait failed. fontDataStagingBuffer must be destroyed explicitly here because
            // it is temporary object, not associated with the font.
            fontDataStagingBuffer.Destroy();

            DESTROYANDRETURNONFAIL( waitResult );
        }

        // Create the image view
        VkStructure<VkImageViewCreateInfo> imageViewCreateInfo;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.image = Image;
        imageViewCreateInfo.format = profiler_font_glyphs::format;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.layerCount = profiler_font_glyphs::arraySize;
        imageViewCreateInfo.subresourceRange.levelCount = profiler_font_glyphs::mipCount;

        DESTROYANDRETURNONFAIL( m_Callbacks.pfnCreateImageView(
            pDevice->Device, &imageViewCreateInfo, nullptr, &m_ImageView ) );

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Frees resources allocated by the profiler overlay font.

    \***********************************************************************************/
    void ProfilerOverlayFont::Destroy()
    {
        if( m_ImageView )
        {
            // Destroy image view
            m_Callbacks.pfnDestroyImageView( m_Device, m_ImageView, nullptr );
        }

        ProfilerImage::Destroy();
        
        ClearMemory( this );
    }
}
