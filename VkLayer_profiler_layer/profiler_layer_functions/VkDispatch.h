#pragma once
#include <vulkan/vk_layer.h>
#include <unordered_map>
#include <mutex>

namespace Profiler
{
    // Helper macro for getting address of function implementation
#   define GETPROCADDR( NAME )                                                          \
        if( strcmp( pName, "vk" #NAME ) == 0 )                                          \
        {                                                                               \
            return reinterpret_cast<PFN_vkVoidFunction>(NAME);                          \
        }

#   define GETDEVICEPROCADDR( DEVICE, NAME )                                            \
        reinterpret_cast<PFN_##NAME>(pfnGetDeviceProcAddr( DEVICE, #NAME ))

#   define GETINSTANCEPROCADDR( INSTANCE, NAME )                                        \
        reinterpret_cast<PFN_##NAME>(pfnGetInstanceProcAddr( INSTANCE, #NAME ))

    template<typename ReturnType, typename... ArgumentTypes>
    using VkFunctionType = VKAPI_ATTR ReturnType( VKAPI_CALL* )(ArgumentTypes...);

    template<typename DispatchableHandleType>
    using VkGetProcAddrFunctionType = VkFunctionType<PFN_vkVoidFunction, DispatchableHandleType, const char*>;

    using VkDispatchable = void*;

    /***********************************************************************************\

    Structure:
        VkDispatchable_EqualFunc

    Description:
        Simple comparator of VkDispatchable objects. Each dispatchable object contains
        pointer to the internal dispatch table managed by the loader. The comparator
        compares these pointers to check if object is the same instance (uses the same
        dispatch table).

    \***********************************************************************************/
    struct VkDispatchable_EqualFunc
    {
        inline bool operator()( const VkDispatchable& a, const VkDispatchable& b ) const
        {
            // Compare addresses of dispatch tables
            return *(const void**)(a) == *(const void**)(b);
        }
    };

    /***********************************************************************************\

    Structure:
        VkDispatchable_HasherFunc

    Description:

    \***********************************************************************************/
    struct VkDispatchable_HasherFunc
    {
        inline size_t operator()( const VkDispatchable& a ) const
        {
            return reinterpret_cast<size_t>(*(const void**)a);
        }
    };

    /***********************************************************************************\

    Class:
        GuardedUnorderedMap

    Description:
        Thread-safe wrapper for stl unordered_map class.

    \***********************************************************************************/
    template<typename KeyT, typename ValueT,
        typename HasherT = std::hash<KeyT>,
        typename EqualFnT = std::equal_to<KeyT>
    > class GuardedUnorderedMap
        : public std::unordered_map<KeyT, ValueT, HasherT, EqualFnT>
    {
    public:
        inline GuardedUnorderedMap()
            : unordered_map()
        {
        }

        inline GuardedUnorderedMap( const GuardedUnorderedMap& other )
            : GuardedUnorderedMap()
        {
            // Avoid deadlocks
            if( &other == this )
            {
                return;
            }

            std::scoped_lock lk( m_MapAccessMutex, other.m_MapAccessMutex );

            // Copy elements to new map
            for( auto element : other )
            {
                try_emplace( element );
            }
        }

        inline GuardedUnorderedMap( GuardedUnorderedMap&& other )
            : GuardedUnorderedMap()
        {
            // Avoid deadlocks
            if( &other == this )
            {
                return;
            }

            std::scoped_lock lk( m_MapAccessMutex, other.m_MapAccessMutex );

            // Swap contents of maps
            swap( other );
        }

        template<typename DispatchableType>
        inline auto at( const DispatchableType& key )
        {
            std::scoped_lock lk( m_MapAccessMutex );
            return unordered_map::at( (KeyT)key );
        }

        inline auto try_emplace( const KeyT& key, const ValueT& value )
        {
            std::scoped_lock lk( m_MapAccessMutex );
            return unordered_map::try_emplace( key, value );
        }

        inline void lock()
        {
            // Lock access to the map contents
            m_MapAccessMutex.lock();
        }

        inline void unlock()
        {
            // Unlock access to the map contents
            m_MapAccessMutex.unlock();
        }

        inline bool try_lock()
        {
            // Try to lock access to the map contents
            return m_MapAccessMutex.try_lock();
        }

    private:
        mutable std::recursive_mutex m_MapAccessMutex;
    };

    /***********************************************************************************\

    Class:
        VkDispatchableMap

    Description:
        Map with dispatchable handle keys.

    \***********************************************************************************/
    template<typename DispatchableType, typename ValueType>
    using VkDispatchableMap = GuardedUnorderedMap<
        DispatchableType,
        ValueType,
        VkDispatchable_HasherFunc,
        VkDispatchable_EqualFunc>;

    /***********************************************************************************\

    Class:
        VkDispatch

    Description:
        Object manager, stores dispatch tables for each instance created with this layer
        enabled.

    \***********************************************************************************/
    template<typename DispatchableType, typename LayerDispatchTableType>
    class VkDispatch
    {
    public:
        using GetProcAddrType = VkGetProcAddrFunctionType<DispatchableType>;

        /*******************************************************************************\

        Function:
            GetDispatchTable

        Description:
            Retrieves layer dispatch table from the dispatcher object.

        \*******************************************************************************/
        inline LayerDispatchTableType& GetDispatchTable( VkDispatchable handle )
        {
            std::scoped_lock lk( m_Dispatch );

            auto it = m_Dispatch.find( reinterpret_cast<DispatchableType>(handle) );
            if( it == m_Dispatch.end() )
            {
                // TODO error
            }

            return it->second;
        }

        /*******************************************************************************\

        Function:
            operator[]

        Description:
            Alias for GetDispatchTable, for convenience.

        \*******************************************************************************/
        inline LayerDispatchTableType& operator[]( VkDispatchable handle )
        {
            return GetDispatchTable( handle );
        }

        /*******************************************************************************\

        Function:
            CreateDispatchTable

        Description:
            Creates new layer dispatch table and stores it in the dispatcher object.

        \*******************************************************************************/
        inline LayerDispatchTableType& CreateDispatchTable( VkDispatchable handle, GetProcAddrType gpa )
        {
            std::scoped_lock lk( m_Dispatch );

            auto it = m_Dispatch.try_emplace( reinterpret_cast<DispatchableType>(handle),
                LayerDispatchTableType( reinterpret_cast<DispatchableType>(handle), gpa ) );

            if( !it.second )
            {
                // TODO error, should have created new value
            }

            return it.first->second;
        }

        /*******************************************************************************\

        Function:
            DestroyDispatchTable

        Description:
            Removes layer dispatch table from the dispatcher object.

        \*******************************************************************************/
        inline void DestroyDispatchTable( VkDispatchable handle )
        {
            std::scoped_lock lk( m_Dispatch );
            m_Dispatch.erase( reinterpret_cast<DispatchableType>(handle) );
        }

    private:
        VkDispatchableMap<DispatchableType, LayerDispatchTableType> m_Dispatch;
    };


    template<typename DispatchableT>
    struct Functions
    {
        using DispatchableType = DispatchableT;

        static PFN_vkVoidFunction GetInterceptedProcAddr( const char* ) = delete;
        static PFN_vkVoidFunction GetProcAddr( DispatchableType, const char* ) = delete;
    };

    template<typename Functions, typename FunctionType = PFN_vkVoidFunction>
    inline FunctionType GetInterceptedProcAddr( const char* pName )
    {
        return reinterpret_cast<FunctionType>(Functions::GetInterceptedProcAddr( pName ));
    }

    template<typename Functions, typename FunctionType = PFN_vkVoidFunction>
    inline FunctionType GetProcAddr( typename Functions::DispatchableType dispatchable, const char* pName )
    {
        return reinterpret_cast<FunctionType>(Functions::GetProcAddr( dispatchable, pName ));
    }
}
