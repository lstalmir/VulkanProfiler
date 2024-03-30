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

#include "imgui_impl_win32.h"
#include <backends/imgui_impl_win32.h>

#include <queue>
#include <windowsx.h>

#include "profiler/profiler_helpers.h"

typedef std::unordered_map<HWND, ImGui_ImplWin32_Context*> ImGui_ImplWin32_ContextMap;

static ImGui_ImplWin32_ContextMap g_pWin32Contexts;
static ImGui_ImplWin32_Context*   g_pWin32CurrentContext;

namespace Profiler
{
    extern std::mutex s_ImGuiMutex;
}

static RECT ImGui_ImplWin32_Context_GetVirtualScreenRect()
{
    static bool hasRect = false;
    static RECT rect = {};
    if( !hasRect )
    {
        rect.left = GetSystemMetrics( SM_XVIRTUALSCREEN );
        rect.top = GetSystemMetrics( SM_YVIRTUALSCREEN );
        rect.right = GetSystemMetrics( SM_CXVIRTUALSCREEN );
        rect.bottom = GetSystemMetrics( SM_CYVIRTUALSCREEN );
        hasRect = true;
    }
    return rect;
}

static RECT ImGui_ImplWin32_Context_GetScreenRect()
{
    static bool hasRect = false;
    static RECT rect = {};
    if( !hasRect )
    {
        rect.left = 0;
        rect.top = 0;
        rect.right = GetSystemMetrics( SM_CXSCREEN );
        rect.bottom = GetSystemMetrics( SM_CYSCREEN );
        hasRect = true;
    }
    return rect;
}

static void ImGui_ImplWin32_Context_GetRawMousePosition( const RAWMOUSE& mouse, POINT& p )
{
    if( mouse.usFlags & MOUSE_MOVE_ABSOLUTE )
    {
        RECT screenRect;
        if( mouse.usFlags & MOUSE_VIRTUAL_DESKTOP )
            screenRect = ImGui_ImplWin32_Context_GetVirtualScreenRect();
        else screenRect = ImGui_ImplWin32_Context_GetScreenRect();

        float screenWidth = static_cast<float>( screenRect.right - screenRect.left );
        float screenHeight = static_cast<float>( screenRect.bottom - screenRect.top );
        float normalizedX = (mouse.lLastX / 65535.f);
        float normalizedY = (mouse.lLastY / 65535.f);

        p.x = static_cast<int>( normalizedX * screenWidth ) + screenRect.left;
        p.y = static_cast<int>( normalizedY * screenHeight ) + screenRect.top;
    }
    else
    {
        p.x += mouse.lLastX;
        p.y += mouse.lLastY;
    }
}

static constexpr LPARAM ImGui_ImplWin32_Context_MakeMousePositionLParam( POINT p )
{
    return (p.x & 0xffff) | ((p.y & 0xffff) << 16);
}

static_assert( GET_X_LPARAM( ImGui_ImplWin32_Context_MakeMousePositionLParam({ -10, 20 }) ) == -10 );
static_assert( GET_Y_LPARAM( ImGui_ImplWin32_Context_MakeMousePositionLParam({ -10, 20 }) ) == 20 );

HWND ImGui_ImplWin32_Context_GetCapture()
{
    assert( g_pWin32CurrentContext );

    // Prevent ImGui from acquiring the capture if the window is being moved down and
    // cursor is over the ImGui window.
    if( g_pWin32CurrentContext->m_EnableCapture )
    {
        return ::GetCapture();
    }

    // Return the handle to trick ImGui into thinking it has the capture.
    return g_pWin32CurrentContext->m_AppWindow;
}

HWND ImGui_ImplWin32_Context_SetCapture( HWND hWnd )
{
    assert( g_pWin32CurrentContext );

    // Check the capture state only on the first button down message.
    g_pWin32CurrentContext->m_EnableCapture = ImGui::GetIO().WantCaptureMouse;

    // ImGui incorrectly captures the mouse when the application's window is moved.
    // This leads to reverting to the previous position, making the window immovable.
    // To avoid this, begin capture only if the mouse is over the overlay.
    if( g_pWin32CurrentContext->m_EnableCapture )
    {
        g_pWin32CurrentContext->m_HasCapture = true;
        return SetCapture( hWnd );
    }

    return nullptr;
}

