// Copyright (c) 2019-2021 Lukasz Stalmirski
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

#include "VkSwapchainKhr_functions.h"
#include "profiler_layer_functions/Helpers.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        CreateSwapchainKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkSwapchainKhr_Functions::CreateSwapchainKHR(
        VkDevice device,
        const VkSwapchainCreateInfoKHR* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkSwapchainKHR* pSwapchain )
    {
        auto& dd = DeviceDispatch.Get( device );

        VkSwapchainCreateInfoKHR createInfo = *pCreateInfo;

        // TODO: Move to separate layer
        const bool createProfilerOverlay =
            (dd.Profiler.m_Config.m_Output == Profiler::output_t::overlay);

        if( createProfilerOverlay )
        {
            // Make sure we are able to write to presented image
            createInfo.imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }

        // Create the swapchain
        VkResult result = dd.Device.Callbacks.CreateSwapchainKHR(
            device, &createInfo, pAllocator, pSwapchain );

        // Create wrapping object
        if( result == VK_SUCCESS )
        {
            VkSwapchainKhr_Object swapchainObject = {};
            swapchainObject.Handle = *pSwapchain;
            swapchainObject.pSurface = &dd.Device.pInstance->Surfaces[ pCreateInfo->surface ];

            uint32_t swapchainImageCount = 0;
            dd.Device.Callbacks.GetSwapchainImagesKHR(
                device, *pSwapchain, &swapchainImageCount, nullptr );

            swapchainObject.Images.resize( swapchainImageCount );
            dd.Device.Callbacks.GetSwapchainImagesKHR(
                device, *pSwapchain, &swapchainImageCount, swapchainObject.Images.data() );

            dd.Device.Swapchains.emplace( *pSwapchain, swapchainObject );

            // Resize the buffers to fit all frames in flight so that the data is not dropped when the next present is late.
            dd.Profiler.SetMinDataBufferSize( swapchainImageCount + 1 );
        }

        if( createProfilerOverlay )
        {
            if( dd.pOutput )
            {
                // Destroy the previous output before creating an overlay
                dd.pOutput->Destroy();
            }

            if( result == VK_SUCCESS && !dd.OverlayBackend.IsInitialized() )
            {
                // Initialize overlay backend
                result = dd.OverlayBackend.Initialize( dd.Device );
            }

            if( result == VK_SUCCESS )
            {
                // Set the target swapchain for the overlay
                result = dd.OverlayBackend.SetSwapchain( *pSwapchain, *pCreateInfo );
            }

            if( result == VK_SUCCESS && !dynamic_cast<ProfilerOverlayOutput*>( dd.pOutput.get() ) )
            {
                // Destructor doesn't call Destroy, so explicit call is required to avoid resource leaks
                if( dd.pOutput )
                {
                    dd.pOutput->Destroy();
                    dd.pOutput.reset();
                }

                // Initialize overlay for the first time
                result = CreateUniqueObject<ProfilerOverlayOutput>(
                    &dd.pOutput,
                    dd.ProfilerFrontend,
                    dd.OverlayBackend );
            }

            if( result == VK_SUCCESS )
            {
                // Initialize the overlay output
                bool success = dd.pOutput->Initialize();
                if( !success )
                {
                    result = VK_ERROR_INITIALIZATION_FAILED;
                }
            }
        }

        return result;
    }

    /***********************************************************************************\

    Function:
        DestroySwapchainKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR void VKAPI_CALL VkSwapchainKhr_Functions::DestroySwapchainKHR(
        VkDevice device,
        VkSwapchainKHR swapchain,
        const VkAllocationCallbacks* pAllocator )
    {
        auto& dd = DeviceDispatch.Get( device );

        // After recreating swapchain using CreateSwapchainKHR parent swapchain of the overlay has changed.
        // The old swapchain is then destroyed and will invalidate the overlay if we don't check which
        // swapchain is actually being destreoyed.
        if( dd.OverlayBackend.GetSwapchain() == swapchain )
        {
            // Destroy the overlay output associated with the backend.
            if( dynamic_cast<ProfilerOverlayOutput*>( dd.pOutput.get() ) )
            {
                dd.pOutput->Destroy();
                dd.pOutput.reset();
            }

            dd.OverlayBackend.Destroy();
        }

        dd.Device.Swapchains.erase( swapchain );

        // Destroy the swapchain
        dd.Device.Callbacks.DestroySwapchainKHR( device, swapchain, pAllocator );
    }

    /***********************************************************************************\

    Function:
        QueuePresentKHR

    Description:

    \***********************************************************************************/
    VKAPI_ATTR VkResult VKAPI_CALL VkSwapchainKhr_Functions::QueuePresentKHR(
        VkQueue queue,
        const VkPresentInfoKHR* pPresentInfo )
    {
        auto& dd = DeviceDispatch.Get( queue );

        // End profiling of the previous frame
        dd.Profiler.FinishFrame();

        // Overlay rendering may be executed on a different queue than the one used for presenting.
        // Synchronization of rendering is required, so override the pointer to the present info.
        const bool overridePresentInfo =
            ( dd.OverlayBackend.IsInitialized() ) &&
            ( dd.OverlayBackend.GetSwapchain() == pPresentInfo->pSwapchains[0] );

        if( overridePresentInfo )
        {
            dd.OverlayBackend.SetFramePresentInfo( *pPresentInfo );
        }

        if( dd.pOutput )
        {
            // Consume the collected data from the profiler.
            // Treat QueuePresentKHR as a submit to collect at least one frame of data before the presentation.
            if( dd.Profiler.m_Config.m_FrameDelimiter >= VK_PROFILER_FRAME_DELIMITER_PRESENT_EXT )
            {
                dd.pOutput->Update();
            }

            // Present the data
            dd.pOutput->Present();
        }

        if( overridePresentInfo )
        {
            pPresentInfo = &dd.OverlayBackend.GetFramePresentInfo();
        }

        dd.Device.TIP.Reset();

        // Present the image
        std::shared_lock lock( dd.Device.Queues.at( queue ).Mutex );
        return dd.Device.Callbacks.QueuePresentKHR( queue, pPresentInfo );
    }
}
