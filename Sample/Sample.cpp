/*
 * Vulkan Windowed Program
 *
 * Copyright (C) 2016, 2018 Valve Corporation
 * Copyright (C) 2016, 2018 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 /*
 Vulkan C++ Windowed Project Template
 Create and destroy a Vulkan surface on an SDL window.
 */

 // Enable the WSI extensions
#if defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "Device.h"
#include "SwapChain.h"

// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED

#include <glm.hpp>
#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include <iostream>
#include <vector>

int main()
{
    // Create an SDL window that supports Vulkan rendering.
    if( SDL_Init( SDL_INIT_VIDEO ) != 0 )
    {
        std::cout << "Could not initialize SDL." << std::endl;
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow( "Vulkan Window", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN );
    if( window == NULL )
    {
        std::cout << "Could not create SDL window." << std::endl;
        return 1;
    }

    // Get WSI extensions from SDL (we can add more if we like - we just can't remove these)
    unsigned extension_count;
    if( !SDL_Vulkan_GetInstanceExtensions( window, &extension_count, NULL ) )
    {
        std::cout << "Could not get the number of required instance extensions from SDL." << std::endl;
        return 1;
    }
    std::vector<const char*> extensions( extension_count );
    if( !SDL_Vulkan_GetInstanceExtensions( window, &extension_count, extensions.data() ) )
    {
        std::cout << "Could not get the names of required instance extensions from SDL." << std::endl;
        return 1;
    }

    // Use validation layers if this is a debug build
    std::vector<const char*> layers;
#if defined(_DEBUG)
    layers.push_back( "VK_LAYER_LUNARG_standard_validation" );
#endif
    layers.push_back( "VK_LAYER_profiler" );

    // vk::ApplicationInfo allows the programmer to specifiy some basic information about the
    // program, which can be useful for layers and tools to provide more debug information.
    vk::ApplicationInfo appInfo = vk::ApplicationInfo()
        .setPApplicationName( "Vulkan C++ Windowed Program Template" )
        .setApplicationVersion( 1 )
        .setPEngineName( "LunarG SDK" )
        .setEngineVersion( 1 )
        .setApiVersion( VK_API_VERSION_1_0 );

    // vk::InstanceCreateInfo is where the programmer specifies the layers and/or extensions that
    // are needed.
    vk::InstanceCreateInfo instInfo = vk::InstanceCreateInfo()
        .setFlags( vk::InstanceCreateFlags() )
        .setPApplicationInfo( &appInfo )
        .setEnabledExtensionCount( static_cast<uint32_t>(extensions.size()) )
        .setPpEnabledExtensionNames( extensions.data() )
        .setEnabledLayerCount( static_cast<uint32_t>(layers.size()) )
        .setPpEnabledLayerNames( layers.data() );

    // Create the Vulkan instance.
    vk::Instance instance;
    try
    {
        instance = vk::createInstance( instInfo );
    }
    catch( const std::exception& e )
    {
        std::cout << "Could not create a Vulkan instance: " << e.what() << std::endl;
        return 1;
    }

    // Create a Vulkan surface for rendering
    VkSurfaceKHR c_surface;
    if( !SDL_Vulkan_CreateSurface( window, static_cast<VkInstance>(instance), &c_surface ) )
    {
        std::cout << "Could not create a Vulkan surface." << std::endl;
        return 1;
    }
    vk::SurfaceKHR surface( c_surface );

    // Create the Vulkan device
    Sample::Device device( instance, surface, layers,
        {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
        } );

    Sample::SwapChain swapchain( device, surface, true );

    std::vector<vk::AttachmentDescription> renderPassAttachments = {
        vk::AttachmentDescription(
            vk::AttachmentDescriptionFlags(),
            swapchain.m_Format,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eColorAttachmentOptimal ) };

    std::vector<vk::AttachmentReference> renderPassAttachmentReferences = {
        vk::AttachmentReference( 0, vk::ImageLayout::eColorAttachmentOptimal ) };

    std::vector<vk::SubpassDescription> renderPassSubpasses = {
        vk::SubpassDescription(
            vk::SubpassDescriptionFlags(),
            vk::PipelineBindPoint::eGraphics,
            static_cast<uint32_t>(0), nullptr,
            static_cast<uint32_t>(renderPassAttachmentReferences.size()), renderPassAttachmentReferences.data(),
            nullptr,
            nullptr,
            static_cast<uint32_t>(0), nullptr ) };

    vk::RenderPass renderPass = device.m_Device.createRenderPass(
        vk::RenderPassCreateInfo(
            vk::RenderPassCreateFlags(),
            static_cast<uint32_t>(renderPassAttachments.size()), renderPassAttachments.data(),
            static_cast<uint32_t>(renderPassSubpasses.size()), renderPassSubpasses.data(),
            static_cast<uint32_t>(0), nullptr ) );

    std::vector<vk::ImageView> imageViews;
    std::vector<vk::Framebuffer> framebuffers;

    for( auto& image : swapchain.m_Images )
    {
        imageViews.push_back( device.m_Device.createImageView( vk::ImageViewCreateInfo(
            vk::ImageViewCreateFlags(),
            image.m_Image,
            vk::ImageViewType::e2D,
            image.m_Format,
            vk::ComponentMapping(),
            vk::ImageSubresourceRange( vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 ) ) ) );

        framebuffers.push_back( device.m_Device.createFramebuffer( vk::FramebufferCreateInfo(
            vk::FramebufferCreateFlags(),
            renderPass,
            1, &imageViews.back(),
            image.m_Extent.width,
            image.m_Extent.height,
            1 ) ) );
    }

    vk::ClearValue clearValue( std::array<float, 4>( { 0.f, 0.f, 0.f, 1.f } ) );

    vk::CommandPool commandPool = device.m_Device.createCommandPool(
        vk::CommandPoolCreateInfo(
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            device.m_QueueFamilyIndices.m_GraphicsQueueFamilyIndex ) );

    vk::CommandBuffer commandBuffer = device.m_Device.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo(
            commandPool,
            vk::CommandBufferLevel::ePrimary,
            1 ) ).front();


    // This is where most initializtion for a program should be performed
 
    // Poll for user input.
    bool stillRunning = true;
    while( stillRunning )
    {

        SDL_Event event;
        while( SDL_PollEvent( &event ) )
        {

            switch( event.type )
            {

            case SDL_QUIT:
                stillRunning = false;
                break;

            default:
                // Do nothing.
                break;
            }
        }

        swapchain.acquireNextImage();

        commandBuffer.begin( vk::CommandBufferBeginInfo( vk::CommandBufferUsageFlagBits::eSimultaneousUse ) );
        commandBuffer.beginRenderPass( vk::RenderPassBeginInfo(
            renderPass,
            framebuffers[swapchain.m_AcquiredImageIndex],
            vk::Rect2D( swapchain.m_Extent.width, swapchain.m_Extent.height ),
            1, &clearValue ),
            vk::SubpassContents::eInline );

        commandBuffer.endRenderPass();
        commandBuffer.end();

        device.m_GraphicsQueue.submit( { vk::SubmitInfo( 0, nullptr, nullptr, 1, &commandBuffer, 0, nullptr ) },
            nullptr );

        device.m_PresentQueue.presentKHR(
            vk::PresentInfoKHR(
                static_cast<uint32_t>(1), &swapchain.m_ImageRenderedSemaphores[swapchain.m_AcquiredImageIndex],
                static_cast<uint32_t>(1), &swapchain.m_Swapchain,
                &swapchain.m_AcquiredImageIndex ) );
    }

    // Clean up.
    for( auto& framebuffer : framebuffers ) device.m_Device.destroyFramebuffer( framebuffer );
    for( auto& imageView : imageViews ) device.m_Device.destroyImageView( imageView );
    swapchain.destroy();
    instance.destroySurfaceKHR( surface );
    SDL_DestroyWindow( window );
    SDL_Quit();
    device.destroy();
    instance.destroy();

    return 0;
}

