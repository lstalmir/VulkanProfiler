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
#include <mutex>
#include <unistd.h>
#include <sys/mman.h>

namespace Profiler
{
    extern std::mutex s_ImGuiMutex;

    const wl_registry_listener OverlayLayerWaylandPlatformBackend::m_scRegistryListener = {
        &OverlayLayerWaylandPlatformBackend::HandleGlobal,
        &OverlayLayerWaylandPlatformBackend::HandleGlobalRemove 
    };

    const xdg_toplevel_listener OverlayLayerWaylandPlatformBackend::m_scXdgToplevelListener = {
        &OverlayLayerWaylandPlatformBackend::HandleToplevelConfigure
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
    static void ReleaseIfNull( T** ppObject, void( *pfnRelease )(T*) )
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

        s_ImGuiMutex must be locked before creating the window context.

    \***********************************************************************************/
    OverlayLayerWaylandPlatformBackend::OverlayLayerWaylandPlatformBackend( wl_surface* surface ) try
        : m_pImGuiContext( nullptr )
        , m_pXkbBackend( nullptr )
        , m_Display( nullptr )
        , m_Registry( nullptr )
        , m_Compositor( nullptr )
        , m_AppSurface( surface )
        , m_InputSurface( nullptr )
        , m_InputSubsurface( nullptr )
        , m_InputRegion( nullptr )
        , m_Shell( nullptr )
        , m_XdgShell( nullptr )
        , m_XdgSurface( nullptr )
        , m_XdgToplevel( nullptr )
        , m_Seat( nullptr )
        , m_SeatCapabilities( 0 )
        , m_Pointer( nullptr )
        , m_PointerEvent{}
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

        if( m_XdgShell )
        {
            // Get XDG surface of the application's window
            m_XdgSurface = xdg_wm_base_get_xdg_surface( m_XdgShell, m_AppSurface );
            m_XdgToplevel = xdg_surface_get_toplevel( m_XdgSurface );

            xdg_toplevel_add_listener( m_XdgToplevel, &m_scXdgToplevelListener, this );
        }

        // Create a subsurface to capture input events
        m_InputSurface = wl_compositor_create_surface( m_Compositor );
        if( !m_InputSurface )
            throw;

        m_InputRegion = wl_compositor_create_region( m_Compositor );
        if( !m_InputRegion )
            throw;

        // Make the input surface a subsurface of the application's surface to capture its input
        m_InputSubsurface = wl_subcompositor_get_subsurface( m_Subcompositor, m_InputSurface, m_AppSurface );
        if( !m_InputSubsurface )
            throw;

        wl_subsurface_set_sync( m_InputSubsurface );

        wl_subsurface_place_above( m_InputSubsurface, m_AppSurface );
        wl_surface_set_input_region( m_InputSurface, m_InputRegion );
        wl_surface_commit( m_InputSurface );

        // Setup backend info
        ImGuiIO& io = ImGui::GetIO();
        io.BackendFlags = 0;
        io.BackendPlatformName = "wayland";
        io.BackendPlatformUserData = this;

        // ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
        // platformIO.Platform_GetClipboardTextFn = nullptr;
        // platformIO.Platform_SetClipboardTextFn = OverlayLayerXcbPlatformBackend::SetClipboardTextFn;

        m_pImGuiContext = ImGui::GetCurrentContext();
    }
    catch (...)
    {
        OverlayLayerWaylandPlatformBackend::~OverlayLayerWaylandPlatformBackend();
        throw;
    }

    /***********************************************************************************\

    Function:
        ~OverlayLayerWaylandPlatformBackend

    Description:
        Destructor.

        s_ImGuiMutex must be locked before destroying the window context.

    \***********************************************************************************/
    OverlayLayerWaylandPlatformBackend::~OverlayLayerWaylandPlatformBackend()
    {
        m_AppSurface = nullptr;

        ReleaseIfNull( &m_XdgToplevel, xdg_toplevel_destroy );
        ReleaseIfNull( &m_XdgSurface, xdg_surface_destroy );
        ReleaseIfNull( &m_XdgShell, xdg_wm_base_destroy );
        ReleaseIfNull( &m_Shell, wl_shell_destroy );
        ReleaseIfNull( &m_Pointer, wl_pointer_release );
        ReleaseIfNull( &m_Keyboard, wl_keyboard_release );
        ReleaseIfNull( &m_Seat, wl_seat_release );
        ReleaseIfNull( &m_InputRegion, wl_region_destroy );
        ReleaseIfNull( &m_InputSubsurface, wl_subsurface_destroy );
        ReleaseIfNull( &m_InputSurface, wl_surface_destroy );
        ReleaseIfNull( &m_Subcompositor, wl_subcompositor_destroy );
        ReleaseIfNull( &m_Compositor, wl_compositor_destroy );
        ReleaseIfNull( &m_Registry, wl_registry_destroy );
        ReleaseIfNull( &m_Display, wl_display_disconnect );

        delete m_pXkbBackend;
        m_pXkbBackend = nullptr;

        if( m_pImGuiContext )
        {
            assert( ImGui::GetCurrentContext() == m_pImGuiContext );

            ImGuiIO& io = ImGui::GetIO();
            io.BackendFlags = 0;
            io.BackendPlatformName = nullptr;
            io.BackendPlatformUserData = nullptr;

            ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
            platformIO.Platform_GetClipboardTextFn = nullptr;
            platformIO.Platform_SetClipboardTextFn = nullptr;
        }
    }