BOOL ImGui_ImplWin32_Context_ReleaseCapture()
{
    assert( g_pWin32CurrentContext );

    // Mouse up received, so try to capture again on next mouse down message.
    g_pWin32CurrentContext->m_EnableCapture = true;

    // Release the capture only if the overlay window acquired it.
    // Otherwise we would release capture acquired by the application.
    if( g_pWin32CurrentContext->m_HasCapture )
    {
        g_pWin32CurrentContext->m_HasCapture = false;
        return ReleaseCapture();
    }

    return true;
}

LRESULT ImGui_ImplWin32_WndProcHandler( HWND, UINT, WPARAM, LPARAM );

/***********************************************************************************\

Function:
    ImGui_ImpWin32_Context

Description:
    Constructor.

\***********************************************************************************/
ImGui_ImplWin32_Context::ImGui_ImplWin32_Context( HWND hWnd ) try
    : m_AppWindow( hWnd )
    , m_hGetMessageHook( nullptr )
    , m_pImGuiContext( nullptr )
    , m_RawMouseX( 0 )
    , m_RawMouseY( 0 )
    , m_RawMouseButtons( 0 )
    , m_HasCapture( false )
    , m_EnableCapture( true )
    , m_WindowMoveLoop( false )
{
    std::scoped_lock lk( Profiler::s_ImGuiMutex );

    // Access to map controlled with s_ImGuiMutex.
    g_pWin32Contexts.emplace( m_AppWindow, this );

    if( !ImGui_ImplWin32_Init( m_AppWindow ) )
    {
        throw;
    }

    // Get the current ImGui context.
    // It must happen after the backend has been initialized to indicate the initialization was successful.
    m_pImGuiContext = ImGui::GetCurrentContext();

    HINSTANCE hProfilerDllInstance =
        static_cast<HINSTANCE>( Profiler::ProfilerPlatformFunctions::GetLibraryInstanceHandle() );

    // Get thread owning the window.
    DWORD dwWindowThreadId = GetWindowThreadProcessId( hWnd, nullptr );

    // Register a window hook on GetMessage/PeekMessage function.
    m_hGetMessageHook = SetWindowsHookEx(
        WH_GETMESSAGE,
        ImGui_ImplWin32_Context::GetMessageHook,
        hProfilerDllInstance,
        dwWindowThreadId );

    if( !m_hGetMessageHook )
    {
        // Failed to register hook on GetMessage.
        throw;
    }
}
catch (...)
{
    // Cleanup the partially initialized context.
    ImGui_ImplWin32_Context::~ImGui_ImplWin32_Context();
    throw;
}

