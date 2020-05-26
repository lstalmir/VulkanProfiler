#pragma once
#include <cmath>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vulkan/vk_layer.h>
#include <vulkan/vulkan.h>
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
    // Windows-linux compatibility
    #ifndef _MSC_VER

    /***********************************************************************************\

    Function:
        printf_s

    \***********************************************************************************/
    template<typename... Args>
    inline int printf_s( const char* fmt, Args... args )
    {
        return printf( fmt, args... );
    }

    /***********************************************************************************\

    Function:
        sprintf_s

    \***********************************************************************************/
    template<typename... Args>
    inline int sprintf_s( char* dst, size_t size, const char* fmt, Args... args )
    {
        (size); // Welp, we should check if buffer will fit the args...
        return sprintf( dst, fmt, args... );
    }

    template<size_t N, typename... Args>
    inline int sprintf_s( char( &dst )[N], const char* fmt, Args... args )
    {
        (N); // ... and here too
        return sprintf( dst, fmt, args... );
    }

    /***********************************************************************************\

    Function:
        vsprintf_s

    \***********************************************************************************/
    inline int vsprintf_s( char* dst, size_t size, const char* fmt, va_list args )
    {
        (size); // ... and here
        return vsprintf( dst, fmt, args );
    }

    template<size_t N>
    inline int vsprintf_s( char( &dst )[N], const char* fmt, va_list args )
    {
        (N); // ...
        return vsprintf( dst, fmt, args );
    }

    #else // _MSC_VER defined

    // Use Posix function signatures
    #define strdup _strdup

    #endif /// _MSC_VER

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
        u32tohex

    Description:
        Convert 32-bit unsigned number to hexadecimal string.

    \***********************************************************************************/
    inline void u32tohex( char* pBuffer, uint32_t value )
    {
        static const char hexDigits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
        static_assert( sizeof( hexDigits ) == 16 );

        for( int i = 0; i < 8; ++i )
        {
            // Begin with most significant bit:
            // out[0] = hex[ (V >> 28) & 0xF ]
            // out[1] = hex[ (V >> 24) & 0xF ] ...
            pBuffer[ i ] = hexDigits[ (value >> (32 - ((i + 1) << 2))) & 0xF ];
        }
    }

    /***********************************************************************************\

    Function:
        u64tohex

    Description:
        Convert 64-bit unsigned number to hexadecimal string.

    \***********************************************************************************/
    inline void u64tohex( char* pBuffer, uint64_t value )
    {
        static const char hexDigits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
        static_assert( sizeof( hexDigits ) == 16 );

        for( int i = 0; i < 16; ++i )
        {
            // Begin with most significant bit:
            // out[0] = hex[ (V >> 60) & 0xF ]
            // out[1] = hex[ (V >> 56) & 0xF ]
            pBuffer[ i ] = hexDigits[ (value >> (64 - ((i + 1) << 2))) & 0xF ];
        }
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
    template<typename KeyType, typename ValueType,
        typename HashFn = std::hash<KeyType>,
        typename EqualFn = std::equal_to<KeyType>,
        typename AllocFn = std::allocator<std::pair<const KeyType, ValueType>>>
    class LockableUnorderedMap
        : public std::unordered_map<KeyType, ValueType, HashFn, EqualFn, AllocFn>
    {
    private:
        using BaseType = std::unordered_map<KeyType, ValueType, HashFn, EqualFn, AllocFn>;

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
        const ValueType& interlocked_at( const KeyType& key ) const
        {
            std::scoped_lock lk( m_Mtx );
            return BaseType::at( key );
        }

        // Get map's element atomically
        ValueType& interlocked_at( const KeyType& key )
        {
            std::scoped_lock lk( m_Mtx );
            return BaseType::at( key );
        }

        // Remove element from map atomically
        size_t interlocked_erase( const KeyType& key )
        {
            std::scoped_lock lk( m_Mtx );
            return BaseType::erase( key );
        }

        // Try to insert new element to map atomically
        template<typename... Args>
        auto interlocked_try_emplace( const KeyType& key, Args... arguments )
        {
            std::scoped_lock<std::mutex> lk( m_Mtx );
            return BaseType::try_emplace( key, arguments... );
        }

        // Insert new element to map atomically
        template<typename... Args>
        auto interlocked_emplace( const KeyType& key, Args... arguments )
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
