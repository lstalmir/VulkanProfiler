#include "VkQueue_functions.h"

namespace Profiler
{
    // Define VkQueue dispatch tables map
    VkDispatch<VkDevice, VkQueue_Functions::DispatchTable> VkQueue_Functions::QueueFunctions;

    /***********************************************************************************\

    Function:
        GetProcAddr

    Description:
        Gets address of this layer's function implementation.

    \***********************************************************************************/
    PFN_vkVoidFunction VkQueue_Functions::GetInterceptedProcAddr( const char* pName )
    {
        // Intercepted functions
        GETPROCADDR( QueuePresentKHR );
        GETPROCADDR( QueueSubmit );

        // Function not overloaded
        return nullptr;
    }

    /***********************************************************************************\

    Function:
        GetProcAddr

    Description:
        Gets address of this layer's function implementation.

    \***********************************************************************************/
    PFN_vkVoidFunction VkQueue_Functions::GetProcAddr( VkDevice device, const char* pName )
    {
        // Overloaded functions
        if( PFN_vkVoidFunction function = GetInterceptedProcAddr( pName ) )
        {
            return function;
        }

        // Get address from the next layer
        auto dispatchTable = DeviceFunctions.GetDispatchTable( device );

        return dispatchTable.pfnGetDeviceProcAddr( device, pName );
    }

    /***********************************************************************************\

    Function:
        OnDeviceCreate

    Description:
        Initializes VkCommandBuffer function callbacks for new device.

    \***********************************************************************************/
    void VkQueue_Functions::OnDeviceCreate( VkDevice device, PFN_vkGetDeviceProcAddr gpa )
    {
        // Create dispatch table for overloaded VkCommandBuffer functions
        QueueFunctions.CreateDispatchTable( device, gpa );
    }

    /***********************************************************************************\

    Function:
        OnDeviceCreate

    Description:
        Removes VkCommandBuffer function callbacks for the device from the memory.

    \***********************************************************************************/
    void VkQueue_Functions::OnDeviceDestroy( VkDevice device )
    {
        // Remove dispatch table for the destroyed device
        QueueFunctions.DestroyDispatchTable( device );
    }

    /***********************************************************************************\

    Function:
        QueuePresentKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkQueue_Functions::QueuePresentKHR(
        VkQueue queue,
        const VkPresentInfoKHR* pPresentInfo )
    {
        Profiler* deviceProfiler = DeviceProfilers.at( queue );

        deviceProfiler->PrePresent( queue );

        // Present the image
        VkResult result = QueueFunctions.GetDispatchTable( queue ).pfnQueuePresentKHR(
            queue, pPresentInfo );

        deviceProfiler->PostPresent( queue );

        return result;
    }

    /***********************************************************************************\

    Function:
        QueueSubmit

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkQueue_Functions::QueueSubmit(
        VkQueue queue,
        uint32_t submitCount,
        const VkSubmitInfo* pSubmits,
        VkFence fence )
    {
        Profiler* deviceProfiler = DeviceProfilers.at( queue );

        // Increment submit counter
        deviceProfiler->GetCurrentFrameStats().submitCount += submitCount;

        // Submit the command buffers
        VkResult result = QueueFunctions.GetDispatchTable( queue ).pfnQueueSubmit(
            queue, submitCount, pSubmits, fence );

        return result;
    }
}
