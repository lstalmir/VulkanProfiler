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

#pragma once
#include "profiler/profiler_memory_manager.h"

struct VkDevice_Object;
struct VkQueue_Object;
struct ImFont;
class ImGui_ImplVulkan_Context;

namespace Profiler
{
    struct OverlayImage
    {
        VkImage Image;
        VkImageView ImageView;
        VkDescriptorSet ImageDescriptorSet;
        VmaAllocation ImageAllocation;
        VkExtent2D ImageExtent;

        VkBuffer UploadBuffer;
        VmaAllocation UploadBufferAllocation;

        bool RequiresUpload = false;
    };

    /***********************************************************************************\

    Class:
        OverlayResources

    Description:
        Manages the fonts and images used by the overlay.

    \***********************************************************************************/
    class OverlayResources
    {
    public:
        VkResult InitializeFonts();
        VkResult InitializeImages( VkDevice_Object& device, ImGui_ImplVulkan_Context& context );
        void Destroy();

        void RecordUploadCommands( VkCommandBuffer commandBuffer );
        void FreeUploadResources();

        ImFont* GetDefaultFont() const;
        ImFont* GetBoldFont() const;
        ImFont* GetCodeFont() const;

        VkDescriptorSet GetCopyIconImage() const;

    private:
        VkDevice_Object* m_pDevice = nullptr;
        ImGui_ImplVulkan_Context* m_pContext = nullptr;

        DeviceProfilerMemoryManager m_MemoryManager;

        VkEvent m_UploadEvent = VK_NULL_HANDLE;
        VkSampler m_LinearSampler = VK_NULL_HANDLE;

        ImFont* m_pDefaultFont = nullptr;
        ImFont* m_pBoldFont = nullptr;
        ImFont* m_pCodeFont = nullptr;

        OverlayImage m_CopyIconImage;

        VkResult CreateImage( OverlayImage& image, const uint8_t* pAsset, size_t assetSize );
        void UploadImage( VkCommandBuffer commandBuffer, OverlayImage& image );
        void TransitionImage( VkCommandBuffer commandBuffer, OverlayImage& image, VkImageLayout oldLayout, VkImageLayout newLayout );
        void DestroyImage( OverlayImage& image );
        void DestroyImageUploadResources( OverlayImage& image );
    };
}
