#pragma once
#include <vulkan/vk_layer.h>
#include <unordered_map>
#include <mutex>

namespace Profiler
{
    template<typename ReturnType, typename... ArgumentTypes>
    using VkApiFunction = VKAPI_ATTR ReturnType( VKAPI_CALL* )(ArgumentTypes...);

    /***********************************************************************************\

    Structure:
        VkInstance_LayerFunction

    Description:
        VkInstance API-level function wrapper, which automatizes fetching address of the
        next layer's implementation of the function.

    \***********************************************************************************/
    template<typename FunctionType>
    struct VkInstance_LayerFunction;

    template<typename ReturnType, typename... ArgumentTypes>
    struct VkInstance_LayerFunction<VkApiFunction<ReturnType, ArgumentTypes...>>
    {
    public:
        using FunctionType = VkApiFunction<ReturnType, ArgumentTypes...>;

        // Create new function wrapper object and prefetch address of the implementation
        VkInstance_LayerFunction( VkInstance instance, PFN_vkGetInstanceProcAddr gpa, const char* pName )
            : m_pfnNextFunction( reinterpret_cast<FunctionType>(gpa( instance, pName )) )
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
        VkInstance_LayerDispatchTable

    Description:
        Set of pointers to the next layer's implementations of functions overloaded in
        this layer. Each pointer is wrapped in self-initializing, callable object.

    \***********************************************************************************/
    struct VkInstance_LayerDispatchTable
    {
        VkInstance_LayerFunction< PFN_vkGetInstanceProcAddr                 > pfnGetInstanceProcAddr;
        VkInstance_LayerFunction< PFN_vkDestroyInstance                     > pfnDestroyInstance;
        VkInstance_LayerFunction< PFN_vkEnumerateDeviceExtensionProperties  > pfnEnumerateDeviceExtensionProperties;

        VkInstance_LayerDispatchTable( VkInstance instance, PFN_vkGetInstanceProcAddr gpa )
            : pfnGetInstanceProcAddr( instance, gpa, "vkGetInstanceProcAddr" )
            , pfnDestroyInstance( instance, gpa, "vkDestroyInstance" )
            , pfnEnumerateDeviceExtensionProperties( instance, gpa, "vkEnumerateDeviceExtensionProperties" )
        {
        }
    };

    using VkDispatchableHandle = void*;
    using VkDispatchableHandle_Hasher = std::hash<VkDispatchableHandle>;

    /***********************************************************************************\

    Structure:
        VkDispatchableHandle_EqualFunc

    Description:
        Simple comparator of VkDispatchableHandle objects. Each dispatchable object
        contains pointer to the internal dispatch table managed by the loader. The
        comparator compares these pointers to check if object is the same instance
        (uses the same dispatch table).

    \***********************************************************************************/
    struct VkDispatchableHandle_EqualFunc
    {
        inline bool operator()( const VkDispatchableHandle& a, const VkDispatchableHandle& b ) const
        {
            // Compare addresses of dispatch tables
            return *(const void**)(a) == *(const void**)(b);
        }
    };

    /***********************************************************************************\

    Class:
        VkInstance_Dispatch

    Description:
        VkInstance object manager, stores dispatch tables for each instance created with
        this layer enabled.

    \***********************************************************************************/
    class VkInstance_Dispatch
    {
    public:
        VkInstance_LayerDispatchTable& GetInstanceDispatchTable( VkInstance instance );
        VkInstance_LayerDispatchTable& CreateInstanceDispatchTable( VkInstance instance, PFN_vkGetInstanceProcAddr gpa );
        void                           DestroyInstanceDispatchTable( VkInstance instance );

    private:
        using InstanceDispatchMap = std::unordered_map<
            VkDispatchableHandle,
            VkInstance_LayerDispatchTable,
            VkDispatchableHandle_Hasher,
            VkDispatchableHandle_EqualFunc>;

        std::mutex m_InstanceDispatchMtx;
        InstanceDispatchMap m_InstanceDispatch;
    };

    extern VkInstance_Dispatch InstanceDispatch;
}
