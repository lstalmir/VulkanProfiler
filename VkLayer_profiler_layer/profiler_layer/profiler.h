#pragma once
#include "VkDispatch.h"
#include <vulkan/vulkan.h>

namespace Profiler
{
    struct ProfilerCallbacks
    {
        PFN_vkCreateQueryPool   pfnCreateQueryPool;
        PFN_vkCmdWriteTimestamp pfnCmdWriteTimestamp;

        ProfilerCallbacks()
            : pfnCreateQueryPool( nullptr )
            , pfnCmdWriteTimestamp( nullptr )
        {
        }

        ProfilerCallbacks( VkDevice device, PFN_vkGetDeviceProcAddr pfnGetDeviceProcAddr )
            : pfnCreateQueryPool( GETDEVICEPROCADDR( device, vkCreateQueryPool ) )
            , pfnCmdWriteTimestamp( GETDEVICEPROCADDR( device, vkCmdWriteTimestamp ) )
        {
        }
    };

    class Profiler
    {
    public:
        Profiler();

        VkResult Initialize( VkDevice device, ProfilerCallbacks callbacks );

        void PreDraw( VkCommandBuffer );
        void PostDraw( VkCommandBuffer );

    protected:
        VkQueryPool m_TimestampQueryPool;
        ProfilerCallbacks m_Callbacks;
    };
}
