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

#include "profiler_overlay_settings.h"
#include "VkLayer_profiler_layer.generated.h"

#include <string_view>

#include <imgui_internal.h>

namespace Profiler
{
    /***********************************************************************************\

    Structure:
        Setting

    Description:
        An interface for all settings.

        Provides a name which can be used to find a setting and abstract methods for
        clearing, reading and writing from the ini file.

    \***********************************************************************************/
    struct OverlaySettings::Setting
    {
        Setting* const m_pNext;
        std::string const m_Name;

        Setting( Setting*& pSettings, const char* pName )
            : m_pNext( std::exchange( pSettings, this ) )
            , m_Name( pName ) {}

        virtual ~Setting() {}

        virtual void Reset() = 0;
        virtual void Read( const std::string_view& value ) = 0;
        virtual void Write( ImGuiTextBuffer* pOut ) const = 0;
    };

    /***********************************************************************************\

    Structure:
        BoolSetting

    Description:
        Implements a setting that holds a boolean value.

    \***********************************************************************************/
    struct OverlaySettings::BoolSetting : public OverlaySettings::Setting
    {
        bool m_Value;
        bool m_Default;

        BoolSetting( Setting*& pSettings, const char* pName, bool value )
            : Setting( pSettings, pName )
            , m_Value( value )
            , m_Default( value ) {}

        void Reset() final { m_Value = m_Default; }
        void Read( const std::string_view& value ) final { m_Value = (value != "0"); }
        void Write( ImGuiTextBuffer* pOut ) const final { pOut->appendf( "%d", static_cast<int>(m_Value) ); }
    };

    /***********************************************************************************\

    Structure:
        IntSetting

    Description:
        Implements a setting that holds an integer value.

    \***********************************************************************************/
    struct OverlaySettings::IntSetting : public OverlaySettings::Setting
    {
        int m_Value;
        int m_Default;

        IntSetting( Setting*& pSettings, const char* pName, int value )
            : Setting( pSettings, pName )
            , m_Value( value )
            , m_Default( value ) {}

        void Reset() final { m_Value = m_Default; }
        void Read( const std::string_view& value ) final { m_Value = std::stoi( std::string( value ) ); }
        void Write( ImGuiTextBuffer* pOut ) const final { pOut->appendf( "%d", m_Value ); }
    };

    /***********************************************************************************\

    Structure:
        StringSetting

    Description:
        Implements a setting that holds a string value.

    \***********************************************************************************/
    struct OverlaySettings::StringSetting : public OverlaySettings::Setting
    {
        std::string m_Value;
        std::string m_Default;

        StringSetting( Setting*& pSettings, const char* pName, const std::string& value )
            : Setting( pSettings, pName )
            , m_Value( value )
            , m_Default( value ) {}

        void Reset() final { m_Value = m_Default; }
        void Read( const std::string_view& value ) final { m_Value = value; }
        void Write( ImGuiTextBuffer* pOut ) const final { pOut->append( m_Value.c_str() ); }
    };

    /***********************************************************************************\

    Function:
        OverlaySettings

    Description:
        Constructor.

    \***********************************************************************************/
    OverlaySettings::OverlaySettings()
        : m_pSettings( nullptr )
    {}

    /***********************************************************************************\

    Function:
        ~OverlaySettings

    Description:
        Destructor.

    \***********************************************************************************/
    OverlaySettings::~OverlaySettings()
    {
        Setting* pSetting = m_pSettings;
        while( pSetting )
        {
            delete std::exchange( pSetting, pSetting->m_pNext );
        }
    }

    /***********************************************************************************\

    Function:
        RegisterHandler

    Description:
        Adds an ImGui settings handler to the current context.

    \***********************************************************************************/
    void OverlaySettings::RegisterHandler()
    {
        ImGuiSettingsHandler handler = {};
        handler.TypeName = "Layer";
        handler.TypeHash = ImHashStr( handler.TypeName );
        handler.ClearAllFn = ClearAll;
        handler.ReadInitFn = ClearAll;
        handler.ReadOpenFn = ReadEntry;
        handler.ReadLineFn = ReadLine;
        handler.WriteAllFn = WriteAll;
        handler.UserData = this;
        ImGui::AddSettingsHandler( &handler );
    }

