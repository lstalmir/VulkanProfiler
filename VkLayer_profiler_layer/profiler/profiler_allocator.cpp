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

#include "profiler_allocator.h"
#include "profiler_helpers.h"
#include <assert.h>

namespace Profiler
{
    static MemoryProfilerAllocator& GetAllocator( void* pUserData )
    {
        return *static_cast<MemoryProfilerAllocator*>(pUserData);
    }

    MemoryProfilerAllocator::MemoryProfilerAllocator( MemoryProfiler& profiler, const VkAllocationCallbacks* pAllocator, VkObjectType objectType )
        : m_Profiler( profiler )
        , m_pNext( pAllocator )
        , m_ObjectHandle( 0 )
        , m_ObjectType( objectType )
        , m_AllocatedMemorySize( 0 )
        , m_Allocations()
        , m_DeviceMemoryHandle( VK_NULL_HANDLE )
        , m_DeviceMemoryOffset( 0 )
        , m_DeviceMemorySize( 0 )
    {
        pUserData = this;
        pfnAllocation = Allocate;
        pfnReallocation = Reallocate;
        pfnFree = Free;
        pfnInternalAllocation = InternalAllocationNotification;
        pfnInternalFree = InternalFreeNotification;
    }

    void* MemoryProfilerAllocator::Allocate( void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope scope )
    {
        auto& allocator = GetAllocator( pUserData );
        void* pMemory = nullptr;

        if( allocator.m_pNext )
        {
            pMemory = allocator.m_pNext->pfnAllocation( allocator.m_pNext->pUserData, size, alignment, scope );
        }
        else
        {
            pMemory = _aligned_malloc( size, alignment );
        }

        if( pMemory )
        {
            // Track the allocation.
            MemoryProfilerEvent event = {};
            event.m_ObjectType = allocator.m_ObjectType;
            event.m_AllocatedSize = size;

            allocator.m_Profiler.PushEvent( event );

            // Save the allocation in the local array.
            MemoryProfilerSystemAllocationInfo allocationInfo = {};
            allocationInfo.m_Size = size;
            allocationInfo.m_Alignment = alignment;
            allocationInfo.m_Scope = scope;

            allocator.m_Allocations.emplace( pMemory, allocationInfo );
            allocator.m_AllocatedMemorySize += size;
        }

        return pMemory;
    }

    void* MemoryProfilerAllocator::Reallocate( void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope scope )
    {
        auto& allocator = GetAllocator( pUserData );
        void* pMemory = nullptr;

        if( allocator.m_pNext )
        {
            pMemory = allocator.m_pNext->pfnReallocation( allocator.m_pNext->pUserData, pOriginal, size, alignment, scope );
        }
        else
        {
            pMemory = _aligned_realloc( pOriginal, size, alignment );
        }

        if( pMemory )
        {
            // Track the new allocation.
            MemoryProfilerEvent event = {};
            event.m_ObjectType = allocator.m_ObjectType;
            event.m_AllocatedSize = size;

            // Remove the original allocation record.
            auto it = allocator.m_Allocations.find( pOriginal );
            if( it != allocator.m_Allocations.end() )
            {
                size_t originalSize = it->second.m_Size;
                event.m_DeallocatedSize = originalSize;
                allocator.m_AllocatedMemorySize -= originalSize;
                allocator.m_Allocations.erase( pOriginal );
            }

            allocator.m_Profiler.PushEvent( event );

            // Save the allocation in the local array.
            MemoryProfilerSystemAllocationInfo allocationInfo = {};
            allocationInfo.m_Size = size;
            allocationInfo.m_Alignment = alignment;
            allocationInfo.m_Scope = scope;

            allocator.m_Allocations.emplace( pMemory, allocationInfo );
            allocator.m_AllocatedMemorySize += size;
        }

        return pMemory;
    }

