#include "profiler_console_output.h"
#include "profiler_helpers.h"
#include <vulkan/vulkan.h>

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
        AttachConsole( ATTACH_PARENT_PROCESS );

        // Get handle to the console output
        m_ConsoleOutputHandle = GetStdHandle( STD_OUTPUT_HANDLE );

        // Get size of the console
        CONSOLE_SCREEN_BUFFER_INFO consoleScreenBufferInfo;

        GetConsoleScreenBufferInfo( m_ConsoleOutputHandle, &consoleScreenBufferInfo );

        m_Width = consoleScreenBufferInfo.dwSize.X;
        m_Height = consoleScreenBufferInfo.dwSize.Y;
        m_DefaultAttributes = consoleScreenBufferInfo.wAttributes;

        m_BufferSize = m_Width * m_Height;

        // Allocate back buffer
        m_pBuffer = reinterpret_cast<char*>(malloc( m_BufferSize ));

        if( m_pBuffer == nullptr )
        {
            // Allocation failed
            throw std::bad_alloc();
        }

        m_pAttributesBuffer = reinterpret_cast<uint16_t*>(malloc( m_BufferSize * sizeof( uint16_t ) ));

        if( m_pAttributesBuffer == nullptr )
        {
            free( m_pBuffer );

            // Allocation failed
            throw std::bad_alloc();
        }

        memset( m_pBuffer, 0, m_BufferSize );

        FillAttributes( m_DefaultAttributes | COMMON_LVB_REVERSE_VIDEO, 0, m_Width );
        FillAttributes( m_DefaultAttributes, m_Width, m_BufferSize );

        m_BackBufferLineCount = 2;
        m_FrontBufferLineCount = 0;
    #else
    #error Not implemented
    #endif

        ClearMemory( &Summary );
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
        NextLineVisible

    Description:

    \***********************************************************************************/
    bool ProfilerConsoleOutput::NextLinesVisible( int count ) const
    {
        return m_BackBufferLineCount + count >= m_FirstVisibleLine
            && m_BackBufferLineCount - count <= m_LastVisibleLine;
    }

    /***********************************************************************************\

    Function:
        SkipLines

    Description:

    \***********************************************************************************/
    void ProfilerConsoleOutput::SkipLines( int count )
    {
        m_BackBufferLineCount += count;
    }

    /***********************************************************************************\

    Function:
        WriteLine

    Description:

    \***********************************************************************************/
    void ProfilerConsoleOutput::WriteLine( const char* fmt, ... )
    {
        if( m_BackBufferLineCount * m_Width >= m_BufferSize )
        {
            return;
        }

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

        m_FirstVisibleLine = consoleScreenBufferInfo.srWindow.Top;
        m_LastVisibleLine = consoleScreenBufferInfo.srWindow.Bottom;

        // Update only currently visible region
        DWORD writeRegionBeginOffset = m_Width * consoleScreenBufferInfo.srWindow.Top;

        DWORD writeSize = m_Width * (consoleScreenBufferInfo.srWindow.Bottom);

        DrawSummary();

        WriteConsoleOutputCharacterA(
            m_ConsoleOutputHandle,
            m_pBuffer + writeRegionBeginOffset,
            writeSize,
            { 0, consoleScreenBufferInfo.srWindow.Top },
            &numCharactersWritten );

        WriteConsoleOutputAttribute(
            m_ConsoleOutputHandle,
            m_pAttributesBuffer + writeRegionBeginOffset,
            writeSize,
            { 0, consoleScreenBufferInfo.srWindow.Top },
            &numCharactersWritten );

        // Clear the back buffer
        memset( m_pBuffer, 0, m_BufferSize );

        FillAttributes( m_DefaultAttributes | COMMON_LVB_REVERSE_VIDEO, 0, m_Width );
        FillAttributes( m_DefaultAttributes, m_Width, m_BufferSize );

        m_FrontBufferLineCount = m_BackBufferLineCount;
        m_BackBufferLineCount = 2;

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

    /***********************************************************************************\

    Function:
        FillAttributes

    Description:

    \***********************************************************************************/
    void ProfilerConsoleOutput::FillAttributes( uint16_t attributes, uint32_t begin, uint32_t count )
    {
        const uint32_t end = begin + count;

        for( uint32_t i = begin; i < end; ++i )
        {
            m_pAttributesBuffer[i] = attributes;
        }
    }

    /***********************************************************************************\

    Function:
        DrawUI

    Description:

    \***********************************************************************************/
    void ProfilerConsoleOutput::DrawSummary()
    {
        const char* mode = "Unknown";
        switch( Summary.Mode )
        {
        case ProfilerMode::ePerFrame: mode = "Frame"; break;
        case ProfilerMode::ePerRenderPass: mode = "RenderPass"; break;
        case ProfilerMode::ePerPipeline: mode = "Pipeline"; break;
        case ProfilerMode::ePerDrawcall: mode = "Drawcall"; break;
        }

        char modeStr[32];
        sprintf_s( modeStr, " Mode: %s ", mode );

        char versionStr[16];
        sprintf_s( versionStr, " Vulkan %u.%u ",
            (Summary.Version >> 22) & 0x3FF,
            (Summary.Version >> 12) & 0x3FF);

        char fpsStr[16];
        sprintf_s( fpsStr, " %8.2f fps ", Summary.FPS );

        DrawButton( modeStr, true, 1 );
        DrawButton( versionStr, false, 22 );
        DrawButton( fpsStr, false, 38 );
    }

    /***********************************************************************************\

    Function:
        DrawButton

    Description:

    \***********************************************************************************/
    void ProfilerConsoleOutput::DrawButton( const char* pTitle, bool selected, uint32_t offset )
    {
        char* pCharacterBuffer = m_pBuffer + offset;

        // Copy string and set attributes
        while( *pTitle )
        {
            *pCharacterBuffer = *pTitle;
            pCharacterBuffer++;
            pTitle++;
        }
    }
}
