#pragma once
#include <mutex>

// TODO: Move to CMake options
#define PROFILER_MAP_PERF_COUNTERS 1
#define PROFILER_MAP_USE_ORDERED 0

#if PROFILER_MAP_USE_ORDERED
#include <map>
#define PROFILER_MAP_BASE_T map
#else
#include <unordered_map>
#define PROFILER_MAP_BASE_T unordered_map
#endif
#define PROFILER_MAP_BASE std::PROFILER_MAP_BASE_T

#if PROFILER_MAP_PERF_COUNTERS
#include "profiler/profiler_counters.h"

// Construct name of the performance counter
#define PROFILER_MAP_PERF_COUNTER_SCOPE_NAME( LINE ) __counter_##LINE

// Create new performance counter at specified source code line
#define PROFILER_MAP_PERF_COUNTER_SCOPE_LINE( COUNTER, LINE ) \
    Profiler::CpuScopedTimestampCounter<std::chrono::nanoseconds> \
    PROFILER_MAP_PERF_COUNTER_SCOPE_NAME( LINE )( COUNTER )

// Create new performance counter at current line
#define PROFILER_MAP_PERF_COUNTER_SCOPE( COUNTER ) \
    PROFILER_MAP_PERF_COUNTER_SCOPE_LINE( COUNTER, __LINE__ )

#else
// Disable all internal performance counters
#define PROFILER_MAP_PERF_COUNTER_SCOPE( COUNTER )
#endif

/***********************************************************************************\

Class:
    ConcurrentMap

Description:
    std::(unordered_)map with mutex

\***********************************************************************************/
template<typename KeyType, typename ValueType>
class ConcurrentMap : PROFILER_MAP_BASE<KeyType, ValueType>
{
private:
    using BaseType = PROFILER_MAP_BASE<KeyType, ValueType>;

    mutable std::mutex m_Mtx;

    // Performance counters
    mutable uint64_t m_AccessTimeNs;

public:
    // Inherit constructors and types
    using BaseType::PROFILER_MAP_BASE_T;
    using typename BaseType::iterator;
    using typename BaseType::const_iterator;

    // Lock access to the map
    void lock() { m_Mtx.lock(); }

    // Unlock access to the map
    void unlock() { m_Mtx.unlock(); }

    // Try to lock access to the map
    bool try_lock() { return m_Mtx.try_lock(); }

    // Reset performance counter
    void reset_perf_counters()
    {
        m_AccessTimeNs = 0;
    }

    // Get performance counter value
    uint64_t get_accumulated_access_time() const
    {
        return m_AccessTimeNs;
    }

    // Remove all elements from the map (thread-safe)
    void clear()
    {
        PROFILER_MAP_PERF_COUNTER_SCOPE( m_AccessTimeNs );
        std::scoped_lock lk( m_Mtx );
        BaseType::clear();
    }

    // Check if collection contains any elements
    bool empty() const
    {
        PROFILER_MAP_PERF_COUNTER_SCOPE( m_AccessTimeNs );
        std::scoped_lock lk( m_Mtx );
        return BaseType::empty();
    }

    // Get number of elements in the collection
    size_t size() const
    {
        PROFILER_MAP_PERF_COUNTER_SCOPE( m_AccessTimeNs );
        std::scoped_lock lk( m_Mtx );
        return BaseType::size();
    }

    // Insert new value into map (thread-safe)
    void insert( const KeyType& key, const ValueType& value )
    {
        PROFILER_MAP_PERF_COUNTER_SCOPE( m_AccessTimeNs );
        std::scoped_lock lk( m_Mtx );
        BaseType::insert( { key, value } );
    }

    // Insert new value into map
    void unsafe_insert( const KeyType& key, const ValueType& value )
    {
        PROFILER_MAP_PERF_COUNTER_SCOPE( m_AccessTimeNs );
        BaseType::insert( { key, value } );
    }

    // Remove value at key (thread-safe)
    void remove( const KeyType& key )
    {
        PROFILER_MAP_PERF_COUNTER_SCOPE( m_AccessTimeNs );
        std::scoped_lock lk( m_Mtx );
        BaseType::erase( key );
    }

    // Remove value at key
    void unsafe_remove( const KeyType& key )
    {
        PROFILER_MAP_PERF_COUNTER_SCOPE( m_AccessTimeNs );
        BaseType::erase( key );
    }

    // Remove value at key (thread-safe)
    iterator remove( const iterator& it )
    {
        PROFILER_MAP_PERF_COUNTER_SCOPE( m_AccessTimeNs );
        std::scoped_lock lk( m_Mtx );
        return BaseType::erase( it );
    }

    // Remove value at iterator
    iterator unsafe_remove( const iterator& it )
    {
        PROFILER_MAP_PERF_COUNTER_SCOPE( m_AccessTimeNs );
        return BaseType::erase( it );
    }

    // Get value at key (thread-safe)
    ValueType& at( const KeyType& key )
    {
        PROFILER_MAP_PERF_COUNTER_SCOPE( m_AccessTimeNs );
        std::scoped_lock lk( m_Mtx );
        return BaseType::at( key );
    }

    // Get value at key
    ValueType& unsafe_at( const KeyType& key )
    {
        PROFILER_MAP_PERF_COUNTER_SCOPE( m_AccessTimeNs );
        return BaseType::at( key );
    }

    // Get value at key (thread-safe)
    const ValueType& at( const KeyType& key ) const
    {
        PROFILER_MAP_PERF_COUNTER_SCOPE( m_AccessTimeNs );
        std::scoped_lock lk( m_Mtx );
        return BaseType::at( key );
    }

    // Get value at key
    const ValueType& unsafe_at( const KeyType& key ) const
    {
        PROFILER_MAP_PERF_COUNTER_SCOPE( m_AccessTimeNs );
        return BaseType::at( key );
    }

    // Check if map contains value at key, return that value (thread-safe)
    bool find( const KeyType& key, ValueType* out )
    {
        PROFILER_MAP_PERF_COUNTER_SCOPE( m_AccessTimeNs );
        std::scoped_lock lk( m_Mtx );
        auto it = BaseType::find( key );
        if( it != BaseType::end() )
        {
            (*out) = it->second;
            return true;
        }
        return false;
    }

    // Check if map contains value at key, return that value
    auto unsafe_find( const KeyType& key )
    {
        PROFILER_MAP_PERF_COUNTER_SCOPE( m_AccessTimeNs );
        return BaseType::find( key );
    }

    // Iterators (unsafe)
    iterator begin() { return BaseType::begin(); }
    iterator end() { return BaseType::end(); }
    const_iterator begin() const { return BaseType::begin(); }
    const_iterator end() const { return BaseType::end(); }
    const_iterator cbegin() const { return BaseType::begin(); }
    const_iterator cend() const { return BaseType::end(); }
};
