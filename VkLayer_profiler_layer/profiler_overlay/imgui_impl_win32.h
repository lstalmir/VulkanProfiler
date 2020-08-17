#pragma once
#include "imgui_window.h"
#include "lockable_unordered_map.h"

#include <Windows.h>

struct ImGuiContext;

class ImGui_ImplWin32_Context : public ImGui_Window_Context
{
public:
    ImGui_ImplWin32_Context( ImGuiContext* pImGuiContext, HWND hWnd );
    ~ImGui_ImplWin32_Context();

    const char* GetName() const override;

    void NewFrame() override;

private:
    HMODULE m_AppModule;
    HWND m_AppWindow;
    WNDPROC m_AppWindowProc;
    ImGuiContext* m_pImGuiContext;

    // Must be available from static WindowProc
    static LockableUnorderedMap<HWND, ImGui_ImplWin32_Context*> s_pWin32Contexts;

    void InitError();

    static LRESULT CALLBACK WindowProc( HWND, UINT, WPARAM, LPARAM );
};
