#include "imgui_impl_win32.h"
#include "imgui/examples/imgui_impl_win32.h"

#include "profiler/profiler_helpers.h"

// Use implementation provided by the ImGui
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND, UINT, WPARAM, LPARAM );

static LockableUnorderedMap<HWND, ImGui_ImplWin32_Context*> g_pWin32Contexts;
static HHOOK g_GetMessageHook;

/***********************************************************************************\

Function:
    ImGui_ImpWin32_Context

Description:
    Constructor.

\***********************************************************************************/
ImGui_ImplWin32_Context::ImGui_ImplWin32_Context( HWND hWnd )
    : m_AppWindow( hWnd )
{
    // Context is a kind of lock for processing - WindowProc will invoke ImGui implementation as long
    // as this context resides in the map
    g_pWin32Contexts.interlocked_try_emplace( m_AppWindow, this );

    if( !ImGui_ImplWin32_Init( m_AppWindow ) )
        InitError();

    if( !g_GetMessageHook )
    {
        // Register window hook on GetMessage/PeekMessage function
        g_GetMessageHook = SetWindowsHook( WH_GETMESSAGE, ImGui_ImplWin32_Context::GetMessageHook );
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
    g_pWin32Contexts.interlocked_erase( m_AppWindow );

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
            // Prepare message for processing
            TranslateMessage( &msg );

            // Process message in ImGui
            ImGui_ImplWin32_Context* context = nullptr;

            if( g_pWin32Contexts.interlocked_find( msg.hwnd, &context ) )
            {
                // Capture only mouse events
                if( (msg.message >= WM_MOUSEFIRST) && (msg.message <= WM_MOUSELAST) )
                {
                    ImGui_ImplWin32_WndProcHandler( msg.hwnd, msg.message, msg.wParam, msg.lParam );

                    // Don't pass captured events to the application
                    if( ImGui::GetIO().WantCaptureMouse )
                    {
                        filterMessage = true;
                    }
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
