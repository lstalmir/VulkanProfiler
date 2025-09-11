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

namespace Profiler
{
    extern std::mutex s_ImGuiMutex;

    /***********************************************************************************\

    Function:
        OverlayLayerWaylandPlatformBackend

    Description:
        Constructor.

        s_ImGuiMutex must be locked before creating the window context.

    \***********************************************************************************/
    OverlayLayerWaylandPlatformBackend::OverlayLayerWaylandPlatformBackend( wl_surface* window ) try
        : m_pImGuiContext( nullptr )
        , m_pXkbBackend( nullptr )
        , m_Display( nullptr )
        , m_Registry( nullptr )
        , m_Shm( nullptr )
        , m_Compositor( nullptr )
        , m_Seat( nullptr )
        , m_AppWindow( window )
        , m_InputWindow( 0 )
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
        wl_registry_listener registryListener = {};
        registryListener.global = &OverlayLayerWaylandPlatformBackend::HandleGlobal;
        registryListener.global_remove = &OverlayLayerWaylandPlatformBackend::HandleGlobalRemove;
        wl_registry_add_listener( m_Registry, &registryListener, this );
        wl_display_roundtrip( m_Display );

        // Register mouse listener
        wl_pointer_listener pointerListener = {};
        pointerListener.enter = &OverlayLayerWaylandPlatformBackend::HandlePointerEnter;
        // TODO

        // Get app window attributes
        auto geometry = GetGeometry( m_AppWindow );

        const int mask = XCB_CW_EVENT_MASK;
        const int valwin =
            XCB_EVENT_MASK_POINTER_MOTION |
            XCB_EVENT_MASK_BUTTON_PRESS |
            XCB_EVENT_MASK_BUTTON_RELEASE |
            XCB_EVENT_MASK_KEY_PRESS |
            XCB_EVENT_MASK_KEY_RELEASE;

        xcb_create_window(
            m_Connection,
            XCB_COPY_FROM_PARENT,
            m_InputWindow,
            m_AppWindow,
            0, 0,
            geometry.width,
            geometry.height,
            0,
            XCB_WINDOW_CLASS_INPUT_ONLY,
            XCB_COPY_FROM_PARENT,
            mask, &valwin );

        xcb_map_window( m_Connection, m_InputWindow );
        xcb_flush( m_Connection );

        ImGuiIO& io = ImGui::GetIO();
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
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
        wl_surface_destroy( m_InputWindow );
        m_InputWindow = 0;
        m_AppWindow = 0;

        wl_display_disconnect( m_Display );
        m_Display = nullptr;
        m_Registry = nullptr;
        m_Shm = nullptr;
        m_Compositor = nullptr;
        m_Seat = nullptr;

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
        IM_ASSERT(pContext && "OverlayLayerXcbPlatformBackend::NewFrame called when no ImGui context was set.");
        IM_ASSERT(pContext == m_pImGuiContext && "OverlayLayerXcbPlatformBackend::NewFrame called with different context than the one used for initialization.");
        if( !pContext )
            return;

        ImGuiIO& io = ImGui::GetIO();
        IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

        // Setup display size (every frame to accommodate for window resizing)
        auto geometry = GetGeometry( m_AppWindow );
        io.DisplaySize = ImVec2((float)(geometry.width), (float)(geometry.height));

        const uint32_t inputWindowChanges[2] = {
            static_cast<uint32_t>( geometry.width ),
            static_cast<uint32_t>( geometry.height )
        };

        xcb_configure_window(
            m_Connection,
            m_InputWindow,
            XCB_CONFIG_WINDOW_WIDTH |
            XCB_CONFIG_WINDOW_HEIGHT,
            inputWindowChanges );

        // Update OS mouse position
        UpdateMousePos();

        // Update input capture rects
        m_InputRects.resize( 0 );

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