    void MemoryProfilerAllocator::Free( void* pUserData, void* pMemory )
    {
        auto& allocator = GetAllocator( pUserData );

        if( allocator.m_pNext )
        {
            allocator.m_pNext->pfnFree( allocator.m_pNext->pUserData, pMemory );
        }
        else
        {
            _aligned_free( pMemory );
        }

        auto it = allocator.m_Allocations.find( pMemory );
        if( it != allocator.m_Allocations.end() )
        {
            size_t size = it->second.m_Size;

            MemoryProfilerEvent event = {};
            event.m_ObjectType = allocator.m_ObjectType;
            event.m_DeallocatedSize = size;

            allocator.m_Profiler.PushEvent( event );

            // Remove the allocation record.
            allocator.m_Allocations.erase( pMemory );
            allocator.m_AllocatedMemorySize -= size;
        }
    }

    void MemoryProfilerAllocator::InternalAllocationNotification( void* pUserData, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope )
    {
        auto& allocator = GetAllocator( pUserData );
        if( allocator.m_pNext )
        {
            allocator.m_pNext->pfnInternalAllocation( allocator.m_pNext->pUserData, size, type, scope );
        }
    }

    void MemoryProfilerAllocator::InternalFreeNotification( void* pUserData, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope )
    {
        auto& allocator = GetAllocator( pUserData );
        if( allocator.m_pNext )
        {
            allocator.m_pNext->pfnInternalFree( allocator.m_pNext->pUserData, size, type, scope );
        }
    }

    void MemoryProfilerAllocator::SetObject( VkObject object )
    {
        assert( m_ObjectType == object.m_Type );
        m_ObjectHandle = object.m_Handle;
    }

    VkObject MemoryProfilerAllocator::GetObject() const
    {
        return VkObject( m_ObjectHandle, m_ObjectType );
    }

    void MemoryProfilerAllocator::SetDeviceMemory( VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size )
    {
        m_DeviceMemoryHandle = memory;
        m_DeviceMemoryOffset = offset;
        m_DeviceMemorySize = size;
    }

    size_t MemoryProfilerAllocator::GetHostAllocationSize() const
    {
        return m_AllocatedMemorySize;
    }

    size_t MemoryProfilerAllocator::GetHostAllocationCount() const
    {
        return m_Allocations.size();
    }

    VkDeviceMemory MemoryProfilerAllocator::GetDeviceAllocation() const
    {
        return m_DeviceMemoryHandle;
    }

    VkDeviceSize MemoryProfilerAllocator::GetDeviceAllocationOffset() const
    {
        return m_DeviceMemoryOffset;
    }

    VkDeviceSize MemoryProfilerAllocator::GetDeviceAllocationSize() const
    {
        return m_DeviceMemorySize;
    }

    MemoryProfiler::MemoryProfiler()
    {
        m_TotalHostMemoryUsage = 0;
        m_TotalDeviceMemoryUsage = 0;
    }

    VkResult MemoryProfiler::Initialize()
    {
        return VK_SUCCESS;
    }

    void MemoryProfiler::Destroy()
    {
        assert( m_pAllocators.empty() );
    }

    std::shared_ptr<MemoryProfilerAllocator> MemoryProfiler::CreateAllocator( const VkAllocationCallbacks* pAllocator, const char* pFunction, VkObjectType objectType )
    {
        return std::make_shared<MemoryProfilerAllocator>( *this, pAllocator, objectType );
    }

    void MemoryProfiler::BindAllocator( VkObject object, std::shared_ptr<MemoryProfilerAllocator> pAllocator )
    {
        m_pAllocators.insert( object, pAllocator );
        pAllocator->SetObject( object );

        std::scoped_lock lock( m_ObjectTypeInternalData );

        auto& data = GetObjectTypeInternalData( object.m_Type );
        data.m_ObjectCount++;
    }

    void MemoryProfiler::DestroyAllocator( VkObject object )
    {
        std::unique_lock allocatorsLock( m_pAllocators );
        std::shared_ptr<MemoryProfilerAllocator> pAllocator;

        auto it = m_pAllocators.unsafe_find( object );
        if( it != m_pAllocators.end() )
        {
            pAllocator = it->second;
            m_pAllocators.unsafe_remove( it );
        }
        allocatorsLock.unlock();

        std::unique_lock dataLock( m_ObjectTypeInternalData );

        auto& data = GetObjectTypeInternalData( object.m_Type );
        data.m_ObjectCount--;

        if( pAllocator )
        {
            data.m_DeviceMemorySize -= pAllocator->GetDeviceAllocationSize();
        }
    }

