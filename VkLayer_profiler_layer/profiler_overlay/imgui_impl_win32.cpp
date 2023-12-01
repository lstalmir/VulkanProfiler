// Copyright (c) 2019-2021 Lukasz Stalmirski
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

#include "imgui_impl_win32.h"
#include <backends/imgui_impl_win32.h>

#include <queue>
#include <windowsx.h>

#include "profiler/profiler_helpers.h"

// Use implementation provided by the ImGui
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND, UINT, WPARAM, LPARAM );

class GetMessageHook& ImGui_ImplWin32_Context_GetMessageHook();

// Thread-safe and ref-counted wrapper for windows hook
class GetMessageHook
{
public:
    bool Acquire( HWND hWnd, ImGui_ImplWin32_Context* pContext )
    {
        std::scoped_lock lk( m_Mutex );

        if( m_pContexts.empty() )
        {
            // First reference to the hook has been acquired, initialize the hook.
            assert( !m_hHook );

            HINSTANCE hProfilerDllInstance =
                static_cast<HINSTANCE>( Profiler::ProfilerPlatformFunctions::GetLibraryInstanceHandle() );

            // Register a global window hook on GetMessage/PeekMessage function.
            m_hHook = SetWindowsHookEx(
                WH_GETMESSAGE,
                GetMessageHook::Execute,
                hProfilerDllInstance,
                0 /*dwThreadId*/ );

            if( !m_hHook )
            {
                // Failed to register the hook.
                return false;
            }
        }

        // Associate context with the window handle.
        return m_pContexts.try_emplace( hWnd, pContext ).second;
    }

    void Release( HWND hWnd )
    {
        std::scoped_lock lk( m_Mutex );

        // Remove context association.
        m_pContexts.erase( hWnd );

        if( m_pContexts.empty() )
        {
            // Last hook reference has been released, unhook from Windows.
            assert( m_hHook );
            UnhookWindowsHookEx( m_hHook );

            m_hHook = nullptr;
        }
    }

private:
    HHOOK m_hHook = nullptr;
    std::unordered_map<HWND, ImGui_ImplWin32_Context*> m_pContexts = {};
    std::mutex m_Mutex;

    ImGui_ImplWin32_Context* GetContext( HWND hWnd )
    {
        std::scoped_lock lk( m_Mutex );

        // Find context associated with the window.
        auto contextIter = m_pContexts.find( hWnd );
        if( contextIter != m_pContexts.end() )
        {
            return contextIter->second;
        }

        // No context found.
        return nullptr;
    }

    static LRESULT CALLBACK Execute( int nCode, WPARAM wParam, LPARAM lParam )
    {
        GetMessageHook& hook = ImGui_ImplWin32_Context_GetMessageHook();
        bool filterMessage = false;

        // MSDN: GetMsgHook procedure must process messages when (nCode == HC_ACTION)
        // https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms644981(v=vs.85)
        if( ( nCode == HC_ACTION ) || ( nCode > 0 ) )
        {
            // Make local copy of MSG structure which will be passed to the application
            MSG msg = *reinterpret_cast<const MSG*>( lParam );

            if( msg.hwnd )
            {
                ImGui_ImplWin32_Context* pContext = hook.GetContext( msg.hwnd );

                // Process message in ImGui
                if( pContext )
                {
                    filterMessage = pContext->ProcessMessage( msg );
                }
            }
        }

        // Invoke next hook in chain
        // Call this before modifying lParam (MSG) so that all hooks receive the same message
        const LRESULT result = CallNextHookEx( nullptr, nCode, wParam, lParam );

        if( filterMessage )
        {
            // Change message type to WM_NULL to ignore it in window procedure
            reinterpret_cast<MSG*>( lParam )->message = WM_NULL;
        }

        return result;
    }
};

static GetMessageHook& ImGui_ImplWin32_Context_GetMessageHook()
{
    static GetMessageHook hook;
    return hook;
}

// Helper function for constructing cursor position LPARAM from POINT
static constexpr LPARAM ImGui_ImplWin32_Context_CursorPosToLParam( POINT p )
{
    return ( p.x & 0xFFFF ) | ( ( p.y & 0xFFFF ) << 16 );
}

// Make sure our helper function works with GET_X_LPARAM and GET_Y_LPARAM,
// i.e. constructs correct LPARAM value.
static_assert( GET_X_LPARAM( ImGui_ImplWin32_Context_CursorPosToLParam( { -10, 20 } ) ) == -10 );
static_assert( GET_Y_LPARAM( ImGui_ImplWin32_Context_CursorPosToLParam( { -10, 20 } ) ) == 20 );

