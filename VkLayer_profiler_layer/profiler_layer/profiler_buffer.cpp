#include "profiler_buffer.h"
#include "profiler_helpers.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        ProfilerBuffer

    Description:
        Constructor

    \***********************************************************************************/
    ProfilerBuffer::ProfilerBuffer()
    {
        ClearMemory( this );
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:

    \***********************************************************************************/
    VkResult ProfilerBuffer::Initialize(
        VkDevice_Object* pDevice,
        const VkBufferCreateInfo* pCreateInfo,
        VkMemoryPropertyFlags memoryPropertyFlags,
        ProfilerCallbacks callbacks )
    {
        m_Callbacks = callbacks;
        m_Device = pDevice->Device;

        Size = static_cast<uint32_t>(pCreateInfo->size);

        // Get physical device memory properties
        VkPhysicalDeviceMemoryProperties memoryProperties;
        m_Callbacks.pfnGetPhysicalDeviceMemoryProperties( pDevice->PhysicalDevice, &memoryProperties );

        // Create the buffer
        DESTROYANDRETURNONFAIL( m_Callbacks.pfnCreateBuffer(
            m_Device, pCreateInfo, nullptr, &Buffer ) );

        // Get buffer memory requirements
        VkMemoryRequirements memoryRequirements;
        m_Callbacks.pfnGetBufferMemoryRequirements( m_Device, Buffer, &memoryRequirements );

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

        // Allocate memory for the buffer
        DESTROYANDRETURNONFAIL( m_Callbacks.pfnAllocateMemory(
            m_Device, &memoryAllocateInfo, nullptr, &m_DeviceMemory ) );

        // Bind the memory to the buffer
        DESTROYANDRETURNONFAIL( m_Callbacks.pfnBindBufferMemory(
            m_Device, Buffer, m_DeviceMemory, 0 ) );

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Release resources allocated by the buffer instance.

    \***********************************************************************************/
    void ProfilerBuffer::Destroy()
    {
        if( Buffer )
        {
            // Destroy the buffer object
            m_Callbacks.pfnDestroyBuffer( m_Device, Buffer, nullptr );
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
        Map

    Description:

    \***********************************************************************************/
    VkResult ProfilerBuffer::Map( void** ppMappedMemoryAddress )
    {
        return m_Callbacks.pfnMapMemory( m_Device, m_DeviceMemory, 0, Size, 0, ppMappedMemoryAddress );
    }

    /***********************************************************************************\

    Function:
        Unmap

    Description:

    \***********************************************************************************/
    void ProfilerBuffer::Unmap()
    {
        m_Callbacks.pfnUnmapMemory( m_Device, m_DeviceMemory );
    }
}
