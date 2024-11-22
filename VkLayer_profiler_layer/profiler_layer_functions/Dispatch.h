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
#include <map>
#include <mutex>
#include <memory>

#include "profiler_layer_objects/VkObject.h"

namespace Profiler
{
    // Compile-time function signature check
    #define CHECKPROCSIGNATURE( NAME )                                                  \
        {                                                                               \
            using ActualProcT = std::remove_pointer_t<decltype(NAME)>;                  \
            using ExpectedProcT = std::remove_pointer_t<PFN_vk##NAME>;                  \
            static_assert(std::is_same_v<ExpectedProcT, ActualProcT>,                   \
                #NAME " function signature mismatch (see vk" #NAME ")" );               \
        }

    // Helper macro for getting address of function implementation
    #define GETPROCADDR( NAME )                                                         \
        CHECKPROCSIGNATURE( NAME );                                                     \
        if( strcmp( pName, "vk" #NAME ) == 0 )                                          \
        {                                                                               \
            return reinterpret_cast<PFN_vkVoidFunction>(NAME);                          \
        }

    #define GETPROCADDR_EXT( NAME )                                                     \
        if( strcmp( pName, #NAME ) == 0 )                                               \
        {                                                                               \
            return reinterpret_cast<PFN_vkVoidFunction>(NAME);                          \
        }

    /***********************************************************************************\

    Class:
        Dispatch

    Description:
        Object manager, stores dispatch tables for each instance created with this layer
        enabled.

    \***********************************************************************************/
    template<typename ValueType>
    class DispatchableMap
    {
    public:
        /*******************************************************************************\

        Function:
            Get

        Description:
            Retrieves layer dispatch table from the dispatcher object.

        \*******************************************************************************/
        template<typename VkObjectT>
        inline auto Get( VkObjectT handle ) ->
            std::enable_if_t<VkObject_Traits<VkObjectT>::IsDispatchable, ValueType>&
        {
            std::scoped_lock lk( m_DispatchMutex );

            auto it = m_Dispatch.find( handle );
            if( it == m_Dispatch.end() )
            {
                // TODO error
            }

            return *(it->second);
        }

        /*******************************************************************************\

        Function:
            Insert

        Description:
            Creates new layer dispatch table and stores it in the dispatcher object.

        \*******************************************************************************/
        template<typename VkObjectT, typename... Args>
        inline auto Create( VkObjectT handle ) ->
            std::enable_if_t<VkObject_Traits<VkObjectT>::IsDispatchable, ValueType>&
        {
            std::scoped_lock lk( m_DispatchMutex );

            ValueType* pTable = new ValueType;

            auto it = m_Dispatch.emplace( handle, pTable );
            if( !it.second )
            {
                // TODO error, should have created new value
            }

            return *pTable;
        }

        /*******************************************************************************\

        Function:
            Erase

        Description:
            Removes layer dispatch table from the dispatcher object.

        \*******************************************************************************/
        template<typename VkObjectT>
        inline auto Erase( VkObjectT handle ) ->
            std::enable_if_t<VkObject_Traits<VkObjectT>::IsDispatchable, void>
        {
            std::scoped_lock lk( m_DispatchMutex );

            auto it = m_Dispatch.find( handle );
            if( it != m_Dispatch.end() )
            {
                delete it->second;

                m_Dispatch.erase( it );
            }
        }

    private:
        /*******************************************************************************\

          Comparators and hashers for use of raw dispatchable Vulkan objects in
          STL containers.

          All dispatchable objects have a pointer to the dispatch table at the offset 0.
          It can be used to match different pointers that should share the same dispatch
          table, e.g.:
          - VkInstance and VkPhysicalDevices
          - VkDevice and VkQueues, VkCommandBuffers...

        \*******************************************************************************/
        struct VkObjectHash { inline size_t operator()( void* a ) const { return reinterpret_cast<size_t>(*(const void**)(a)); } };
        struct VkObjectEqual { inline bool operator()( void* a, void* b ) const { return *(const void**)(a) == *(const void**)(b); } };
        struct VkObjectLess { inline bool operator()( void* a, void* b ) const { return*(const void**)(a) < *(const void**)(b); } };

        std::map<void*, ValueType*, VkObjectLess> m_Dispatch;

        mutable std::mutex m_DispatchMutex;
    };
}
