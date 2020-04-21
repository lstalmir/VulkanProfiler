#pragma once

// Public interface
#include "profiler_ext/VkProfilerEXT.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdarg.h>
#include <string>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        ProfilerConsoleOutput

    Description:
        Writes profiling output to the console.

    \***********************************************************************************/
    class ProfilerConsoleOutput
    {
    public:
        ProfilerConsoleOutput();

        uint32_t Width() const;

        bool NextLinesVisible( int count ) const;

        void SkipLines( int count );

        void WriteLine( const char* fmt, ... );

        void WriteAt( int X, int Y, const char* fmt, ... );

        void Flush();

        struct
        {
            uint32_t Width;
            uint32_t Height;
            uint32_t Version;
            VkProfilerModeEXT Mode;
            float FPS;
            std::string Message;
        }   Summary;

    protected:
        void* m_ConsoleOutputHandle;

        uint32_t m_Width;
        uint32_t m_Height;
        uint32_t m_BufferSize;
        char* m_pBuffer;

        int m_FrontBufferLineCount;
        int m_BackBufferLineCount;

        uint16_t m_DefaultAttributes;
        uint16_t* m_pAttributesBuffer;

        int m_FirstVisibleLine;
        int m_LastVisibleLine;

        void FillAttributes( uint16_t, uint32_t, uint32_t );

        void DrawSummary();

        void DrawButton( const char*, bool, uint32_t );
    };
}
