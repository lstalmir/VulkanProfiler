// Copyright (c) 2025 Lukasz Stalmirski
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

#include "profiler_standalone_client.h"

namespace Profiler
{
    NetworkClient::NetworkClient()
        : m_Socket()
        , m_ApplicationInfo()
        , m_ApplicationName()
        , m_EngineName()
    {
        memset( &m_ApplicationInfo, 0, sizeof( m_ApplicationInfo ) );
    }

    bool NetworkClient::Initialize( const char* pServerAddress, uint16_t port )
    {
        if( !NetworkPlatformFunctions::Initialize() )
        {
            return false;
        }

        if( !m_Socket.Connect( pServerAddress, port ) )
        {
            Destroy();
            return false;
        }

        return true;
    }

    void NetworkClient::Destroy()
    {
        m_Socket.Destroy();

        NetworkPlatformFunctions::Destroy();
    }

    bool NetworkClient::Update()
    {
        NetworkRequest request = NetworkRequest::eGetApplicationInfo;
        m_Socket.Send( &request, sizeof( request ) );

        NetworkBuffer response;
        m_Socket.Receive( *response.GetFirstPacket() );

        const NetworkPacket* pPacket = response.GetFirstPacket();
        const uint8_t* pData = reinterpret_cast<const uint8_t*>( pPacket->GetDataPointer() );
        const uint8_t* pDataEnd = pData + pPacket->GetDataSize();

        bool endOfStream = false;
        while( !endOfStream )
        {
            VkStructureType sType = le( *reinterpret_cast<const VkStructureType*>( pData ) );
            pData += sizeof( VkStructureType );

            switch( sType )
            {
            case NetworkBuffer::EndOfStream:
                endOfStream = true;
                break;

            case VK_STRUCTURE_TYPE_APPLICATION_INFO:
                m_ApplicationInfo.sType = sType;
                m_ApplicationInfo.pNext = nullptr;

                m_ApplicationName.resize( le( *reinterpret_cast<const uint32_t*>( pData ) ) );
                pData += sizeof( uint32_t );
                memcpy( &m_ApplicationName[0], pData, m_ApplicationName.size() );
                pData += m_ApplicationName.size();
                m_ApplicationInfo.pApplicationName = m_ApplicationName.c_str();

                m_ApplicationInfo.applicationVersion = le( *reinterpret_cast<const uint32_t*>( pData ) );
                pData += sizeof( uint32_t );

                m_EngineName.resize( le( *reinterpret_cast<const uint32_t*>( pData ) ) );
                pData += sizeof( uint32_t );
                memcpy( &m_EngineName[0], pData, m_EngineName.size() );
                pData += m_EngineName.size();
                m_ApplicationInfo.pEngineName = m_EngineName.c_str();

                m_ApplicationInfo.engineVersion = le( *reinterpret_cast<const uint32_t*>( pData ) );
                pData += sizeof( uint32_t );

                m_ApplicationInfo.apiVersion = le( *reinterpret_cast<const uint32_t*>( pData ) );
                pData += sizeof( uint32_t );
                break;
            }

            if( pData >= pDataEnd )
            {
                pPacket = pPacket->GetNextPacket();

                if( pPacket )
                {
                    pData = reinterpret_cast<const uint8_t*>( pPacket->GetDataPointer() );
                    pDataEnd = pData + pPacket->GetDataSize();
                }
                else
                {
                    break;
                }
            }
        }

        return endOfStream;
    }

    const VkApplicationInfo& NetworkClient::GetApplicationInfo() const
    {
        return m_ApplicationInfo;
    }
}
