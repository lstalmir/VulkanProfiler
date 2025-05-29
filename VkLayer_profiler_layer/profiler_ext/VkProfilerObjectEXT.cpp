// Copyright (c) 2025 Lukasz Stalmirski
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

#include "VkProfilerObjectEXT.h"
#include "VkDevice_functions.h"

using namespace Profiler;

/***************************************************************************************\

Function:
    vkGetProfilerEXT

Description:
    Return profiler object associated with the device.

\***************************************************************************************/
VKAPI_ATTR void VKAPI_CALL vkGetProfilerEXT(
    VkDevice device,
    VkProfilerEXT* pProfiler )
{
    (*pProfiler) = (VkProfilerEXT)( &VkDevice_Functions::DeviceDispatch.Get( device ).Profiler );
}

/***************************************************************************************\

Function:
    vkGetProfilerOverlayEXT

Description:
    Return profiler overlay object associated with the device.

\***************************************************************************************/
VKAPI_ATTR void VKAPI_CALL vkGetProfilerOverlayEXT(
    VkDevice device,
    VkProfilerOverlayEXT* pOverlay )
{
    auto& dd = VkDevice_Functions::DeviceDispatch.Get( device );

    auto pOverlayOutput = dynamic_cast<ProfilerOverlayOutput*>( dd.pOutput.get() );
    if( pOverlayOutput && pOverlayOutput->IsAvailable() )
    {
        (*pOverlay) = (VkProfilerOverlayEXT)( pOverlayOutput );
    }
    else
    {
        (*pOverlay) = VK_NULL_HANDLE;
    }
}
