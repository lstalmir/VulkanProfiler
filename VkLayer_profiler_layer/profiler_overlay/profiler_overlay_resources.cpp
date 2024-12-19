// Copyright (c) 2024 Lukasz Stalmirski
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

#include "profiler_overlay_resources.h"
#include "profiler/profiler_helpers.h"
#include "profiler_overlay/profiler_overlay_assets.h"
#include "profiler_layer_objects/VkDevice_object.h"

#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

#include <imgui.h>
#include "imgui_impl_vulkan_layer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifdef WIN32
#include <ShlObj.h> // SHGetKnownFolderPath
#endif

static constexpr const char* g_scDefaultFonts[] = {
#ifdef WIN32
    "segoeui.ttf",
    "tahoma.ttf",
#endif
#ifdef __linux__
    "Ubuntu-R.ttf",
    "LiberationSans-Regural.ttf",
#endif
    "DejaVuSans.ttf",
};

static constexpr const char* g_scBoldFonts[] = {
#ifdef WIN32
    "segoeuib.ttf",
    "tahomabd.ttf",
#endif
#ifdef __linux__
    "Ubuntu-B.ttf",
    "LiberationSans-Bold.ttf",
#endif
    "DejaVuSansBold.ttf",
};

static constexpr const char* g_scCodeFonts[] = {
#ifdef WIN32
    "consola.ttf",
    "cour.ttf",
#endif
#ifdef __linux__
    "UbuntuMono-R.ttf",
#endif
    "DejaVuSansMono.ttf",
};

namespace Profiler
{
    class OverlayFontFinder
    {
    public:
        OverlayFontFinder();

        std::filesystem::path GetFirstSupportedFont( const char* const* ppFonts, size_t fontCount ) const;

    private:
        std::vector<std::filesystem::path> m_FontSearchPaths;
    };

    /***********************************************************************************\

    Function:
        OverlayFontFinder

    Description:
        Constructor.

    \***********************************************************************************/
    OverlayFontFinder::OverlayFontFinder()
        : m_FontSearchPaths()
    {
#ifdef WIN32
        PWSTR pFontsDirectoryPath = nullptr;

        if( SUCCEEDED( SHGetKnownFolderPath( FOLDERID_Fonts, KF_FLAG_DEFAULT, nullptr, &pFontsDirectoryPath ) ) )
        {
            m_FontSearchPaths.push_back( pFontsDirectoryPath );
            CoTaskMemFree( pFontsDirectoryPath );
        }
#endif // WIN32
#ifdef __linux__
        // Linux distros use multiple font directories (or X server, TODO)
        m_FontSearchPaths = {
            "/usr/share/fonts",
            "/usr/local/share/fonts",
            "~/.fonts" };

        // Some systems may have these directories specified in conf file
        // https://stackoverflow.com/questions/3954223/platform-independent-way-to-get-font-directory
        const char* fontConfigurationFiles[] = {
            "/etc/fonts/fonts.conf",
            "/etc/fonts/local.conf" };

        std::vector<std::filesystem::path> configurationDirectories = {};

        for( const char* fontConfigurationFile : fontConfigurationFiles )
        {
            if( std::filesystem::exists( fontConfigurationFile ) )
            {
                // Try to open configuration file for reading
                std::ifstream conf( fontConfigurationFile );

                if( conf.is_open() )
                {
                    std::string line;

                    // conf is XML file, read line by line and find <dir> tag
                    while( std::getline( conf, line ) )
                    {
                        const size_t dirTagOpen = line.find( "<dir>" );
                        const size_t dirTagClose = line.find( "</dir>" );

                        // TODO: tags can be in different lines
                        if( ( dirTagOpen != std::string::npos ) && ( dirTagClose != std::string::npos ) )
                        {
                            configurationDirectories.push_back( line.substr( dirTagOpen + 5, dirTagClose - dirTagOpen - 5 ) );
                        }
                    }
                }
            }
        }

        if( !configurationDirectories.empty() )
        {
            // Override predefined font directories
            m_FontSearchPaths = configurationDirectories;
        }
#endif // __linux__
    }

    /***********************************************************************************\

    Function:
        GetFirstSupportedFont

    Description:
        Returns the first supported font from the list.

    \***********************************************************************************/
    std::filesystem::path OverlayFontFinder::GetFirstSupportedFont( const char* const* ppFonts, size_t fontCount ) const
    {
        for( size_t i = 0; i < fontCount; ++i )
        {
            const char* font = ppFonts[ i ];

            for( const std::filesystem::path& dir : m_FontSearchPaths )
            {
                // Fast path
                std::filesystem::path path = dir / font;
                if( std::filesystem::exists( path ) )
                {
                    return path;
                }

#ifdef __linux__
                // Search recursively
                path = ProfilerPlatformFunctions::FindFile( dir, font );
                if( !path.empty() )
                {
                    return path;
                }
#endif
            }
        }

        return std::filesystem::path();
    }

