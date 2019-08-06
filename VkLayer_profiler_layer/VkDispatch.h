#pragma once
#include <vulkan/vk_layer.h>
#include <unordered_map>
#include <mutex>

namespace Profiler
{
    // Helper macro for getting address of function implementation
#   define GETPROCADDR( NAME )                                                          \
        if( strcmp( name, "vk" #NAME ) == 0 )                                           \
        {                                                                               \
            return reinterpret_cast<PFN_vkVoidFunction>(NAME);                          \
        }

    template<typename ReturnType, typename... ArgumentTypes>
    using VkFunctionType = VKAPI_ATTR ReturnType( VKAPI_CALL* )(ArgumentTypes...);

    template<typename DispatchableHandleType>
    using VkGetProcAddrFunctionType = VkFunctionType<PFN_vkVoidFunction, DispatchableHandleType, const char*>;

    using VkDispatchable = void*;
    using VkDispatchable_Hasher = std::hash<VkDispatchable>;

    /***********************************************************************************\

    Structure:
        VkFunction

    Description:
        Vulkan API-level function wrapper, which automatizes fetching address of the
        next layer's implementation of the function.

    \***********************************************************************************/
    template<typename FunctionType>
    struct VkFunction;

    template<typename ReturnType, typename... ArgumentTypes>
    struct VkFunction<VkFunctionType<ReturnType, ArgumentTypes...>>
    {
    public:
        using FunctionType = VkFunctionType<ReturnType, ArgumentTypes...>;

        // Create new function wrapper object and prefetch address of the implementation
        template<typename DispatchableType>
        VkFunction( DispatchableType handle, VkGetProcAddrFunctionType<DispatchableType> gpa, const char* pName )
            : m_pfnNextFunction( reinterpret_cast<FunctionType>(gpa( handle, pName )) )
        {
        }

        // Invoke next layer's function implementation
        inline ReturnType operator()( ArgumentTypes... arguments ) const
        {
            return m_pfnNextFunction( arguments... );
        }

    private:
        FunctionType m_pfnNextFunction;
    };

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
            std::unique_lock<std::mutex> lk( m_DispatchMtx );

            auto it = m_Dispatch.find( (DispatchableType)handle );
            if( it == m_Dispatch.end() )
            {
                // TODO error
            }

            return it->second;
        }

        /*******************************************************************************\

        Function:
            CreateDispatchTable

        Description:
            Creates new layer dispatch table and stores it in the dispatcher object.

        \*******************************************************************************/
        inline LayerDispatchTableType& CreateDispatchTable( VkDispatchable handle, GetProcAddrType gpa )
        {
            std::unique_lock<std::mutex> lk( m_DispatchMtx );

            auto it = m_Dispatch.try_emplace( (DispatchableType)handle,
                LayerDispatchTableType( (DispatchableType)handle, gpa ) );

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
            std::unique_lock<std::mutex> lk( m_DispatchMtx );
            m_Dispatch.erase( (DispatchableType)handle );
        }

    private:
        using DispatchMap = std::unordered_map<
            DispatchableType,
            LayerDispatchTableType,
            VkDispatchable_Hasher,
            VkDispatchable_EqualFunc>;

        std::mutex m_DispatchMtx;
        DispatchMap m_Dispatch;
    };
}
