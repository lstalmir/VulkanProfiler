// Copyright (c) 2019-2024 Lukasz Stalmirski
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
#include <mutex>
#include <imgui.h>
#include <stdlib.h>

namespace Profiler
{
    extern std::mutex s_ImGuiMutex;
}

/***********************************************************************************\

Function:
    ImGui_ImplXlib_Context

Description:
    Constructor.

\***********************************************************************************/
ImGui_ImplXlib_Context::ImGui_ImplXlib_Context( Window window ) try
    : m_pImGuiContext( nullptr )
    , m_Display( nullptr )
    , m_IM( None )
    , m_AppWindow( window )
    , m_InputWindow( None )
{
    std::scoped_lock lk( Profiler::s_ImGuiMutex );

    m_Display = XOpenDisplay( nullptr );
    if( !m_Display )
        throw;

    m_IM = XOpenIM( m_Display, nullptr, nullptr, nullptr );
    if( !m_IM )
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

\***********************************************************************************/
ImGui_ImplXlib_Context::~ImGui_ImplXlib_Context()
{
    std::scoped_lock lk( Profiler::s_ImGuiMutex );

    if( m_InputWindow ) XDestroyWindow( m_Display, m_InputWindow );
    m_InputWindow = None;
    m_AppWindow = None;

    if( m_IM ) XCloseIM( m_IM );
    m_IM = None;

    if( m_Display ) XCloseDisplay( m_Display );
    m_Display = nullptr;

    if( m_pImGuiContext )
    {
        ImGui::SetCurrentContext( m_pImGuiContext );
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

    // Update OS mouse position
    UpdateMousePos();

    // Update input capture rects
    XShapeCombineRectangles(
        m_Display,
        m_InputWindow,
        ShapeInput,
        0, 0,
        m_InputRects.data(),
        m_InputRects.size(),
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
        }
    }

    // Rebuild input capture rects on each frame
    m_InputRects.clear();
}

/***********************************************************************************\

Function:
    AddInputCaptureRect

Description:
    X-platform implementations require setting input clipping rects to pass messages
    to the parent window. This can be only done between ImGui::Begin and ImGui::End,
    when g.CurrentWindow is set.

\***********************************************************************************/
void ImGui_ImplXlib_Context::AddInputCaptureRect( int x, int y, int width, int height )
{
    // Set input clipping rectangle
    XRectangle& inputRect = m_InputRects.emplace_back();
    inputRect.x = x;
    inputRect.y = y;
    inputRect.width = width;
    inputRect.height = height;
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
