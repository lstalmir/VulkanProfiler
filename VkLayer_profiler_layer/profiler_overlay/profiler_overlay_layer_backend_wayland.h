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
#include <xdg-shell.h>

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
        OverlayLayerWaylandPlatformBackend( wl_surface* surface );
        ~OverlayLayerWaylandPlatformBackend();

        void Destroy();

        bool PrepareImGuiBackend() override;
        void DestroyImGuiBackend() override;

        void NewFrame() override;

    private:
        ImGuiContext* m_pImGuiContext;
        OverlayLayerXkbBackend* m_pXkbBackend;

        wl_display* m_Display;
        wl_registry* m_Registry;
        wl_compositor* m_Compositor;
        wl_subcompositor* m_Subcompositor;
        wl_surface* m_AppSurface;
        wl_surface* m_InputSurface;
        wl_subsurface* m_InputSubsurface;
        wl_region* m_InputRegion;
        wl_buffer* m_InputBuffer;
        int m_InputBufferFd;
        int m_InputBufferSize;
        void* m_pInputBufferMemory;

        wl_shm* m_Shm;
        wl_shm_pool* m_ShmPool;

        wl_shell* m_Shell;
        wl_shell_surface* m_ShellSurface;

        xdg_wm_base* m_XdgShell;
        xdg_surface* m_XdgSurface;
        xdg_toplevel* m_XdgToplevel;

        bool ValidateContext() const;

        // Registry event handlers.
        static void HandleGlobal( void*, wl_registry*, uint32_t, const char*, uint32_t );
        static void HandleGlobalRemove( void*, wl_registry*, uint32_t );

        static const wl_registry_listener m_scRegistryListener;

        // Shell surface event handlers.
        static void HandleShellSurfacePing( void*, wl_shell_surface*, uint32_t );
        static void HandleShellSurfaceConfigure( void*, wl_shell_surface*, uint32_t, int32_t, int32_t );

        static const wl_shell_surface_listener m_scShellSurfaceListener;

        // XDG surface event handlers.
        static void HandleXdgSurfaceConfigure( void*, xdg_surface*, uint32_t );

        static const xdg_surface_listener m_scXdgSurfaceListener;

        // XDG toplevel event handlers.
        static void HandleXdgToplevelConfigure( void*, xdg_toplevel*, int32_t, int32_t, wl_array* );
        static void HandleXdgToplevelClose( void*, xdg_toplevel* );
        static void HandleXdgToplevelConfigureBounds( void*, xdg_toplevel*, int32_t, int32_t );
        static void HandleXdgToplevelCapabilities( void*, xdg_toplevel*, wl_array* );

        static const xdg_toplevel_listener m_scXdgToplevelListener;

        // Seat event handlers.
        static void HandleSeatCapabilities( void*, wl_seat*, uint32_t );
        static void HandleSeatName( void*, wl_seat*, const char* );

        static const wl_seat_listener m_scSeatListener;

        wl_seat* m_Seat;
        uint32_t m_SeatCapabilities;

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

        static const wl_pointer_listener m_scPointerListener;

        wl_pointer* m_Pointer;

        // Keyboard event handlers.
        static void HandleKeyboardEnter( void*, wl_keyboard*, uint32_t, wl_surface*, wl_array* );
        static void HandleKeyboardLeave( void*, wl_keyboard*, uint32_t, wl_surface* );
        static void HandleKeyboardKeymap( void*, wl_keyboard*, uint32_t, int32_t, uint32_t );
        static void HandleKeyboardKey( void*, wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t );
        static void HandleKeyboardModifiers( void*, wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t );
        static void HandleKeyboardRepeat( void*, wl_keyboard*, int32_t, int32_t );

        static const wl_keyboard_listener m_scKeyboardListener;

        wl_keyboard* m_Keyboard;
    };
}
