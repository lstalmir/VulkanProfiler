#pragma once
#include <vulkan/vk_layer.h>
#include <unordered_map>
#include <mutex>

////////////////////////////////////////////////////////////////////////////////////////
struct InstanceEqualFunc
{
    inline bool operator()( const VkInstance& a, const VkInstance& b ) const
    {
        // Compare addresses of dispatch tables
        return *reinterpret_cast<void**>(a) == *reinterpret_cast<void**>(b);
    }
};

////////////////////////////////////////////////////////////////////////////////////////
template<typename T>
struct FunctionName
{
    static constexpr bool Defined = false;
};

#define DEFINE_FUNCTION_NAME( FUNCTION )        \
template<> struct FunctionName<PFN_##FUNCTION>  \
{                                               \
    static constexpr char Value[] = #FUNCTION;  \
    static constexpr bool Defined = true;       \
}

DEFINE_FUNCTION_NAME( vkGetInstanceProcAddr );
DEFINE_FUNCTION_NAME( vkDestroyInstance );
DEFINE_FUNCTION_NAME( vkEnumerateDeviceExtensionProperties );

////////////////////////////////////////////////////////////////////////////////////////
template<typename FunctionType>
struct InstanceFunction;

template<typename ReturnType, typename... ArgumentTypes>
struct InstanceFunction<VKAPI_ATTR ReturnType( VKAPI_CALL* )(ArgumentTypes...)>
{
    using FunctionType = VKAPI_ATTR ReturnType( VKAPI_CALL* )(ArgumentTypes...);

    inline InstanceFunction()
        : pFunc( nullptr )
    {
    }

    template<std::enable_if_t<FunctionName<FunctionType>::Defined>* = nullptr>
    inline InstanceFunction( VkInstance instance, PFN_vkGetInstanceProcAddr gpa )
    {
        pFunc = reinterpret_cast<FunctionType>(gpa( instance, FunctionName<FunctionType>::Value ));
    }

    inline InstanceFunction( VkInstance instance, PFN_vkGetInstanceProcAddr gpa, const char* name )
    {
        pFunc = reinterpret_cast<FunctionType>(gpa( instance, name ));
    }

    inline ReturnType operator()( ArgumentTypes... arguments )
    {
        return pFunc( arguments... );
    }

    FunctionType pFunc;
};

////////////////////////////////////////////////////////////////////////////////////////
struct LayerInstanceDispatchTable
{
    InstanceFunction< PFN_vkGetInstanceProcAddr                > GetInstanceProcAddr;
    InstanceFunction< PFN_vkDestroyInstance                    > DestroyInstance;
    InstanceFunction< PFN_vkEnumerateDeviceExtensionProperties > EnumerateDeviceExtensionProperties;

    inline LayerInstanceDispatchTable()
    {
    }

    inline LayerInstanceDispatchTable( VkInstance instance, PFN_vkGetInstanceProcAddr gpa )
        : GetInstanceProcAddr( instance, gpa )
        , DestroyInstance( instance, gpa )
        , EnumerateDeviceExtensionProperties( instance, gpa )
    {
    }
};

////////////////////////////////////////////////////////////////////////////////////////
using InstanceFunctionMap = std::unordered_map<
    VkInstance,
    LayerInstanceDispatchTable,
    std::hash<VkInstance>,
    InstanceEqualFunc>;

////////////////////////////////////////////////////////////////////////////////////////
extern std::mutex           g_InstanceMtx;
extern InstanceFunctionMap  g_InstanceFunctions;