    /***********************************************************************************\

    Function:
        NewFrame

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::NewFrame()
    {
        // Validate the current ImGui context.
        ImGuiContext* pContext = ImGui::GetCurrentContext();
        IM_ASSERT( pContext && "OverlayLayerXcbPlatformBackend::NewFrame called when no ImGui context was set." );
        IM_ASSERT( pContext == m_pImGuiContext && "OverlayLayerXcbPlatformBackend::NewFrame called with different context than the one used for initialization." );
        if( !pContext )
            return;

        ImGuiIO& io = ImGui::GetIO();
        IM_ASSERT( io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame()." );

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

        // Update input capture rects.
        wl_region_add( m_InputRegion, 0, 0, 1000, 1000 ); 

        #if 0
        for( ImGuiWindow* pWindow : GImGui->Windows )
        {
            if( pWindow && pWindow->WasActive )
            {
                xcb_rectangle_t rect;
                rect.x = static_cast<int16_t>( pWindow->Pos.x );
                rect.y = static_cast<int16_t>( pWindow->Pos.y );
                rect.width = static_cast<uint16_t>( pWindow->Size.x );
                rect.height = static_cast<uint16_t>( pWindow->Size.y );
                m_InputRects.push_back( rect );
            }
        }
        #endif

        wl_surface_set_input_region( m_InputSurface, m_InputRegion );
        wl_surface_commit( m_InputSurface );

        // Process pending messages.
        wl_display_roundtrip( m_Display );
    }

    /***********************************************************************************\

    Function:
        HandleGlobal

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleGlobal( void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version )
    {
        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        assert( bd );

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
        HandleGlobalRemove

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleToplevelConfigure( void* data, xdg_toplevel* toplevel, int32_t width, int32_t height, wl_array* states )
    {
        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        assert( bd );
    }

    /***********************************************************************************\

    Function:
        HandleSeatCapabilities

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleSeatCapabilities( void* data, wl_seat* seat, uint32_t capabilities )
    {
        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        assert( bd );

        bd->m_SeatCapabilities = capabilities;

        // Register mouse handlers
        if( capabilities & WL_SEAT_CAPABILITY_POINTER )
        {
            wl_pointer* pointer = wl_seat_get_pointer( seat );
            if( bd->m_Pointer != pointer )
            {
                ReleaseIfNull( &bd->m_Pointer, wl_pointer_release );
                bd->m_Pointer = pointer;

                wl_pointer_add_listener( bd->m_Pointer, &m_scPointerListener, data );
            }
        }
        else
        {
            // Pointer device disconnected.
            ReleaseIfNull( &bd->m_Pointer, wl_pointer_release );
        }

        // Register keyboard handlers
        if( capabilities & WL_SEAT_CAPABILITY_KEYBOARD )
        {
            wl_keyboard* keyboard = wl_seat_get_keyboard( seat );
            if( bd->m_Keyboard != keyboard )
            {
                ReleaseIfNull( &bd->m_Keyboard, wl_keyboard_release );
                bd->m_Keyboard = keyboard;

                wl_keyboard_add_listener( bd->m_Keyboard, &m_scKeyboardListener, data );
            }
        }
        else
        {
            // Keyboard device disconnected.
            ReleaseIfNull( &bd->m_Keyboard, wl_keyboard_release );
        }
    }

    /***********************************************************************************\

    Function:
        HandleSeatName

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleSeatName( void* data, wl_seat* seat, const char* name )
    {
    }

    /***********************************************************************************\

    Function:
        HandlePointerEnter

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandlePointerEnter( void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y )
    {
        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        assert( bd );

        bd->m_PointerEvent.m_Mask |= POINTER_EVENT_ENTER;
        bd->m_PointerEvent.m_Serial = serial;
        bd->m_PointerEvent.m_Position.x = wl_fixed_to_double( x );
        bd->m_PointerEvent.m_Position.y = wl_fixed_to_double( y );
    }

    /***********************************************************************************\

    Function:
        HandlePointerLeave

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandlePointerLeave( void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface )
    {
        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        assert( bd );

        bd->m_PointerEvent.m_Mask |= POINTER_EVENT_LEAVE;
        bd->m_PointerEvent.m_Serial = serial;
    }

    /***********************************************************************************\

    Function:
        HandlePointerMotion

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandlePointerMotion( void* data, wl_pointer* pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y )
    {
        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        assert( bd );

        bd->m_PointerEvent.m_Mask |= POINTER_EVENT_MOTION;
        bd->m_PointerEvent.m_Time = time;
        bd->m_PointerEvent.m_Position.x = wl_fixed_to_double( x );
        bd->m_PointerEvent.m_Position.y = wl_fixed_to_double( y );
    }

    /***********************************************************************************\

    Function:
        HandlePointerButton

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandlePointerButton( void* data, wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state )
    {
        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        assert( bd );

        bd->m_PointerEvent.m_Mask |= POINTER_EVENT_BUTTON;
        bd->m_PointerEvent.m_Time = time;
        bd->m_PointerEvent.m_Serial = serial;
        bd->m_PointerEvent.m_Button = button;
        bd->m_PointerEvent.m_State = state;
    }

    /***********************************************************************************\

    Function:
        HandlePointerAxis

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandlePointerAxis( void* data, wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value )
    {
        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        assert( bd );

        bd->m_PointerEvent.m_Mask |= POINTER_EVENT_AXIS;
        bd->m_PointerEvent.m_Time = time;
        bd->m_PointerEvent.m_Axes[axis].m_Valid = true;
        bd->m_PointerEvent.m_Axes[axis].m_Value = wl_fixed_to_double( value );
    }

    /***********************************************************************************\

    Function:
        HandlePointerAxisSource

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandlePointerAxisSource( void* data, wl_pointer* pointer, uint32_t source )
    {
        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        assert( bd );

        bd->m_PointerEvent.m_Mask |= POINTER_EVENT_AXIS_SOURCE;
        bd->m_PointerEvent.m_AxisSource = source;
    }

    /***********************************************************************************\

    Function:
        HandlePointerAxisStop

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandlePointerAxisStop( void* data, wl_pointer* pointer, uint32_t time, uint32_t axis )
    {
        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        assert( bd );

        bd->m_PointerEvent.m_Mask |= POINTER_EVENT_AXIS_STOP;
        bd->m_PointerEvent.m_Time = time;
        bd->m_PointerEvent.m_Axes[axis].m_Valid = true;
    }

    /***********************************************************************************\

    Function:
        HandlePointerAxisDiscrete

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandlePointerAxisDiscrete( void* data, wl_pointer* pointer, uint32_t axis, int32_t value )
    {
        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        assert( bd );

        bd->m_PointerEvent.m_Mask |= POINTER_EVENT_AXIS_DISCRETE;
        bd->m_PointerEvent.m_Axes[axis].m_Valid = true;
        bd->m_PointerEvent.m_Axes[axis].m_Discrete = value;
    }

    /***********************************************************************************\

    Function:
        HandlePointerFrame

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandlePointerFrame( void* data, wl_pointer* pointer )
    {
        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        assert( bd );

        // TODO

        memset( &bd->m_PointerEvent, 0, sizeof( bd->m_PointerEvent ) );
    }

    /***********************************************************************************\

    Function:
        HandleKeyboardEnter

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleKeyboardEnter( void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys )
    {
    }

    /***********************************************************************************\

    Function:
        HandleKeyboardLeave

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleKeyboardLeave( void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface )
    {
    }

    /***********************************************************************************\

    Function:
        HandleKeyboardKeymap

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleKeyboardKeymap( void* data, wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size )
    {
        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        assert( bd );

        // Only classic XKB text format is currently supported.
        if( format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 )
        {
            void* mappedFd = mmap( NULL, size, PROT_READ, MAP_SHARED, fd, 0 );
            if( mappedFd != MAP_FAILED )
            {
                // Set the new keymap from the provided string.
                bd->m_pXkbBackend->SetKeymapFromString(
                    static_cast<const char*>( mappedFd ),
                    XKB_KEYMAP_FORMAT_TEXT_V1,
                    XKB_KEYMAP_COMPILE_NO_FLAGS );

                munmap( mappedFd, size );
            }
        }

        close( fd );
    }

    /***********************************************************************************\

    Function:
        HandleKeyboardKey

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleKeyboardKey( void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state )
    {
        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        assert( bd );

        bd->m_pXkbBackend->AddKeyEvent( key, state == WL_KEYBOARD_KEY_STATE_PRESSED );
    }

    /***********************************************************************************\

    Function:
        HandleKeyboardModifiers

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleKeyboardModifiers( void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group )
    {
        auto* bd = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        assert( bd );

        bd->m_pXkbBackend->SetKeyModifiers( depressed, latched, locked, group );
    }

    /***********************************************************************************\

    Function:
        HandleKeyboardRepeat

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleKeyboardRepeat( void* data, wl_keyboard* keyboard, int32_t rate, int32_t delay )
    {
    }
}
