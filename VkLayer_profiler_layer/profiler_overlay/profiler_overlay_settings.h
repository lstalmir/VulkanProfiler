// Copyright (c) 2024 Lukasz Stalmirski
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
#include <string>

struct ImGuiContext;
struct ImGuiSettingsHandler;
struct ImGuiTextBuffer;

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        OverlaySettings

    Description:
        Handles serialization and deserialization of the overlay gui settings.

    \***********************************************************************************/
    class OverlaySettings
    {
        inline static const char* m_scSettingsEntry = "Settings";

    public:
        OverlaySettings();
        ~OverlaySettings();

        OverlaySettings( const OverlaySettings& ) = delete;
        OverlaySettings( OverlaySettings&& ) = delete;

        void InitializeHandlers();
        void Validate( const char* pFileName );

        bool* AddBool( const char* pName, bool defaultValue = false );
        int* AddInt( const char* pName, int defaultValue = 0 );
        std::string* AddString( const char* pName, const std::string& defaultValue = std::string() );

    private:
        // ImGui settings handler callbacks
        static void ClearAll( ImGuiContext* pCtx, ImGuiSettingsHandler* pHandler );
        static void* ReadEntry( ImGuiContext* pCtx, ImGuiSettingsHandler* pHandler, const char* pName );
        static void ReadLine( ImGuiContext* pCtx, ImGuiSettingsHandler* pHandler, void* pTag, const char* pLine );
        static void WriteAll( ImGuiContext* pCtx, ImGuiSettingsHandler* pHandler, ImGuiTextBuffer* pOut );

    private:
        struct Setting;
        struct BoolSetting;
        struct IntSetting;
        struct StringSetting;

        Setting* m_pSettings;
    };
}