    /***********************************************************************************\

    Function:
        AddBool

    Description:
        Registers a boolean setting and returns a pointer to the value that will be
        read from the ini file.

    \***********************************************************************************/
    bool* OverlaySettings::AddBool( const char* pName, bool defaultValue )
    {
        return &(new BoolSetting( m_pSettings, pName, defaultValue ))->m_Value;
    }

    /***********************************************************************************\

    Function:
        AddInt

    Description:
        Registers an integer setting and returns a pointer to the value that will be
        read from the ini file.

    \***********************************************************************************/
    int* OverlaySettings::AddInt( const char* pName, int defaultValue )
    {
        return &(new IntSetting( m_pSettings, pName, defaultValue ))->m_Value;
    }

    /***********************************************************************************\

    Function:
        AddString

    Description:
        Registers a string setting and returns a pointer to the value that will be
        read from the ini file.

    \***********************************************************************************/
    std::string* OverlaySettings::AddString( const char* pName, const std::string& defaultValue )
    {
        return &(new StringSetting( m_pSettings, pName, defaultValue ))->m_Value;
    }

    /***********************************************************************************\

    Function:
        ClearAll

    Description:
        Invoked when ImGui starts reading ini file.
        Restores all settings to their default values.

    \***********************************************************************************/
    void OverlaySettings::ClearAll( ImGuiContext*, ImGuiSettingsHandler* pHandler )
    {
        OverlaySettings* pSettings = static_cast<OverlaySettings*>(pHandler->UserData);

        Setting* pSetting = pSettings->m_pSettings;
        while( pSetting )
        {
            pSetting->Reset();
            pSetting = pSetting->m_pNext;
        }
    }

    /***********************************************************************************\

    Function:
        ReadEntry

    Description:
        Invoked at beginning of each new entry associated with this handler,
        i.e.: "[Layer][???]".

    \***********************************************************************************/
    void* OverlaySettings::ReadEntry( ImGuiContext*, ImGuiSettingsHandler* pHandler, const char* pEntry )
    {
        OverlaySettings* pSettings = static_cast<OverlaySettings*>(pHandler->UserData);
        if( !strcmp( pEntry, "Settings" ) )
        {
            return m_scSettingsEntryTag;
        }
        return nullptr;
    }

    /***********************************************************************************\

    Function:
        ReadLine

    Description:
        Invoked for each line of the recognized entry.

    \***********************************************************************************/
    void OverlaySettings::ReadLine( ImGuiContext*, ImGuiSettingsHandler* pHandler, void* pTag, const char* pLine )
    {
        OverlaySettings* pSettings = static_cast<OverlaySettings*>(pHandler->UserData);

        if( pTag == m_scSettingsEntryTag )
        {
            std::string_view line( pLine );

            size_t sep = line.find( '=' );
            if( sep == std::string_view::npos )
                return;

            std::string_view name = line.substr( 0, sep );
            Setting* pSetting = pSettings->m_pSettings;
            while( pSetting && (pSetting->m_Name != name) )
                pSetting = pSetting->m_pNext;
            if( !pSetting )
                return;

            pSetting->Read( line.substr( sep + 1 ) );
        }
    }

    /***********************************************************************************\

    Function:
        WriteAll

    Description:
        Writes all settings to pOut buffer.

    \***********************************************************************************/
    void OverlaySettings::WriteAll( ImGuiContext*, ImGuiSettingsHandler* pHandler, ImGuiTextBuffer* pOut )
    {
        OverlaySettings* pSettings = static_cast<OverlaySettings*>(pHandler->UserData);

        // Settings entry.
        pOut->appendf( "[%s][Settings]\n", pHandler->TypeName );

        Setting* pSetting = pSettings->m_pSettings;
        while( pSetting )
        {
            pOut->appendf( "%s=", pSetting->m_Name.c_str() );
            pSetting->Write( pOut );
            pSetting = pSetting->m_pNext;
            pOut->append( "\n" );
        }
    }
}
