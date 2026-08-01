// Copyright (c) 2026 Lukasz Stalmirski
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
#include <vector>

namespace Profiler
{
    class DeviceProfilerCopyResult
    {
    public:
        DeviceProfilerCopyResult( std::vector<std::byte>& buffer, size_t offset )
            : m_Buffer( buffer )
            , m_Offset( offset )
        {
        }

        size_t GetOffset() const
        {
            return m_Offset;
        }

        void* GetPointer() const
        {
            if( m_Offset != SIZE_MAX )
            {
                return m_Buffer.data() + m_Offset;
            }

            return nullptr;
        }

    private:
        std::vector<std::byte>& m_Buffer;
        size_t m_Offset;
    };

    template<typename T>
    class DeviceProfilerCopyTypedResult : public DeviceProfilerCopyResult
    {
    public:
        using DeviceProfilerCopyResult::DeviceProfilerCopyResult;

        T* GetPointer() const
        {
            return reinterpret_cast<T*>( DeviceProfilerCopyResult::GetPointer() );
        }

        T* operator->() const
        {
            return GetPointer();
        }

        T& operator*() const
        {
            return *GetPointer();
        }
    };

    template<typename T>
    class DeviceProfilerCopyTypedResult<T[]> : public DeviceProfilerCopyResult
    {
    public:
        DeviceProfilerCopyTypedResult( std::vector<std::byte>& buffer, size_t offset, size_t count )
            : DeviceProfilerCopyResult( buffer, offset )
            , m_Count( count )
        {
        }

        T* GetPointer() const
        {
            return reinterpret_cast<T*>( DeviceProfilerCopyResult::GetPointer() );
        }

        DeviceProfilerCopyTypedResult<T> operator[]( size_t index ) const
        {
            return DeviceProfilerCopyTypedResult<T>( m_Buffer, m_Offset + index * sizeof(T) );
        }

    private:
        size_t m_Count;
    };

    class DeviceProfilerCopyBuilder
    {
    public:
        DeviceProfilerCopyBuilder()
            : m_Buffer()
            , m_WrittenPointers()
        {
        }

        void Reset()
        {
            m_Buffer.clear();
            m_WrittenPointers.clear();
        }

        DeviceProfilerCopyResult Write( const void* pData, size_t size )
        {
            if( pData != nullptr && size > 0 )
            {
                const size_t dataOffset = m_Buffer.size();
                ResizeBuffer( dataOffset + size );

                std::memcpy( m_Buffer.data() + dataOffset, pData, size );

                return { m_Buffer, dataOffset };
            }

            return { m_Buffer, SIZE_MAX };
        }

        template<typename T>
        DeviceProfilerCopyTypedResult<std::remove_const_t<T>> Write( const T* const pTypedData )
        {
            return Write( reinterpret_cast<const void*>( pTypedData ), sizeof( T ) );
        }

        template<typename T>
        DeviceProfilerCopyTypedResult<std::remove_const_t<T>[]> Write( const T* const pTypedData, size_t count )
        {
            return Write( reinterpret_cast<const void*>( pTypedData ), sizeof( T ) * count );
        }

        template<typename T>
        DeviceProfilerCopyTypedResult<std::remove_const_t<T>> WriteUninitialized()
        {
            const size_t dataOffset = m_Buffer.size();
            ResizeBuffer( dataOffset + sizeof( T ) );

            return { m_Buffer, dataOffset };
        }

        template<typename T>
        DeviceProfilerCopyTypedResult<std::remove_const_t<T>[]> WriteUninitialized( size_t count )
        {
            const size_t dataOffset = m_Buffer.size();
            ResizeBuffer( dataOffset + sizeof( T ) * count );

            return { m_Buffer, dataOffset, count };
        }

        DeviceProfilerCopyResult WriteUninitialized( void** ppDest, size_t size )
        {
            if( size > 0 )
            {
                if( Allocate( &ppDest, size ) )
                {
                    return { m_Buffer, *reinterpret_cast<size_t*>( ppDest ) };
                }
            }

            return { m_Buffer, SIZE_MAX };
        }

        template<typename T>
        DeviceProfilerCopyTypedResult<std::remove_const_t<T>> WriteUninitialized( T** ppDest )
        {
            return WriteUninitialized( reinterpret_cast<void**>( ppDest ), sizeof( T ) );
        }

        template<typename T>
        DeviceProfilerCopyTypedResult<std::remove_const_t<T>[]> WriteUninitialized( T** ppDest, size_t count )
        {
            return WriteUninitialized( reinterpret_cast<void**>( ppDest ), sizeof( T ) * count );
        }

        void Write( void** ppDest, const void* const pData, size_t size )
        {
            if( pData != nullptr && size > 0 )
            {
                if( Allocate( &ppDest, size ) )
                {
                    std::memcpy( *ppDest, pData, size );
                }
            }
        }

        template<typename T, typename U>
        void Write( T** ppDest, const U* const pTypedData, size_t count = 1 )
        {
            Write( reinterpret_cast<void**>( ppDest ),
                reinterpret_cast<const void*>( pTypedData ),
                sizeof( U ) * count );
        }

        void* GetMemoryCopy() const
        {
            void* pCopy = malloc( m_Buffer.size() );
            if( pCopy != nullptr )
            {
                std::memcpy( pCopy, m_Buffer.data(), m_Buffer.size() );
            }

            return pCopy;
        }

        template<typename T>
        T* GetMemoryCopy() const
        {
            return reinterpret_cast<T*>( GetMemoryCopy() );
        }

    private:
        std::vector<std::byte> m_Buffer;
        std::vector<std::pair<size_t, size_t>> m_WrittenPointers;

        bool Allocate( void*** pppDest, size_t size )
        {
            if( size == 0 )
            {
                return false;
            }

            const size_t dataOffset = m_Buffer.size();
            const size_t destOffset = *pppDest - reinterpret_cast<void**>( m_Buffer.data() );
            ResizeBuffer( dataOffset + size );

            m_WrittenPointers.push_back( { destOffset, dataOffset } );

            // Fix the destination pointer and set it to point to the newly allocated data.
            *pppDest = reinterpret_cast<void**>( m_Buffer.data() + destOffset );
            **pppDest = m_Buffer.data() + dataOffset;

            return true;
        }

        void ResizeBuffer( size_t newSize )
        {
            const bool reallocated = ( m_Buffer.capacity() < newSize );

            m_Buffer.resize( newSize );

            // Fix pointers in case the buffer was reallocated.
            if( reallocated )
            {
                for( auto& [pointerOffset, dataOffset] : m_WrittenPointers )
                {
                    void** pPointer = reinterpret_cast<void**>( m_Buffer.data() + pointerOffset );
                    *pPointer = m_Buffer.data() + dataOffset;
                }
            }
        }
    };
}
