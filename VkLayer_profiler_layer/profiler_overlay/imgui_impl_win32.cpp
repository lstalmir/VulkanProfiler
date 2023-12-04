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

static ConcurrentMap<HWND, ImGui_ImplWin32_Context*> g_pWin32Contexts;
static HHOOK g_GetMessageHook;

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

static void ImGui_ImplWin32_Context_GetRawMousePosition( const RAWMOUSE& mouse, int& x, int& y )
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

        x = static_cast<int>( normalizedX * screenWidth ) + screenRect.left;
        y = static_cast<int>( normalizedY * screenHeight ) + screenRect.top;
    }
    else
    {
        x += mouse.lLastX;
        y += mouse.lLastY;
    }
}

static constexpr LPARAM ImGui_ImplWin32_Context_MakeMousePositionLParam( POINT p )
{
    return (p.x & 0xffff) | ((p.y & 0xffff) << 16);
}

static_assert( GET_X_LPARAM( ImGui_ImplWin32_Context_MakeMousePositionLParam({ -10, 20 }) ) == -10 );
static_assert( GET_Y_LPARAM( ImGui_ImplWin32_Context_MakeMousePositionLParam({ -10, 20 }) ) == 20 );

/***********************************************************************************\

Function:
    ImGui_ImpWin32_Context

Description:
    Constructor.

\***********************************************************************************/
ImGui_ImplWin32_Context::ImGui_ImplWin32_Context( HWND hWnd )
    : m_AppWindow( hWnd )
    , m_RawMouseX( 0 )
    , m_RawMouseY( 0 )
    , m_RawMouseButtons( 0 )
{
    // Context is a kind of lock for processing - WindowProc will invoke ImGui implementation as long
    // as this context resides in the map
    g_pWin32Contexts.insert( m_AppWindow, this );

    if( !ImGui_ImplWin32_Init( m_AppWindow ) )
    {
        InitError();
    }

    if( !g_GetMessageHook )
    {
        HINSTANCE hProfilerDllInstance =
            static_cast<HINSTANCE>( Profiler::ProfilerPlatformFunctions::GetLibraryInstanceHandle() );

        // Register a global window hook on GetMessage/PeekMessage function.
        g_GetMessageHook = SetWindowsHookEx(
            WH_GETMESSAGE,
            ImGui_ImplWin32_Context::GetMessageHook,
            hProfilerDllInstance,
            0 /*dwThreadId*/ );

        if( !g_GetMessageHook )
        {
            // Failed to register hook on GetMessage.
            InitError();
        }
    }
}

/***********************************************************************************\

Function:
    ~ImGui_ImpWin32_Context

Description:
    Destructor.

\***********************************************************************************/
ImGui_ImplWin32_Context::~ImGui_ImplWin32_Context()
{
    // Erase context from map
    g_pWin32Contexts.remove( m_AppWindow );

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
            // Process message in ImGui
            ImGui_ImplWin32_Context* context = nullptr;

            if( g_pWin32Contexts.find( msg.hwnd, &context ) )
            {
                ImGuiIO& io = ImGui::GetIO();

                // Translate the message so that character input is handled correctly
                std::queue<MSG> translatedMsgs;
                translatedMsgs.push( msg );
                TranslateMessage( &translatedMsgs.front() );

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
                        ImGui_ImplWin32_Context_GetRawMousePosition( mouse, context->m_RawMouseX, context->m_RawMouseY );

                        // Convert to coordinates relative to the client area.
                        POINT p = { context->m_RawMouseX, context->m_RawMouseY };
                        ScreenToClient( msg.hwnd, &p );
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

                        // Save mouse buttons state.
                        context->m_RawMouseButtons = keymods & ~(MK_CONTROL | MK_SHIFT);
                    }
                }

                // Handle translated messages.
                while( !translatedMsgs.empty() )
                {
                    MSG translatedMsg = translatedMsgs.front();
                    translatedMsgs.pop();

                    // Capture mouse and keyboard events
                    if( (IsMouseMessage( translatedMsg )) ||
                        (IsKeyboardMessage( translatedMsg )) )
                    {
                        ImGui_ImplWin32_WndProcHandler( translatedMsg.hwnd, translatedMsg.message, translatedMsg.wParam, translatedMsg.lParam );

                        // Don't pass captured events to the application
                        filterMessage |= io.WantCaptureMouse || io.WantCaptureKeyboard;
                    }
                }

                // Resize window
                if( msg.message == WM_SIZE )
                {
                    ImGui::GetIO().DisplaySize.x = LOWORD( msg.lParam );
                    ImGui::GetIO().DisplaySize.y = HIWORD( msg.lParam );
                }
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
