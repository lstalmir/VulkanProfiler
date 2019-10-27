#pragma once
#include <memory>
#include <mutex>

namespace Profiler
{
    class ProfilerAllocator
    {
    public:
        inline ProfilerAllocator()
            : m_pAllocations( nullptr )
            , m_BlockCount( 0 )
        {
        }

        inline ProfilerAllocator( uint32_t blockCount )
            : m_pAllocations( nullptr )
            , m_BlockCount( blockCount >> 5 )
        {
            m_pAllocations = new uint32_t[m_BlockCount];

            // Clear the allocation table
            memset( m_pAllocations, 0, m_BlockCount * sizeof( uint32_t ) );
        }

        inline ProfilerAllocator& operator=( const ProfilerAllocator& other )
        {
            delete[] m_pAllocations;

            m_pAllocations = new uint32_t[other.m_BlockCount];
            m_BlockCount = other.m_BlockCount;

            // Copy the allocation table
            memcpy( m_pAllocations, other.m_pAllocations, m_BlockCount * sizeof( uint32_t ) );

            return *this;
        }

        inline ProfilerAllocator& operator=( ProfilerAllocator&& other )
        {
            delete[] m_pAllocations;

            m_pAllocations = nullptr;
            m_BlockCount = 0;

            std::swap( m_pAllocations, other.m_pAllocations );
            std::swap( m_BlockCount, other.m_BlockCount );

            return *this;
        }

        inline ~ProfilerAllocator()
        {
            delete[] m_pAllocations;
        }

        // Allocate one block
        inline uint32_t Allocate()
        {
        }

        // Allocate two blocks
        inline std::pair<uint32_t, uint32_t> AllocatePair()
        {
            std::scoped_lock lk( m_AllocationMutex );
            std::pair<uint32_t, uint32_t> pair( 0xFFFFFFFF, 0xFFFFFFFF );

            bool firstAllocated = false;

            for( uint32_t block = 0; block < m_BlockCount; ++block )
            {
                for( uint32_t subblock = 0; subblock < 32; ++subblock )
                {
                    if( ((m_pAllocations[block] >> subblock) & 1U) == 0U )
                    {
                        if( !firstAllocated )
                        {
                            pair.first = (block << 5) + subblock;

                            // Update the allocation table
                            m_pAllocations[block] |= (1 << subblock);

                            firstAllocated = true;
                        }
                        else
                        {
                            pair.second = (block << 5) + subblock;

                            // Update the allocation table
                            m_pAllocations[block] |= (1 << subblock);

                            return pair;
                        }
                    }
                }
            }

            // Avoid partial allocation
            if( firstAllocated )
            {
                m_pAllocations[pair.first >> 5] &= ~(1 << (pair.first & 0xFFFF));
                pair.first = 0xFFFFFFFF;
            }

            return pair;
        }

        // Free allocated block
        inline void Free( uint32_t block )
        {
            std::scoped_lock lk( m_AllocationMutex );

            if( (block >> 5) < m_BlockCount )
            {
                // Clear the allocation bit
                m_pAllocations[block >> 5] &= ~(1 << (block & 0xFFFF));
            }
        }

    protected:
        std::mutex m_AllocationMutex;
        uint32_t* m_pAllocations;
        uint32_t m_BlockCount;
    };
}
