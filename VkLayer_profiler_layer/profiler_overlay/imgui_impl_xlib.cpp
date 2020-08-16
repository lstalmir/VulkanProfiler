#include "imgui_impl_xlib.h"
#include "imgui/imgui.h"

/***********************************************************************************\

Function:
    ImGui_ImplXlib_Context

Description:
    Constructor.

\***********************************************************************************/
ImGui_ImplXlib_Context::ImGui_ImplXlib_Context( Window window )
    : m_Display( nullptr )
    , m_IM( None )
    , m_AppWindow( window )
    , m_InputWindow( None )
{
    m_Display = XOpenDisplay( nullptr );
    if( !m_Display )
        InitError();

    m_IM = XOpenIM( m_Display, nullptr, nullptr, nullptr );
    if( !m_IM )
        InitError();

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
        InitError();

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
        InitError();

    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    io.BackendPlatformName = "imgui_impl_xlib";
    io.ImeWindowHandle = (void*)m_InputWindow;

    // TODO: keyboard mapping to ImGui
}

/***********************************************************************************\

Function:
    ~ImGui_ImplXlib_Context

Description:
    Destructor.

\***********************************************************************************/
ImGui_ImplXlib_Context::~ImGui_ImplXlib_Context()
{
    if( m_InputWindow ) XDestroyWindow( m_Display, m_InputWindow );
    m_InputWindow = None;
    m_AppWindow = None;

    if( m_IM ) XCloseIM( m_IM );
    m_IM = None;

    if( m_Display ) XCloseDisplay( m_Display );
    m_Display = nullptr;
}

/***********************************************************************************\

Function:
    NewFrame

Description:
    Handle incoming events.

\***********************************************************************************/
void ImGui_ImplXlib_Context::NewFrame()
{
    if( !ImGui::GetCurrentContext() )
        return;

    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    // Setup display size (every frame to accommodate for window resizing)
    XWindowAttributes windowAttributes;
    XGetWindowAttributes( m_Display, m_AppWindow, &windowAttributes );
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
    UpdateMousePos();

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
}

/***********************************************************************************\

Function:
    InitError

Description:
    Cleanup from partially initialized state

\***********************************************************************************/
void ImGui_ImplXlib_Context::InitError()
{
    this->~ImGui_ImplXlib_Context();
    throw;
}

/***********************************************************************************\

Function:
    IsChild

Description:
    Check if window a is child of window b

\***********************************************************************************/
bool ImGui_ImplXlib_Context::IsChild( Window a, Window b ) const
{
    Window root, parent, * pChildren;
    unsigned int childrenCount;

    // Traverse the tree bottom-up for faster lookup
    if( !XQueryTree( m_Display, a, &root, &parent, &pChildren, &childrenCount ) )
        return false;

    if( pChildren ) XFree( pChildren );
    if( parent == b ) return true;

    // a is child of b if parent of a is child of b
    return IsChild( parent, b );
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
