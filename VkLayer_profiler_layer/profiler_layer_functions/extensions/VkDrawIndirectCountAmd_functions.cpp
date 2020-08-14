#include "VkDrawIndirectCountAmd_functions.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        CmdDrawIndirectCountAMD

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDrawIndirectCountAmd_Functions::CmdDrawIndirectCountAMD(
        VkCommandBuffer commandBuffer,
        VkBuffer argsBuffer,
        VkDeviceSize argsOffset,
        VkBuffer countBuffer,
        VkDeviceSize countOffset,
        uint32_t maxDrawCount,
        uint32_t stride )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eDrawIndirectCount;
        drawcall.m_Payload.m_DrawIndirectCount.m_Buffer = argsBuffer;
        drawcall.m_Payload.m_DrawIndirectCount.m_Offset = argsOffset;
        drawcall.m_Payload.m_DrawIndirectCount.m_CountBuffer = countBuffer;
        drawcall.m_Payload.m_DrawIndirectCount.m_CountOffset = countOffset;
        drawcall.m_Payload.m_DrawIndirectCount.m_MaxDrawCount = maxDrawCount;
        drawcall.m_Payload.m_DrawIndirectCount.m_Stride = stride;

        profiledCommandBuffer.PreDraw( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndirectCountAMD(
            commandBuffer, argsBuffer, argsOffset, countBuffer, countOffset, maxDrawCount, stride );

        profiledCommandBuffer.PostDraw();
    }

    /***********************************************************************************\

    Function:
        CmdDrawIndexedIndirectCountAMD

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDrawIndirectCountAmd_Functions::CmdDrawIndexedIndirectCountAMD(
        VkCommandBuffer commandBuffer,
        VkBuffer argsBuffer,
        VkDeviceSize argsOffset,
        VkBuffer countBuffer,
        VkDeviceSize countOffset,
        uint32_t maxDrawCount,
        uint32_t stride )
    {
        auto& dd = DeviceDispatch.Get( commandBuffer );
        auto& profiledCommandBuffer = dd.Profiler.GetCommandBuffer( commandBuffer );

        // Setup drawcall descriptor
        DeviceProfilerDrawcall drawcall;
        drawcall.m_Type = DeviceProfilerDrawcallType::eDrawIndexedIndirectCount;
        drawcall.m_Payload.m_DrawIndirectCount.m_Buffer = argsBuffer;
        drawcall.m_Payload.m_DrawIndirectCount.m_Offset = argsOffset;
        drawcall.m_Payload.m_DrawIndirectCount.m_CountBuffer = countBuffer;
        drawcall.m_Payload.m_DrawIndirectCount.m_CountOffset = countOffset;
        drawcall.m_Payload.m_DrawIndirectCount.m_MaxDrawCount = maxDrawCount;
        drawcall.m_Payload.m_DrawIndirectCount.m_Stride = stride;

        profiledCommandBuffer.PreDraw( drawcall );

        // Invoke next layer's implementation
        dd.Device.Callbacks.CmdDrawIndexedIndirectCountAMD(
            commandBuffer, argsBuffer, argsOffset, countBuffer, countOffset, maxDrawCount, stride );

        profiledCommandBuffer.PostDraw();
    }
}
