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

#include "profiler_overlay_layer_backend_android.h"
#include <backends/imgui_impl_android.h>

#include <stdlib.h>
#include <array>
#include <mutex>

namespace Profiler
{
    extern std::mutex s_ImGuiMutex;

    /***********************************************************************************\

    Function:
        OverlayLayerAndroidPlatformBackend

    Description:
        Constructor.

        s_ImGuiMutex must be locked before creating the window context.

    \***********************************************************************************/
    OverlayLayerAndroidPlatformBackend::OverlayLayerAndroidPlatformBackend( ANativeWindow* window ) try
        : m_pImGuiContext( nullptr )
        , m_AppWindow( window )
    {
        if( !ImGui_ImplAndroid_Init( m_AppWindow ) )
        {
            throw;
        }

        m_pImGuiContext = ImGui::GetCurrentContext();
    }
    catch (...)
    {
        // Cleanup the partially initialized context.
        OverlayLayerAndroidPlatformBackend::~OverlayLayerAndroidPlatformBackend();
        throw;
    }

    /***********************************************************************************\

    Function:
        ~OverlayLayerAndroidPlatformBackend

    Description:
        Destructor.

        s_ImGuiMutex must be locked before destroying the window context.

    \***********************************************************************************/
    OverlayLayerAndroidPlatformBackend::~OverlayLayerAndroidPlatformBackend()
    {
        m_AppWindow = nullptr;

        // Uninitialize the backend
        if( m_pImGuiContext )
        {
            IM_ASSERT( ImGui::GetCurrentContext() == m_pImGuiContext );
            ImGui_ImplAndroid_Shutdown();
        }
    }

    /***********************************************************************************\

    Function:
        NewFrame

    Description:
        Handle incoming events.

    \***********************************************************************************/
    void OverlayLayerAndroidPlatformBackend::NewFrame()
    {
        IM_ASSERT( ImGui::GetCurrentContext() == m_pImGuiContext );
        ImGui_ImplAndroid_NewFrame();
    }

    /***********************************************************************************\

    Function:
        GetDPIScale

    Description:
        Increase scaling factor to improve readability on mobile devices.

    \***********************************************************************************/
    float OverlayLayerAndroidPlatformBackend::GetDPIScale() const
    {
        return 2.0f;
    }
}

// Include the actual backend implementation.
#include <backends/imgui_impl_android.cpp>
