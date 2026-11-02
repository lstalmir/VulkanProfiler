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

#include "profiler_overlay_layer_backend_wayland.h"
#include <imgui_internal.h>
#include <stdlib.h>
#include <array>
#include <unistd.h>
#include <sys/mman.h>
#include <syscall.h>

#define trace_fn( args, ... ) printf( "%s (" args ")\n", __func__, __VA_ARGS__ )

namespace Profiler
{
    // Flags provided by this backend.
    static constexpr ImGuiBackendFlags s_WaylandPlatformBackendFlags = 0;

    const wl_registry_listener OverlayLayerWaylandPlatformBackend::m_scRegistryListener = {
        &OverlayLayerWaylandPlatformBackend::HandleGlobal,
        &OverlayLayerWaylandPlatformBackend::HandleGlobalRemove 
    };

    const wl_shell_surface_listener OverlayLayerWaylandPlatformBackend::m_scShellSurfaceListener = {
        &OverlayLayerWaylandPlatformBackend::HandleShellSurfacePing,
        &OverlayLayerWaylandPlatformBackend::HandleShellSurfaceConfigure,
        nullptr
    };

    const xdg_surface_listener OverlayLayerWaylandPlatformBackend::m_scXdgSurfaceListener = {
        &OverlayLayerWaylandPlatformBackend::HandleXdgSurfaceConfigure
    };

    const xdg_toplevel_listener OverlayLayerWaylandPlatformBackend::m_scXdgToplevelListener = {
        &OverlayLayerWaylandPlatformBackend::HandleXdgToplevelConfigure,
        &OverlayLayerWaylandPlatformBackend::HandleXdgToplevelClose,
        &OverlayLayerWaylandPlatformBackend::HandleXdgToplevelConfigureBounds,
        &OverlayLayerWaylandPlatformBackend::HandleXdgToplevelCapabilities
    };

    const wl_seat_listener OverlayLayerWaylandPlatformBackend::m_scSeatListener = {
        &OverlayLayerWaylandPlatformBackend::HandleSeatCapabilities,
        &OverlayLayerWaylandPlatformBackend::HandleSeatName
    };

    const wl_pointer_listener OverlayLayerWaylandPlatformBackend::m_scPointerListener = {
        &OverlayLayerWaylandPlatformBackend::HandlePointerEnter,
        &OverlayLayerWaylandPlatformBackend::HandlePointerLeave,
        &OverlayLayerWaylandPlatformBackend::HandlePointerMotion,
        &OverlayLayerWaylandPlatformBackend::HandlePointerButton,
        &OverlayLayerWaylandPlatformBackend::HandlePointerAxis,
        &OverlayLayerWaylandPlatformBackend::HandlePointerFrame,
        &OverlayLayerWaylandPlatformBackend::HandlePointerAxisSource,
        &OverlayLayerWaylandPlatformBackend::HandlePointerAxisStop,
        &OverlayLayerWaylandPlatformBackend::HandlePointerAxisDiscrete
    };

    const wl_keyboard_listener OverlayLayerWaylandPlatformBackend::m_scKeyboardListener = {
        &OverlayLayerWaylandPlatformBackend::HandleKeyboardKeymap,
        &OverlayLayerWaylandPlatformBackend::HandleKeyboardEnter,
        &OverlayLayerWaylandPlatformBackend::HandleKeyboardLeave,
        &OverlayLayerWaylandPlatformBackend::HandleKeyboardKey,
        &OverlayLayerWaylandPlatformBackend::HandleKeyboardModifiers,
        &OverlayLayerWaylandPlatformBackend::HandleKeyboardRepeat
    };

    template<typename T>
    static void DestroyObject( T** ppObject, void( *pfnRelease )(T*) )
    {
        if( *ppObject != nullptr )
        {
            pfnRelease( *ppObject );
            *ppObject = nullptr;
        }
    }

