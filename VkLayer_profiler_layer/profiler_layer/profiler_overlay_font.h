#pragma once
#include "profiler_callbacks.h"
#include "profiler_image.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include <vulkan/vulkan.h>

namespace Profiler
{
    class ProfilerOverlay;

    class ProfilerOverlayFont
        : ProfilerImage
    {
    public:
        ProfilerOverlayFont();

        VkResult Initialize( VkDevice_Object* pDevice, ProfilerOverlay* pOverlay, ProfilerCallbacks callbacks );
        void Destroy();

        void Bind( VkCommandBuffer commandBuffer, uint32_t slot );

    protected:
        ProfilerOverlay*        m_pProfilerOverlay;
        ProfilerCallbacks       m_Callbacks;

        VkDevice                m_Device;

        VkImageView             m_ImageView;
    };
}
