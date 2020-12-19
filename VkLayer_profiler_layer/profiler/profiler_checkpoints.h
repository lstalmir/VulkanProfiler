// Copyright (c) 2020 Lukasz Stalmirski
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
#include <vulkan/vulkan.h>
#include <algorithm>
#include <mutex>
#include <vector>

namespace Profiler
{
    enum class DeviceProfilerCheckpointType
    {
        eGeneric,
        ePushMarker,
        ePopMarker,
        eBeginRenderPass,
        eEndRenderPass
    };

    // CLASS DeviceProfilerCheckpoint
    class DeviceProfilerCheckpoint
    {
    public:
        DeviceProfilerCheckpoint();

        DeviceProfilerCheckpoint( DeviceProfilerCheckpointType type, const char* pName, size_t nameLength );

        template<size_t NameLength>
        DeviceProfilerCheckpoint( DeviceProfilerCheckpointType type, const char( &pName )[ NameLength ] );

        DeviceProfilerCheckpointType GetType() const;
        const char* GetTypeName() const;
        const char* GetName() const;

    private:
        DeviceProfilerCheckpointType m_Type;
        char m_Name[ 64 ];
    };

    // CLASS DeviceProfilerCheckpointAllocator
    class DeviceProfilerCheckpointAllocator
    {
    public:
        DeviceProfilerCheckpointAllocator( size_t maxCheckpointCount );

        template<typename... Args>
        DeviceProfilerCheckpoint* AllocateCheckpoint( Args&&... args );

    private:
        std::mutex m_CheckpointPoolMutex;
        std::vector<DeviceProfilerCheckpoint> m_CheckpointPool;

        size_t m_AllocationOffset;
        size_t m_AllocationCount;
    };


    inline DeviceProfilerCheckpoint::DeviceProfilerCheckpoint()
        : m_Type( DeviceProfilerCheckpointType::eGeneric )
        , m_Name{ 0 }
    {
    }

    inline DeviceProfilerCheckpoint::DeviceProfilerCheckpoint( DeviceProfilerCheckpointType type, const char* pName, size_t nameLength )
        : m_Type( type )
    {
        strncpy_s( m_Name, pName, nameLength );
    }

    template<size_t NameLength>
    inline DeviceProfilerCheckpoint::DeviceProfilerCheckpoint( DeviceProfilerCheckpointType type, const char( &pName )[ NameLength ] )
        : DeviceProfilerCheckpoint( type, pName, NameLength )
    {
        // This can be compile-time check
        static_assert(NameLength <= std::extent<decltype(m_Name)>::value);
    }

    inline DeviceProfilerCheckpointType DeviceProfilerCheckpoint::GetType() const
    {
        return m_Type;
    }

    inline const char* DeviceProfilerCheckpoint::GetTypeName() const
    {
        switch( m_Type )
        {
        default:
        case DeviceProfilerCheckpointType::eGeneric:
            return "Generic";
        case DeviceProfilerCheckpointType::ePushMarker:
            return "PushMarker";
        case DeviceProfilerCheckpointType::ePopMarker:
            return "PopMarker";
        case DeviceProfilerCheckpointType::eBeginRenderPass:
            return "BeginRenderPass";
        case DeviceProfilerCheckpointType::eEndRenderPass:
            return "EndRenderPass";
        }
    }

    inline const char* DeviceProfilerCheckpoint::GetName() const
    {
        return m_Name;
    }

    inline DeviceProfilerCheckpointAllocator::DeviceProfilerCheckpointAllocator( size_t maxCheckpointCount )
        : m_CheckpointPoolMutex()
        , m_CheckpointPool( maxCheckpointCount )
        , m_AllocationOffset( 0 )
        , m_AllocationCount( 0 )
    {
    }

    template<typename... Args>
    inline DeviceProfilerCheckpoint* DeviceProfilerCheckpointAllocator::AllocateCheckpoint( Args&&... args )
    {
        std::scoped_lock lk( m_CheckpointPoolMutex );

        // Allocate next checkpoint
        DeviceProfilerCheckpoint* pCheckpoint = &m_CheckpointPool[ m_AllocationOffset ];

        // Move to the next element in the pool
        m_AllocationOffset = (m_AllocationOffset + 1) % m_CheckpointPool.size();
        m_AllocationCount = std::max( (m_AllocationCount + 1), m_CheckpointPool.size() );

        return pCheckpoint;
    }
}