    /***********************************************************************************\

    Function:
        OverlayLayerWaylandPlatformBackend

    Description:
        Constructor.

    \***********************************************************************************/
    OverlayLayerWaylandPlatformBackend::OverlayLayerWaylandPlatformBackend( wl_surface* surface ) try
        : m_pXkbBackend( nullptr )
        , m_Display( nullptr )
        , m_Registry( nullptr )
        , m_Compositor( nullptr )
        , m_AppSurface( surface )
        , m_InputSurface( nullptr )
        , m_InputSubsurface( nullptr )
        , m_InputRegion( nullptr )
        , m_InputBuffer( nullptr )
        , m_InputBufferFd( -1 )
        , m_InputBufferSize( 0 )
        , m_pInputBufferMemory( nullptr )
        , m_Shell( nullptr )
        , m_XdgShell( nullptr )
        , m_XdgSurface( nullptr )
        , m_XdgToplevel( nullptr )
        , m_Seat( nullptr )
        , m_SeatCapabilities( 0 )
        , m_Pointer( nullptr )
        , m_Keyboard( nullptr )
    {
        // Create XKB backend
        m_pXkbBackend = new OverlayLayerXkbBackend();

        // Connect to Wayland server
        m_Display = wl_display_connect( nullptr );
        if( !m_Display )
            throw;

        m_Registry  = wl_display_get_registry( m_Display );
        if( !m_Registry )
            throw;

        // Register globals
        wl_registry_add_listener( m_Registry, &m_scRegistryListener, this );
        wl_display_roundtrip( m_Display );
        if( !m_Compositor || !m_Subcompositor || !m_Seat )
            throw;

        // Another roundtrip needed to initialize input devices
        wl_display_roundtrip( m_Display );

        // Create a subsurface to capture input events
        m_InputSurface = wl_compositor_create_surface( m_Compositor );
        if( !m_InputSurface )
            throw;

        // Create top-level surface depending on the available interfaces
        if( m_XdgShell )
        {
            m_XdgSurface = xdg_wm_base_get_xdg_surface( m_XdgShell, m_InputSurface );
            if( !m_XdgSurface )
                throw;

            m_XdgToplevel = xdg_surface_get_toplevel( m_XdgSurface );
            if( !m_XdgToplevel )
                throw;

            xdg_surface_add_listener( m_XdgSurface, &m_scXdgSurfaceListener, this );
            xdg_toplevel_add_listener( m_XdgToplevel, &m_scXdgToplevelListener, this );
        }
        else if( m_Shell )
        {
            m_ShellSurface = wl_shell_get_shell_surface( m_Shell, m_InputSurface );
            if( !m_ShellSurface )
                throw;

            wl_shell_surface_add_listener( m_ShellSurface, &m_scShellSurfaceListener, this );
            wl_shell_surface_set_toplevel( m_ShellSurface );
        }

        // Allocate memory buffer for the input surface
        m_InputBufferFd = static_cast<int>( ::syscall( SYS_memfd_create, "buffer", 0 ) );
        if( m_InputBufferFd < 0 )
            throw;

        m_InputBufferSize = 4096 * 4096 * 4;
        if( ::ftruncate( m_InputBufferFd, m_InputBufferSize ) != 0 )
            throw;

        m_ShmPool = wl_shm_create_pool( m_Shm, m_InputBufferFd, m_InputBufferSize );
        if( !m_ShmPool )
            throw;

        m_pInputBufferMemory = ::mmap( nullptr, m_InputBufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, m_InputBufferFd, 0 );
        if( !m_pInputBufferMemory )
            throw;

        m_InputBuffer = wl_shm_pool_create_buffer( m_ShmPool, 0, 1024, 1024, 4096, WL_SHM_FORMAT_ABGR8888 );
        if( !m_InputBuffer )
            throw;

        //m_InputRegion = wl_compositor_create_region( m_Compositor );
        //if( !m_InputRegion )
        //    throw;

        // Make the input surface a subsurface of the application's surface to capture its input
        //m_InputSubsurface = wl_subcompositor_get_subsurface( m_Subcompositor, m_InputSurface, m_AppSurface );
        //if( !m_InputSubsurface )
        //    throw;

        //wl_subsurface_set_sync( m_InputSubsurface );

        //wl_subsurface_place_above( m_InputSubsurface, m_AppSurface );
        //wl_surface_set_input_region( m_InputSurface, m_InputRegion );

        wl_surface_commit( m_InputSurface );
    }
    catch (...)
    {
        Destroy();
        throw;
    }

