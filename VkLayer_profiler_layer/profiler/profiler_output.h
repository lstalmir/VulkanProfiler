#pragma once
#include <fstream>
#include <iostream>
#include <stdarg.h>
#include <string>

#if defined(_WIN32)
extern "C" void __stdcall OutputDebugStringA( const char* );
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
            fopen_s( &m_OutputHandle, filename.c_str(), "a" );
#       else
            m_OutputHandle = fopen( filename.c_str(), "a" );
#       endif
        }

        // Close the profiler output
        inline ~ProfilerOutput()
        {
            // Flush all pending writes
            fflush( m_OutputHandle );

            if( m_OutputHandle && m_OutputHandle != stdout )
            {
                fclose( m_OutputHandle );
                m_OutputHandle = nullptr;
            }
        }

        // Print message to the profiler output
        inline void Printf( const char* fmt, ... )
        {
            va_list args;
            va_start( args, fmt );
            
            [[maybe_unused]]
            const int charactersWritten = vfprintf( m_OutputHandle, fmt, args );

            va_end( args, fmt );

#       if defined(_WIN32)
            // Additionally write to the debug console
            std::string debugMessage( charactersWritten + 2, '\0' );

            va_start( args, fmt );
            vsnprintf( debugMessage.data(), debugMessage.max_size(), fmt, args );
            va_end( args );

            // Append end-of-line character to the message
            debugMessage += '\n';

            OutputDebugStringA( debugMessage.c_str() );
#       endif
        }

    protected:
        FILE* m_OutputHandle;
    };
}