/***********************************************************************************\

Function:
    ImGui_ImpWin32_Context

Description:
    Constructor.

\***********************************************************************************/
ImGui_ImplWin32_Context::ImGui_ImplWin32_Context( HWND hWnd )
    : m_AppWindow( hWnd )
{
    if( !ImGui_ImplWin32_Init( m_AppWindow ) )
    {
        InitError();
    }

    if( !ImGui_ImplWin32_Context_GetMessageHook().Acquire( m_AppWindow, this ) )
    {
        // Failed to register hook on GetMessage.
        InitError();
    }

    // Load screen and virtual screen (in case of multi-display configurations) rects.
    m_ScreenRect.left = 0;
    m_ScreenRect.top = 0;
    m_ScreenRect.right = GetSystemMetrics( SM_CXSCREEN );
    m_ScreenRect.bottom = GetSystemMetrics( SM_CYSCREEN );

    m_VirtualScreenRect.left = GetSystemMetrics( SM_XVIRTUALSCREEN );
    m_VirtualScreenRect.top = GetSystemMetrics( SM_YVIRTUALSCREEN );
    m_VirtualScreenRect.right = GetSystemMetrics( SM_CXVIRTUALSCREEN );
    m_VirtualScreenRect.bottom = GetSystemMetrics( SM_CYVIRTUALSCREEN );

    m_RawMousePos = { 0, 0 };
    m_AppCursorPos = { 0, 0 };
}

/***********************************************************************************\

Function:
    ~ImGui_ImpWin32_Context

Description:
    Destructor.

\***********************************************************************************/
ImGui_ImplWin32_Context::~ImGui_ImplWin32_Context()
{
    ImGui_ImplWin32_Context_GetMessageHook().Release( m_AppWindow );
    ImGui_ImplWin32_Shutdown();
}

/***********************************************************************************\

Function:
    GetName

Description:

\***********************************************************************************/
const char* ImGui_ImplWin32_Context::GetName() const
{
    return "Win32";
}

/***********************************************************************************\

Function:
    NewFrame

Description:

\***********************************************************************************/
void ImGui_ImplWin32_Context::NewFrame()
{
    ImGui_ImplWin32_NewFrame();
}

/***********************************************************************************\

Function:
    GetDPIScale

Description:

\***********************************************************************************/
float ImGui_ImplWin32_Context::GetDPIScale() const
{
    return ImGui_ImplWin32_GetDpiScaleForHwnd( m_AppWindow );
}

/***********************************************************************************\

Function:
    InitError

Description:
    Cleanup from partially initialized state

\***********************************************************************************/
void ImGui_ImplWin32_Context::InitError()
{
    ImGui_ImplWin32_Context::~ImGui_ImplWin32_Context();
    throw;
}

/***********************************************************************************\

Function:
    UpdateCursorPosition

Description:
    Reconstructs current cursor position based on raw input data.

Parameters:
    mouse [in]      Raw mouse input data.
    pos   [in,out]  Cursor position in virtual screen coordinates.

\***********************************************************************************/
void ImGui_ImplWin32_Context::UpdateCursorPosition( const RAWMOUSE& mouse )
{
    if( mouse.usFlags & MOUSE_MOVE_ABSOLUTE )
    {
        // Get the reference screen rect
        const RECT& screenRect =
            ( mouse.usFlags & MOUSE_VIRTUAL_DESKTOP )
                ? m_VirtualScreenRect
                : m_ScreenRect;

        // Absolute mouse position is normalized to 0-65535, where 0 is left/top,
        // and 65535 is right/bottom corner of the reference screen rect.
        float x = mouse.lLastX / 65535.f;
        float y = mouse.lLastY / 65535.f;

        // Compute dimensions of the reference screen
        float screenWidth = screenRect.right - screenRect.left;
        float screenHeight = screenRect.bottom - screenRect.top;

        m_RawMousePos.x = static_cast<LONG>( x * screenWidth ) + screenRect.left;
        m_RawMousePos.y = static_cast<LONG>( y * screenHeight ) + screenRect.top;
    }
    else
    {
        // Accumulate relative movement
        m_RawMousePos.x += mouse.lLastX;
        m_RawMousePos.y += mouse.lLastY;
    }

    // Compute position in window coordinates
    m_AppCursorPos = m_RawMousePos;
    ScreenToClient( m_AppWindow, &m_AppCursorPos );
}

