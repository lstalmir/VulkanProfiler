#include "VkCommandBuffer_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        BeginCommandBuffer

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkCommandBuffer_Functions::BeginCommandBuffer(
        VkCommandBuffer commandBuffer,
        const VkCommandBufferBeginInfo* pBeginInfo )
    {
        return VK_ERROR_DEVICE_LOST;
    }

    /***********************************************************************************\

    Function:
        EndCommandBuffer

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkCommandBuffer_Functions::EndCommandBuffer(
        VkCommandBuffer commandBuffer )
    {
        return VK_ERROR_DEVICE_LOST;
    }

    /***********************************************************************************\

    Function:
        CmdDraw

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdDraw(
        VkCommandBuffer commandBuffer,
        uint32_t vertexCount,
        uint32_t instanceCount,
        uint32_t firstVertex,
        uint32_t firstInstance )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        // Increment drawcall counter
        dd.Profiler.GetCurrentFrameStats().drawCount++;

        dd.Profiler.PreDraw( commandBuffer );

        // Invoke next layer's implementation
        dd.DispatchTable.CmdDraw(
            commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance );

        dd.Profiler.PostDraw( commandBuffer );
    }

    /***********************************************************************************\

    Function:
        CmdDrawIndexed

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkCommandBuffer_Functions::CmdDrawIndexed(
        VkCommandBuffer commandBuffer,
        uint32_t indexCount,
        uint32_t instanceCount,
        uint32_t firstIndex,
        int32_t vertexOffset,
        uint32_t firstInstance )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );

        // Increment drawcall counter
        dd.Profiler.GetCurrentFrameStats().drawCount++;

        dd.Profiler.PreDraw( commandBuffer );

        // Invoke next layer's implementation
        dd.DispatchTable.CmdDrawIndexed(
            commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );

        dd.Profiler.PostDraw( commandBuffer );
    }
}
