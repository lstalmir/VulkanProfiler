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

#include "profiler_memory_comparator.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        DeviceProfilerMemoryComparator

    Description:
        Constructor.

    \***********************************************************************************/
    DeviceProfilerMemoryComparator::DeviceProfilerMemoryComparator()
    {
    }

    /***********************************************************************************\

    Function:
        ~DeviceProfilerMemoryComparator

    Description:
        Destructor.

    \***********************************************************************************/
    DeviceProfilerMemoryComparator::~DeviceProfilerMemoryComparator()
    {
    }

    /***********************************************************************************\

    Function:
        Reset

    Description:
        Reset the comparator to the initial state.

    \***********************************************************************************/
    void DeviceProfilerMemoryComparator::Reset()
    {
        m_pReferenceData = nullptr;
        m_pComparisonData = nullptr;

        m_Results.m_AllocatedBuffers.clear();
        m_Results.m_FreedBuffers.clear();
    }

    /***********************************************************************************\

    Function:
        SetReferenceData

    Description:
        Sets the reference data for comparison.

    \***********************************************************************************/
    void DeviceProfilerMemoryComparator::SetReferenceData( const std::shared_ptr<DeviceProfilerFrameData>& pData )
    {
        if( m_pReferenceData != pData )
        {
            m_pReferenceData = pData;
            m_Dirty = true;
        }
    }

    /***********************************************************************************\

    Function:
        SetComparisonData

    Description:
        Sets the comparison data for comparison.

    \***********************************************************************************/
    void DeviceProfilerMemoryComparator::SetComparisonData( const std::shared_ptr<DeviceProfilerFrameData>& pData )
    {
        if( m_pComparisonData != pData )
        {
            m_pComparisonData = pData;
            m_Dirty = true;
        }
    }

    /***********************************************************************************\

    Function:
        HasValidInput

    Description:
        Checks if the comparator has valid input data for comparison.

    \***********************************************************************************/
    bool DeviceProfilerMemoryComparator::HasValidInput() const
    {
        return m_pReferenceData &&
               m_pComparisonData &&
               ( m_pReferenceData != m_pComparisonData ) &&
               ( m_pReferenceData->m_Memory.m_Heaps.size() == m_pComparisonData->m_Memory.m_Heaps.size() );
    }

    /***********************************************************************************\

    Function:
        GetReferenceData

    Description:
        Returns the reference data.

    \***********************************************************************************/
    std::shared_ptr<DeviceProfilerFrameData> DeviceProfilerMemoryComparator::GetReferenceData() const
    {
        return m_pReferenceData;
    }

    /***********************************************************************************\

    Function:
        GetComparisonData

    Description:
        Returns the comparison data.

    \***********************************************************************************/
    std::shared_ptr<DeviceProfilerFrameData> DeviceProfilerMemoryComparator::GetComparisonData() const
    {
        return m_pComparisonData;
    }

    /***********************************************************************************\

    Function:
        GetResults

    Description:
        Returns the comparison results.
        Performs the comparison if the inputs have changed.

    \***********************************************************************************/
    const DeviceProfilerMemoryComparisonResults& DeviceProfilerMemoryComparator::GetResults()
    {
        if( m_Dirty )
        {
            Compare();
            m_Dirty = false;
        }

        return m_Results;
    }

    /***********************************************************************************\

    Function:
        Compare

    Description:
        Compares the reference and comparison data for memory traces.
        Populates the results with allocated and freed buffers and images.

    \***********************************************************************************/
    void DeviceProfilerMemoryComparator::Compare()
    {
        // Avoid redundant calls.
        assert( m_Dirty );

        m_Results.m_MemoryHeapDifferences.clear();

        m_Results.m_AllocatedBuffers.clear();
        m_Results.m_FreedBuffers.clear();

        if( !HasValidInput() )
        {
            // No data to compare.
            return;
        }

        // Calculate differences in memory heaps.
        const size_t memoryHeapCount = m_pReferenceData->m_Memory.m_Heaps.size();
        for( size_t i = 0; i < memoryHeapCount; ++i )
        {
            const auto& refHeap = m_pReferenceData->m_Memory.m_Heaps[i];
            const auto& cmpHeap = m_pComparisonData->m_Memory.m_Heaps[i];

            auto& diff = m_Results.m_MemoryHeapDifferences.emplace_back();
            diff.m_SizeDifference = cmpHeap.m_AllocationSize - refHeap.m_AllocationSize;
            diff.m_CountDifference = cmpHeap.m_AllocationCount - refHeap.m_AllocationCount;
        }

        // Find differences between the reference and comparison data.
        for( const auto& [buffer, data] : m_pReferenceData->m_Memory.m_Buffers )
        {
            if( !m_pComparisonData->m_Memory.m_Buffers.count( buffer ) )
            {
                // Buffer was freed in the comparison data.
                m_Results.m_FreedBuffers.emplace( buffer, &data );
            }
        }

        for( const auto& [buffer, data] : m_pComparisonData->m_Memory.m_Buffers )
        {
            if( !m_pReferenceData->m_Memory.m_Buffers.count( buffer ) )
            {
                // Buffer was allocated in the comparison data.
                m_Results.m_AllocatedBuffers.emplace( buffer, &data );
            }
        }
    }
}
