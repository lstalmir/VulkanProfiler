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
#include "vk_dispatch_tables.h"
#include <stdint.h>
#include <shared_mutex>

namespace Profiler
{
    struct VkQueue_Object
    {
        VkQueue Handle;
        VkQueueFlags Flags;
        uint32_t Family;
        uint32_t Index;
        std::shared_mutex Mutex;

        VkQueue_Object( VkQueue queue, VkQueueFlags flags, uint32_t family, uint32_t index )
            : Handle( queue )
            , Flags( flags )
            , Family( family )
            , Index( index )
        {
        }
    };

    struct VkQueue_Object_Scope
    {
        // Store the pointer to the current queue object to reduce locks.
        inline static thread_local VkQueue_Object* pCurrentQueue = nullptr;

        std::shared_lock<std::shared_mutex> Lock;

        VkQueue_Object_Scope( VkQueue_Object& queueObject )
            : Lock( queueObject.Mutex )
        {
            // Prevent additional locking in the thread that already holds the lock.
            pCurrentQueue = &queueObject;
        }

        ~VkQueue_Object_Scope()
        {
            pCurrentQueue = nullptr;
        }
    };

    struct VkQueue_Object_InternalScope
    {
        std::unique_lock<std::shared_mutex> Lock;

        VkQueue_Object_InternalScope( VkQueue_Object& queueObject )
            : Lock( queueObject.Mutex, std::defer_lock )
        {
            if( &queueObject != VkQueue_Object_Scope::pCurrentQueue )
            {
                Lock.lock();
            }
        }
    };
}
