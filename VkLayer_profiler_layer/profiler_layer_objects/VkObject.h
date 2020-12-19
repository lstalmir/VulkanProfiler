// Copyright (c) 2020 Lukasz Stalmirski
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
#include <utility>

namespace Profiler
{
    template<typename VkObjectT, bool IsPointer>
    struct VkObject_Traits_Base;

    template<typename VkObjectT>
    struct VkObject_Traits_Base<VkObjectT, true /*IsPointer*/>
    {
        static constexpr uint64_t GetObjectHandle( VkObjectT object )
        {
            return static_cast<uint64_t>(*reinterpret_cast<uintptr_t*>(&object));
        }
    };

    template<typename VkObjectT>
    struct VkObject_Traits_Base<VkObjectT, false /*IsPointer*/>
    {
        static constexpr uint64_t GetObjectHandle( VkObjectT object )
        {
            return static_cast<uint64_t>(object);
        }
    };

    template<typename VkObjectT>
    struct VkObject_Traits : VkObject_Traits_Base<VkObjectT, std::is_pointer_v<VkObjectT>>
    {
        using VkObject_Traits_Base::GetObjectHandle;
        static constexpr VkObjectType ObjectType = VK_OBJECT_TYPE_UNKNOWN;
        static constexpr const char ObjectTypeName[] = "Unknown object type";
    };
    
