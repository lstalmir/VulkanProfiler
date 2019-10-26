#pragma once
#include <vulkan/vk_layer.h>

namespace Profiler
{
    class VkAllocator_Object
    {
    public:
        VkAllocator_Object( VkAllocationCallbacks* pOriginalAllocationCallbacks );

        VkAllocationCallbacks* GetAllocationCallbacks() const;

    protected:
        VkAllocationCallbacks m_AllocationCallbacks;
        VkAllocationCallbacks* m_pOriginalAllocationCallbacks;

        static void* VKAPI_PTR Allocate( void* pThis, size_t size, size_t alignment, VkSystemAllocationScope allocationScope );
        static void VKAPI_PTR Free( void* pThis, void* pMemory );
    };
}