    void MemoryProfiler::BindDeviceMemory( VkObject object, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size )
    {
        auto pAllocator = m_pAllocators.at( object );
        assert( pAllocator );

        pAllocator->SetDeviceMemory( memory, offset, size );

        std::scoped_lock lock( m_ObjectTypeInternalData );

        auto& data = GetObjectTypeInternalData( object.m_Type );
        data.m_DeviceMemorySize += size;
    }

    void MemoryProfiler::PushEvent( MemoryProfilerEvent& event )
    {
        // If there are any pending events in the queue, the order must not be changed.
        std::unique_lock eventLock( m_EventQueueMutex );
        if( !m_EventQueue.empty() )
        {
            m_EventQueue.push( event );
            return;
        }

        // If the profiler was able to lock the allocations map, apply the changes immediatelly.
        std::unique_lock allocationsLock( m_Data, std::try_to_lock );
        if( allocationsLock.owns_lock() )
        {
            ProcessEvent( event );
            return;
        }

        // Failed to lock m_Allocations map, defer the action to avoid stalling the application.
        m_EventQueue.push( event );
    }

    void MemoryProfiler::ProcessEvents()
    {
        std::unique_lock eventLock( m_EventQueueMutex );

        // Process any pending events.
        while( !m_EventQueue.empty() )
        {
            ProcessEvent( m_EventQueue.front() );
            m_EventQueue.pop();
        }
    }

    void MemoryProfiler::UpdateData( std::chrono::high_resolution_clock::time_point tp )
    {
        // Update the data.
        std::scoped_lock dataLock( m_Data, m_ObjectTypeInternalData );

        // Compute offset of the first object for each object type.
        size_t offset = 0;
        std::unordered_map<VkObjectType, std::pair<size_t, size_t>> objectOffsets;
        for( const auto& [objectType, data] : m_ObjectTypeInternalData )
        {
            objectOffsets[ objectType ] = std::pair( offset, offset );
            offset += data.m_ObjectCount;
        }

        m_Data.m_ObjectData.resize( offset );

        // Get object data.
        for( const auto& [object, pAllocator] : m_pAllocators )
        {
            size_t offset = objectOffsets[ object.m_Type ].second++;
            auto& data = m_Data.m_ObjectData[ offset ];
            data.m_Object = object;
            data.m_HostMemorySize = pAllocator->GetHostAllocationSize();
            data.m_HostMemoryAllocationCount = pAllocator->GetHostAllocationCount();
            data.m_DeviceMemory = pAllocator->GetDeviceAllocation();
            data.m_DeviceMemorySize = pAllocator->GetDeviceAllocationSize();
            data.m_DeviceMemoryOffset = pAllocator->GetDeviceAllocationOffset();
        }

        // Fill object type data.
        m_Data.m_MemoryUsageTimePoints.push_back( tp );
        m_Data.m_TotalMemoryUsageSamples.push_back( m_TotalHostMemoryUsage );

        size_t sampleCount = (m_Data.m_TotalMemoryUsageSamples.size() % m_Data.m_TotalMemoryUsageSamples.capacity());

        for( auto [objectType, data] : m_ObjectTypeInternalData )
        {
            auto emplaced = m_Data.m_ObjectTypeData.try_emplace( objectType );
            auto& objectTypeData = emplaced.first->second;

            if( emplaced.second )
            {
                // Fill the new object type previous samples with zeros.
                for( size_t i = 0; i < sampleCount; ++i )
                {
                    objectTypeData.m_HostMemoryUsageSamples.push_back( 0 );
                    objectTypeData.m_DeviceMemoryUsageSamples.push_back( 0 );
                }
            }

            objectTypeData.m_HostMemorySize = data.m_HostMemorySize;
            objectTypeData.m_HostMemoryAllocationCount = data.m_HostMemoryAllocationCount;
            objectTypeData.m_DeviceMemorySize = data.m_DeviceMemorySize;

            objectTypeData.m_ObjectCount = data.m_ObjectCount;
            objectTypeData.m_pObjects = m_Data.m_ObjectData.data() + objectOffsets[ objectType ].first;

            objectTypeData.m_HostMemoryUsageSamples.push_back( data.m_HostMemorySize );
            objectTypeData.m_DeviceMemoryUsageSamples.push_back( data.m_DeviceMemorySize );
        }
    }

