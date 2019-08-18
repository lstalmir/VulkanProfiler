#include "profiler_overlay_font.h"
#include "profiler_helpers.h"
#include "profiler_resources.generated.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        ProfilerOverlayFont

    Description:
        Constructor

    \***********************************************************************************/
    ProfilerOverlayFont::ProfilerOverlayFont()
        : m_FontGlyphsImage( VK_NULL_HANDLE )
        , m_FontGlyphsImageView( VK_NULL_HANDLE )
        , m_Callbacks()
    {
        memset( &m_Callbacks, 0, sizeof( m_Callbacks ) );
    }

    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Initializes profiler overlay font resources.

    \***********************************************************************************/
    VkResult ProfilerOverlayFont::Initialize( VkDevice device, ProfilerCallbacks callbacks )
    {
        // Helper macro for rolling-back to valid state
#       define DESTROYANDRETURNONFAIL( VKRESULT )   \
            {                                       \
                VkResult result = (VKRESULT);       \
                if( result != VK_SUCCESS )          \
                {                                   \
                    Destroy( device );              \
                    return result;                  \
                }                                   \
            }

        m_Callbacks = callbacks;

        // Create the image
        VkStructure<VkImageCreateInfo> imageCreateInfo;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = profiler_font_glyphs::width;
        imageCreateInfo.extent.height = profiler_font_glyphs::height;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.format = profiler_font_glyphs::format;
        imageCreateInfo.mipLevels = profiler_font_glyphs::mipCount;
        imageCreateInfo.arrayLayers = profiler_font_glyphs::arraySize;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        DESTROYANDRETURNONFAIL( m_Callbacks.pfnCreateImage( device, &imageCreateInfo, nullptr, &m_FontGlyphsImage ) );

        // Fill the image with data


        // Create the image view
        VkStructure<VkImageViewCreateInfo> imageViewCreateInfo;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.image = m_FontGlyphsImage;
        imageViewCreateInfo.format = profiler_font_glyphs::format;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.layerCount = profiler_font_glyphs::arraySize;
        imageViewCreateInfo.subresourceRange.levelCount = profiler_font_glyphs::mipCount;

        DESTROYANDRETURNONFAIL( m_Callbacks.pfnCreateImageView( device, &imageViewCreateInfo, nullptr, &m_FontGlyphsImageView ) );

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Frees resources allocated by the profiler overlay font.

    \***********************************************************************************/
    void ProfilerOverlayFont::Destroy( VkDevice device )
    {
        // Destroy image view
        if( m_FontGlyphsImageView )
        {
            m_Callbacks.pfnDestroyImageView( device, m_FontGlyphsImageView, nullptr );

            m_FontGlyphsImageView = VK_NULL_HANDLE;
        }

        // Destroy the image
        if( m_FontGlyphsImage )
        {
            m_Callbacks.pfnDestroyImage( device, m_FontGlyphsImage, nullptr );

            m_FontGlyphsImage = VK_NULL_HANDLE;
        }
    }
}
