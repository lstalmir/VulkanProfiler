#pragma once
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdarg.h>
#include <string>

namespace Profiler
{
    class ProfilerOutput
    {
    public:
        // Initialize console profiler output
        inline ProfilerOutput()
            : m_OutputHandle( stdout )
        {
        }

        // Initialize file profiler output
        inline ProfilerOutput( std::string filename )
            : m_OutputHandle( nullptr )
        {
#       if defined(_WIN32)
            fopen_s( &m_OutputHandle, filename.c_str(), "w" );
#       else
            m_OutputHandle = fopen( filename.c_str(), "w" );
#       endif
        }

        // Close the profiler output
        inline ~ProfilerOutput()
        {
            if( m_OutputHandle )
            {
                // Flush all pending writes
                fflush( m_OutputHandle );

                if( m_OutputHandle && m_OutputHandle != stdout )
                {
                    fclose( m_OutputHandle );
                    m_OutputHandle = nullptr;
                }
            }
        }

        // Move the profiler output
        inline ProfilerOutput( ProfilerOutput&& output )
            : m_OutputHandle( nullptr )
        {
            std::swap( m_OutputHandle, output.m_OutputHandle );
        }

        // Move the profiler output
        inline ProfilerOutput& operator=( ProfilerOutput&& output )
        {
            ProfilerOutput tmp( std::move( output ) );
            std::swap( m_OutputHandle, tmp.m_OutputHandle );
            return *this;
        }

        // Print message to the profiler output
        inline virtual void WriteLine( const char* fmt, ... )
        {
            va_list args;
            va_start( args, fmt );
            
            [[maybe_unused]]
            const int charactersWritten = vfprintf_s( m_OutputHandle, fmt, args );

            fprintf_s( m_OutputHandle, "\n" );

            va_end( args, fmt );

        }

        // Flush the profiler output stream
        inline virtual void Flush()
        {
            fflush( m_OutputHandle );
        }

    protected:
        FILE* m_OutputHandle;
    };

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

    protected:
        void* m_ConsoleOutputHandle;

        uint32_t m_Width;
        uint32_t m_Height;
        uint32_t m_BufferSize;
        char* m_pBuffer;

        uint32_t m_FrontBufferLineCount;
        uint32_t m_BackBufferLineCount;
    };
}