    const MemoryProfilerData& MemoryProfiler::GetData() const
    {
        return m_Data;
    }

    void MemoryProfiler::ProcessEvent( MemoryProfilerEvent& event )
    {
        // m_Allocations map must already be locked.
        m_TotalHostMemoryUsage -= event.m_DeallocatedSize;
        m_TotalHostMemoryUsage += event.m_AllocatedSize;

        auto& data = GetObjectTypeInternalData( event.m_ObjectType );
        data.m_HostMemorySize -= event.m_DeallocatedSize;
        data.m_HostMemorySize += event.m_AllocatedSize;

        if( event.m_AllocatedSize ) data.m_HostMemoryAllocationCount++;
        if( event.m_DeallocatedSize ) data.m_HostMemoryAllocationCount--;
    }

    MemoryProfiler::ObjectTypeInternalData& MemoryProfiler::GetObjectTypeInternalData( VkObjectType type )
    {
        auto it = m_ObjectTypeInternalData.unsafe_find( type );
        if( it == m_ObjectTypeInternalData.end() )
        {
            ObjectTypeInternalData internalData = {};
            internalData.m_ObjectCount = 0;
            internalData.m_HostMemorySize = 0;
            internalData.m_HostMemoryAllocationCount = 0;
            internalData.m_DeviceMemorySize = 0;
            m_ObjectTypeInternalData.unsafe_insert( type, internalData );
        }
        return m_ObjectTypeInternalData.unsafe_at( type );
    }

    MemoryProfilerManager::MemoryProfilerManager()
        : m_Thread()
        , m_ThreadWakeCv()
        , m_ThreadQuitSignal( true )
        , m_InitTime()
        , m_pMemoryProfilers()
    {}

    VkResult MemoryProfilerManager::Initialize()
    {
        m_InitTime = std::chrono::high_resolution_clock::now();

        m_ThreadQuitSignal = false;
        m_Thread = std::thread( &MemoryProfilerManager::ThreadProc, this );

        return VK_SUCCESS;
    }

    void MemoryProfilerManager::Destroy()
    {
        std::unique_lock lock( m_pMemoryProfilers );
        assert( m_pMemoryProfilers.empty() );

        if( m_Thread.joinable() )
        {
            m_ThreadQuitSignal = true;
            m_ThreadWakeCv.notify_one();
            lock.unlock();

            m_Thread.join();
            m_Thread = std::thread();
        }
    }

    void MemoryProfilerManager::RegisterMemoryProfiler( VkObject object, MemoryProfiler* pMemoryProfiler )
    {
        m_pMemoryProfilers.insert( object, pMemoryProfiler );
    }

    void MemoryProfilerManager::UnregisterMemoryProfiler( VkObject object )
    {
        m_pMemoryProfilers.remove( object );
    }

    void MemoryProfilerManager::ThreadProc()
    {
        using clock = std::chrono::high_resolution_clock;
        clock::time_point nextUpdateTime = clock::now();

        while( !m_ThreadQuitSignal )
        {
            std::shared_lock lock( m_pMemoryProfilers );
            std::cv_status cvStatus = m_ThreadWakeCv.wait_until( lock, nextUpdateTime );

            // Process any pending events.
            for( const auto& [_, pProfiler] : m_pMemoryProfilers )
            {
                pProfiler->ProcessEvents();
            }

            clock::time_point currentTime = clock::now();
            if( currentTime < nextUpdateTime )
                continue;

            // Update the profilers.
            for( const auto& [_, pProfiler] : m_pMemoryProfilers )
            {
                pProfiler->UpdateData( currentTime );
            }

            nextUpdateTime += std::chrono::milliseconds( 100 );
        }
    }
}