/***********************************************************************************\

Function:
    GetMessageHook

Description:

\***********************************************************************************/
bool ImGui_ImplWin32_Context::ProcessMessage( MSG msg )
{
    ImGuiIO& io = ImGui::GetIO();
    bool filterMessage = false;

    // Translate the message so that character input is handled correctly
    TranslateMessage( &msg );

    // Store the message in a temporary queue for further processing
    std::queue<MSG> msgQueue;
    msgQueue.push( msg );

    // Process messages
    while( !msgQueue.empty() )
    {
        msg = msgQueue.front();
        msgQueue.pop();

        // Capture mouse and keyboard events
        if( ( IsMouseMessage( msg ) ) ||
            ( IsKeyboardMessage( msg ) ) )
        {
            ImGui_ImplWin32_WndProcHandler( msg.hwnd, msg.message, msg.wParam, msg.lParam );

            // Don't pass captured events to the application
            filterMessage |= io.WantCaptureMouse || io.WantCaptureKeyboard;
        }

        // Resize window
        else if( msg.message == WM_SIZE )
        {
            io.DisplaySize.x = LOWORD( msg.lParam );
            io.DisplaySize.y = HIWORD( msg.lParam );
        }

        // Translate raw input message into series of corresponding software input messages
        else if( msg.message == WM_INPUT )
        {
            RAWINPUT rawInputDesc;
            UINT rawInputDescSize = sizeof( rawInputDesc );
            if( GetRawInputData(
                    reinterpret_cast<HRAWINPUT>( msg.lParam ),
                    RID_INPUT,
                    &rawInputDesc,
                    &rawInputDescSize,
                    sizeof( rawInputDesc.header ) ) == (UINT)-1 )
            {
                // GetRawInputData failed
                assert( false );
                continue;
            }

            if( rawInputDesc.header.dwType == RIM_TYPEMOUSE )
            {
                const RAWMOUSE& mouse = rawInputDesc.data.mouse;

                // Reconstruct current cursor position
                UpdateCursorPosition( mouse );
                LPARAM cursorPos = ImGui_ImplWin32_Context_CursorPosToLParam( m_AppCursorPos );

                // Get active key modifiers
                WPARAM keyMods = 0;
                if( GetAsyncKeyState( VK_CONTROL ) ) keyMods |= MK_CONTROL;
                if( GetAsyncKeyState( VK_SHIFT ) ) keyMods |= MK_SHIFT;

                if( mouse.usButtonFlags & RI_MOUSE_WHEEL )
                {
                    // Mouse wheel scrolled
                    WPARAM wheelPos = keyMods | ( ( static_cast<WPARAM>( mouse.usButtonData ) & 0xFFFF ) << 16 );
                    msgQueue.push( { msg.hwnd, WM_MOUSEWHEEL, wheelPos, cursorPos, msg.time, msg.pt } );
                }

                if( mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN )
                {
                    // Left mouse button pressed
                    msgQueue.push( { msg.hwnd, WM_LBUTTONDOWN, keyMods, cursorPos, msg.time, msg.pt } );
                }

                if( mouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP )
                {
                    // Left mouse button released
                    msgQueue.push( { msg.hwnd, WM_LBUTTONUP, keyMods, cursorPos, msg.time, msg.pt } );
                }

                if( mouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN )
                {
                    // Right mouse button pressed
                    msgQueue.push( { msg.hwnd, WM_RBUTTONDOWN, keyMods, cursorPos, msg.time, msg.pt } );
                }

                if( mouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP )
                {
                    // Right mouse button released
                    msgQueue.push( { msg.hwnd, WM_RBUTTONUP, keyMods, cursorPos, msg.time, msg.pt } );
                }

                if( mouse.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN )
                {
                    // Middle mouse button pressed
                    msgQueue.push( { msg.hwnd, WM_MBUTTONDOWN, keyMods, cursorPos, msg.time, msg.pt } );
                }

                if( mouse.usButtonFlags & RI_MOUSE_BUTTON_3_UP )
                {
                    // Middle mouse button released
                    msgQueue.push( { msg.hwnd, WM_MBUTTONUP, keyMods, cursorPos, msg.time, msg.pt } );
                }

                // Push additional mouse move message on each incoming raw mouse input data
                msgQueue.push( { msg.hwnd, WM_MOUSEMOVE, keyMods, cursorPos, msg.time, msg.pt } );
            }
        }
    }

    return filterMessage;
}

/***********************************************************************************\

Function:
    IsMouseMessage

Description:
    Checks if MSG describes mouse message.

\***********************************************************************************/
bool ImGui_ImplWin32_Context::IsMouseMessage( const MSG& msg )
{
    return ( msg.message >= WM_MOUSEFIRST ) && ( msg.message <= WM_MOUSELAST );
}

/***********************************************************************************\

Function:
    IsKeyboardMessage

Description:
    Checks if MSG describes keyboard message.

\***********************************************************************************/
bool ImGui_ImplWin32_Context::IsKeyboardMessage( const MSG& msg )
{
    return ( msg.message >= WM_KEYFIRST ) && ( msg.message <= WM_KEYLAST );
}
