// Copyright (c) 2024 Lukasz Stalmirski
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
#include "profiler_test_icd_base.h"

namespace Profiler::ICD
{
    struct Device;
    struct CommandBuffer;
    struct QueryPool;

    struct Queue : QueueBase
    {
        Queue( Device& device, const VkDeviceQueueCreateInfo& createInfo );
        ~Queue();

        VkResult vkQueueSubmit( uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence ) override;
        VkResult vkQueueSubmit2( uint32_t submitCount, const VkSubmitInfo2* pSubmits, VkFence fence ) override;

#ifdef VK_KHR_swapchain
        VkResult vkQueuePresentKHR( const VkPresentInfoKHR* pPresentInfo ) override;
#endif

        void Exec_CommandBuffer( CommandBuffer& commandBuffer );
        void Exec_Draw( uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance );
        void Exec_Dispatch( uint32_t x, uint32_t y, uint32_t z );
        void Exec_WriteTimestamp( QueryPool& queryPool, uint32_t query );
    };
}
