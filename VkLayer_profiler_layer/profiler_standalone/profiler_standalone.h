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

#pragma once
#include <stdint.h>
#include <vector>
#include <string>

#include <vulkan/vulkan.h>

namespace Profiler
{
    inline constexpr uint16_t bswap( uint16_t value )
    {
        return ( value << 8 ) | ( value >> 8 );
    }

    inline constexpr int16_t bswap( int16_t value )
    {
        return ( value << 8 ) | ( value >> 8 );
    }

    inline constexpr uint32_t bswap( uint32_t value )
    {
        return ( ( value & 0x000000FF ) << 24 ) |
               ( ( value & 0x0000FF00 ) << 8 ) |
               ( ( value & 0x00FF0000 ) >> 8 ) |
               ( ( value & 0xFF000000 ) >> 24 );
    }

    inline constexpr int32_t bswap( int32_t value )
    {
        return ( ( value & 0x000000FF ) << 24 ) |
               ( ( value & 0x0000FF00 ) << 8 ) |
               ( ( value & 0x00FF0000 ) >> 8 ) |
               ( ( value & 0xFF000000 ) >> 24 );
    }

    inline constexpr uint64_t bswap( uint64_t value )
    {
        return ( ( value & 0x00000000000000FF ) << 56 ) |
               ( ( value & 0x000000000000FF00 ) << 40 ) |
               ( ( value & 0x0000000000FF0000 ) << 24 ) |
               ( ( value & 0x00000000FF000000 ) << 8 ) |
               ( ( value & 0x000000FF00000000 ) >> 8 ) |
               ( ( value & 0x0000FF0000000000 ) >> 24 ) |
               ( ( value & 0x00FF000000000000 ) >> 40 ) |
               ( ( value & 0xFF00000000000000 ) >> 56 );
    }

    inline constexpr int64_t bswap( int64_t value )
    {
        return ( ( value & 0x00000000000000FF ) << 56 ) |
               ( ( value & 0x000000000000FF00 ) << 40 ) |
               ( ( value & 0x0000000000FF0000 ) << 24 ) |
               ( ( value & 0x00000000FF000000 ) << 8 ) |
               ( ( value & 0x000000FF00000000 ) >> 8 ) |
               ( ( value & 0x0000FF0000000000 ) >> 24 ) |
               ( ( value & 0x00FF000000000000 ) >> 40 ) |
               ( ( value & 0xFF00000000000000 ) >> 56 );
    }

    inline constexpr float bswap( float value )
    {
        union
        {
            float f;
            uint32_t i;
        } u = { value };
        uint32_t u = bswap( u.i );
        return u.f;
    }

    inline constexpr double bswap( double value )
    {
        union
        {
            double d;
            uint64_t i;
        } u = { value };
        u.i = bswap( u.i );
        return u.d;
    }

    template<typename T>
    inline constexpr T le( T value )
    {
        if constexpr( PROFILER_IS_BIG_ENDIAN )
            return bswap( value );
        return value;
    }

    template<typename T>
    inline constexpr T be( T value )
    {
        if constexpr( PROFILER_IS_BIG_ENDIAN )
            return value;
        return bswap( value );
    }

    template<typename T>
    struct Binary
    {
        T value;
    };

    template<typename T>
    inline Binary<T&> binary( T& value ) { return { value }; }

    template<typename T>
    inline Binary<const T&> binary( const T& value ) { return { value }; }

    template<typename Stream, typename T>
    Stream& operator<<( Stream& stream, Binary<T> binary )
    {
        stream.write( reinterpret_cast<const char*>( &binary.value ), sizeof( T ) );
        return stream;
    }

    template<typename Stream, typename T>
    Stream& operator>>( Stream& stream, Binary<T&> binary )
    {
        stream.read( reinterpret_cast<char*>( &binary.value ), sizeof( T ) );
        return stream;
    }

    enum class NetworkRequest : uint8_t
    {
        eGetServerInfo = 0,
        eGetApplicationInfo = 1,
        eGetObjectName = 2,
        eGetPerformanceData = 3,
        eGetMemoryData = 4,
        eQuit = 0xFF
    };

    class NetworkSocket
    {
    public:
        NetworkSocket();
        ~NetworkSocket();

        NetworkSocket( NetworkSocket&& );
        NetworkSocket& operator=( NetworkSocket&& );

        NetworkSocket( const NetworkSocket& ) = delete;
        NetworkSocket& operator=( const NetworkSocket& ) = delete;

        bool Initialize();
        void Destroy();

        bool IsValid() const;
        bool IsSet() const;

        bool Bind( const char* pAddress, uint16_t port );
        bool Listen();
        bool Connect( const char* pAddress, uint16_t port );
        bool Accept( NetworkSocket& client );

        int Send( const void* pData, size_t size );
        int Receive( void* pBuffer, size_t size );

