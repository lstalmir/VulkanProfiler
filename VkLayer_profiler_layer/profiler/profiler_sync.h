// Copyright (c) 2019-2021 Lukasz Stalmirski
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once
#include "profiler_layer_objects/VkDevice_object.h"

namespace Profiler
{
    class ProfilerCommandBuffer;

    /***********************************************************************************\

    Class:
        DeviceProfilerSynchronization

    Description:
        Manages device synchronization required for profiling.

    \***********************************************************************************/
    class DeviceProfilerSynchronization
    {
    public:
        DeviceProfilerSynchronization();

        VkResult Initialize( VkDevice_Object* pDevice );
        void Destroy();

        void WaitForDevice();
        void WaitForQueue( VkQueue queue );
        void WaitForFence( VkFence fence, uint64_t timeout = std::numeric_limits<uint64_t>::max() );

        void SendSynchronizationTimestamps();
        std::unordered_map<VkQueue, uint64_t> GetSynchronizationTimestamps() const;

    private:
        VkDevice_Object* m_pDevice;

        // Command pool for each queue family
        std::unordered_map<uint32_t, VkCommandPool> m_CommandPools;
        // Command buffer for each queue
        std::unordered_map<VkQueue, VkCommandBuffer> m_CommandBuffers;

        uint32_t m_TimestampQueryPoolSize;
        VkQueryPool m_TimestampQueryPool;
        // Resources needed to reset query pool
        VkCommandBuffer m_TimestampQueryPoolResetCommandBuffer;
        VkSemaphore m_TimestampQueryPoolResetSemaphore;
        VkQueue m_TimestampQueryPoolResetQueue;

        bool m_SynchronizationTimestampsSent;

        VkResult RecordTimestmapQueryCommandBuffers();
    };
}
