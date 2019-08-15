#pragma once
#include "profiler_callbacks.h"
#include <vulkan/vulkan.h>

namespace Profiler
{
    class ProfilerOverlayFont
    {
    public:
        ProfilerOverlayFont();

        VkResult Initialize( VkDevice device, ProfilerCallbacks callbacks );
        void Destroy( VkDevice device );

    protected:
        VkImage m_FontGlyphsImage;
        VkImageView m_FontGlyphsImageView;

        ProfilerCallbacks m_Callbacks;
    };
}
