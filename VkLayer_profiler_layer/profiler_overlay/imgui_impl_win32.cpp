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

struct ImGui_ImplWin32_Context_Hook
{
    HHOOK Handle;
    int   Refs;
};

typedef std::unordered_map<HWND, ImGui_ImplWin32_Context*> ImGui_ImplWin32_ContextMap;
typedef std::unordered_map<DWORD, ImGui_ImplWin32_Context_Hook> ImGui_ImplWin32_ThreadHooksMap;

static ImGui_ImplWin32_ContextMap g_pWin32Contexts;
static ImGui_ImplWin32_Context*   g_pWin32CurrentContext;
static ImGui_ImplWin32_Context*   g_pWin32CapturedContext;

static ImGui_ImplWin32_ThreadHooksMap g_Win32ThreadHooks;

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

// Keep track of ImGui capture.
// Windows captures the mouse when moving the window, resulting in WM_LBUTTONUP being sent at the end.
// This results in releasing the capture by ImGui (because no buttons are pressed), and reverting the
// window to its original position.
static HWND ImGui_ImplWin32_Context_SetCapture( HWND hWnd )
{
    g_pWin32CapturedContext = g_pWin32CurrentContext;
    return ::SetCapture( hWnd );
}

static HWND ImGui_ImplWin32_Context_GetCapture()
{
    return (g_pWin32CapturedContext ? g_pWin32CapturedContext->GetWindow() : nullptr);
}

static BOOL ImGui_ImplWin32_Context_ReleaseCapture()
{
    if( g_pWin32CapturedContext )
    {
        g_pWin32CapturedContext = nullptr;
        return ReleaseCapture();
    }
    return true;
}

// Helper functions for handling key messages.
static constexpr WORD ImGui_ImplWin32_Context_GetVirtualKey( WPARAM wParam )
{
    return LOWORD( wParam );
}

static constexpr WORD ImGui_ImplWin32_Context_GetScanCode( LPARAM lParam )
{
    WORD keyFlags = HIWORD( lParam );
    WORD scanCode = LOBYTE( keyFlags );

    if( (keyFlags & KF_EXTENDED) != 0 )
    {
        scanCode = MAKEWORD( scanCode, 0xe0 );
    }

    return scanCode;
}

// Defined in imgui_impl_win32.cpp
LRESULT ImGui_ImplWin32_WndProcHandler( HWND, UINT, WPARAM, LPARAM );

