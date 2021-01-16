// Copyright (c) 2019-2021 Lukasz Stalmirski
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

#ifdef WIN32

#include "profiler_etw_logger.h"
#include "profiler_helpers.h"

#include <Windows.h>
#include <evntcons.h>
#include <evntrace.h>

namespace
{
    /***********************************************************************************\

    Class:
        Handle

    Description:
        Wraps HANDLE.

    \***********************************************************************************/
    class Handle
    {
        Handle( const Handle& ) = delete;

    public:
        inline Handle( HANDLE handle = INVALID_HANDLE_VALUE ) : m_Handle( handle ) {}
        inline Handle( Handle&& other ) : Handle() { std::swap( m_Handle, other.m_Handle ); }
        inline ~Handle() { if( IsValid() ) CloseHandle( m_Handle ); }

        inline operator HANDLE() const { return m_Handle; }
        inline PHANDLE operator&() { return &m_Handle; }
        inline bool IsValid() const { return m_Handle != INVALID_HANDLE_VALUE; }

    private:
        HANDLE m_Handle;
    };

    /***********************************************************************************\

    Structure:
        EventTraceProperties

    Description:
        Wraps EVENT_TRACE_PROPERTIES with additional memory for session name.

    \***********************************************************************************/
    struct EventTraceProperties : EVENT_TRACE_PROPERTIES
    {
        char m_pSessionName[ MAX_PATH ];
    };

    /***********************************************************************************\

    Class:
        UserTokenInformation

    Description:
        Wraps TOKEN_USER buffer memory.

    \***********************************************************************************/
    class UserTokenInformation
    {
    public:
        const TOKEN_USER& Token;

        inline UserTokenInformation( std::unique_ptr<std::byte[]>&& pData )
            : Token( *reinterpret_cast<const TOKEN_USER*>(pData.get()) )
        { // Construct helper structure
            m_pData = std::move( pData );
        }

        inline bool IsValid() const
        { // Check if returned token information is valid
            return static_cast<bool>(m_pData);
        }

    private:
        std::unique_ptr<std::byte[]> m_pData;
    };

    /***********************************************************************************\

    Function:
        GetCurrentUserTokenInformation

    Description:
        Query current user token information - SID and attributes.

    \***********************************************************************************/
    inline static UserTokenInformation GetCurrentUserTokenInformation()
    {
        // Get token of the current process
        Handle hCurrentProcessToken;

        if( !OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hCurrentProcessToken ) )
        {
            return nullptr;
        }

        // Get size of the memory buffer needed for the SID
        DWORD bufferSize = 0;
        GetTokenInformation( hCurrentProcessToken, TokenUser, nullptr, 0, &bufferSize );

        // Allocate memory buffer for token information
        std::unique_ptr<std::byte[]> pBuffer = std::make_unique<std::byte[]>( bufferSize );

        if( !GetTokenInformation( hCurrentProcessToken, TokenUser, pBuffer.get(), bufferSize, &bufferSize ) )
        {
            return nullptr;
        }

        return pBuffer;
    }
}

