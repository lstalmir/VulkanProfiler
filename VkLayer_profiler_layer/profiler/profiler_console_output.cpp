#include "profiler_console_output.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

namespace Profiler
{
    /***********************************************************************************\

    Function:
        ProfilerConsoleOutput

    Description:
        Constructor.

    \***********************************************************************************/
    ProfilerConsoleOutput::ProfilerConsoleOutput()
    {
    #if defined(_WIN32)
        AllocConsole();
        AttachConsole( GetCurrentProcessId() );

        // Get handle to the console output
        m_ConsoleOutputHandle = GetStdHandle( STD_OUTPUT_HANDLE );

        // Get size of the console
        CONSOLE_SCREEN_BUFFER_INFO consoleScreenBufferInfo;

        GetConsoleScreenBufferInfo( m_ConsoleOutputHandle, &consoleScreenBufferInfo );

        m_Width = consoleScreenBufferInfo.dwSize.X;
        m_Height = consoleScreenBufferInfo.dwSize.Y;

        m_BufferSize = m_Width * m_Height;

        // Allocate back buffer
        m_pBuffer = reinterpret_cast<char*>(malloc( m_BufferSize ));

        if( m_pBuffer == nullptr )
        {
            // Allocation failed
            throw std::bad_alloc();
        }

        memset( m_pBuffer, 0, m_BufferSize );

        m_BackBufferLineCount = 0;
        m_FrontBufferLineCount = 0;
    #else
    #error Not implemented
    #endif
    }

    /***********************************************************************************\

    Function:
        Width

    Description:
        Get width of the console.

    \***********************************************************************************/
    uint32_t ProfilerConsoleOutput::Width() const
    {
        return m_Width;
    }

    /***********************************************************************************\

    Function:
        WriteLine

    Description:

    \***********************************************************************************/
    void ProfilerConsoleOutput::WriteLine( const char* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );

        // Write to the back buffer to avoid flickering
        [[maybe_unused]]
        int charactersWritten = vsnprintf(
            m_pBuffer + m_BackBufferLineCount * m_Width, m_Width, fmt, args );

        va_end( args );

    #if defined(_WIN32) && defined(_DEBUG) && 0
        // Additionally write to the debug console
        OutputDebugStringA( m_pBuffer + m_BackBufferLineCount * m_Width );
        OutputDebugStringA( "\n" );
    #endif

        // Increment line counter
        m_BackBufferLineCount++;
    }

    /***********************************************************************************\

    Function:
        Flush

    Description:
        Swap buffers.

    \***********************************************************************************/
    void ProfilerConsoleOutput::Flush()
    {
        // Get size of the console
        CONSOLE_SCREEN_BUFFER_INFO consoleScreenBufferInfo;

        GetConsoleScreenBufferInfo( m_ConsoleOutputHandle, &consoleScreenBufferInfo );

        [[maybe_unused]]
        DWORD numCharactersWritten = 0;

        // Update only currently visible region
        DWORD writeRegionBeginOffset = m_Width * consoleScreenBufferInfo.srWindow.Top;

        DWORD writeSize = m_Width *
            max( 0, max( m_BackBufferLineCount, m_FrontBufferLineCount ) - consoleScreenBufferInfo.srWindow.Top );

        WriteConsoleOutputCharacterA(
            m_ConsoleOutputHandle,
            m_pBuffer + writeRegionBeginOffset,
            writeSize,
            { 0, consoleScreenBufferInfo.srWindow.Top },
            &numCharactersWritten );

        // Clear the back buffer
        memset( m_pBuffer, 0, m_BufferSize );

        m_FrontBufferLineCount = m_BackBufferLineCount;
        m_BackBufferLineCount = 0;

        // Update buffer size
        uint32_t width = consoleScreenBufferInfo.dwSize.X;
        uint32_t height = consoleScreenBufferInfo.dwSize.Y;

        // Check if buffer needs to be resized
        uint32_t newBufferSize = width * height;

        if( newBufferSize > m_BufferSize )
        {
            char* pNewBuffer = reinterpret_cast<char*>(realloc( m_pBuffer, newBufferSize ));

            if( pNewBuffer == nullptr )
            {
                // Reallocation failed
                throw std::bad_alloc();
            }

            m_pBuffer = pNewBuffer;
            m_BufferSize = newBufferSize;
        }
        else if( newBufferSize < m_BufferSize )
        {
            m_FrontBufferLineCount = (m_FrontBufferLineCount * m_Width) / width + 1;
        }

        m_Width = width;
        m_Height = height;
    }
}