        xcb_shape_rectangles(
            m_Connection,
            XCB_SHAPE_SO_SET,
            XCB_SHAPE_SK_INPUT,
            XCB_CLIP_ORDERING_UNSORTED,
            m_InputWindow,
            0, 0,
            m_InputRects.Size,
            m_InputRects.Data );

        // Handle incoming input events
        // Don't block if there are no pending events
        xcb_generic_event_t* event = nullptr;

        while( event = xcb_poll_for_event( m_Connection ) )
        {
            switch( event->response_type & (~0x80) )
            {
            case XCB_SELECTION_REQUEST:
            {
                // Send current selection
                xcb_selection_request_event_t* selectionRequestEvent =
                    reinterpret_cast<xcb_selection_request_event_t*>( event );

                xcb_selection_notify_event_t selectionNotifyEvent = {};
                selectionNotifyEvent.response_type = XCB_SELECTION_NOTIFY;
                selectionNotifyEvent.requestor = selectionRequestEvent->requestor;
                selectionNotifyEvent.selection = selectionRequestEvent->selection;
                selectionNotifyEvent.target = selectionRequestEvent->target;
                selectionNotifyEvent.time = selectionRequestEvent->time;

                if( selectionRequestEvent->target == m_TargetsAtom )
                {
                    // Send list of available conversions
                    selectionNotifyEvent.property = selectionRequestEvent->property;

                    xcb_atom_t targets[] = {
                        m_TargetsAtom,
                        m_TextAtom,
                        m_StringAtom,
                        m_Utf8StringAtom
                    };

                    xcb_change_property(
                        m_Connection,
                        XCB_PROP_MODE_REPLACE,
                        selectionRequestEvent->requestor,
                        selectionRequestEvent->property,
                        selectionRequestEvent->target, 32,
                        std::size( targets ),
                        targets );
                }

                if( selectionRequestEvent->target == m_TextAtom ||
                    selectionRequestEvent->target == m_StringAtom ||
                    selectionRequestEvent->target == m_Utf8StringAtom )
                {
                    // Send selection as string
                    selectionNotifyEvent.property = selectionRequestEvent->property;

                    uint32_t clipboardTextLength = 0;
                    if( m_pClipboardText )
                    {
                        clipboardTextLength = strlen( m_pClipboardText );
                    }

                    xcb_change_property(
                        m_Connection,
                        XCB_PROP_MODE_REPLACE,
                        selectionRequestEvent->requestor,
                        selectionRequestEvent->property,
                        selectionRequestEvent->target, 8,
                        clipboardTextLength,
                        m_pClipboardText );
                }

                // Notify the requestor that the selection is ready
                xcb_send_event( m_Connection,
                    false,
                    selectionRequestEvent->requestor,
                    XCB_EVENT_MASK_NO_EVENT,
                    reinterpret_cast<const char*>( &selectionNotifyEvent ) );

                break;
            }

            case XCB_MOTION_NOTIFY:
            {
                // Update mouse position
                xcb_motion_notify_event_t* motionNotifyEvent =
                    reinterpret_cast<xcb_motion_notify_event_t*>(event);

                io.MousePos.x = motionNotifyEvent->event_x;
                io.MousePos.y = motionNotifyEvent->event_y;
                break;
            }

            case XCB_BUTTON_PRESS:
            {
                // Handle mouse click
                xcb_button_press_event_t* buttonPressEvent =
                    reinterpret_cast<xcb_button_press_event_t*>(event);

                // First 3 buttons are mouse buttons, 4 and 5 are wheel scroll
                if( buttonPressEvent->detail < XCB_BUTTON_INDEX_4 )
                {
                    int button = 0;
                    if( buttonPressEvent->detail == XCB_BUTTON_INDEX_1 ) button = 0;
                    if( buttonPressEvent->detail == XCB_BUTTON_INDEX_2 ) button = 2;
                    if( buttonPressEvent->detail == XCB_BUTTON_INDEX_3 ) button = 1;
                    io.MouseDown[ button ] = true;
                }
                else
                {
                    // TODO: scroll speed
                    io.MouseWheel += (buttonPressEvent->detail == XCB_BUTTON_INDEX_4) ? 1 : -1;
                }
                break;
            }

            case XCB_BUTTON_RELEASE:
            {
                // Handle mouse release
                xcb_button_release_event_t* buttonReleaseEvent =
                    reinterpret_cast<xcb_button_release_event_t*>(event);

                // First 3 buttons are mouse buttons, 4 and 5 are wheel scroll
                if( buttonReleaseEvent->detail < XCB_BUTTON_INDEX_4 )
                {
                    int button = 0;
                    if( buttonReleaseEvent->detail == XCB_BUTTON_INDEX_1 ) button = 0;
                    if( buttonReleaseEvent->detail == XCB_BUTTON_INDEX_2 ) button = 2;
                    if( buttonReleaseEvent->detail == XCB_BUTTON_INDEX_3 ) button = 1;
                    io.MouseDown[ button ] = false;
                }
                break;
            }

            case XCB_KEY_PRESS:
            {
                // Handle key press
                xcb_key_press_event_t* keyPressEvent =
                    reinterpret_cast<xcb_key_press_event_t*>(event);

                m_pXkbBackend->AddKeyEvent( keyPressEvent->detail, true );
                break;
            }

            case XCB_KEY_RELEASE:
            {
                // Handle key release
                xcb_key_release_event_t* keyReleaseEvent =
                    reinterpret_cast<xcb_key_release_event_t*>(event);

                m_pXkbBackend->AddKeyEvent( keyReleaseEvent->detail, false );
                break;
            }
            }

            // Free received event
            free( event );
        }

