#pragma once
#include <map>
#include <mutex>

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
        template<typename... Args>
        inline ValueType& Create( DispatchableHandle handle )
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
