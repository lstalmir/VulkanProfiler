#include "imgui_impl_xlib.h"
#include "imgui/imgui.h"

static Display* g_Display;
static XIM g_IM;
static Window g_AppWindow;
static Window g_InputWindow;

// Cleanup from partially initialized state
static bool InitError()
{
    ImGui_ImplXlib_Shutdown();
    return false;
}

// Check if window a is child of window b
static bool IsChild( Window a, Window b )
{
    Window root, parent, *pChildren;
    unsigned int childrenCount;
    
    // Traverse the tree bottom-up for faster lookup
    if( !XQueryTree( g_Display, a, &root, &parent, &pChildren, &childrenCount ) )
        return false;

    if( pChildren ) XFree( pChildren );
    if( parent == b ) return true;

    // a is child of b if parent of a is child of b
    return IsChild( parent, b );
}

/***********************************************************************************\

Function:
    ImGui_ImplXlib_UpdateMousePos

Description:

\***********************************************************************************/
static void ImGui_ImplXlib_UpdateMousePos()
{
    ImGuiIO& io = ImGui::GetIO();

    // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
    if (io.WantSetMousePos)
    {
        XPoint pos = { (short)io.MousePos.x, (short)io.MousePos.y };
        XWarpPointer( g_Display, None, g_InputWindow, 0, 0, 0, 0, pos.x, pos.y );
    }
}

/***********************************************************************************\

Function:
    ImGui_ImplXlib_Init

Description:
    Initialize XLIB implementation of ImGui.

\***********************************************************************************/
bool ImGui_ImplXlib_Init( Window window )
{
    // Must be uninitialized
    assert( g_Display == nullptr );

    g_Display = XOpenDisplay( nullptr );
    if( !g_Display )
        return InitError();

    g_IM = XOpenIM( g_Display, nullptr, nullptr, nullptr );
    if( !g_IM )
        return InitError();

    XWindowAttributes windowAttributes;
    // TODO: error check
    XGetWindowAttributes( g_Display, window, &windowAttributes );

    g_AppWindow = window;
    g_InputWindow = XCreateWindow(
        g_Display,
        g_AppWindow,
        0, 0,
        windowAttributes.width,
        windowAttributes.height,
        0, CopyFromParent,
        InputOnly,
        CopyFromParent,
        0, nullptr );

    if( !g_InputWindow )
        return InitError();

    // TODO: error check
    XMapWindow( g_Display, g_InputWindow );

    // Get supported input mask of the created window
    XGetWindowAttributes( g_Display, g_InputWindow, &windowAttributes );

    const int inputEventMask =
        ButtonPressMask |
        ButtonReleaseMask |
        PointerMotionMask;

    //if( (windowAttributes.all_event_masks & inputEventMask) != inputEventMask )
    //    return InitError();

    // Start listening
    if( !XSelectInput( g_Display, g_InputWindow, inputEventMask ) )
        return InitError();

    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    io.BackendPlatformName = "imgui_impl_xlib";
    io.ImeWindowHandle = (void*)g_InputWindow;

    // TODO: keyboard mapping to ImGui

    return true;
}

/***********************************************************************************\

Function:
    ImGui_ImplXlib_Shutdown

Description:
    Cleanup XLIB implementation.

\***********************************************************************************/
void ImGui_ImplXlib_Shutdown()
{
    if( g_InputWindow ) XDestroyWindow( g_Display, g_InputWindow );
    g_InputWindow = None;
    g_AppWindow = None;

    if( g_IM ) XCloseIM( g_IM );
    g_IM = None;

    if( g_Display ) XCloseDisplay( g_Display );
    g_Display = nullptr;
}

/***********************************************************************************\

Function:
    ImGui_ImplXlib_NewFrame

Description:
    Handle incoming events.

\***********************************************************************************/
void ImGui_ImplXlib_NewFrame()
{
    if( !ImGui::GetCurrentContext() )
        return;

    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    // Setup display size (every frame to accommodate for window resizing)
    XWindowAttributes windowAttributes;
    XGetWindowAttributes( g_Display, g_AppWindow, &windowAttributes );
    io.DisplaySize = ImVec2((float)(windowAttributes.width), (float)(windowAttributes.height));

    #if 0
    // Read keyboard modifiers inputs
    io.KeyCtrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
    io.KeyShift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
    io.KeyAlt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
    io.KeySuper = false;
    // io.KeysDown[], io.MousePos, io.MouseDown[], io.MouseWheel: filled by the WndProc handler below.
    #endif

    // Update OS mouse position
    ImGui_ImplXlib_UpdateMousePos();

    // Handle incoming input events
    // Don't block if there are no pending events
    while( XEventsQueued( g_Display, QueuedAlready ) )
    {
        XEvent event;
        // TODO: error check
        XNextEvent( g_Display, &event );

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
}
