#pragma once
#include "profiler_mode.h"
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

        void WriteLine( const char* fmt, ... );

        void Flush();

        struct
        {
            uint32_t Width;
            uint32_t Height;
            uint32_t Version;
            ProfilerMode Mode;
        }   Summary;

    protected:
        void* m_ConsoleOutputHandle;

        uint32_t m_Width;
        uint32_t m_Height;
        uint32_t m_BufferSize;
        char* m_pBuffer;

        uint32_t m_FrontBufferLineCount;
        uint32_t m_BackBufferLineCount;

        uint16_t m_DefaultAttributes;
        uint16_t* m_pAttributesBuffer;

        void FillAttributes( uint16_t, uint32_t, uint32_t );

        void DrawSummary();

        void DrawButton( const char*, bool, uint32_t );
    };
}
