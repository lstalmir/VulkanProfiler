#include "VkDevice_functions.h"
#include "VkInstance_functions.h"
#include "VkCommandBuffer_functions.h"
#include "VkQueue_functions.h"
#include "VkLayer_profiler_layer.generated.h"

namespace Profiler
{
    // Define VkDevice dispatch tables map
    VkDispatch<VkDevice, VkDevice_Functions::DispatchTable> VkDevice_Functions::DeviceFunctions;

    // Define VkDevice profilers map
    VkDispatchableMap<VkDevice, Profiler*> VkDevice_Functions::DeviceProfilers;

    // Define VkDevice_Objects map
    VkDispatchableMap<VkDevice, VkDevice_Object> VkDevice_Functions::DeviceObjects;

    /***********************************************************************************\

    Function:
        GetProcAddr

    Description:
        Gets address of this layer's function implementation.

    \***********************************************************************************/
    PFN_vkVoidFunction VkDevice_Functions::GetInterceptedProcAddr( const char* pName )
    {
        // Intercepted functions
        GETPROCADDR( GetDeviceProcAddr );
        GETPROCADDR( CreateDevice );
        GETPROCADDR( DestroyDevice );
        GETPROCADDR( EnumerateDeviceLayerProperties );
        GETPROCADDR( EnumerateDeviceExtensionProperties );

        // CommandBuffer functions
        if( PFN_vkVoidFunction function = VkCommandBuffer_Functions::GetInterceptedProcAddr( pName ) )
        {
            return function;
        }

        // Queue functions
        if( PFN_vkVoidFunction function = VkQueue_Functions::GetInterceptedProcAddr( pName ) )
        {
            return function;
        }

        // Function not overloaded
        return nullptr;
    }

    /***********************************************************************************\

    Function:
        GetProcAddr

    Description:
        Gets pointer to the VkDevice function implementation.

    \***********************************************************************************/
    PFN_vkVoidFunction VkDevice_Functions::GetProcAddr( VkDevice device, const char* pName )
    {
        return GetDeviceProcAddr( device, pName );
    }


    /***********************************************************************************\

    Function:
        GetDeviceProcAddr

    Description:
        Gets pointer to the VkDevice function implementation.

    \***********************************************************************************/
    VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL VkDevice_Functions::GetDeviceProcAddr(
        VkDevice device,
        const char* pName )
    {
        // Overloaded functions
        if( PFN_vkVoidFunction function = GetInterceptedProcAddr( pName ) )
        {
            return function;
        }

        // Get address from the next layer
        return DeviceFunctions[device].pfnGetDeviceProcAddr( device, pName );
    }

