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

#include "VkLayer_profiler_layer.generated.h"

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
        u16tohex

    Description:
        Convert 16-bit unsigned number to hexadecimal string.

    \***********************************************************************************/
    inline void u16tohex( char* pBuffer, uint16_t value )
    {
        static const char hexDigits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
        static_assert(sizeof( hexDigits ) == 16);

        for( int i = 0; i < 4; ++i )
        {
            // Begin with most significant bit:
            // out[0] = hex[ (V >> 28) & 0xF ]
            // out[1] = hex[ (V >> 24) & 0xF ] ...
            pBuffer[ i ] = hexDigits[ (value >> (16 - ((i + 1) << 2))) & 0xF ];
        }
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
        structtohex

    Description:
        Convert structure to hexadecimal string.

    \***********************************************************************************/
    template<typename T, size_t Size>
    inline void structtohex( char( &pBuffer )[ Size ], const T& value )
    {
        static const char hexDigits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
        static_assert(sizeof( hexDigits ) == 16);
        static_assert(sizeof( T ) <= (Size / 2));

        for( int i = 0; i < sizeof( T ); ++i )
        {
            const int byte = reinterpret_cast<const char*>(&value)[ i ] & 0xFF;

            pBuffer[ 2 * i ] = hexDigits[ byte >> 4 ];
            pBuffer[ 2 * i + 1 ] = hexDigits[ byte & 0xF ];
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

    Class:
        __PNextTypeTraits

    Description:
        For internal use by PNextIterator.

    \***********************************************************************************/
    template<typename T> struct __PNextTypeTraits;
    template<> struct __PNextTypeTraits<void*> { using StructureType = VkBaseOutStructure*; };
    template<> struct __PNextTypeTraits<const void*> { using StructureType = const VkBaseInStructure*; };

    /***********************************************************************************\

    Class:
        PNextIterator

    Description:
        Helper class for iterating over pNext structure chain.

    \***********************************************************************************/
    template<typename PNextType>
    class PNextIterator
    {
    private:
        using StructureType = typename __PNextTypeTraits<PNextType>::StructureType;
        using ReferenceType = std::add_lvalue_reference_t<std::remove_pointer_t<StructureType>>;

        StructureType pNext;

    public:
        struct IteratorType
        {
            StructureType pStruct;

            inline explicit IteratorType( StructureType pStruct_ ) : pStruct( pStruct_ ) {}

            inline IteratorType operator++( int ) { pStruct = pStruct->pNext; return *this; }
            inline IteratorType operator++() { IteratorType it( pStruct ); pStruct = pStruct->pNext; return it; }
            inline ReferenceType operator*() { return *pStruct; }
            inline bool operator==( const IteratorType& rh ) const { return pStruct == rh.pStruct; }
            inline bool operator!=( const IteratorType& rh ) const { return pStruct != rh.pStruct; }
        };

        inline PNextIterator( PNextType pNext_ ) : pNext( reinterpret_cast<StructureType>(pNext_) ) {}

        inline IteratorType begin() { return IteratorType( pNext ); }
        inline IteratorType end()   { return IteratorType( nullptr ); }
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

        template<typename... Args>
        inline static void WriteDebug( const char* fmt, Args... args )
        {
            // Include layer prefix to filter debug output
            // Skip ' ' at the end and include string terminator instead
            static constexpr size_t messagePrefixLength = sizeof( VK_LAYER_profiler_name ":" );

            char debugMessageBuffer[ 256 ] = VK_LAYER_profiler_name ": ";
            sprintf( debugMessageBuffer + messagePrefixLength, fmt, args... );

            // Invoke OS-specific implementation
            WriteDebugUnformatted( debugMessageBuffer );
        }

        static void WriteDebugUnformatted( const char* str );

    };
}
