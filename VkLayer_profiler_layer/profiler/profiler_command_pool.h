// Copyright (c) 2019-2025 Lukasz Stalmirski
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
#include <mutex>
#include <vulkan/vulkan.h>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        DeviceProfilerCommandPool

    Description:
        Wrapper for VkCommandPool object.

    \***********************************************************************************/
    class DeviceProfilerCommandPool
    {
    public:
        DeviceProfilerCommandPool( class DeviceProfiler&, VkCommandPool, const VkCommandPoolCreateInfo& );

        DeviceProfilerCommandPool( const DeviceProfilerCommandPool& ) = delete;

        VkCommandPool GetHandle() const;
        uint32_t GetQueueFamilyIndex() const;
        bool SupportsTimestampQuery() const;

    private:
        VkCommandPool m_CommandPool;
        uint32_t m_QueueFamilyIndex;
        bool m_SupportsTimestampQuery;
    };

    /***********************************************************************************\

    Class:
        DeviceProfilerInternalCommandPool

    Description:
        Wrapper for internally allocated VkCommandPool object.

    \***********************************************************************************/
    class DeviceProfilerInternalCommandPool
        : public DeviceProfilerCommandPool
    {
    public:
        DeviceProfilerInternalCommandPool( class DeviceProfiler&, VkCommandPool, const VkCommandPoolCreateInfo& );
        ~DeviceProfilerInternalCommandPool();

        std::mutex& GetMutex();

    private:
        class DeviceProfiler& m_Profiler;
        std::mutex m_Mutex;
    };
}