    /***********************************************************************************\

    Function:
        ~OverlayLayerWaylandPlatformBackend

    Description:
        Destructor.

    \***********************************************************************************/
    OverlayLayerWaylandPlatformBackend::~OverlayLayerWaylandPlatformBackend()
    {
        Destroy();
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Free all Wayland resources created for this backend.

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::Destroy()
    {
        m_AppSurface = nullptr;

        DestroyObject( &m_XdgToplevel, xdg_toplevel_destroy );
        DestroyObject( &m_XdgSurface, xdg_surface_destroy );
        DestroyObject( &m_XdgShell, xdg_wm_base_destroy );

        DestroyObject( &m_ShellSurface, wl_shell_surface_destroy );
        DestroyObject( &m_Shell, wl_shell_destroy );

        DestroyObject( &m_ShmPool, wl_shm_pool_destroy );
        DestroyObject( &m_Shm, wl_shm_destroy );

        DestroyObject( &m_Pointer, wl_pointer_release );
        DestroyObject( &m_Keyboard, wl_keyboard_release );
        DestroyObject( &m_Seat, wl_seat_release );
        DestroyObject( &m_InputBuffer, wl_buffer_destroy );
        DestroyObject( &m_InputRegion, wl_region_destroy );
        DestroyObject( &m_InputSubsurface, wl_subsurface_destroy );
        DestroyObject( &m_InputSurface, wl_surface_destroy );
        DestroyObject( &m_Subcompositor, wl_subcompositor_destroy );
        DestroyObject( &m_Compositor, wl_compositor_destroy );
        DestroyObject( &m_Registry, wl_registry_destroy );
        DestroyObject( &m_Display, wl_display_disconnect );

        if( m_pInputBufferMemory )
        {
            ::munmap( m_pInputBufferMemory, m_InputBufferSize );
            m_pInputBufferMemory = nullptr;
            m_InputBufferSize = 0;
        }

        if( m_InputBufferFd >= 0 )
        {
            ::close( m_InputBufferFd );
            m_InputBufferFd = -1;
        }

        delete m_pXkbBackend;
        m_pXkbBackend = nullptr;
    }

    /***********************************************************************************\

    Function:
        PrepareImGuiBackend

    Description:
        Initialize ImGui backend for Xlib platform.

    \***********************************************************************************/
    bool OverlayLayerWaylandPlatformBackend::PrepareImGuiBackend()
    {
        ImGuiContext* pContext = ImGui::GetCurrentContext();
        IM_ASSERT( pContext && "OverlayLayerWaylandPlatformBackend::PrepareImGuiBackend called when no ImGui context was set." );
        if( !pContext )
        {
            return false;
        }

        ImGuiIO& io = ImGui::GetIO();
        io.BackendFlags |= s_WaylandPlatformBackendFlags;
        io.BackendPlatformName = "wayland";
        io.BackendPlatformUserData = this;

        ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
        platformIO.Platform_GetClipboardTextFn = nullptr;
        platformIO.Platform_SetClipboardTextFn = nullptr;

        return true;
    }

    /***********************************************************************************\

    Function:
        DestroyImGuiBackend

    Description:
        Destroy ImGui backend.

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::DestroyImGuiBackend()
    {
        if( !ValidateContext() )
        {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        io.BackendFlags &= ~s_WaylandPlatformBackendFlags;
        io.BackendPlatformName = nullptr;
        io.BackendPlatformUserData = nullptr;

        ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
        platformIO.ClearPlatformHandlers();
    }

    /***********************************************************************************\

    Function:
        NewFrame

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::NewFrame()
    {
        // Validate the current ImGui context.
        if( !ValidateContext() )
        {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();

#if 0
        // Setup display size (every frame to accommodate for window resizing).
        auto geometry = GetGeometry( m_AppWindow );
        io.DisplaySize = ImVec2( (float)(geometry.width), (float)(geometry.height));

        const uint32_t inputWindowChanges[2] = {
            static_cast<uint32_t>( geometry.width ),
            static_cast<uint32_t>( geometry.height )
        };

        // Update OS mouse position.
        UpdateMousePos();
#endif

#if 0
        // Update input capture rects.
        for( ImGuiWindow* pWindow : GImGui->Windows )
        {
            if( pWindow && pWindow->WasActive )
            {
                wl_region_add( m_InputRegion,
                    static_cast<int32_t>( pWindow->Pos.x ),
                    static_cast<int32_t>( pWindow->Pos.y ),
                    static_cast<int32_t>( pWindow->Size.x ),
                    static_cast<int32_t>( pWindow->Size.y ) );
            }
        }

        wl_surface_set_input_region( m_InputSurface, m_InputRegion );
        wl_surface_commit( m_InputSurface );
#endif

        // Process pending messages.
        wl_display_roundtrip( m_Display );
    }

    /***********************************************************************************\

    Function:
        ValidateContext

    Description:

    \***********************************************************************************/
    bool OverlayLayerWaylandPlatformBackend::ValidateContext() const
    {
        ImGuiContext* pContext = ImGui::GetCurrentContext();
        IM_ASSERT( pContext && "OverlayLayerWaylandPlatformBackend function called when no ImGui context was set." );
        if( !pContext )
        {
            return false;
        }

        ImGuiIO& io = ImGui::GetIO();
        IM_ASSERT( io.BackendPlatformUserData == this && "OverlayLayerWaylandPlatformBackend function called but different backend is set." );
        if( io.BackendPlatformUserData != this )
        {
            return false;
        }

        return true;
    }

    /***********************************************************************************\

    Function:
        HandleGlobal

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleGlobal( void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version )
    {
        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        if( !bd )
            return;

        if( strcmp( interface, wl_compositor_interface.name ) == 0 )
        {
            bd->m_Compositor = static_cast<wl_compositor*>( wl_registry_bind(
                registry,
                name,
                &wl_compositor_interface,
                version ) );
            return;
        }

        if( strcmp( interface, wl_subcompositor_interface.name ) == 0 )
        {
            bd->m_Subcompositor = static_cast<wl_subcompositor*>( wl_registry_bind(
                registry,
                name,
                &wl_subcompositor_interface,
                version ) );
            return;
        }

        if( strcmp( interface, wl_seat_interface.name ) == 0 )
        {
            bd->m_Seat = static_cast<wl_seat*>( wl_registry_bind(
                registry,
                name,
                &wl_seat_interface,
                version ) );

            wl_seat_add_listener( bd->m_Seat, &m_scSeatListener, data );
            return;
        }

        if( strcmp( interface, wl_shell_interface.name ) == 0 )
        {
            bd->m_Shell = static_cast<wl_shell*>( wl_registry_bind(
                registry,
                name,
                &wl_shell_interface,
                version ) );
            return;
        }

        if( strcmp( interface, xdg_wm_base_interface.name ) == 0 )
        {
            bd->m_XdgShell = static_cast<xdg_wm_base*>( wl_registry_bind(
                registry,
                name,
                &xdg_wm_base_interface,
                version ) );
            return;
        }

        if ( strcmp( interface, wl_shm_interface.name ) == 0 )
        {
            bd->m_Shm = static_cast<wl_shm*>( wl_registry_bind(
                registry,
                name,
                &wl_shm_interface,
                version ) );
            return;
        }
    }

    /***********************************************************************************\

    Function:
        HandleGlobalRemove

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleGlobalRemove( void* data, wl_registry* registry, uint32_t name )
    {
    }

    /***********************************************************************************\

    Function:
        HandleShellSurfacePing

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleShellSurfacePing( void* data, wl_shell_surface* surface, uint32_t serial )
    {
        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        if( !bd || bd->m_ShellSurface != surface )
        {
            return;
        }

        wl_shell_surface_pong( surface, serial );
    }

    /***********************************************************************************\

    Function:
        HandleShellSurfaceConfigure

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleShellSurfaceConfigure( void* data, wl_shell_surface* surface, uint32_t edges, int32_t width, int32_t height )
    {
        trace_fn( "%p, %p, %u, %i, %i", data, surface, edges, width, height );

        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        if( !bd || bd->m_ShellSurface != surface )
        {
            return;
        }

        wl_surface_attach( bd->m_InputSurface, bd->m_InputBuffer, 0, 0 );
        wl_surface_commit( bd->m_InputSurface );
    }

    /***********************************************************************************\

    Function:
        HandleXdgSurfaceConfigure

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleXdgSurfaceConfigure( void* data, xdg_surface* surface, uint32_t serial )
    {
        trace_fn( "%p, %p, %u", data, surface, serial );

        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        if( !bd || bd->m_XdgSurface != surface )
        {
            return;
        }

        xdg_surface_ack_configure( surface, serial );

        wl_surface_attach( bd->m_InputSurface, bd->m_InputBuffer, 0, 0 );
        wl_surface_commit( bd->m_InputSurface );
    }

    /***********************************************************************************\

    Function:
        HandleXdgToplevelConfigure

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleXdgToplevelConfigure( void* data, xdg_toplevel* toplevel, int32_t width, int32_t height, wl_array* states )
    {
        trace_fn( "%p, %p, %i, %i, %p", data, toplevel, width, height, states );
    }

    /***********************************************************************************\

    Function:
        HandleXdgToplevelClose

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleXdgToplevelClose( void* data, xdg_toplevel* toplevel )
    {
        trace_fn( "%p, %p", data, toplevel );
    }

    /***********************************************************************************\

    Function:
        HandleXdgToplevelConfigureBounds

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleXdgToplevelConfigureBounds( void* data, xdg_toplevel* toplevel, int32_t width, int32_t height )
    {
        trace_fn( "%p, %p, %i, %i", data, toplevel, width, height );
    }

    /***********************************************************************************\

    Function:
        HandleXdgToplevelCapabilities

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleXdgToplevelCapabilities( void* data, xdg_toplevel* toplevel, wl_array* capabilities )
    {
        trace_fn( "%p, %p, %p", data, toplevel, capabilities );
    }

    /***********************************************************************************\

    Function:
        HandleSeatCapabilities

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleSeatCapabilities( void* data, wl_seat* seat, uint32_t capabilities )
    {
        trace_fn( "%p, %p, %u", data, seat, capabilities );

        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        if( !bd )
        {
            return;
        }

        bd->m_SeatCapabilities = capabilities;

        // Register mouse handlers
        if( capabilities & WL_SEAT_CAPABILITY_POINTER )
        {
            wl_pointer* pointer = wl_seat_get_pointer( seat );
            if( bd->m_Pointer != pointer )
            {
                DestroyObject( &bd->m_Pointer, wl_pointer_release );
                bd->m_Pointer = pointer;

                wl_pointer_add_listener( bd->m_Pointer, &m_scPointerListener, data );
            }
        }
        else
        {
            // Pointer device disconnected.
            DestroyObject( &bd->m_Pointer, wl_pointer_release );
        }

        // Register keyboard handlers
        if( capabilities & WL_SEAT_CAPABILITY_KEYBOARD )
        {
            wl_keyboard* keyboard = wl_seat_get_keyboard( seat );
            if( bd->m_Keyboard != keyboard )
            {
                DestroyObject( &bd->m_Keyboard, wl_keyboard_release );
                bd->m_Keyboard = keyboard;

                wl_keyboard_add_listener( bd->m_Keyboard, &m_scKeyboardListener, data );
            }
        }
        else
        {
            // Keyboard device disconnected.
            DestroyObject( &bd->m_Keyboard, wl_keyboard_release );
        }
    }

    /***********************************************************************************\

    Function:
        HandleSeatName

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleSeatName( void* data, wl_seat* seat, const char* name )
    {
        trace_fn( "%p, %p, %s", data, seat, name );
    }

    /***********************************************************************************\

    Function:
        HandlePointerEnter

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandlePointerEnter( void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y )
    {
        trace_fn( "%p, %p, %u, %p, %lf, %lf", data, pointer, serial, surface, wl_fixed_to_double( x ), wl_fixed_to_double( y ) );

        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        if( !bd || !bd->ValidateContext() )
        {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        io.AddMousePosEvent(
            static_cast<float>( wl_fixed_to_double( x ) ),
            static_cast<float>( wl_fixed_to_double( y ) ) );
    }

    /***********************************************************************************\

    Function:
        HandlePointerLeave

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandlePointerLeave( void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface )
    {
        trace_fn( "%p, %p, %u, %p", data, pointer, serial, surface );
    }

    /***********************************************************************************\

    Function:
        HandlePointerMotion

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandlePointerMotion( void* data, wl_pointer* pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y )
    {
        trace_fn( "%p, %p, %u, %lf, %lf", data, pointer, time, wl_fixed_to_double( x ), wl_fixed_to_double( y ) );

        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        if( !bd || !bd->ValidateContext() )
        {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        io.AddMousePosEvent(
            static_cast<float>( wl_fixed_to_double( x ) ),
            static_cast<float>( wl_fixed_to_double( y ) ) );
    }

    /***********************************************************************************\

    Function:
        HandlePointerButton

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandlePointerButton( void* data, wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state )
    {
        trace_fn( "%p, %p, %u, %u, %u, %u", data, pointer, serial, time, button, state );

        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        if( !bd || !bd->ValidateContext() )
        {
            return;
        }

        // TODO: Impl
    }

    /***********************************************************************************\

    Function:
        HandlePointerAxis

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandlePointerAxis( void* data, wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value )
    {
        trace_fn( "%p, %p, %u, %u, %lf", data, pointer, time, axis, wl_fixed_to_double( value ) );

        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        if( !bd || !bd->ValidateContext() )
        {
            return;
        }

        // TODO: Impl
    }

    /***********************************************************************************\

    Function:
        HandlePointerAxisSource

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandlePointerAxisSource( void* data, wl_pointer* pointer, uint32_t source )
    {
        trace_fn( "%p, %p, %u", data, pointer, source );

        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        if( !bd || !bd->ValidateContext() )
        {
            return;
        }

        // TODO: Impl
    }

    /***********************************************************************************\

    Function:
        HandlePointerAxisStop

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandlePointerAxisStop( void* data, wl_pointer* pointer, uint32_t time, uint32_t axis )
    {
        trace_fn( "%p, %p, %u, %u", data, pointer, time, axis );

        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        if( !bd || !bd->ValidateContext() )
        {
            return;
        }

        // TODO: Impl
    }

    /***********************************************************************************\

    Function:
        HandlePointerAxisDiscrete

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandlePointerAxisDiscrete( void* data, wl_pointer* pointer, uint32_t axis, int32_t value )
    {
        trace_fn( "%p, %p, %u, %i", data, pointer, axis, value );

        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        if( !bd || !bd->ValidateContext() )
        {
            return;
        }

        // TODO: Impl
    }

    /***********************************************************************************\

    Function:
        HandlePointerFrame

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandlePointerFrame( void* data, wl_pointer* pointer )
    {
        trace_fn( "%p, %p", data, pointer );

        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        if( !bd || !bd->ValidateContext() )
        {
            return;
        }

        // TODO: Impl
    }

    /***********************************************************************************\

    Function:
        HandleKeyboardEnter

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleKeyboardEnter( void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys )
    {
        trace_fn( "%p, %p, %u, %p, %p", data, keyboard, serial, surface, keys );
    }

    /***********************************************************************************\

    Function:
        HandleKeyboardLeave

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleKeyboardLeave( void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface )
    {
        trace_fn( "%p, %p, %u, %p", data, keyboard, serial, surface );
    }

    /***********************************************************************************\

    Function:
        HandleKeyboardKeymap

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleKeyboardKeymap( void* data, wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size )
    {
        trace_fn( "%p, %p, %u, %i, %u", data, keyboard, format, fd, size );

        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        if( !bd || !bd->ValidateContext() )
        {
            return;
        }

        // Only classic XKB text format is currently supported.
        if( format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 )
        {
            void* mappedFd = ::mmap( NULL, size, PROT_READ, MAP_SHARED, fd, 0 );
            if( mappedFd != MAP_FAILED )
            {
                // Set the new keymap from the provided string.
                bd->m_pXkbBackend->SetKeymapFromString(
                    static_cast<const char*>( mappedFd ),
                    XKB_KEYMAP_FORMAT_TEXT_V1,
                    XKB_KEYMAP_COMPILE_NO_FLAGS );

                ::munmap( mappedFd, size );
            }
        }

        ::close( fd );
    }

    /***********************************************************************************\

    Function:
        HandleKeyboardKey

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleKeyboardKey( void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state )
    {
        trace_fn( "%p, %p, %u, %u, %u, %u", data, keyboard, serial, time, key, state );

        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        if( !bd || !bd->ValidateContext() )
        {
            return;
        }

        bd->m_pXkbBackend->AddKeyEvent( key, state == WL_KEYBOARD_KEY_STATE_PRESSED );
    }

    /***********************************************************************************\

    Function:
        HandleKeyboardModifiers

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleKeyboardModifiers( void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group )
    {
        trace_fn( "%p, %p, %u, %u, %u, %u, %u", data, keyboard, serial, depressed, latched, locked, group );

        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        if( !bd || !bd->ValidateContext() )
        {
            return;
        }

        bd->m_pXkbBackend->SetKeyModifiers( depressed, latched, locked, group );
    }

    /***********************************************************************************\

    Function:
        HandleKeyboardRepeat

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleKeyboardRepeat( void* data, wl_keyboard* keyboard, int32_t rate, int32_t delay )
    {
        trace_fn( "%p, %p, %i, %i", data, keyboard, rate, delay );
    }
}