    /***********************************************************************************\

    Function:
        InitializeFonts

    Description:
        Loads fonts for the overlay.

    \***********************************************************************************/
    VkResult OverlayResources::InitializeFonts()
    {
        ImFontAtlas* fonts = ImGui::GetIO().Fonts;
        ImFont* defaultFont = fonts->AddFontDefault();
        m_pDefaultFont = defaultFont;
        m_pBoldFont = defaultFont;
        m_pCodeFont = defaultFont;

        // Find font files
        OverlayFontFinder fontFinder;
        std::filesystem::path defaultFontPath = fontFinder.GetFirstSupportedFont( g_scDefaultFonts, std::size( g_scDefaultFonts ) );
        std::filesystem::path boldFontPath = fontFinder.GetFirstSupportedFont( g_scBoldFonts, std::size( g_scBoldFonts ) );
        std::filesystem::path codeFontPath = fontFinder.GetFirstSupportedFont( g_scCodeFonts, std::size( g_scCodeFonts ) );

        // Include all glyphs in the font to support non-latin letters
        const ImWchar fontGlyphRange[] = { 0x20, 0xFFFF, 0 };

        if( !defaultFontPath.empty() )
        {
            m_pDefaultFont = fonts->AddFontFromFileTTF(
                defaultFontPath.string().c_str(), 16.0f, nullptr, fontGlyphRange );
        }

        if( !boldFontPath.empty() )
        {
            m_pBoldFont = fonts->AddFontFromFileTTF(
                boldFontPath.string().c_str(), 16.0f, nullptr, fontGlyphRange );
        }

        if( !codeFontPath.empty() )
        {
            m_pCodeFont = fonts->AddFontFromFileTTF(
                codeFontPath.string().c_str(), 12.0f, nullptr, fontGlyphRange );
        }

        // Build atlas
        unsigned char* tex_pixels = NULL;
        int tex_w, tex_h;
        fonts->GetTexDataAsRGBA32( &tex_pixels, &tex_w, &tex_h );

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        InitializeImages

    Description:
        Loads images for the overlay.

    \***********************************************************************************/
    VkResult OverlayResources::InitializeImages( VkDevice_Object& device, ImGui_ImplVulkan_Context& context )
    {
        VkResult result;

        m_pDevice = &device;
        m_pContext = &context;

        // Initialize memory manager for the overlay resources.
        result = m_MemoryManager.Initialize( m_pDevice );

        // Create image sampler.
        if( result == VK_SUCCESS )
        {
            VkSamplerCreateInfo samplerCreateInfo = {};
            samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
            samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
            samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCreateInfo.mipLodBias = 0.0f;

            result = m_pDevice->Callbacks.CreateSampler(
                m_pDevice->Handle,
                &samplerCreateInfo,
                nullptr,
                &m_LinearSampler );
        }

        // Create image objects
        if( result == VK_SUCCESS )
        {
            result = CreateImage( m_CopyIconImage, OverlayAssets::CopyImg, sizeof( OverlayAssets::CopyImg ) );
        }

        // Destroy resources if any of the steps failed.
        if( result != VK_SUCCESS )
        {
            DestroyImages();
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Frees all resources.

    \***********************************************************************************/
    void OverlayResources::Destroy()
    {
        DestroyImages();

        m_pDefaultFont = nullptr;
        m_pBoldFont = nullptr;
        m_pCodeFont = nullptr;
    }

    /***********************************************************************************\

    Function:
        DestroyImages

    Description:
        Frees image resources.

    \***********************************************************************************/
    void OverlayResources::DestroyImages()
    {
        if( m_pDevice && m_pContext )
        {
            DestroyImage( m_CopyIconImage );
        }

        if( m_pDevice )
        {
            if( m_LinearSampler )
            {
                m_pDevice->Callbacks.DestroySampler( m_pDevice->Handle, m_LinearSampler, nullptr );
                m_LinearSampler = VK_NULL_HANDLE;
            }

            if( m_UploadEvent )
            {
                m_pDevice->Callbacks.DestroyEvent( m_pDevice->Handle, m_UploadEvent, nullptr );
                m_UploadEvent = VK_NULL_HANDLE;
            }
        }

        m_MemoryManager.Destroy();

        m_pContext = nullptr;
        m_pDevice = nullptr;
    }

    /***********************************************************************************\

    Function:
        RecordUploadCommands

    Description:
        Records commands to upload all resources to the GPU.

    \***********************************************************************************/
    void OverlayResources::RecordUploadCommands( VkCommandBuffer commandBuffer )
    {
        UploadImage( commandBuffer, m_CopyIconImage );

        // Signal event to notify that all resources have been uploaded.
        VkEventCreateInfo eventCreateInfo = {};
        eventCreateInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;

        m_pDevice->Callbacks.CreateEvent(
            m_pDevice->Handle,
            &eventCreateInfo,
            nullptr,
            &m_UploadEvent );

        m_pDevice->Callbacks.CmdSetEvent(
            commandBuffer,
            m_UploadEvent,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT );
    }

    /***********************************************************************************\

    Function:
        FreeUploadResources

    Description:
        Frees resources used for uploading.

    \***********************************************************************************/
    void OverlayResources::FreeUploadResources()
    {
        if( m_UploadEvent != VK_NULL_HANDLE )
        {
            VkResult result = m_pDevice->Callbacks.GetEventStatus(
                m_pDevice->Handle,
                m_UploadEvent );

            if( result == VK_EVENT_SET )
            {
                DestroyImageUploadResources( m_CopyIconImage );

                m_pDevice->Callbacks.DestroyEvent( m_pDevice->Handle, m_UploadEvent, nullptr );
                m_UploadEvent = VK_NULL_HANDLE;
            }
        }
    }

    /***********************************************************************************\

    Function:
        GetDefaultFont

    Description:
        Returns the default font.

    \***********************************************************************************/
    ImFont* OverlayResources::GetDefaultFont() const
    {
        return m_pDefaultFont;
    }

    /***********************************************************************************\

    Function:
        GetBoldFont

    Description:
        Returns the bold font.

    \***********************************************************************************/
    ImFont* OverlayResources::GetBoldFont() const
    {
        return m_pBoldFont;
    }

    /***********************************************************************************\

    Function:
        GetCodeFont

    Description:
        Returns the code font.

    \***********************************************************************************/
    ImFont* OverlayResources::GetCodeFont() const
    {
        return m_pCodeFont;
    }

    /***********************************************************************************\

    Function:
        GetCopyIconImage

    Description:
        Returns the copy icon image descriptor set.

    \***********************************************************************************/
    VkDescriptorSet OverlayResources::GetCopyIconImage() const
    {
        return m_CopyIconImage.ImageDescriptorSet;
    }

    /***********************************************************************************\

    Function:
        CreateImage

    Description:
        Creates an image object from the asset data.

    \***********************************************************************************/
    VkResult OverlayResources::CreateImage( OverlayImage& image, const uint8_t* pAsset, size_t assetSize )
    {
        VkResult result;
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        VmaAllocationInfo uploadBufferAllocationInfo = {};

        int width, height, channels;
        std::unique_ptr<stbi_uc[]> pixels;

        // Load image data from asset.
        pixels.reset( stbi_load_from_memory( pAsset, static_cast<int>( assetSize ), &width, &height, &channels, STBI_rgb_alpha ) );

        if( !pixels || channels != 4 )
        {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        // Save image size for upload.
        const size_t imageDataSize = sizeof( stbi_uc ) * width * height * channels;
        image.ImageExtent.width = static_cast<uint32_t>( width );
        image.ImageExtent.height = static_cast<uint32_t>( height );

        // Create image object.
        {
            VkImageCreateInfo imageCreateInfo = {};
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            imageCreateInfo.format = format;
            imageCreateInfo.extent.width = static_cast<uint32_t>( width );
            imageCreateInfo.extent.height = static_cast<uint32_t>( height );
            imageCreateInfo.extent.depth = 1;
            imageCreateInfo.mipLevels = 1;
            imageCreateInfo.arrayLayers = 1;
            imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VmaAllocationCreateInfo allocationCreateInfo = {};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

            result = m_MemoryManager.AllocateImage(
                imageCreateInfo,
                allocationCreateInfo,
                &image.Image,
                &image.ImageAllocation,
                nullptr );
        }

        // Create image view.
        if( result == VK_SUCCESS )
        {
            VkImageViewCreateInfo imageViewCreateInfo = {};
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCreateInfo.image = image.Image;
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.format = format;
            imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            imageViewCreateInfo.subresourceRange.levelCount = 1;
            imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            imageViewCreateInfo.subresourceRange.layerCount = 1;

            result = m_pDevice->Callbacks.CreateImageView(
                m_pDevice->Handle,
                &imageViewCreateInfo,
                nullptr,
                &image.ImageView );
        }

        // Create descriptor set for ImGui binding.
        if( result == VK_SUCCESS )
        {
            image.ImageDescriptorSet = m_pContext->AddTexture(
                m_LinearSampler,
                image.ImageView,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

            if( !image.ImageDescriptorSet )
            {
                result = VK_ERROR_INITIALIZATION_FAILED;
            }
        }

        // Create buffer for uploading.
        if( result == VK_SUCCESS )
        {
            VkBufferCreateInfo bufferCreateInfo = {};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size = imageDataSize;
            bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            VmaAllocationCreateInfo bufferAllocationCreateInfo = {};
            bufferAllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
            bufferAllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

            result = m_MemoryManager.AllocateBuffer(
                bufferCreateInfo,
                bufferAllocationCreateInfo,
                &image.UploadBuffer,
                &image.UploadBufferAllocation,
                &uploadBufferAllocationInfo );
        }

        // Copy texture data to the upload buffer.
        if( result == VK_SUCCESS )
        {
            if( uploadBufferAllocationInfo.pMappedData != nullptr )
            {
                memcpy( uploadBufferAllocationInfo.pMappedData, pixels.get(), imageDataSize );

                // Flush the buffer to make it visible to the GPU.
                result = m_MemoryManager.Flush(image.UploadBufferAllocation);

                image.RequiresUpload = true;
            }
            else
            {
                // Failed to allocate mapped host-visible memory.
                result = VK_ERROR_INITIALIZATION_FAILED;
            }
        }

        // Destroy the image if any of the steps failed.
        if( result != VK_SUCCESS )
        {
            DestroyImage( image );
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        UploadImage

    Description:
        Records commands to upload image data to the GPU.

    \***********************************************************************************/
    void OverlayResources::UploadImage( VkCommandBuffer commandBuffer, OverlayImage& image )
    {
        if( image.RequiresUpload )
        {
            TransitionImage( commandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

            VkBufferImageCopy region = {};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.layerCount = 1;
            region.imageExtent.width = image.ImageExtent.width;
            region.imageExtent.height = image.ImageExtent.height;
            region.imageExtent.depth = 1;

            m_pDevice->Callbacks.CmdCopyBufferToImage(
                commandBuffer,
                image.UploadBuffer,
                image.Image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region );

            TransitionImage( commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

            image.RequiresUpload = false;
        }
    }

    /***********************************************************************************\

    Function:
        TransitionImage

    Description:
        Inserts a pipeline barrier with image layout transition.

    \***********************************************************************************/
    void OverlayResources::TransitionImage( VkCommandBuffer commandBuffer, OverlayImage& image, VkImageLayout oldLayout, VkImageLayout newLayout )
    {
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image.Image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        m_pDevice->Callbacks.CmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier );
    }

    /***********************************************************************************\

    Function:
        DestroyImage

    Description:
        Destroys the image object.

    \***********************************************************************************/
    void OverlayResources::DestroyImage( OverlayImage& image )
    {
        DestroyImageUploadResources( image );

        if( image.ImageDescriptorSet )
        {
            m_pContext->RemoveTexture( image.ImageDescriptorSet );
            image.ImageDescriptorSet = VK_NULL_HANDLE;
        }

        if( image.ImageView )
        {
            m_pDevice->Callbacks.DestroyImageView( m_pDevice->Handle, image.ImageView, nullptr );
            image.ImageView = VK_NULL_HANDLE;
        }

        if( image.Image )
        {
            m_MemoryManager.FreeImage( image.Image, image.ImageAllocation );
            image.Image = VK_NULL_HANDLE;
            image.ImageAllocation = VK_NULL_HANDLE;
        }
    }

    /***********************************************************************************\

    Function:
        DestroyImageUploadResources

    Description:
        Frees resources used for uploading the image.

    \***********************************************************************************/
    void OverlayResources::DestroyImageUploadResources( OverlayImage& image )
    {
        if( image.UploadBuffer )
        {
            m_MemoryManager.FreeBuffer( image.UploadBuffer, image.UploadBufferAllocation );
            image.UploadBuffer = VK_NULL_HANDLE;
            image.UploadBufferAllocation = VK_NULL_HANDLE;
        }
    }
}
