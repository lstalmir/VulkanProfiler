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
            dd.Profiler.m_Config.m_EnableOverlay;

        if( createProfilerOverlay )
        {
            // Make sure we are able to write to presented image
            createInfo.imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
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
            dd.Profiler.SetDataBufferSize( swapchainImageCount + 1 );
        }

        if( createProfilerOverlay )
        {
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

            if( result == VK_SUCCESS && !dd.Overlay.IsAvailable() )
            {
                // Initialize overlay for the first time
                bool success = dd.Overlay.Initialize( dd.ProfilerFrontend, dd.OverlayBackend );

                if( success )
                {
                    // Initialize overlay with configuration
                    dd.Overlay.SetMaxFrameCount( std::max( dd.Profiler.m_Config.m_FrameCount, 0 ) );

                    if( !dd.Profiler.m_Config.m_RefPipelines.empty() )
                    {
                        dd.Overlay.LoadTopPipelinesFromFile( dd.Profiler.m_Config.m_RefPipelines );
                    }

                    if( !dd.Profiler.m_Config.m_RefMetrics.empty() )
                    {
                        dd.Overlay.LoadPerformanceCountersFromFile( dd.Profiler.m_Config.m_RefMetrics );
                    }
                }

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
        if( (dd.Overlay.IsAvailable()) && (dd.OverlayBackend.GetSwapchain() == swapchain) )
        {
            dd.Overlay.Destroy();
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

        // Display overlay
        if( (dd.Overlay.IsAvailable()) &&
            (dd.OverlayBackend.GetSwapchain() == pPresentInfo->pSwapchains[ 0 ]) )
        {
            dd.OverlayBackend.SetFramePresentInfo( dd.Device.Queues.at( queue ), *pPresentInfo );
            dd.Overlay.Update();
            pPresentInfo = &dd.OverlayBackend.GetFramePresentInfo();
        }

        dd.Device.TIP.Reset();

        // Present the image
        return dd.Device.Callbacks.QueuePresentKHR( queue, pPresentInfo );
    }
}
