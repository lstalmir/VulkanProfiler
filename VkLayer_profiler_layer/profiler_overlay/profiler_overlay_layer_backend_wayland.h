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

#pragma once
#include "profiler_overlay_layer_backend.h"
#include "profiler_overlay_layer_backend_xkb.h"
#include <imgui.h>
#include <wayland-client.h>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        OverlayLayerWaylandPlatformBackend

    Description:
        Implementation of the backend for Wayland.

    \***********************************************************************************/
    class OverlayLayerWaylandPlatformBackend
        : public OverlayLayerPlatformBackend
    {
    public:
        OverlayLayerWaylandPlatformBackend( wl_surface* window );
        ~OverlayLayerWaylandPlatformBackend();

        void NewFrame() override;

    private:
        ImGuiContext* m_pImGuiContext;
        OverlayLayerXkbBackend* m_pXkbBackend;

        wl_display* m_Display;
        wl_registry* m_Registry;
        wl_shm* m_Shm;
        wl_compositor* m_Compositor;
        wl_seat* m_Seat;
        wl_surface* m_AppWindow;
        wl_surface* m_InputWindow;
        ImVector<wl_region> m_InputRects;

        xcb_get_geometry_reply_t GetGeometry( xcb_drawable_t );

        void UpdateMousePos();

        // Registry event handlers.
        static void HandleGlobal( void*, wl_registry*, uint32_t, const char*, uint32_t );
        static void HandleGlobalRemove( void*, wl_registry*, uint32_t );

        // Pointer event handlers.
        static void HandlePointerEnter( void*, wl_pointer*, uint32_t, wl_surface*, wl_fixed_t, wl_fixed_t );
        static void HandlePointerLeave( void*, wl_pointer*, uint32_t, wl_surface* );
        static void HandlePointerMotion( void*, wl_pointer*, uint32_t, wl_fixed_t, wl_fixed_t );
        static void HandlePointerButton( void*, wl_pointer*, uint32_t, uint32_t, uint32_t, uint32_t );
        static void HandlePointerAxis( void*, wl_pointer*, uint32_t, uint32_t, wl_fixed_t );
        static void HandlePointerAxisSource( void*, wl_pointer*, uint32_t );
        static void HandlePointerAxisStop( void*, wl_pointer*, uint32_t, uint32_t );
        static void HandlePointerAxisDiscrete( void*, wl_pointer*, uint32_t, int32_t );
        static void HandlePointerFrame( void*, wl_pointer* );
    };
}
