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
#include <vulkan/vulkan.h>

#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <memory>

#include "profiler_counters.h"
#include "utils/lockable_unordered_map.h"
#include "utils/ring_buffer.h"
#include "utils/concurrency.h"
#include "profiler_layer_objects/VkObject.h"

namespace Profiler
{
    class MemoryProfilerAllocator;

    struct MemoryProfilerSystemAllocationInfo
    {
        size_t                               m_Size;
        size_t                               m_Alignment : 32;
        size_t                               m_Scope : 32;
    };

    struct MemoryProfilerEvent
    {
        VkObjectType                         m_ObjectType;
        size_t                               m_AllocatedSize;
        size_t                               m_DeallocatedSize;
    };

    struct MemoryProfilerObjectData
    {
        VkObject                             m_Object;
        size_t                               m_HostMemorySize;
        size_t                               m_HostMemoryAllocationCount;
        VkDeviceMemory                       m_DeviceMemory;
        VkDeviceSize                         m_DeviceMemorySize;
        VkDeviceSize                         m_DeviceMemoryOffset;
    };

    struct MemoryProfilerObjectTypeData
    {
        const MemoryProfilerObjectData*      m_pObjects;
        size_t                               m_ObjectCount;
        size_t                               m_HostMemorySize;
        size_t                               m_HostMemoryAllocationCount;
        VkDeviceSize                         m_DeviceMemorySize;
        RingBuffer<size_t>                   m_HostMemoryUsageSamples{ 128 };
        RingBuffer<VkDeviceSize>             m_DeviceMemoryUsageSamples{ 128 };
    };

    struct MemoryProfilerData : public SharedLockable<>
    {
        using Clock = std::chrono::high_resolution_clock;
        RingBuffer<Clock::time_point>        m_MemoryUsageTimePoints{ 128 };
        RingBuffer<size_t>                   m_TotalMemoryUsageSamples{ 128 };
        std::unordered_map<VkObjectType, MemoryProfilerObjectTypeData> m_ObjectTypeData;
        std::vector<MemoryProfilerObjectData> m_ObjectData;
    };

    /***********************************************************************************\

    Class:
        MemoryProfilerAllocator

    Description:
        Implementation of VkAllocationCallbacks that is injected between the callbacks
        provided by the layer above, or application, and the ICD.

        It's main goal is to log all allocations made by the layers below the profiler
        to the MemoryProfiler. Along with the data supplied to the callback, it also
        stores the type of the object that it was originally intended for, and name
        of the function that created the allocator.

        Once logged, the allocator passes the control to the original callback, or
        performs _aligned_malloc, _aligned_realloc or _aligned_free.

    \***********************************************************************************/
    class MemoryProfilerAllocator : public VkAllocationCallbacks
        , std::enable_shared_from_this<MemoryProfilerAllocator>
    {
    public:
        explicit MemoryProfilerAllocator( class MemoryProfiler& profiler, const VkAllocationCallbacks* pAllocator, VkObjectType objectType );

        static void* Allocate( void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope scope );
        static void* Reallocate( void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope scope );
        static void Free( void* pUserData, void* pMemory );
        static void InternalAllocationNotification( void* pUserData, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope );
        static void InternalFreeNotification( void* pUserData, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope );

        void SetObject( VkObject object );
        VkObject GetObject() const;

        void SetDeviceMemory( VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size );

        size_t GetHostAllocationSize() const;
        size_t GetHostAllocationCount() const;

        VkDeviceMemory GetDeviceAllocation() const;
        VkDeviceSize GetDeviceAllocationOffset() const;
        VkDeviceSize GetDeviceAllocationSize() const;

    private:
        class MemoryProfiler&        m_Profiler;
        const VkAllocationCallbacks* m_pNext;

        uint64_t                     m_ObjectHandle;
        VkObjectType                 m_ObjectType;

        VkDeviceMemory               m_DeviceMemoryHandle;
        VkDeviceSize                 m_DeviceMemoryOffset;
        VkDeviceSize                 m_DeviceMemorySize;

        size_t                       m_AllocatedMemorySize;
        std::unordered_map<void*, MemoryProfilerSystemAllocationInfo> m_Allocations;
    };

    /***********************************************************************************\

    Class:
        MemoryProfiler

    Description:
        Keeps track of all host memory allocations and provides summarized inforamtion
        on the current memory usage.

    \***********************************************************************************/
    class MemoryProfiler
    {
        struct ObjectTypeInternalData
        {
            size_t                           m_HostMemorySize;
            size_t                           m_HostMemoryAllocationCount;
            size_t                           m_ObjectCount;
            VkDeviceSize                     m_DeviceMemorySize;
        };

    public:
        MemoryProfiler();

        VkResult Initialize();
        void Destroy();

        std::shared_ptr<MemoryProfilerAllocator> CreateAllocator( const VkAllocationCallbacks* pAllocator, const char* pFunction, VkObjectType objectType );
        void BindAllocator( VkObject object, std::shared_ptr<MemoryProfilerAllocator> pAllocator );
        void DestroyAllocator( VkObject object );

        void BindDeviceMemory( VkObject object, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size );

        void PushEvent( MemoryProfilerEvent& event );
        void ProcessEvents();

        void UpdateData( std::chrono::high_resolution_clock::time_point tp );

        const MemoryProfilerData& GetData() const;

    private:
        MemoryProfilerData                   m_Data;

        std::mutex                           m_EventQueueMutex;
        std::queue<MemoryProfilerEvent>      m_EventQueue;

        size_t                               m_TotalHostMemoryUsage;
        size_t                               m_TotalDeviceMemoryUsage;
        ConcurrentMap<VkObjectType, ObjectTypeInternalData>  m_ObjectTypeInternalData;

        ConcurrentMap<VkObject, std::shared_ptr<MemoryProfilerAllocator>> m_pAllocators;

        void ProcessEvent( MemoryProfilerEvent& event );

        ObjectTypeInternalData& GetObjectTypeInternalData( VkObjectType type );
    };

    /***********************************************************************************\

    Class:
        MemoryProfilerManager

    Description:
        Synchronizes multiple memory profilers to provide data in regular time intervals.

    \***********************************************************************************/
    class MemoryProfilerManager
    {
    public:
        MemoryProfilerManager();

        VkResult Initialize();
        void Destroy();

        void RegisterMemoryProfiler( VkObject parent, MemoryProfiler* pMemoryProfiler );
        void UnregisterMemoryProfiler( VkObject parent );

        auto GetInitTime() const { return m_InitTime; }

        void Pause( bool pause ) { m_ThreadPaused = pause; }

        auto GetUpdateInterval() const { return m_ThreadUpdateInterval; }

        template<typename DurationT>
        void SetUpdateInterval( DurationT interval ) { m_ThreadUpdateInterval = interval; }

    private:
        std::thread                 m_Thread;
        std::condition_variable_any m_ThreadWakeCv;
        bool                        m_ThreadQuitSignal;
        bool                        m_ThreadPaused;
        std::chrono::nanoseconds    m_ThreadUpdateInterval;

        std::chrono::high_resolution_clock::time_point m_InitTime;

        ConcurrentMap<VkObject, MemoryProfiler*> m_pMemoryProfilers;

        void ThreadProc();
    };
}
