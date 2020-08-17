#include "imgui_impl_xcb.h"
#include "imgui/imgui.h"
#include <stdlib.h>
#include <unordered_set>
#include <xcb/shape.h>

/***********************************************************************************\

Function:
    ImGui_ImplXcb_Context

Description:
    Constructor.

\***********************************************************************************/
ImGui_ImplXcb_Context::ImGui_ImplXcb_Context( xcb_window_t window )
    : m_Connection( nullptr )
    , m_AppWindow( window )
    , m_InputWindow( 0 )
{
    // Connect to X server
    m_Connection = xcb_connect( nullptr, nullptr );
    if( !m_Connection )
        InitError();

    m_InputWindow = xcb_generate_id( m_Connection );

    // Get app window attributes
    auto geometry = GetGeometry( m_AppWindow );

    const int mask = XCB_CW_EVENT_MASK;
    const int valwin =
        XCB_EVENT_MASK_POINTER_MOTION |
        XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE;

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
    io.BackendPlatformName = "imgui_impl_xcb";
    io.ImeWindowHandle = (void*)(size_t)m_InputWindow;
}

/***********************************************************************************\

Function:
    ~ImGui_ImplXcb_Context

Description:
    Destructor.

\***********************************************************************************/
ImGui_ImplXcb_Context::~ImGui_ImplXcb_Context()
{
    xcb_destroy_window( m_Connection, m_InputWindow );
    m_InputWindow = 0;
    m_AppWindow = 0;

    xcb_disconnect( m_Connection );
    m_Connection = nullptr;
}

/***********************************************************************************\

Function:
    GetName

Description:

\***********************************************************************************/
const char* ImGui_ImplXcb_Context::GetName() const
{
    return "XCB";
}

/***********************************************************************************\

Function:
    NewFrame

Description:

\***********************************************************************************/
void ImGui_ImplXcb_Context::NewFrame()
{
    if( !ImGui::GetCurrentContext() )
        return;

    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    // Setup display size (every frame to accommodate for window resizing)
    auto geometry = GetGeometry( m_AppWindow );
    io.DisplaySize = ImVec2((float)(geometry.width), (float)(geometry.height));

    // Update OS mouse position
    UpdateMousePos();

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
}

/***********************************************************************************\

Function:
    UpdateWindowRect

Description:
    X-platform implementations require setting input clipping rects to pass messages
    to the parent window. This can be only done between ImGui::Begin and ImGui::End,
    when g.CurrentWindow is set.

\***********************************************************************************/
void ImGui_ImplXcb_Context::UpdateWindowRect()
{
    // Set input clipping rectangle
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();

    xcb_rectangle_t inputRect = {};
    inputRect.x = pos.x;
    inputRect.y = pos.y;
    inputRect.width = size.x;
    inputRect.height = size.y;

    xcb_shape_rectangles(
        m_Connection,
        XCB_SHAPE_SO_SET,
        XCB_SHAPE_SK_INPUT,
        XCB_CLIP_ORDERING_UNSORTED,
        m_InputWindow,
        0, 0,
        1, &inputRect );
}

/***********************************************************************************\

Function:
    InitError

Description:

\***********************************************************************************/
void ImGui_ImplXcb_Context::InitError()
{
    this->~ImGui_ImplXcb_Context();
    throw;
}

/***********************************************************************************\

Function:
    GetGeometry

Description:
    Get geometry properties of the window.

\***********************************************************************************/
xcb_get_geometry_reply_t ImGui_ImplXcb_Context::GetGeometry( xcb_drawable_t drawable )
{
    // Send request to the server
    auto cookie = xcb_get_geometry( m_Connection, drawable );
    xcb_flush( m_Connection );

    // Wait for the response
    auto* response = xcb_get_geometry_reply( m_Connection, cookie, nullptr );

    xcb_get_geometry_reply_t geometry = *response;

    // Free returned pointer
    free( response );

    return geometry;
}

/***********************************************************************************\

Function:
    UpdateMousePos

Description:

\***********************************************************************************/
void ImGui_ImplXcb_Context::UpdateMousePos()
{
    ImGuiIO& io = ImGui::GetIO();

    // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
    if( io.WantSetMousePos )
    {
        xcb_point_t pos = { (short)io.MousePos.x, (short)io.MousePos.y };
        xcb_warp_pointer( m_Connection, 0, m_InputWindow, 0, 0, 0, 0, pos.x, pos.y );
    }
}
