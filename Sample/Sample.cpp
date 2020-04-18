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

#include <fstream>
#include <iostream>
#include <vector>

VkBool32 VKAPI_PTR DebugUtilsMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    printf( "%s\n", pCallbackData->pMessage );
    return true;
}

std::vector<char> ReadFile( std::string filename )
{
    std::ifstream file( filename, std::ios::binary | std::ios::in | std::ios::ate );
    std::vector<char> content( file.tellg(), 0 );
    file.seekg( std::ios::beg )
        .read( content.data(), content.size() );
    return content;
}

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
    extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );

    // Use validation layers if this is a debug build
    std::vector<const char*> layers;
    #if defined(_DEBUG)
    //layers.push_back( "VK_LAYER_LUNARG_standard_validation" );
    #endif
    //layers.push_back( "VK_LAYER_profiler" );

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
    catch( const std::exception & e )
    {
        std::cout << "Could not create a Vulkan instance: " << e.what() << std::endl;
        return 1;
    }

    // Prepare dispatch for debug marker extension
    struct DebugUtilsEXT_Dispatch
    {
        PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectNameEXT;
        PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTagEXT;
        PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
        PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
    };

    DebugUtilsEXT_Dispatch debugUtilsEXT;
    debugUtilsEXT.vkDebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT)instance.getProcAddr( "vkDebugMarkerSetObjectNameEXT" );
    debugUtilsEXT.vkDebugMarkerSetObjectTagEXT = (PFN_vkDebugMarkerSetObjectTagEXT)instance.getProcAddr( "vkDebugMarkerSetObjectTagEXT" );
    debugUtilsEXT.vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)instance.getProcAddr( "vkCreateDebugUtilsMessengerEXT" );
    debugUtilsEXT.vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)instance.getProcAddr( "vkDestroyDebugUtilsMessengerEXT" );

    // Create a debug messenger
    vk::DebugUtilsMessengerEXT debugMessenger = instance.createDebugUtilsMessengerEXT(
        vk::DebugUtilsMessengerCreateInfoEXT(
            vk::DebugUtilsMessengerCreateFlagsEXT(),
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            DebugUtilsMessengerCallback ),
        nullptr, debugUtilsEXT );

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
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::ePresentSrcKHR ) };

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

    std::vector<vk::CommandBuffer> commandBuffer = device.m_Device.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo(
            commandPool,
            vk::CommandBufferLevel::ePrimary,
            swapchain.m_Images.size() ) );

    vk::PipelineLayout pipelineLayout = device.m_Device.createPipelineLayout(
        vk::PipelineLayoutCreateInfo(
            vk::PipelineLayoutCreateFlags(),
            0, nullptr,
            0, nullptr ) );

    std::vector<char> vertexShaderModuleBytecode = ReadFile( "VertexShader.vert.hlsl.spv" );

    vk::ShaderModule vertexShaderModule = device.m_Device.createShaderModule(
        vk::ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlags(),
            vertexShaderModuleBytecode.size(),
            (uint32_t*)vertexShaderModuleBytecode.data() ) );

    std::vector<char> pixelShaderModuleBytecode = ReadFile( "FragmentShader.frag.hlsl.spv" );

    vk::ShaderModule pixelShaderModule = device.m_Device.createShaderModule(
        vk::ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlags(),
            pixelShaderModuleBytecode.size(),
            (uint32_t*)pixelShaderModuleBytecode.data() ) );

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = {
        vk::PipelineShaderStageCreateInfo(
            vk::PipelineShaderStageCreateFlags(),
            vk::ShaderStageFlagBits::eVertex,
            vertexShaderModule, "main", nullptr ),
        vk::PipelineShaderStageCreateInfo(
            vk::PipelineShaderStageCreateFlags(),
            vk::ShaderStageFlagBits::eFragment,
            pixelShaderModule, "main", nullptr ) };

    std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments = {
        vk::PipelineColorBlendAttachmentState()
            .setColorWriteMask(
                vk::ColorComponentFlagBits::eA |
                vk::ColorComponentFlagBits::eR |
                vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eB ) };

    vk::Pipeline pipeline = device.createGraphicsPipeline(
        pipelineLayout,
        renderPass,
        shaderStages,
        vk::PipelineVertexInputStateCreateInfo(),
        vk::PipelineInputAssemblyStateCreateInfo(
            vk::PipelineInputAssemblyStateCreateFlags(),
            vk::PrimitiveTopology::eTriangleList ),
        vk::PipelineViewportStateCreateInfo(
            vk::PipelineViewportStateCreateFlags(),
            1, &swapchain.m_Viewport,
            1, &swapchain.m_ScissorRect ),
        vk::PipelineRasterizationStateCreateInfo(
            vk::PipelineRasterizationStateCreateFlags(),
            false,
            false,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack,
            vk::FrontFace::eCounterClockwise,
            false, 0, 0, 0, 1 ),
        vk::PipelineMultisampleStateCreateInfo(),
        vk::PipelineDepthStencilStateCreateInfo(
            vk::PipelineDepthStencilStateCreateFlags(),
            false,
            false,
            vk::CompareOp::eAlways ),
        vk::PipelineColorBlendStateCreateInfo(
            vk::PipelineColorBlendStateCreateFlags(),
            false, vk::LogicOp::eClear,
            static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data(),
            { { 1.f, 1.f, 1.f, 1.f } } ) );

    vk::Viewport viewport = swapchain.m_Viewport;
    viewport.width /= 2;
    viewport.height /= 2;

    vk::Rect2D scissor = swapchain.m_ScissorRect;
    scissor.extent.width /= 2;
    scissor.extent.height /= 2;

    vk::Pipeline pipeline2 = device.createGraphicsPipeline(
        pipelineLayout,
        renderPass,
        shaderStages,
        vk::PipelineVertexInputStateCreateInfo(),
        vk::PipelineInputAssemblyStateCreateInfo(
            vk::PipelineInputAssemblyStateCreateFlags(),
            vk::PrimitiveTopology::eTriangleList ),
        vk::PipelineViewportStateCreateInfo(
            vk::PipelineViewportStateCreateFlags(),
            1, &viewport,
            1, &scissor ),
        vk::PipelineRasterizationStateCreateInfo(
            vk::PipelineRasterizationStateCreateFlags(),
            false,
            false,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack,
            vk::FrontFace::eCounterClockwise,
            false, 0, 0, 0, 1 ),
        vk::PipelineMultisampleStateCreateInfo(),
        vk::PipelineDepthStencilStateCreateInfo(
            vk::PipelineDepthStencilStateCreateFlags(),
            false,
            false,
            vk::CompareOp::eAlways ),
        vk::PipelineColorBlendStateCreateInfo(
            vk::PipelineColorBlendStateCreateFlags(),
            false, vk::LogicOp::eClear,
            static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data(),
            { { 1.f, 1.f, 1.f, 1.f } } ) );

    // This is where most initializtion for a program should be performed

    for( int i = 0; i < commandBuffer.size(); ++i )
    {
        commandBuffer[ i ].begin( vk::CommandBufferBeginInfo( vk::CommandBufferUsageFlagBits::eSimultaneousUse ) );
        commandBuffer[ i ].beginRenderPass( vk::RenderPassBeginInfo(
            renderPass,
            framebuffers[ i ],
            vk::Rect2D( vk::Offset2D(), swapchain.m_Extent ),
            1, &clearValue ),
            vk::SubpassContents::eInline );

        commandBuffer[ i ].bindPipeline( vk::PipelineBindPoint::eGraphics, pipeline );
        commandBuffer[ i ].draw( 3, 1, 0, 0 );
        commandBuffer[ i ].draw( 3, 1, 0, 0 );
        commandBuffer[ i ].draw( 3, 1, 0, 0 );
        commandBuffer[ i ].draw( 3, 1, 0, 0 );
        commandBuffer[ i ].draw( 3, 1, 0, 0 );
        commandBuffer[ i ].draw( 3, 1, 0, 0 );
        commandBuffer[ i ].draw( 3, 1, 0, 0 );
        commandBuffer[ i ].draw( 3, 1, 0, 0 );
        commandBuffer[ i ].draw( 3, 1, 0, 0 );

        commandBuffer[ i ].bindPipeline( vk::PipelineBindPoint::eGraphics, pipeline2 );
        commandBuffer[ i ].draw( 3, 1, 0, 0 );

        commandBuffer[ i ].endRenderPass();
        commandBuffer[ i ].end();

    }

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

        vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eTopOfPipe;

        device.m_GraphicsQueue.submit( { vk::SubmitInfo( 
            1, &swapchain.m_NextImageAvailableSemaphore, &waitStage,
            1, &commandBuffer[swapchain.m_AcquiredImageIndex],
            1, &swapchain.m_ImageRenderedSemaphores[swapchain.m_AcquiredImageIndex] ) },
            nullptr );

        device.m_PresentQueue.presentKHR(
            vk::PresentInfoKHR(
                1, &swapchain.m_ImageRenderedSemaphores[swapchain.m_AcquiredImageIndex],
                1, &swapchain.m_Swapchain,
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
    instance.destroyDebugUtilsMessengerEXT( debugMessenger, nullptr, debugUtilsEXT );
    instance.destroy();

    return 0;
}

