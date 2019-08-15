#include "profiler_overlay_font.h"
#include "profiler_helpers.h"
#include "profiler_resources.h"

namespace Profiler
{
    ProfilerOverlayFont::ProfilerOverlayFont()
        : m_FontGlyphsImage( VK_NULL_HANDLE )
        , m_FontGlyphsImageView( VK_NULL_HANDLE )
        , m_Callbacks()
    {
        memset( &m_Callbacks, 0, sizeof( m_Callbacks ) );
    }

    VkResult ProfilerOverlayFont::Initialize( VkDevice device, ProfilerCallbacks callbacks )
    {
        m_Callbacks = callbacks;



        VkStructure<VkImageCreateInfo> imageCreateInfo;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = 512;
        imageCreateInfo.extent.height = 512;

        return VK_ERROR_INITIALIZATION_FAILED;
    }
}