    #define DEFINE_VK_OBJECT_TRAITS( TYPE, OBJECT_TYPE ) \
    template<> struct VkObject_Traits<TYPE> : VkObject_Traits_Base<TYPE, std::is_pointer_v<TYPE>> { \
        using VkObject_Traits_Base::GetObjectHandle; \
        static constexpr VkObjectType ObjectType = OBJECT_TYPE; \
        static constexpr const char ObjectTypeName[] = #TYPE; }

    DEFINE_VK_OBJECT_TRAITS( VkInstance, VK_OBJECT_TYPE_INSTANCE );
    DEFINE_VK_OBJECT_TRAITS( VkPhysicalDevice, VK_OBJECT_TYPE_PHYSICAL_DEVICE );
    DEFINE_VK_OBJECT_TRAITS( VkDevice, VK_OBJECT_TYPE_DEVICE );
    DEFINE_VK_OBJECT_TRAITS( VkQueue, VK_OBJECT_TYPE_QUEUE );
    DEFINE_VK_OBJECT_TRAITS( VkSemaphore, VK_OBJECT_TYPE_SEMAPHORE );
    DEFINE_VK_OBJECT_TRAITS( VkCommandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER );
    DEFINE_VK_OBJECT_TRAITS( VkFence, VK_OBJECT_TYPE_FENCE );
    DEFINE_VK_OBJECT_TRAITS( VkDeviceMemory, VK_OBJECT_TYPE_DEVICE_MEMORY );
    DEFINE_VK_OBJECT_TRAITS( VkBuffer, VK_OBJECT_TYPE_BUFFER );
    DEFINE_VK_OBJECT_TRAITS( VkImage, VK_OBJECT_TYPE_IMAGE );
    DEFINE_VK_OBJECT_TRAITS( VkEvent, VK_OBJECT_TYPE_EVENT );
    DEFINE_VK_OBJECT_TRAITS( VkQueryPool, VK_OBJECT_TYPE_QUERY_POOL );
    DEFINE_VK_OBJECT_TRAITS( VkBufferView, VK_OBJECT_TYPE_BUFFER_VIEW );
    DEFINE_VK_OBJECT_TRAITS( VkImageView, VK_OBJECT_TYPE_IMAGE_VIEW );
    DEFINE_VK_OBJECT_TRAITS( VkShaderModule, VK_OBJECT_TYPE_SHADER_MODULE );
    DEFINE_VK_OBJECT_TRAITS( VkPipelineCache, VK_OBJECT_TYPE_PIPELINE_CACHE );
    DEFINE_VK_OBJECT_TRAITS( VkPipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT );
    DEFINE_VK_OBJECT_TRAITS( VkRenderPass, VK_OBJECT_TYPE_RENDER_PASS );
    DEFINE_VK_OBJECT_TRAITS( VkPipeline, VK_OBJECT_TYPE_PIPELINE );
    DEFINE_VK_OBJECT_TRAITS( VkDescriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT );
    DEFINE_VK_OBJECT_TRAITS( VkSampler, VK_OBJECT_TYPE_SAMPLER );
    DEFINE_VK_OBJECT_TRAITS( VkDescriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL );
    DEFINE_VK_OBJECT_TRAITS( VkDescriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET );
    DEFINE_VK_OBJECT_TRAITS( VkFramebuffer, VK_OBJECT_TYPE_FRAMEBUFFER );
    DEFINE_VK_OBJECT_TRAITS( VkCommandPool, VK_OBJECT_TYPE_COMMAND_POOL );
    DEFINE_VK_OBJECT_TRAITS( VkSamplerYcbcrConversion, VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION );
    DEFINE_VK_OBJECT_TRAITS( VkDescriptorUpdateTemplate, VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE );
    DEFINE_VK_OBJECT_TRAITS( VkSurfaceKHR, VK_OBJECT_TYPE_SURFACE_KHR );
    DEFINE_VK_OBJECT_TRAITS( VkSwapchainKHR, VK_OBJECT_TYPE_SWAPCHAIN_KHR );
    DEFINE_VK_OBJECT_TRAITS( VkDisplayKHR, VK_OBJECT_TYPE_DISPLAY_KHR );
    DEFINE_VK_OBJECT_TRAITS( VkDisplayModeKHR, VK_OBJECT_TYPE_DISPLAY_MODE_KHR );
    DEFINE_VK_OBJECT_TRAITS( VkDebugReportCallbackEXT, VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT );
    DEFINE_VK_OBJECT_TRAITS( VkDebugUtilsMessengerEXT, VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT );
    DEFINE_VK_OBJECT_TRAITS( VkAccelerationStructureKHR, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR );
    DEFINE_VK_OBJECT_TRAITS( VkValidationCacheEXT, VK_OBJECT_TYPE_VALIDATION_CACHE_EXT );
    DEFINE_VK_OBJECT_TRAITS( VkPerformanceConfigurationINTEL, VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL );
    //DEFINE_VK_OBJECT_TRAITS( , VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR );
    DEFINE_VK_OBJECT_TRAITS( VkIndirectCommandsLayoutNV, VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV );
    DEFINE_VK_OBJECT_TRAITS( VkPrivateDataSlotEXT, VK_OBJECT_TYPE_PRIVATE_DATA_SLOT_EXT );

    #undef DEFINE_VK_OBJECT_TRAITS

    struct VkObject
    {
        uint64_t m_Handle;
        VkObjectType m_Type;
        const char* m_pTypeName;

        inline VkObject()
            : m_Handle( 0 )
            , m_Type( VK_OBJECT_TYPE_UNKNOWN )
            , m_pTypeName( "Unknown object type" )
        {
        }

        template<typename VkObjectT>
        inline VkObject( VkObjectT object )
            : m_Handle( VkObject_Traits<VkObjectT>::GetObjectHandle( object ) )
            , m_Type( VkObject_Traits<VkObjectT>::ObjectType )
            , m_pTypeName( VkObject_Traits<VkObjectT>::ObjectTypeName )
        {
        }

        inline bool operator==( const VkObject& rh ) const
        {
            return m_Handle == rh.m_Handle && m_Type == rh.m_Type;
        }
    };
}

template<>
struct std::hash<Profiler::VkObject>
{
    inline size_t operator()( const Profiler::VkObject& obj ) const
    {
        // Combine handle value and object type using boost way
        return static_cast<size_t>(obj.m_Handle ^ (
            (static_cast<uint64_t>(obj.m_Type)) +
            (0x9e3779b97f4a7c16) +
            (obj.m_Handle << 6) +
            (obj.m_Handle >> 2)));
    }
};