        xcb_flush( m_Connection );
    }

    /***********************************************************************************\

    Function:
        GetGeometry

    Description:
        Get geometry properties of the window.

    \***********************************************************************************/
    xcb_get_geometry_reply_t OverlayLayerWaylandPlatformBackend::GetGeometry( xcb_drawable_t drawable )
    {
        // Send request to the server
        xcb_get_geometry_cookie_t cookie = xcb_get_geometry_unchecked( m_Connection, drawable );
        xcb_flush( m_Connection );

        // Wait for the response
        xcb_get_geometry_reply_t* pReply = xcb_get_geometry_reply( m_Connection, cookie, nullptr );
        xcb_get_geometry_reply_t geometry = {};

        if( pReply )
        {
            geometry = *pReply;
        }

        // Free returned pointer
        free( pReply );

        return geometry;
    }

    /***********************************************************************************\

    Function:
        UpdateMousePos

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::UpdateMousePos()
    {
        ImGuiIO& io = ImGui::GetIO();

        // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
        if( io.WantSetMousePos )
        {
            xcb_point_t pos = { (short)io.MousePos.x, (short)io.MousePos.y };
            wl_pointer_ m_Connection, 0, m_InputWindow, 0, 0, 0, 0, pos.x, pos.y );
        }
    }

    /***********************************************************************************\

    Function:
        HandleGlobal

    Description:

    \***********************************************************************************/
    void OverlayLayerWaylandPlatformBackend::HandleGlobal( void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version )
    {
        auto* pThis = static_cast<OverlayLayerWaylandPlatformBackend*>( data );
        assert( pThis );

        if( strcmp( interface, wl_shm_interface.name ) == 0 )
        {
            pThis->m_Shm = static_cast<wl_shm*>( wl_registry_bind(
                registry,
                name,
                &wl_shm_interface,
                version ) );
            return;
        }

        if( strcmp( interface, wl_compositor_interface.name ) == 0 )
        {
            pThis->m_Compositor = static_cast<wl_compositor*>( wl_registry_bind(
                registry,
                name,
                &wl_compositor_interface,
                version ) );
            return;
        }

        if( strcmp( interface, wl_seat_interface.name ) == 0 )
        {
            pThis->m_Seat = static_cast<wl_seat*>( wl_registry_bind(
                registry,
                name,
                &wl_seat_interface,
                version ) );
            return;
        }
    }
}
