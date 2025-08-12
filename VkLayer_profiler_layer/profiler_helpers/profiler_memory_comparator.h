// Copyright (c) 2025 Lukasz Stalmirski
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
#include "profiler/profiler_data.h"
#include <memory>
#include <unordered_map>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        DeviceProfilerMemoryComparisonResults

    Description:
        Memory trace comparison results.

    \***********************************************************************************/
    struct DeviceProfilerMemoryComparisonResults
    {
        struct Difference
        {
            int64_t m_SizeDifference;
            int64_t m_CountDifference;
        };

        std::vector<Difference> m_MemoryHeapDifferences;

        std::unordered_map<VkObjectHandle<VkBuffer>, const DeviceProfilerBufferMemoryData*> m_AllocatedBuffers;
        std::unordered_map<VkObjectHandle<VkBuffer>, const DeviceProfilerBufferMemoryData*> m_FreedBuffers;

        std::unordered_map<VkObjectHandle<VkImage>, const DeviceProfilerImageMemoryData*> m_AllocatedImages;
        std::unordered_map<VkObjectHandle<VkImage>, const DeviceProfilerImageMemoryData*> m_FreedImages;

        std::unordered_map<VkObjectHandle<VkAccelerationStructureKHR>, const DeviceProfilerAccelerationStructureMemoryData*> m_AllocatedAccelerationStructures;
        std::unordered_map<VkObjectHandle<VkAccelerationStructureKHR>, const DeviceProfilerAccelerationStructureMemoryData*> m_FreedAccelerationStructures;
    };

    /***********************************************************************************\

    Class:
        DeviceProfilerMemoryComparator

    Description:
        Compares two memory traces.

    \***********************************************************************************/
    class DeviceProfilerMemoryComparator
    {
    public:
        DeviceProfilerMemoryComparator();
        ~DeviceProfilerMemoryComparator();

        void Reset();

        void SetReferenceData( const std::shared_ptr<DeviceProfilerFrameData>& pData );
        void SetComparisonData( const std::shared_ptr<DeviceProfilerFrameData>& pData );

        bool HasValidInput() const;

        std::shared_ptr<DeviceProfilerFrameData> GetReferenceData() const;
        std::shared_ptr<DeviceProfilerFrameData> GetComparisonData() const;
        const DeviceProfilerMemoryComparisonResults& GetResults();

    private:
        std::shared_ptr<DeviceProfilerFrameData> m_pReferenceData;
        std::shared_ptr<DeviceProfilerFrameData> m_pComparisonData;

        // Comparison results.
        DeviceProfilerMemoryComparisonResults m_Results;

        // Comparison is deferred until the results are requested to save CPU cycles.
        bool m_Dirty = false;
        void Compare();
    };
}