namespace Profiler
{
    /***********************************************************************************\

    Function:
        Initialize

    Description:
        Setup CPU profiler for data collection.

    \***********************************************************************************/
    VkResult DeviceProfilerEtwLogger::Initialize( VkDevice_Object* pDevice )
    {
        DWORD result = ERROR_SUCCESS;

        EventTraceProperties traceProperties = {};

        // Get process ID of the profiled application
        m_ProcessId = GetCurrentProcessId();

        // Construct ETW session name
        char sessionName[ MAX_PATH ] = "DeviceProfilerEtwLogger_session_0xXXXXXXXXXXXXXXXX";
        u64tohex( sessionName + 32, VkObject_Traits<VkDevice>::GetObjectHandleAsUint64( pDevice->Handle ) );

        // Start ETW session
        if( result == ERROR_SUCCESS )
        {
            traceProperties.Wnode.BufferSize = sizeof( traceProperties );
            traceProperties.Wnode.ClientContext = 1;

            // Allow real-time data collection
            traceProperties.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
            traceProperties.LogFileNameOffset = 0;
            traceProperties.LoggerNameOffset = offsetof( EventTraceProperties, m_pSessionName );

            result = StartTraceA( &m_hSession, sessionName, &traceProperties );
        }

        // Get current user's SID
        const UserTokenInformation tokenInformation = GetCurrentUserTokenInformation();

        if( !tokenInformation.IsValid() )
        {
            result = GetLastError();
        }

        // Grant permission to read real-time data
        if( result == ERROR_SUCCESS )
        {
            #if 0
            result = EventAccessControl(
                &traceProperties.Wnode.Guid,
                EventSecurityAddDACL,
                tokenInformation.Token.User.Sid,
                TRACELOG_ACCESS_REALTIME,
                true );
            #endif
        }

        // Setup ETW consumer
        if( result == ERROR_SUCCESS )
        {
            EVENT_TRACE_LOGFILEA traceLogFileInfo = {};

            traceLogFileInfo.Context = this;
            traceLogFileInfo.ProcessTraceMode =
                PROCESS_TRACE_MODE_EVENT_RECORD |
                PROCESS_TRACE_MODE_RAW_TIMESTAMP;

            // Capture real-time data
            traceLogFileInfo.LogFileName = nullptr;
            traceLogFileInfo.LoggerName = sessionName;
            traceLogFileInfo.ProcessTraceMode |= PROCESS_TRACE_MODE_REAL_TIME;

            // Setup callbacks
            traceLogFileInfo.EventRecordCallback = DeviceProfilerEtwLogger::EventRecordCallback;

            m_hTrace = OpenTraceA( &traceLogFileInfo );

            // TODO: On Windows 7 32-bit invalid TRACEHANDLE is 0x00000000FFFFFFFF
            if( m_hTrace == INVALID_PROCESSTRACE_HANDLE )
            {
                result = GetLastError();
            }
        }

        if( result != ERROR_SUCCESS )
        {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        return VK_SUCCESS;
    }

    /***********************************************************************************\

    Function:
        Destroy

    Description:
        Free ETW logger resources.

    \***********************************************************************************/
    void DeviceProfilerEtwLogger::Destroy()
    {
        // TODO: On Windows 7 32-bit invalid TRACEHANDLE is 0x00000000FFFFFFFF
        if( m_hTrace != INVALID_PROCESSTRACE_HANDLE )
        {
            CloseTrace( m_hTrace );
            m_hTrace = INVALID_PROCESSTRACE_HANDLE;
        }

        // TODO: On Windows 7 32-bit invalid TRACEHANDLE is 0x00000000FFFFFFFF
        if( m_hSession != INVALID_PROCESSTRACE_HANDLE )
        {
            EventTraceProperties traceProperties = {};
            traceProperties.Wnode.BufferSize = sizeof( traceProperties );
            traceProperties.LoggerNameOffset = offsetof( EventTraceProperties, m_pSessionName );

            ControlTraceA( m_hSession, nullptr, &traceProperties, EVENT_TRACE_CONTROL_STOP );
            m_hSession = INVALID_PROCESSTRACE_HANDLE;
        }

        m_ProcessId = 0;
    }

    /***********************************************************************************\

    Function:
        EventRecordCallback

    Description:
        Invoked when event is recorded.

    \***********************************************************************************/
    void CALLBACK DeviceProfilerEtwLogger::EventRecordCallback( EVENT_RECORD* pEventRecord )
    {
        DeviceProfilerEtwLogger* pLogger = reinterpret_cast<DeviceProfilerEtwLogger*>(pEventRecord->UserContext);

        // Get provider ID
        const EVENT_HEADER& header = pEventRecord->EventHeader;

        // Check if event was created by the application
        if( header.ProcessId == pLogger->m_ProcessId )
        {
            if( header.ProviderId ==  )
        }

    }
}

#endif // WIN32
