#include "profiler.h"
#include "VkDevice_functions.h"
#include "VkCommandBuffer_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        Profiler

    Description:
        Constructor

    \***********************************************************************************/
    Profiler::Profiler()
    {
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes profiler resources.

    \***********************************************************************************/
    VkResult Profiler::Initialize( VkDevice device, ProfilerCallbacks profilerCallbacks )
    {
        m_Callbacks = profilerCallbacks;

        VkQueryPoolCreateInfo queryPoolCreateInfo;
        memset( &queryPoolCreateInfo, 0, sizeof( queryPoolCreateInfo ) );

        queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolCreateInfo.queryCount = 2;

        // Create the timestamp query pool
        VkResult result = m_Callbacks.pfnCreateQueryPool(
            device, &queryPoolCreateInfo, nullptr, &m_TimestampQueryPool );

        if( result != VK_SUCCESS )
        {
            // Failed to create timestamp query pool
            return result;
        }

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        PreDraw

    Description:
        Executed before drawcall

    \***********************************************************************************/
    void Profiler::PreDraw( VkCommandBuffer commandBuffer )
    {
        // Submit begin query
        m_Callbacks.pfnCmdWriteTimestamp(
            commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, m_TimestampQueryPool, 0 );
    }

    /***********************************************************************************\

    Function:
        PostDraw

    Description:
        Executed after drawcall

    \***********************************************************************************/
    void Profiler::PostDraw( VkCommandBuffer commandBuffer )
    {
        // Submit end query
        m_Callbacks.pfnCmdWriteTimestamp(
            commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, m_TimestampQueryPool, 1 );
    }

}