/***********************************************************************************\

Function:
    ImGui_ImpWin32_Context

Description:
    Constructor.

\***********************************************************************************/
ImGui_ImplWin32_Context::ImGui_ImplWin32_Context( HWND hWnd ) try
    : m_AppWindow( hWnd )
    , m_AppWindowThreadId( 0 )
    , m_pImGuiContext( nullptr )
    , m_RawMouseX( 0 )
    , m_RawMouseY( 0 )
    , m_RawMouseButtons( 0 )
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
    m_AppWindowThreadId = GetWindowThreadProcessId( hWnd, nullptr );

    auto& hook = g_Win32ThreadHooks[ m_AppWindowThreadId ];
    hook.Refs++;

    if( !hook.Handle )
    {
        // Register a window hook on GetMessage/PeekMessage function.
        hook.Handle = SetWindowsHookEx(
            WH_GETMESSAGE,
            ImGui_ImplWin32_Context::GetMessageHook,
            hProfilerDllInstance,
            m_AppWindowThreadId );

        if( !hook.Handle )
        {
            // Failed to register hook on GetMessage.
            throw;
        }
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
    auto& hook = g_Win32ThreadHooks[ m_AppWindowThreadId ];
    hook.Refs--;

    if( !hook.Refs && hook.Handle )
    {
        UnhookWindowsHookEx( hook.Handle );
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
    GetHandle

Description:

\***********************************************************************************/
HWND ImGui_ImplWin32_Context::GetWindow() const
{
    return m_AppWindow;
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
            // Synchronize access to contexts map.
            std::scoped_lock lk( Profiler::s_ImGuiMutex );

            auto it = g_pWin32Contexts.find( msg.hwnd );
            if( it != g_pWin32Contexts.end() )
            {
                // Switch to the context associated with the target window
                g_pWin32CurrentContext = it->second;
                ImGui::SetCurrentContext( g_pWin32CurrentContext->m_pImGuiContext );

                ImGuiIO& io = ImGui::GetIO();

                std::queue<MSG> translatedMsgs;
                translatedMsgs.push( msg );

                // Translate the message so that character input is handled correctly
                if( msg.message == WM_KEYDOWN )
                {
                    WORD virtualKey = ImGui_ImplWin32_Context_GetVirtualKey( msg.wParam );
                    WORD scanCode = ImGui_ImplWin32_Context_GetScanCode( msg.lParam );

                    BYTE keyboardState[ 256 ];
                    if( GetKeyboardState( keyboardState ) )
                    {
                        WCHAR chars[ 8 ];
                        int charCount = ToUnicode( virtualKey, scanCode, keyboardState, chars, ARRAYSIZE( chars ), 0 );
                        if( charCount < 0 )
                        {
                            translatedMsgs.push( { msg.hwnd, WM_DEADCHAR, virtualKey, msg.lParam, msg.time, msg.pt } );
                        }

                        MSG charMsg = { msg.hwnd, WM_CHAR, 0, msg.lParam, msg.time, msg.pt };
                        for( int i = 0; i < charCount; ++i )
                        {
                            charMsg.wParam = static_cast<WPARAM>(chars[ i ]);
                            translatedMsgs.push( charMsg );
                        }
                    }
                }

                // Capture raw input events
                if( msg.message == WM_INPUT )
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
                        POINT p = { g_pWin32CurrentContext->m_RawMouseX, g_pWin32CurrentContext->m_RawMouseY };
                        ImGui_ImplWin32_Context_GetRawMousePosition( mouse, p );

                        // Convert to coordinates relative to the client area.
                        ScreenToClient( msg.hwnd, &p );
                        p.x = std::clamp<int>( p.x, 0, static_cast<int>( io.DisplaySize.x ) );
                        p.y = std::clamp<int>( p.y, 0, static_cast<int>( io.DisplaySize.y ) );

                        LPARAM mousepos = ImGui_ImplWin32_Context_MakeMousePositionLParam( p );

                        // Get active key modifiers
                        WPARAM keymods = g_pWin32CurrentContext->m_RawMouseButtons;
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
                        g_pWin32CurrentContext->m_RawMouseX = p.x;
                        g_pWin32CurrentContext->m_RawMouseY = p.y;
                        g_pWin32CurrentContext->m_RawMouseButtons = keymods & ~(MK_CONTROL | MK_SHIFT);
                    }
                }

                // Handle translated messages.
                while( !translatedMsgs.empty() )
                {
                    const MSG& translatedMsg = translatedMsgs.front();

                    // Pass the message to ImGui backend.
                    ImGui_ImplWin32_WndProcHandler( translatedMsg.hwnd, translatedMsg.message, translatedMsg.wParam, translatedMsg.lParam );

                    // Don't pass captured keyboard and mouse events to the application.
                    filterMessage |= (io.WantCaptureMouse && translatedMsg.message >= WM_MOUSEFIRST && translatedMsg.message <= WM_MOUSELAST) ||
                                     (io.WantCaptureKeyboard && translatedMsg.message >= WM_KEYFIRST && translatedMsg.message <= WM_KEYLAST);

                    translatedMsgs.pop();
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

// Override mouse capture functions with custom wrappers.
#define GetCapture ImGui_ImplWin32_Context_GetCapture
#define SetCapture ImGui_ImplWin32_Context_SetCapture
#define ReleaseCapture ImGui_ImplWin32_Context_ReleaseCapture

// Include the actual backend implementation.
#include <backends/imgui_impl_win32.cpp>
