#pragma once
#include <cmath>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vk_layer.h>
#include <vulkan.hpp>
#include <vector>

// Helper macro for rolling-back to valid state
#define DESTROYANDRETURNONFAIL( VKRESULT )  \
    {                                       \
        VkResult result = (VKRESULT);       \
        if( result != VK_SUCCESS )          \
        {                                   \
            /* Destroy() must be defined */ \
            Destroy();                      \
            return result;                  \
        }                                   \
    }

namespace Profiler
{
    /***********************************************************************************\

    Function:
        ClearMemory

    Description:
        Fill memory region with zeros.

    \***********************************************************************************/
    template<typename T>
    void ClearMemory( T* pMemory )
    {
        memset( pMemory, 0, sizeof( T ) );
    }

    /***********************************************************************************\

    Function:
        ClearStructure

    Description:
        Fill memory region with zeros and set sType member to provided value.

    \***********************************************************************************/
    template<typename T>
    void ClearStructure( T* pStruct, VkStructureType type )
    {
        memset( pStruct, 0, sizeof( T ) );
        pStruct->sType = type;
    }

    /***********************************************************************************\

    Function:
        DigitCount

    Description:
        Get number of digits in the string representation of the number (decimal).

    \***********************************************************************************/
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    uint32_t DigitCount( T value )
    {
        if( value == 0 )
        {
            return 1;
        }

        // Count sign character
        const uint32_t sign = (value < 0) ? 1 : 0;

        // Log needs positive value
        const uint64_t absValue = std::abs( static_cast<int64_t>(value) );

        return static_cast<uint32_t>(std::log10( absValue ) + 1 + sign);
    }

    /***********************************************************************************\

    Function:
        operator-

    Description:

    \***********************************************************************************/
    template<typename T>
    inline std::unordered_set<T> operator-( const std::unordered_set<T>& lh, const std::unordered_set<T>& rh )
    {

    }

    /***********************************************************************************\

    Class:
        LockableUnorderedMap

    Description:
        std::unordered_map with mutex

    \***********************************************************************************/
    template<typename K, typename V,
        typename HashFn = std::hash<K>,
        typename EqualFn = std::equal_to<K>,
        typename AllocFn = std::allocator<std::pair<const K, V>>>
    class LockableUnorderedMap
        : public std::unordered_map<K, V, HashFn, EqualFn, AllocFn>
    {
    private:
        using BaseType = std::unordered_map<K, V, HashFn, EqualFn, AllocFn>;

        // Helper struct for holding mutex in initializer lists
        struct Locked
        {
        public:
            Locked( LockableUnorderedMap& map ) : m_Map( map ) { m_Map.lock(); }
            ~Locked() { m_Map.unlock(); }

            operator LockableUnorderedMap& () { return m_Map; }

        private:
            LockableUnorderedMap& m_Map;
        };

        mutable std::mutex m_Mtx;

    public:
        // Construct empty lockable unordered_map
        LockableUnorderedMap() : BaseType() {}

        // Construct copy of non-lockable unordered_map
        LockableUnorderedMap( const BaseType& o ) : BaseType( o ) {}

        // Move non-lockable unordered_map
        LockableUnorderedMap( BaseType&& o ) : BaseType( std::move( o ) ) {}

        // Construct copy of lockable unordered_map
        LockableUnorderedMap( const LockableUnorderedMap& o ) : BaseType( Locked( o ) ) {}

        // Move lockable unordered_map
        LockableUnorderedMap( LockableUnorderedMap&& o ) : BaseType( std::move( Locked( o ) ) ) {}

        // Lock access to the map
        void lock() { m_Mtx.lock(); }

        // Unlock access to the map
        void unlock() { m_Mtx.unlock(); }

        // Try to lock access to the map
        bool try_lock() { return m_Mtx.try_lock(); }

        // Get map's element atomically
        const V& interlocked_at( const key_type& key ) const
        {
            std::scoped_lock lk( m_Mtx );
            return BaseType::at( key );
        }

        // Get map's element atomically
        V& interlocked_at( const key_type& key )
        {
            std::scoped_lock lk( m_Mtx );
            return BaseType::at( key );
        }

        // Remove element from map atomically
        size_t interlocked_erase( const key_type& key )
        {
            std::scoped_lock lk( m_Mtx );
            return BaseType::erase( key );
        }

        // Try to insert new element to map atomically
        template<typename... Args>
        auto interlocked_try_emplace( const key_type& key, Args... arguments )
        {
            std::scoped_lock<std::mutex> lk( m_Mtx );
            return BaseType::try_emplace( key, arguments... );
        }

        // Insert new element to map atomically
        template<typename... Args>
        auto interlocked_emplace( const key_type& key, Args... arguments )
        {
            std::scoped_lock<std::mutex> lk( m_Mtx );
            return BaseType::emplace( key, arguments... );
        }
    };

    /***********************************************************************************\

    Class:
        ProfilerPlatformFunctions

    Description:

    \***********************************************************************************/
    class ProfilerPlatformFunctions
    {
    public:
        inline static std::filesystem::path GetCustomConfigPath()
        {
            std::filesystem::path customConfigPath = "";

            // Check environment variable
            const char* pEnvProfilerConfigPath = std::getenv( "PROFILER_CONFIG_PATH" );

            if( pEnvProfilerConfigPath )
            {
                customConfigPath = pEnvProfilerConfigPath;
            }

            // Returns empty if custom config path not defined
            return customConfigPath;
        }

        inline static std::filesystem::path GetApplicationDir()
        {
            static std::filesystem::path applicationDir;

            if( applicationDir.empty() )
            {
                // Get full application path and remove filename component
                applicationDir = GetApplicationPath().remove_filename();
            }

            return applicationDir;
        }

        static std::filesystem::path GetApplicationPath();

        static bool IsPreemptionEnabled();

    };
}
