#include "VkCommandBuffer_functions.h"

namespace Profiler
{
    // Define the command buffer dispatcher
    VkDispatch<VkDevice, VkCommandBuffer_Functions::DispatchTable> VkCommandBuffer_Functions::Dispatch;

    /***********************************************************************************\

    Function:
        GetProcAddr

    Description:
        Gets address of this layer's function implementation.

    \***********************************************************************************/
    PFN_vkVoidFunction VkCommandBuffer_Functions::GetInterceptedProcAddr( const char* pName )
    {
        // Intercepted functions
        GETPROCADDR( BeginCommandBuffer );
        GETPROCADDR( EndCommandBuffer );
        GETPROCADDR( CmdDraw );
        GETPROCADDR( CmdDrawIndexed );

        // Function not overloaded
        return nullptr;
    }

    /***********************************************************************************\

    Function:
        GetProcAddr

    Description:
        Gets address of this layer's function implementation.

    \***********************************************************************************/
    PFN_vkVoidFunction VkCommandBuffer_Functions::GetProcAddr( VkDevice device, const char* pName )
    {
        // Overloaded functions
        if( PFN_vkVoidFunction function = GetInterceptedProcAddr( pName ) )
            return function;

        // Get address from the next layer
        auto dispatchTable = VkDevice_Functions::Dispatch.GetDispatchTable( device );

        return dispatchTable.pfnGetDeviceProcAddr( device, pName );
    }

    /***********************************************************************************\

    Function:
        OnDeviceCreate

    Description:
        Initializes VkCommandBuffer function callbacks for new device.

    \***********************************************************************************/
    void VkCommandBuffer_Functions::OnDeviceCreate( VkDevice device, PFN_vkGetDeviceProcAddr gpa )
    {
        // Create dispatch table for overloaded VkCommandBuffer functions
        Dispatch.CreateDispatchTable( device, gpa );
    }

    /***********************************************************************************\

    Function:
        OnDeviceCreate

    Description:
        Removes VkCommandBuffer function callbacks for the device from the memory.

    \***********************************************************************************/
    void VkCommandBuffer_Functions::OnDeviceDestroy( VkDevice device )
    {
        // Remove dispatch table for the destroyed device
        Dispatch.DestroyDispatchTable( device );
    }

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
        Profiler& deviceProfiler = DeviceProfilers.at( commandBuffer );

        deviceProfiler.PreDraw( commandBuffer );

        // Invoke next layer's implementation
        Dispatch.GetDispatchTable( commandBuffer ).pfnCmdDraw(
            commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance );

        deviceProfiler.PostDraw( commandBuffer );
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
        Profiler& deviceProfiler = DeviceProfilers.at( commandBuffer );

        deviceProfiler.PreDraw( commandBuffer );

        // Invoke next layer's implementation
        Dispatch.GetDispatchTable( commandBuffer ).pfnCmdDrawIndexed(
            commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );

        deviceProfiler.PostDraw( commandBuffer );
    }
}
