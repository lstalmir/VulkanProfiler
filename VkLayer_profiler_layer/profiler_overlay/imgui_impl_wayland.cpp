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

#include "imgui_impl_wayland.h"
#include <imgui.h>
#include <stdlib.h>
#include <mutex>

namespace Profiler
{
    extern std::mutex s_ImGuiMutex;
}

/***********************************************************************************\

Function:
    ImGui_ImplWayland_Context

Description:
    Constructor.

    s_ImGuiMutex must be locked before creating the window context.

\***********************************************************************************/
ImGui_ImplWayland_Context::ImGui_ImplWayland_Context( wl_surface* surface ) try
    : m_pImGuiContext( nullptr )
    , m_pXkbContext( nullptr )
    , m_Display( nullptr )
    , m_AppSurface( surface )
    , m_InputSurface( nullptr )
{
    // Create XKB context
    m_pXkbContext = new ImGui_ImplXkb_Context();

    // Connect to Wayland server
    m_Display = wl_display_connect( nullptr );
    if( !m_Display )
        throw;

    ImGuiIO& io = ImGui::GetIO();
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    //io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    io.BackendPlatformName = "imgui_impl_wayland";
    io.BackendPlatformUserData = this;

    m_pImGuiContext = ImGui::GetCurrentContext();
}
catch (...)
{
    ImGui_ImplWayland_Context::~ImGui_ImplWayland_Context();
    throw;
}

/***********************************************************************************\

Function:
    ~ImGui_ImplWayland_Context

Description:
    Destructor.

    s_ImGuiMutex must be locked before destroying the window context.

\***********************************************************************************/
ImGui_ImplWayland_Context::~ImGui_ImplWayland_Context()
{
    wl_display_disconnect( m_Display );
    m_Display = nullptr;

    delete m_pXkbContext;
    m_pXkbContext = nullptr;

    if( m_pImGuiContext )
    {
        assert( ImGui::GetCurrentContext() == m_pImGuiContext );
        ImGuiIO& io = ImGui::GetIO();
        io.BackendFlags = 0;
        io.BackendPlatformName = nullptr;
        io.BackendPlatformUserData = nullptr;
    }
}

/***********************************************************************************\

Function:
    GetName

Description:

\***********************************************************************************/
const char* ImGui_ImplWayland_Context::GetName() const
{
    return "Wayland";
}

/***********************************************************************************\

Function:
    NewFrame

Description:

\***********************************************************************************/
void ImGui_ImplWayland_Context::NewFrame()
{
    // Validate the current ImGui context.
    ImGuiContext* pContext = ImGui::GetCurrentContext();
    IM_ASSERT( pContext && "ImGui_ImplWayland_Context::NewFrame called when no ImGui context was set." );
    IM_ASSERT( pContext == m_pImGuiContext && "ImGui_ImplWayland_Context::NewFrame called with different context than the one used for initialization." );
    if( !pContext )
        return;

    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT( io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame()." );

#if 0
    // Setup display size (every frame to accommodate for window resizing)
    auto geometry = GetGeometry( m_AppWindow );
    io.DisplaySize = ImVec2((float)(geometry.width), (float)(geometry.height));

    // Update OS mouse position
    UpdateMousePos();

    // Update input capture rects
    xcb_shape_rectangles(
        m_Connection,
        XCB_SHAPE_SO_SET,
        XCB_SHAPE_SK_INPUT,
        XCB_CLIP_ORDERING_UNSORTED,
        m_InputWindow,
        0, 0,
        m_InputRects.size(),
        m_InputRects.data() );

    // Handle incoming input events
    // Don't block if there are no pending events
    xcb_generic_event_t* event = nullptr;

    while( event = xcb_poll_for_event( m_Connection ) )
    {
        switch( event->response_type & (~0x80) )
        {
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
        }

        // Free received event
        free( event );
    }

    xcb_flush( m_Connection );

    // Rebuild input capture rects on each frame
    m_InputRects.clear();
#endif
}

/***********************************************************************************\

Function:
    UpdateMousePos

Description:

\***********************************************************************************/
void ImGui_ImplWayland_Context::UpdateMousePos()
{
    ImGuiIO& io = ImGui::GetIO();

    // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
    if( io.WantSetMousePos )
    {
#if 0
        xcb_point_t pos = { (short)io.MousePos.x, (short)io.MousePos.y };
        xcb_warp_pointer( m_Connection, 0, m_InputWindow, 0, 0, 0, 0, pos.x, pos.y );
#endif
    }
}