    /***********************************************************************************\

    Function:
        CreateDevice

    Description:
        Creates VkDevice object and initializes dispatch table.

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::CreateDevice(
        VkPhysicalDevice physicalDevice,
        const VkDeviceCreateInfo* pCreateInfo,
        VkAllocationCallbacks* pAllocator,
        VkDevice* pDevice )
    {
        const VkLayerDeviceCreateInfo* pLayerCreateInfo =
            reinterpret_cast<const VkLayerDeviceCreateInfo*>(pCreateInfo->pNext);

        // Step through the chain of pNext until we get to the link info
        while( (pLayerCreateInfo)
            && (pLayerCreateInfo->sType != VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO ||
                pLayerCreateInfo->function != VK_LAYER_LINK_INFO) )
        {
            pLayerCreateInfo = reinterpret_cast<const VkLayerDeviceCreateInfo*>(pLayerCreateInfo->pNext);
        }

        if( !pLayerCreateInfo )
        {
            // No loader device create info
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        // Get pointers to next layer's vkGetInstanceProcAddr and vkGetDeviceProcAddr
        PFN_vkGetInstanceProcAddr pfnGetInstanceProcAddr =
            pLayerCreateInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr;
        PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr =
            pLayerCreateInfo->u.pLayerInfo->pfnNextGetDeviceProcAddr;

        PFN_vkCreateDevice pfnCreateDevice = GETINSTANCEPROCADDR( VK_NULL_HANDLE, vkCreateDevice );

        // Invoke vkCreateDevice of next layer
        VkResult result = pfnCreateDevice( physicalDevice, pCreateInfo, pAllocator, pDevice );

        if( result == VK_SUCCESS )
        {
            VkDevice_Object deviceObject;
            deviceObject.Device = *pDevice;
            deviceObject.PhysicalDevice = physicalDevice;
            deviceObject.pfnGetDeviceProcAddr = pfnGetDeviceProcAddr;
            deviceObject.pfnGetInstanceProcAddr = pfnGetInstanceProcAddr;

            // Register queues created with the device
            PFN_vkGetDeviceQueue pfnGetDeviceQueue = GETDEVICEPROCADDR( *pDevice, vkGetDeviceQueue );

            // Iterate over pCreateInfo for queue infos
            for( uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; ++i )
            {
                const VkDeviceQueueCreateInfo* pQueueCreateInfo = pCreateInfo->pQueueCreateInfos + i;

                for( uint32_t queueIndex = 0; queueIndex < pQueueCreateInfo->queueCount; ++queueIndex )
                {
                    VkQueue_Object queueObject;
                    queueObject.Device = *pDevice;
                    queueObject.Index = queueIndex;
                    queueObject.FamilyIndex = pQueueCreateInfo->queueFamilyIndex;
                    queueObject.Priority = pQueueCreateInfo->pQueuePriorities[queueIndex];

                    pfnGetDeviceQueue( *pDevice, queueObject.FamilyIndex, queueIndex, &queueObject.Queue );

                    deviceObject.Queues.emplace( queueObject.Queue, queueObject );
                }
            }

            std::scoped_lock lk( DeviceObjects, DeviceProfilers );

            auto deviceObjectIt = DeviceObjects.try_emplace( *pDevice, deviceObject );
            if( !deviceObjectIt.second )
            {
                // TODO error, should have created new element
            }

            // Setup callbacks for the profiler
            ProfilerCallbacks deviceProfilerCallbacks;
            memset( &deviceProfilerCallbacks, 0, sizeof( deviceProfilerCallbacks ) );

            deviceProfilerCallbacks.pfnGetPhysicalDeviceQueueFamilyProperties =
                GETINSTANCEPROCADDR( reinterpret_cast<VkInstance>(physicalDevice), vkGetPhysicalDeviceQueueFamilyProperties );
            deviceProfilerCallbacks.pfnGetPhysicalDeviceMemoryProperties =
                GETINSTANCEPROCADDR( reinterpret_cast<VkInstance>(physicalDevice), vkGetPhysicalDeviceMemoryProperties );
            deviceProfilerCallbacks.pfnGetImageMemoryRequirements = GETDEVICEPROCADDR( *pDevice, vkGetImageMemoryRequirements );
            deviceProfilerCallbacks.pfnGetBufferMemoryRequirements = GETDEVICEPROCADDR( *pDevice, vkGetBufferMemoryRequirements );
            deviceProfilerCallbacks.pfnAllocateMemory = GETDEVICEPROCADDR( *pDevice, vkAllocateMemory );
            deviceProfilerCallbacks.pfnFreeMemory = GETDEVICEPROCADDR( *pDevice, vkFreeMemory );
            deviceProfilerCallbacks.pfnBindImageMemory = GETDEVICEPROCADDR( *pDevice, vkBindImageMemory );
            deviceProfilerCallbacks.pfnBindBufferMemory = GETDEVICEPROCADDR( *pDevice, vkBindBufferMemory );
            deviceProfilerCallbacks.pfnMapMemory = GETDEVICEPROCADDR( *pDevice, vkMapMemory );
            deviceProfilerCallbacks.pfnUnmapMemory = GETDEVICEPROCADDR( *pDevice, vkUnmapMemory );
            deviceProfilerCallbacks.pfnCreateBuffer = GETDEVICEPROCADDR( *pDevice, vkCreateBuffer );
            deviceProfilerCallbacks.pfnDestroyBuffer = GETDEVICEPROCADDR( *pDevice, vkDestroyBuffer );
            deviceProfilerCallbacks.pfnCreateQueryPool = GETDEVICEPROCADDR( *pDevice, vkCreateQueryPool );
            deviceProfilerCallbacks.pfnDestroyQueryPool = GETDEVICEPROCADDR( *pDevice, vkDestroyQueryPool );
            deviceProfilerCallbacks.pfnCreateRenderPass = GETDEVICEPROCADDR( *pDevice, vkCreateRenderPass );
            deviceProfilerCallbacks.pfnDestroyRenderPass = GETDEVICEPROCADDR( *pDevice, vkDestroyRenderPass );
            deviceProfilerCallbacks.pfnCreatePipelineLayout = GETDEVICEPROCADDR( *pDevice, vkCreatePipelineLayout );
            deviceProfilerCallbacks.pfnDestroyPipelineLayout = GETDEVICEPROCADDR( *pDevice, vkDestroyPipelineLayout );
            deviceProfilerCallbacks.pfnCreateShaderModule = GETDEVICEPROCADDR( *pDevice, vkCreateShaderModule );
            deviceProfilerCallbacks.pfnDestroyShaderModule = GETDEVICEPROCADDR( *pDevice, vkDestroyShaderModule );
            deviceProfilerCallbacks.pfnCreateGraphicsPipelines = GETDEVICEPROCADDR( *pDevice, vkCreateGraphicsPipelines );
            deviceProfilerCallbacks.pfnDestroyPipeline = GETDEVICEPROCADDR( *pDevice, vkDestroyPipeline );
            deviceProfilerCallbacks.pfnCreateImage = GETDEVICEPROCADDR( *pDevice, vkCreateImage );
            deviceProfilerCallbacks.pfnDestroyImage = GETDEVICEPROCADDR( *pDevice, vkDestroyImage );
            deviceProfilerCallbacks.pfnCreateImageView = GETDEVICEPROCADDR( *pDevice, vkCreateImageView );
            deviceProfilerCallbacks.pfnDestroyImageView = GETDEVICEPROCADDR( *pDevice, vkDestroyImageView );
            deviceProfilerCallbacks.pfnCreateCommandPool = GETDEVICEPROCADDR( *pDevice, vkCreateCommandPool );
            deviceProfilerCallbacks.pfnDestroyCommandPool = GETDEVICEPROCADDR( *pDevice, vkDestroyCommandPool );
            deviceProfilerCallbacks.pfnAllocateCommandBuffers = GETDEVICEPROCADDR( *pDevice, vkAllocateCommandBuffers );
            deviceProfilerCallbacks.pfnFreeCommandBuffers = GETDEVICEPROCADDR( *pDevice, vkFreeCommandBuffers );
            deviceProfilerCallbacks.pfnBeginCommandBuffer = GETDEVICEPROCADDR( *pDevice, vkBeginCommandBuffer );
            deviceProfilerCallbacks.pfnEndCommandBuffer = GETDEVICEPROCADDR( *pDevice, vkEndCommandBuffer );
            deviceProfilerCallbacks.pfnCmdWriteTimestamp = GETDEVICEPROCADDR( *pDevice, vkCmdWriteTimestamp );
            deviceProfilerCallbacks.pfnCmdPipelineBarrier = GETDEVICEPROCADDR( *pDevice, vkCmdPipelineBarrier );
            deviceProfilerCallbacks.pfnCmdCopyBufferToImage = GETDEVICEPROCADDR( *pDevice, vkCmdCopyBufferToImage );
            deviceProfilerCallbacks.pfnQueueSubmit = GETDEVICEPROCADDR( *pDevice, vkQueueSubmit );
            deviceProfilerCallbacks.pfnQueueWaitIdle = GETDEVICEPROCADDR( *pDevice, vkQueueWaitIdle );

            // Create the profiler object
            Profiler* deviceProfiler = new Profiler;
            result = deviceProfiler->Initialize( &deviceObject, deviceProfilerCallbacks );

            if( result != VK_SUCCESS )
            {
                delete deviceProfiler;

                PFN_vkDestroyDevice pfnDestroyDevice = GETDEVICEPROCADDR( *pDevice, vkDestroyDevice );

                // Destroy the device
                pfnDestroyDevice( *pDevice, pAllocator );

                return result;
            }

            auto deviceProfilerIt = DeviceProfilers.try_emplace( *pDevice, deviceProfiler );
            if( !deviceProfilerIt.second )
            {
                // TODO error, should have created new element
            }

            DeviceFunctions.CreateDispatchTable( *pDevice, pfnGetDeviceProcAddr );

            // Initialize VkCommandBuffer functions for the device
            VkCommandBuffer_Functions::OnDeviceCreate( *pDevice, pfnGetDeviceProcAddr );

            // Initialize VkQueue functions for this device
            VkQueue_Functions::OnDeviceCreate( *pDevice, pfnGetDeviceProcAddr );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        DestroyDevice

    Description:
        Removes dispatch table associated with the VkDevice object.

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkDevice_Functions::DestroyDevice(
        VkDevice device,
        VkAllocationCallbacks pAllocator )
    {
        DeviceFunctions.DestroyDispatchTable( device );

        std::scoped_lock lk( DeviceProfilers );

        auto it = DeviceProfilers.find( device );
        if( it != DeviceProfilers.end() )
        {
            // Cleanup the profiler resources
            it->second->Destroy();

            DeviceProfilers.erase( it );
        }

        // Cleanup VkCommandBuffer function callbacks
        VkCommandBuffer_Functions::OnDeviceDestroy( device );

        // Cleanup VkQueue function callbacks
        VkQueue_Functions::OnDeviceDestroy( device );
    }

    /***********************************************************************************\

    Function:
        EnumerateDeviceLayerProperties

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::EnumerateDeviceLayerProperties(
        uint32_t* pPropertyCount,
        VkLayerProperties* pLayerProperties )
    {
        return VkInstance_Functions::EnumerateInstanceLayerProperties( pPropertyCount, pLayerProperties );
    }

    /***********************************************************************************\

    Function:
        EnumerateDeviceExtensionProperties

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkDevice_Functions::EnumerateDeviceExtensionProperties(
        VkPhysicalDevice physicalDevice,
        const char* pLayerName,
        uint32_t* pPropertyCount,
        VkExtensionProperties* pProperties )
    {
        // Pass through any queries that aren't to us
        if( !pLayerName || strcmp( pLayerName, VK_LAYER_profiler_name ) != 0 )
        {
            if( physicalDevice == VK_NULL_HANDLE )
                return VK_SUCCESS;

            // EnumerateDeviceExtensionProperties is actually VkInstance (VkPhysicalDevice) function.
            // Get dispatch table associated with the VkPhysicalDevice and invoke next layer's
            // vkEnumerateDeviceExtensionProperties implementation.
            auto instanceDispatchTable = VkInstance_Functions::InstanceFunctions[physicalDevice];

            return instanceDispatchTable.pfnEnumerateDeviceExtensionProperties(
                physicalDevice, pLayerName, pPropertyCount, pProperties );
        }

        // Don't expose any extensions
        if( pPropertyCount )
        {
            (*pPropertyCount) = 0;
        }

        return VK_SUCCESS;
    }
}