/***********************************************************************************\

Function:
    ~ImGui_ImpWin32_Context

Description:
    Destructor.

\***********************************************************************************/
ImGui_ImplWin32_Context::~ImGui_ImplWin32_Context()
{
    std::scoped_lock lk( Profiler::s_ImGuiMutex );

    // Unhook from the window
    if( m_hGetMessageHook )
    {
        UnhookWindowsHookEx( m_hGetMessageHook );
    }

    // Uninitialize the backend
    if( m_pImGuiContext )
    {
        ImGui::SetCurrentContext( m_pImGuiContext );
        ImGui_ImplWin32_Shutdown();
    }

    // Erase context from map
    g_pWin32Contexts.erase( m_AppWindow );
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
    GetMessageHook

Description:

\***********************************************************************************/
LRESULT CALLBACK ImGui_ImplWin32_Context::GetMessageHook( int nCode, WPARAM wParam, LPARAM lParam )
{
    bool filterMessage = false;

    // MSDN: GetMsgHook procedure must process messages when (nCode == HC_ACTION)
    // https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms644981(v=vs.85)
    if( (nCode == HC_ACTION) || (nCode > 0) )
    {
        // Make local copy of MSG structure which will be passed to the application
        MSG msg = *reinterpret_cast<const MSG*>(lParam);

        if( msg.hwnd )
        {
            std::scoped_lock lk( Profiler::s_ImGuiMutex );

            auto it = g_pWin32Contexts.find( msg.hwnd );
            if( it != g_pWin32Contexts.end() )
            {
                // Switch to the context associated with the target window
                ImGui_ImplWin32_Context* context = it->second;
                g_pWin32CurrentContext = context;

                ImGui::SetCurrentContext( context->m_pImGuiContext );

                ImGuiIO& io = ImGui::GetIO();

                // Translate the message so that character input is handled correctly
                std::queue<MSG> translatedMsgs;
                translatedMsgs.push( msg );
                //TranslateMessage( &translatedMsgs.front() );

                // Capture raw input events
                if( translatedMsgs.front().message == WM_INPUT )
                {
                    MSG inputMsg = translatedMsgs.front();
                    translatedMsgs.pop();

                    HRAWINPUT hRawInput = reinterpret_cast<HRAWINPUT>( inputMsg.lParam );
                    RAWINPUT rawInputDesc;
                    UINT rawInputDescSize = sizeof( rawInputDesc );
                    GetRawInputData( hRawInput, RID_INPUT, &rawInputDesc, &rawInputDescSize, sizeof( RAWINPUTHEADER ) );

                    if( rawInputDesc.header.dwType == RIM_TYPEMOUSE )
                    {
                        // Determine actual input event type.
                        const RAWMOUSE& mouse = rawInputDesc.data.mouse;

                        // Reconstruct mouse position.
                        POINT p = { context->m_RawMouseX, context->m_RawMouseY };
                        ImGui_ImplWin32_Context_GetRawMousePosition( mouse, p );

                        // Convert to coordinates relative to the client area.
                        ScreenToClient( msg.hwnd, &p );
                        p.x = std::clamp<int>( p.x, 0, static_cast<int>( io.DisplaySize.x ) );
                        p.y = std::clamp<int>( p.y, 0, static_cast<int>( io.DisplaySize.y ) );

                        LPARAM mousepos = ImGui_ImplWin32_Context_MakeMousePositionLParam( p );

                        // Get active key modifiers
                        WPARAM keymods = context->m_RawMouseButtons;
                        if( GetAsyncKeyState( VK_CONTROL ) ) keymods |= MK_CONTROL;
                        if( GetAsyncKeyState( VK_SHIFT ) ) keymods |= MK_SHIFT;

                        if( mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN )
                        {
                            keymods |= MK_LBUTTON;
                            translatedMsgs.push({ msg.hwnd, WM_LBUTTONDOWN, keymods, mousepos, msg.time, msg.pt });
                        }
                        if( mouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP )
                        {
                            keymods &= ~MK_LBUTTON;
                            translatedMsgs.push({ msg.hwnd, WM_LBUTTONUP, keymods, mousepos, msg.time, msg.pt });
                        }
                        if( mouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN )
                        {
                            keymods |= MK_RBUTTON;
                            translatedMsgs.push({ msg.hwnd, WM_RBUTTONDOWN, keymods, mousepos, msg.time, msg.pt });
                        }
                        if( mouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP )
                        {
                            keymods &= ~MK_RBUTTON;
                            translatedMsgs.push({ msg.hwnd, WM_RBUTTONUP, keymods, mousepos, msg.time, msg.pt });
                        }
                        if( mouse.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN )
                        {
                            keymods |= MK_MBUTTON;
                            translatedMsgs.push({ msg.hwnd, WM_MBUTTONDOWN, keymods, mousepos, msg.time, msg.pt });
                        }
                        if( mouse.usButtonFlags & RI_MOUSE_BUTTON_3_UP )
                        {
                            keymods &= ~MK_MBUTTON;
                            translatedMsgs.push({ msg.hwnd, WM_MBUTTONUP, keymods, mousepos, msg.time, msg.pt });
                        }
                        if( mouse.usButtonFlags & RI_MOUSE_WHEEL )
                        {
                            WPARAM wheeldelta = keymods | ((WPARAM)reinterpret_cast<const SHORT&>( mouse.usButtonData ) << 16);
                            translatedMsgs.push({ msg.hwnd, WM_MOUSEWHEEL, wheeldelta, mousepos, msg.time, msg.pt });
                        }

                        // Generate mouse move message.
                        translatedMsgs.push({ msg.hwnd, WM_MOUSEMOVE, 0, mousepos, msg.time, msg.pt });

                        // Save mouse state.
                        context->m_RawMouseX = p.x;
                        context->m_RawMouseY = p.y;
                        context->m_RawMouseButtons = keymods & ~(MK_CONTROL | MK_SHIFT);
                    }
                }

                // Handle translated messages.
                while( !translatedMsgs.empty() )
                {
                    MSG translatedMsg = translatedMsgs.front();
                    translatedMsgs.pop();

                    // Ignore all messages sent during window move.
                    if( translatedMsg.message == WM_ENTERSIZEMOVE )
                    {
                        context->m_WindowMoveLoop = true;
                    }
                    if( translatedMsg.message == WM_EXITSIZEMOVE )
                    {
                        context->m_WindowMoveLoop = false;
                    }
                    if( context->m_WindowMoveLoop )
                    {
                        continue;
                    }

                    // Capture mouse and keyboard events
                    if( (IsMouseMessage( translatedMsg )) ||
                        (IsKeyboardMessage( translatedMsg )) )
                    {
                        //// Don't notify on mouse events targeted outsize of the window, unless the capture is active.
                        //if( IsMouseMessage( translatedMsg ) && !context->m_HasCapture )
                        //{
                        //    int y = GET_Y_LPARAM( translatedMsg.lParam );
                        //    if( y < 0 )
                        //    {
                        //        continue;
                        //    }
                        //}

                        ImGui_ImplWin32_WndProcHandler( translatedMsg.hwnd, translatedMsg.message, translatedMsg.wParam, translatedMsg.lParam );

                        // Don't pass captured events to the application
                        filterMessage |= io.WantCaptureMouse || io.WantCaptureKeyboard;
                    }
                }

                // Resize window.
                // WM_SIZE may not be submitted if the application doesn't call DefWindowProc on WM_WINDOWPOSCHANGED.
                if( msg.message == WM_WINDOWPOSCHANGED )
                {
                    const WINDOWPOS* pWindowPos = reinterpret_cast<const WINDOWPOS*>( msg.lParam );
                    io.DisplaySize.x = static_cast<float>( pWindowPos->cx );
                    io.DisplaySize.y = static_cast<float>( pWindowPos->cy );
                }

                g_pWin32CurrentContext = nullptr;
            }
        }
    }

    // Invoke next hook in chain
    // Call this before modifying lParam (MSG) so that all hooks receive the same message
    const LRESULT result = CallNextHookEx( nullptr, nCode, wParam, lParam );

    if( filterMessage )
    {
        // Change message type to WM_NULL to ignore it in window procedure
        reinterpret_cast<MSG*>(lParam)->message = WM_NULL;
    }

    return result;
}

/***********************************************************************************\

Function:
    IsMouseMessage

Description:
    Checks if MSG describes mouse message.

\***********************************************************************************/
bool ImGui_ImplWin32_Context::IsMouseMessage( const MSG& msg )
{
    return (msg.message >= WM_MOUSEFIRST) && (msg.message <= WM_MOUSELAST);
}

/***********************************************************************************\

Function:
    IsKeyboardMessage

Description:
    Checks if MSG describes keyboard message.

\***********************************************************************************/
bool ImGui_ImplWin32_Context::IsKeyboardMessage( const MSG& msg )
{
    return (msg.message >= WM_KEYFIRST) && (msg.message <= WM_KEYLAST);
}

/////////////////////////////////////////////////////////////////////////////////////

// Override mouse capture functions with custom wrappers.
//#define GetCapture ImGui_ImplWin32_Context_GetCapture
//#define SetCapture ImGui_ImplWin32_Context_SetCapture
//#define ReleaseCapture ImGui_ImplWin32_Context_ReleaseCapture

// Include the actual backend implementation.
#include <backends/imgui_impl_win32.cpp>

/////////////////////////////////////////////////////////////////////////////////////
