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
#include <stdexcept>

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

    Type:
        DispatchableHandle

    \***********************************************************************************/
    using DispatchableHandle = void*;

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
        inline ValueType& Get( DispatchableHandle handle )
        {
            std::scoped_lock lk( m_DispatchMutex );

            auto it = m_Dispatch.find( handle );
            if( it == m_Dispatch.end() )
            {
                // Dispatch table not created for this object.
                throw std::out_of_range( "Dispatch table not found" );
            }

            return *(it->second);
        }

        /*******************************************************************************\

        Function:
            Insert

        Description:
            Creates new layer dispatch table and stores it in the dispatcher object.

        \*******************************************************************************/
        template<typename... Args>
        inline ValueType& Create( DispatchableHandle handle )
        {
            std::scoped_lock lk( m_DispatchMutex );

            ValueType* pTable = new ValueType;

            auto it = m_Dispatch.emplace( handle, pTable );
            if( !it.second )
            {
                // Vulkan spec, 3.3 Object Model
                //  Each object of a dispatchable type must have a unique handle value during its lifetime.
                // Call to Create() should always insert a new value into the map.
                throw std::runtime_error( "Dispatch table already exists" );
            }

            return *pTable;
        }

        /*******************************************************************************\

        Function:
            Erase

        Description:
            Removes layer dispatch table from the dispatcher object.

        \*******************************************************************************/
        inline void Erase( DispatchableHandle handle )
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
        std::map<DispatchableHandle, ValueType*> m_Dispatch;

        mutable std::mutex m_DispatchMutex;
    };
}

// Specialize stl structures for unordered_map

template<>
struct std::hash<Profiler::DispatchableHandle>
{
    inline size_t operator()( Profiler::DispatchableHandle a ) const
    {
        return reinterpret_cast<size_t>(*(const void**)(a));
    }
};

template<>
struct std::equal_to<Profiler::DispatchableHandle>
{
    inline bool operator()( Profiler::DispatchableHandle a, Profiler::DispatchableHandle b ) const
    {
        return *(const void**)(a) == *(const void**)(b);
    }
};

// Specialize stl structures for map

template<>
struct std::less<Profiler::DispatchableHandle>
{
    inline bool operator()( Profiler::DispatchableHandle a, Profiler::DispatchableHandle b ) const
    {
        return *(const void**)(a) < *(const void**)(b);
    }
};
