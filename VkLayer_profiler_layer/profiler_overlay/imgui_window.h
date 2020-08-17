#pragma once

class ImGui_Window_Context
{
public:
    virtual ~ImGui_Window_Context() {}
    virtual void NewFrame() = 0;
    virtual void UpdateWindowRect() {}
};
