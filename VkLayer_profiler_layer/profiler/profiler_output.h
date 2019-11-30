#pragma once
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdarg.h>
#include <string>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#undef CreateEvent
#undef CreateSemaphore
#endif

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

#       if defined(_WIN32) && 0
            // Additionally write to the debug console
            std::string debugMessage( charactersWritten + 3, '\0' );

            va_start( args, fmt );
            vsnprintf( debugMessage.data(), debugMessage.max_size(), fmt, args );
            va_end( args );

            // Append end-of-line character to the message
            debugMessage += '\n';

            OutputDebugStringA( debugMessage.c_str() );
#       endif
        }

        // Flush the profiler output stream
        inline virtual void Flush()
        {
            fflush( m_OutputHandle );
        }

    protected:
        FILE* m_OutputHandle;
    };


    class ProfilerConsoleOutput : public ProfilerOutput
    {
    public:
        inline ProfilerConsoleOutput()
            : ProfilerOutput()
        {
            // Get handle to the console output
            m_ConsoleOutputHandle = GetStdHandle( STD_OUTPUT_HANDLE );

            // Get size of the console
            CONSOLE_SCREEN_BUFFER_INFO consoleScreenBufferInfo;
            
            GetConsoleScreenBufferInfo( m_ConsoleOutputHandle, &consoleScreenBufferInfo );

            m_Width = consoleScreenBufferInfo.dwSize.X;
            m_Height = consoleScreenBufferInfo.dwSize.Y;

            m_BufferSize = m_Width * m_Height;

            // Allocate back buffer
            m_pBuffer = new char[m_BufferSize];

            memset( m_pBuffer, 0, m_BufferSize );

            m_BackBufferLineCount = 0;
            m_FrontBufferLineCount = 0;
        }

        inline virtual void WriteLine( const char* fmt, ... ) override
        {
            va_list args;
            va_start( args, fmt );

            vsnprintf( m_pBuffer + m_BackBufferLineCount * m_Width, m_Width, fmt, args );

            va_end( args );

            m_BackBufferLineCount++;
        }

        virtual void Flush() override
        {
            [[maybe_unused]]
            DWORD numCharactersWritten = 0;

            DWORD writeSize = std::max(
                m_BackBufferLineCount,
                m_FrontBufferLineCount ) * m_Width;

            WriteConsoleOutputCharacterA(
                m_ConsoleOutputHandle,
                m_pBuffer,
                writeSize,
                { 0, 0 },
                &numCharactersWritten );

            memset( m_pBuffer, 0, m_BufferSize );

            m_FrontBufferLineCount = m_BackBufferLineCount;
            m_BackBufferLineCount = 0;
        }

    protected:
        HANDLE m_ConsoleOutputHandle;

        uint32_t m_Width;
        uint32_t m_Height;
        uint32_t m_BufferSize;
        char* m_pBuffer;

        uint32_t m_FrontBufferLineCount;
        uint32_t m_BackBufferLineCount;
    };
}
