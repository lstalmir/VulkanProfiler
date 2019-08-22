#include "profiler_image.h"
#include "profiler_helpers.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        ProfilerImage

    Description:
        Constructor

    \***********************************************************************************/
    ProfilerImage::ProfilerImage()
    {
        ClearMemory( this );
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:

    \***********************************************************************************/
    VkResult ProfilerImage::Initialize(
        VkDevice_Object* pDevice,
        const VkImageCreateInfo* pCreateInfo,
        VkMemoryPropertyFlags memoryPropertyFlags,
        ProfilerCallbacks callbacks )
    {
        m_Callbacks = callbacks;
        m_Device = pDevice->Device;

        Layout = pCreateInfo->initialLayout;

        // Get physical device memory properties
        VkPhysicalDeviceMemoryProperties memoryProperties;
        m_Callbacks.pfnGetPhysicalDeviceMemoryProperties( pDevice->PhysicalDevice, &memoryProperties );

        // Create the image
        DESTROYANDRETURNONFAIL( m_Callbacks.pfnCreateImage(
            m_Device, pCreateInfo, nullptr, &Image ) );

        // Get buffer memory requirements
        VkMemoryRequirements memoryRequirements;
        m_Callbacks.pfnGetImageMemoryRequirements( m_Device, Image, &memoryRequirements );

        uint32_t memoryTypeIndex = static_cast<uint32_t>(~0);

        // Find suitable memory type index
        for( uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i )
        {
            if( (memoryRequirements.memoryTypeBits & (1 << i)) > 0 &&
                (memoryProperties.memoryTypes[i].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags )
            {
                // Memory found
                memoryTypeIndex = i;
                break;
            }
        }

        if( memoryTypeIndex == static_cast<uint32_t>(~0) )
        {
            // Memory not found
            DESTROYANDRETURNONFAIL( VK_ERROR_INITIALIZATION_FAILED );
        }

        // Prepare memory allocate info
        VkStructure<VkMemoryAllocateInfo> memoryAllocateInfo;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

        // Allocate memory for the image
        DESTROYANDRETURNONFAIL( m_Callbacks.pfnAllocateMemory(
            m_Device, &memoryAllocateInfo, nullptr, &m_DeviceMemory ) );

        // Bind the memory to the image
        DESTROYANDRETURNONFAIL( m_Callbacks.pfnBindImageMemory(
            m_Device, Image, m_DeviceMemory, 0 ) );

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Release resources allocated by the image instance.

    \***********************************************************************************/
    void ProfilerImage::Destroy()
    {
        if( Image )
        {
            // Destroy the buffer object
            m_Callbacks.pfnDestroyImage( m_Device, Image, nullptr );
        }

        if( m_DeviceMemory )
        {
            // Free the device memory
            m_Callbacks.pfnFreeMemory( m_Device, m_DeviceMemory, nullptr );
        }

        ClearMemory( this );
    }

    /***********************************************************************************\

    Function:
        LayoutTransition

    Description:

    \***********************************************************************************/
    void ProfilerImage::LayoutTransition( VkCommandBuffer commandBuffer, VkImageLayout newLayout )
    {
        // Prepare the transition barrier
        VkStructure<VkImageMemoryBarrier> imageMemoryBarrier;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.oldLayout = Layout;
        imageMemoryBarrier.newLayout = newLayout;
        imageMemoryBarrier.image = Image;
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageMemoryBarrier.subresourceRange.layerCount = static_cast<uint32_t>(~0);
        imageMemoryBarrier.subresourceRange.levelCount = static_cast<uint32_t>(~0);

        // Submit image memory barrier to the command buffer
        m_Callbacks.pfnCmdPipelineBarrier( commandBuffer,
            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier );

        // Update the image layout
        Layout = newLayout;
    }
}
