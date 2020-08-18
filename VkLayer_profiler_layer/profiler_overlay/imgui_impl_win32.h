#pragma once
#include "imgui_window.h"
#include "lockable_unordered_map.h"

#include <Windows.h>

class ImGui_ImplWin32_Context : public ImGui_Window_Context
{
public:
    ImGui_ImplWin32_Context( HWND hWnd );
    ~ImGui_ImplWin32_Context();

    const char* GetName() const override;

    void NewFrame() override;

private:
    HWND m_AppWindow;

    void InitError();

    static LRESULT CALLBACK GetMessageHook( int, WPARAM, LPARAM );
};
