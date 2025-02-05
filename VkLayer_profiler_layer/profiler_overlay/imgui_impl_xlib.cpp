// Copyright (c) 2019-2025 Lukasz Stalmirski
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

#include "imgui_impl_xlib.h"
#include <imgui_internal.h>
#include <X11/extensions/shape.h>
#include <stdlib.h>
#include <mutex>

namespace Profiler
{
    extern std::mutex s_ImGuiMutex;
}

/***********************************************************************************\

Function:
    ImGui_ImplXlib_Context

Description:
    Constructor.

    s_ImGuiMutex must be locked before creating the window context.

\***********************************************************************************/
ImGui_ImplXlib_Context::ImGui_ImplXlib_Context( Window window ) try
    : m_pImGuiContext( nullptr )
    , m_pXkbContext( nullptr )
    , m_Display( nullptr )
    , m_AppWindow( window )
    , m_InputWindow( None )
{
    // Create XKB context
    m_pXkbContext = new ImGui_ImplXkb_Context();

    // Connect to X server
    m_Display = XOpenDisplay( nullptr );
    if( !m_Display )
        throw;

    XWindowAttributes windowAttributes;
    // TODO: error check
    XGetWindowAttributes( m_Display, window, &windowAttributes );

    m_InputWindow = XCreateWindow(
        m_Display,
        m_AppWindow,
        0, 0,
        windowAttributes.width,
        windowAttributes.height,
        0, CopyFromParent,
        InputOnly,
        CopyFromParent,
        0, nullptr );

    if( !m_InputWindow )
        throw;

    // TODO: error check
    XMapWindow( m_Display, m_InputWindow );

    // Get supported input mask of the created window
    XGetWindowAttributes( m_Display, m_InputWindow, &windowAttributes );

    const int inputEventMask =
        KeyPressMask |
        KeyReleaseMask |
        ButtonPressMask |
        ButtonReleaseMask |
        PointerMotionMask;

    //if( (windowAttributes.all_event_masks & inputEventMask) != inputEventMask )
    //    return InitError();

    // Start listening
    if( !XSelectInput( m_Display, m_InputWindow, inputEventMask ) )
        throw;

    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    io.BackendPlatformName = "imgui_impl_xlib";
    io.BackendPlatformUserData = this;

    m_pImGuiContext = ImGui::GetCurrentContext();

    // TODO: keyboard mapping to ImGui
}
catch (...)
{
    // Cleanup the partially initialized context.
    ImGui_ImplXlib_Context::~ImGui_ImplXlib_Context();
    throw;
}

/***********************************************************************************\

Function:
    ~ImGui_ImplXlib_Context

Description:
    Destructor.

    s_ImGuiMutex must be locked before destroying the window context.

\***********************************************************************************/
ImGui_ImplXlib_Context::~ImGui_ImplXlib_Context()
{
    if( m_InputWindow ) XDestroyWindow( m_Display, m_InputWindow );
    m_InputWindow = None;
    m_AppWindow = None;

    if( m_Display ) XCloseDisplay( m_Display );
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
const char* ImGui_ImplXlib_Context::GetName() const
{
    return "Xlib";
}

/***********************************************************************************\

Function:
    NewFrame

Description:
    Handle incoming events.

\***********************************************************************************/
void ImGui_ImplXlib_Context::NewFrame()
{
    // Validate the current ImGui context.
    ImGuiContext* pContext = ImGui::GetCurrentContext();
    IM_ASSERT(pContext && "ImGui_ImplXlib_Context::NewFrame called when no ImGui context was set.");
    IM_ASSERT(pContext == m_pImGuiContext && "ImGui_ImplXlib_Context::NewFrame called with different context than the one used for initialization.");
    if( !pContext )
        return;

    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    // Setup display size (every frame to accommodate for window resizing)
    XWindowAttributes windowAttributes;
    XGetWindowAttributes( m_Display, m_AppWindow, &windowAttributes );
    io.DisplaySize = ImVec2((float)(windowAttributes.width), (float)(windowAttributes.height));

    XWindowChanges inputWindowChanges = {};
    inputWindowChanges.width = windowAttributes.width;
    inputWindowChanges.height = windowAttributes.height;
    XConfigureWindow( m_Display, m_InputWindow, CWWidth | CWHeight, &inputWindowChanges );

    // Update OS mouse position
    UpdateMousePos();

    // Update input capture rects
    m_InputRects.resize( 0 );

    for( ImGuiWindow* pWindow : GImGui->Windows )
    {
        if( pWindow && pWindow->WasActive )
        {
            XRectangle rect;
            rect.x = static_cast<short>( pWindow->Pos.x );
            rect.y = static_cast<short>( pWindow->Pos.y );
            rect.width = static_cast<unsigned short>( pWindow->Size.x );
            rect.height = static_cast<unsigned short>( pWindow->Size.y );
            m_InputRects.push_back( rect );
        }
    }

    XShapeCombineRectangles(
        m_Display,
        m_InputWindow,
        ShapeInput,
        0, 0,
        m_InputRects.Data,
        m_InputRects.Size,
        ShapeSet,
        Unsorted );

    // Handle incoming input events
    // Don't block if there are no pending events
    while( XEventsQueued( m_Display, QueuedAlready ) )
    {
        XEvent event;
        // TODO: error check
        XNextEvent( m_Display, &event );

        switch( event.type )
        {
        case MotionNotify:
        {
            // Update mouse position
            io.MousePos.x = event.xmotion.x;
            io.MousePos.y = event.xmotion.y;
            break;
        }

        case ButtonPress:
        {
            // First 3 buttons are mouse buttons, 4 and 5 are wheel scroll
            if( event.xbutton.button < Button4 )
            {
                int button = 0;
                if( event.xbutton.button == Button1 ) button = 0;
                if( event.xbutton.button == Button2 ) button = 2;
                if( event.xbutton.button == Button3 ) button = 1;
                // TODO: XGrabPointer?
                io.MouseDown[ button ] = true;
            }
            else
            {
                // TODO: scroll speed
                io.MouseWheel += event.xbutton.button == Button4 ? 1 : -1;
            }
            break;
        }

        case ButtonRelease:
        {
            // First 3 buttons are mouse buttons
            if( event.xbutton.button < Button4 )
            {
                int button = 0;
                if( event.xbutton.button == Button1 ) button = 0;
                if( event.xbutton.button == Button2 ) button = 2;
                if( event.xbutton.button == Button3 ) button = 1;
                io.MouseDown[ button ] = false;
                // TODO: XUngrabPointer?
            }
            break;
        }

        case KeyPress:
        case KeyRelease:
        {
            m_pXkbContext->AddKeyEvent( event.xkey.keycode, (event.type == KeyPress) );
            break;
        }
        }
    }

    XFlush( m_Display );
}

/***********************************************************************************\

Function:
    UpdateMousePos

Description:

\***********************************************************************************/
void ImGui_ImplXlib_Context::UpdateMousePos()
{
    ImGuiIO& io = ImGui::GetIO();

    // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
    if( io.WantSetMousePos )
    {
        XPoint pos = { (short)io.MousePos.x, (short)io.MousePos.y };
        XWarpPointer( m_Display, None, m_InputWindow, 0, 0, 0, 0, pos.x, pos.y );
    }
}