        static bool Select( NetworkSocket** ppSockets, size_t count, uint32_t timeout = 0 );

    private:
        uintptr_t m_Handle;
        bool m_IsSet;
    };

    class NetworkBuffer
    {
    public:
        inline explicit NetworkBuffer( size_t size = 65536 )
            : m_Bytes( size )
            , m_CurrentOffset( 0 )
        {
        }

        inline void Resize( size_t size ) { m_Bytes.resize( size ); }
        inline void Clear() { m_CurrentOffset = 0; }

        inline size_t GetDataSize() const { return m_CurrentOffset; }
        inline void* GetDataPointer() { return m_Bytes.data(); }
        inline const void* GetDataPointer() const { return m_Bytes.data(); }

        template<typename T>
        inline auto Write( T value )
            -> typename std::enable_if<std::is_integral<T>::value && !std::is_enum<T>::value, size_t>::type
        {
            if( m_CurrentOffset + sizeof( T ) > m_Bytes.size() )
            {
                return 0;
            }
            value = le( value );
            memcpy( m_Bytes.data() + m_CurrentOffset, &value, sizeof( T ) );
            m_CurrentOffset += sizeof( T );
            return 1;
        }

        template<typename T>
        inline auto Read( T& value )
            -> typename std::enable_if<std::is_integral<T>::value && !std::is_enum<T>::value, size_t>::type
        {
            if( m_CurrentOffset + sizeof( T ) > m_Bytes.size() )
            {
                return 0;
            }
            memcpy( &value, m_Bytes.data() + m_CurrentOffset, sizeof( T ) );
            value = le( value );
            m_CurrentOffset += sizeof( T );
            return 1;
        }

        template<typename EnumT>
        inline auto Write( EnumT value )
            -> typename std::enable_if<std::is_enum<EnumT>::value, size_t>::type
        {
            return Write( static_cast<typename std::underlying_type<EnumT>::type>( value ) );
        }

        template<typename EnumT>
        inline auto Read( EnumT& value )
            -> typename std::enable_if<std::is_enum<EnumT>::value, size_t>::type
        {
            typename std::underlying_type<EnumT>::type underlyingValue;
            if( !Read( underlyingValue ) )
            {
                return 0;
            }
            value = static_cast<EnumT>( underlyingValue );
            return 1;
        }

        template<size_t N>
        inline size_t Write( const char ( &value )[ N ] )
        {
            const size_t size = strnlen( value, N );
            if( m_CurrentOffset + size + sizeof( uint32_t ) > m_Bytes.size() )
            {
                return 0;
            }

            Write( static_cast<uint32_t>( size ) );
            memcpy( m_Bytes.data() + m_CurrentOffset, value, size );
            m_CurrentOffset += size;
            return 1;
        }

        template<size_t N>
        inline size_t Read( char ( &value )[ N ] )
        {
            uint32_t size;
            if( !Read( size ) )
            {
                return 0;
            }
            memcpy( value, m_Bytes.data() + m_CurrentOffset, size );
            value[ size ] = '\0';
            m_CurrentOffset += size;
            return 1;
        }

        template<typename T>
        inline auto Write( const T& value )
            -> typename std::enable_if<std::is_class<T>::value, size_t>::type;

        template<typename T>
        inline auto Read( T& value )
            -> typename std::enable_if<std::is_class<T>::value, size_t>::type;

        template<typename T>
        inline size_t Write( const std::vector<T>& value )
        {
            const size_t size = value.size();
            if( !Write( static_cast<uint32_t>( size ) ) )
            {
                return 0;
            }

            for( size_t i = 0; i < size; ++i )
            {
                if( !Write( value[ i ] ) )
                {
                    return i;
                }
            }

            return size;
        }

        template<typename T>
        inline size_t Read( std::vector<T>& value )
        {
            uint32_t size;
            if( !Read( size ) )
            {
                return 0;
            }

            value.resize( size );
            for( size_t i = 0; i < size; ++i )
            {
                if( !Read( value[ i ] ) )
                {
                    return i;
                }
            }

            return size;
        }

    private:
        std::vector<uint8_t> m_Bytes;
        size_t m_CurrentOffset;
    };

    template<>
    inline size_t NetworkBuffer::Write( const std::string& value )
    {
        const size_t size = value.size();
        if( m_CurrentOffset + size + sizeof( uint32_t ) > m_Bytes.size() )
        {
            return 0;
        }

        Write( static_cast<uint32_t>( size ) );
        memcpy( m_Bytes.data() + m_CurrentOffset, value.data(), size );
        m_CurrentOffset += size;
        return 1;
    }

    template<>
    inline size_t NetworkBuffer::Read( std::string& value )
    {
        uint32_t size;
        if( !Read( size ) )
        {
            return 0;
        }

        value.resize( size );
        memcpy( value.data(), m_Bytes.data() + m_CurrentOffset, size );
        m_CurrentOffset += size;
        return 1;
    }
}
