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

#pragma once
#include "imgui_window.h"
#include "lockable_unordered_map.h"

#include <Windows.h>

struct ImGuiContext;

class ImGui_ImplWin32_Context : public ImGui_Window_Context
{
public:
    ImGui_ImplWin32_Context( HWND hWnd );
    ~ImGui_ImplWin32_Context();

    HWND        GetWindow() const;
    const char* GetName() const override;
    void        NewFrame() override;
    float       GetDPIScale() const override;

private:
    HWND m_hWindow;
    HHOOK m_hGetMessageHook;
    ImGuiContext* m_pImGuiContext;
    int m_RawMouseX;
    int m_RawMouseY;
    int m_RawMouseButtons;

    static LRESULT CALLBACK GetMessageHook( int, WPARAM, LPARAM );
};
