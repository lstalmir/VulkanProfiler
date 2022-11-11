// Copyright (c) 2019-2022 Lukasz Stalmirski
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
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vulkan/vk_layer.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

#include "VkLayer_profiler_layer.generated.h"

// Exit current function without fixing the state
#define RETURNONFAIL( VKRESULT )            \
    {                                       \
        VkResult result = (VKRESULT);       \
        if( result != VK_SUCCESS )          \
        {                                   \
            return result;                  \
        }                                   \
    }

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

// Helper macro for converting non-string literals to C string literals
#define PROFILER_MAKE_STRING_IMPL( LIT ) #LIT
#define PROFILER_MAKE_STRING( LIT ) PROFILER_MAKE_STRING_IMPL( LIT )

// Force inlining of some functions for performance reasons
#if defined( _MSC_VER )
#define PROFILER_FORCE_INLINE __forceinline
#elif defined( __GNUC__ )
#define PROFILER_FORCE_INLINE inline __attribute__((always_inline))
#else
#define PROFILER_FORCE_INLINE inline
#endif

namespace Profiler
{
    /***********************************************************************************\

    Function:
        ClearMemory

    Description:
        Fill memory region with zeros.

    \***********************************************************************************/
    template<typename T>
    PROFILER_FORCE_INLINE void ClearMemory( T* pMemory )
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
    PROFILER_FORCE_INLINE void ClearStructure( T* pStruct, VkStructureType type )
    {
        memset( pStruct, 0, sizeof( T ) );
        pStruct->sType = type;
    }

    /***********************************************************************************\

    Function:
        u8tohex

    Description:
        Convert 8-bit unsigned number to hexadecimal string.

    \***********************************************************************************/
    PROFILER_FORCE_INLINE void u8tohex( char* pBuffer, uint8_t value )
    {
        static const char hexDigits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
        static_assert(sizeof( hexDigits ) == 16);

        for( int i = 0; i < 2; ++i )
        {
            // Begin with most significant bit:
            // out[0] = hex[ (V >> 28) & 0xF ]
            // out[1] = hex[ (V >> 24) & 0xF ] ...
            pBuffer[ i ] = hexDigits[ (value >> (8 - ((i + 1) << 2))) & 0xF ];
        }
    }

    /***********************************************************************************\

    Function:
        u16tohex

    Description:
        Convert 16-bit unsigned number to hexadecimal string.

    \***********************************************************************************/
    PROFILER_FORCE_INLINE void u16tohex( char* pBuffer, uint16_t value )
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
    PROFILER_FORCE_INLINE void u32tohex( char* pBuffer, uint32_t value )
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
    PROFILER_FORCE_INLINE void u64tohex( char* pBuffer, uint64_t value )
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
    PROFILER_FORCE_INLINE void structtohex( char( &pBuffer )[ Size ], const T& value )
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
    PROFILER_FORCE_INLINE uint32_t DigitCount( T value )
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
        ProfilerStringFunctions

    Description:
        Common operations on strings of characters.

    \***********************************************************************************/
    struct ProfilerStringFunctions
    {
        template<size_t dstSize, typename... ArgsT>
        static void Format( char( &dst )[ dstSize ], const char* fmt, ArgsT... args )
        {
#if defined( _MSC_VER )
            sprintf_s( dst, fmt, args... );
#else
            sprintf( dst, fmt, args... );
#endif
        }
        
        template<typename... ArgsT>
        static void Format( char* dst, size_t dstSize, const char* fmt, ArgsT... args )
        {
#if defined( _MSC_VER )
            sprintf_s( dst, dstSize, fmt, args... );
#else
            (void) dstSize;
            sprintf( dst, fmt, args... );
#endif
        }

        template<typename CharT>
        static void CopyString( CharT* pDst, size_t dstSize, const CharT* pSrc, size_t srcSize )
        {
            if( dstSize > 0 )
            {
                // Avoid buffer overruns.
                const size_t maxCopySize = std::min( dstSize, srcSize );

                size_t copySize = 0;
                while( (*pSrc) && (copySize < maxCopySize) )
                {
                    *(pDst++) = *(pSrc++);
                    copySize++;
                }

                // Write null terminator at the end of the copied string.
                if( (maxCopySize == dstSize) && (copySize == maxCopySize) )
                {
                    // If the source string did not fit in the destination buffer,
                    // the null terminator must be inserted in the place of the last copied character,
                    // because currently pDst points at the end of the available memory.
                    pDst[-1] = 0;
                }
                else
                {
                    // If there is still place in the destination buffer,
                    // insert the null terminator after the last copied character.
                    *pDst = 0;
                }
            }
        }

        template<typename CharT, size_t dstSize, size_t srcSize>
        static void CopyString( CharT( &dst )[ dstSize ], const CharT( &src )[ srcSize ] )
        {
            static_assert( dstSize >= srcSize,
                "The destination buffer is insufficient to receive a full copy of the string. "
                "The resulting string would be truncated." );

            CopyString( dst, dstSize, src, srcSize );
        }

        template<typename CharT, size_t dstSize>
        static void CopyString( CharT( &dst )[ dstSize ], const CharT* pSrc, size_t srcSize )
        {
            CopyString( dst, dstSize, pSrc, srcSize );
        }

        template<typename CharT, size_t srcSize>
        static void CopyString( CharT* pDst, size_t dstSize, const CharT( &src )[ srcSize ] )
        {
            CopyString( pDst, dstSize, src, srcSize );
        }

        template<typename CharT>
        static CharT* DuplicateString( const CharT* pString, size_t maxCount )
        {
            if( maxCount == 0 )
            {
                // Nothing to duplicate.
                return nullptr;
            }

            // Allocate space for the duplicated string.
            CharT* pDuplicated = static_cast<CharT*>( malloc( (maxCount + 1) * sizeof( CharT ) ) );
            if( pDuplicated == nullptr )
            {
                // Failed to duplicate the string.
                return nullptr;
            }

            // Copy the source string to the new string.
            CopyString( pDuplicated, maxCount + 1, pString, maxCount );

            return pDuplicated;
        }

        template<typename CharT, size_t srcSize>
        static CharT* DuplicateString( const CharT( &string )[ srcSize ] )
        {
            return DuplicateString( string, GetLength( string ) );
        }

        template<typename CharT>
        static CharT* DuplicateString( const CharT* const& pString )
        {
            return DuplicateString( pString, GetLength( pString ) );
        }

        template<typename CharT>
        static constexpr size_t GetLength( const CharT* const& pString )
        {
            size_t length = 0;
            if( pString != nullptr )
            {
                while( pString[ length ] )
                    length++;
            }
            return length;
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
            static bool customConfigPathInitialized = false;
            std::filesystem::path customConfigPath = "";

            if( !customConfigPathInitialized )
            {
                // Check environment variable
                const auto envProfilerConfigPath = GetEnvironmentVar( "PROFILER_CONFIG_PATH" );

                if( envProfilerConfigPath.has_value() )
                {
                    customConfigPath = envProfilerConfigPath.value();
                }

                customConfigPathInitialized = true;
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

        static bool SetStablePowerState( struct VkDevice_Object* pDevice, void** ppStateHandle );
        static void ResetStablePowerState( void* pStateHandle );

        template<typename... Args>
        inline static void WriteDebug( const char* fmt, Args... args )
        {
            static constexpr size_t messageBufferLength = 256;

            // Include layer prefix to filter debug output
            // Skip ' ' at the end and include string terminator instead
            static constexpr size_t messagePrefixLength = sizeof( VK_LAYER_profiler_name ":" );

            char debugMessageBuffer[ messageBufferLength ] = VK_LAYER_profiler_name ": ";
            ProfilerStringFunctions::Format(
                debugMessageBuffer + messagePrefixLength,
                messageBufferLength - messagePrefixLength, fmt, args... );

            // Invoke OS-specific implementation
            WriteDebugUnformatted( debugMessageBuffer );
        }

        static void WriteDebugUnformatted( const char* str );

        inline static std::filesystem::path FindFile(
            const std::filesystem::path& directory,
            const std::filesystem::path& filename,
            const bool recurse = true )
        {
            if( std::filesystem::exists( directory ) )
            {
                // Enumerate all files in the directory
                for( auto directoryEntry : std::filesystem::directory_iterator( directory ) )
                {
                    if( directoryEntry.path().filename() == filename )
                    {
                        return directoryEntry;
                    }

                    // Check in subdirectories
                    if( directoryEntry.is_directory() && recurse )
                    {
                        std::filesystem::path result = FindFile( directoryEntry, filename, recurse );

                        if( !result.empty() )
                        {
                            return result;
                        }
                    }
                }
            }
            return "";
        }

        inline static std::string GetProcessName()
        {
            // Extract filename component from the path
            return GetApplicationPath().filename().u8string().c_str();
        }

        static uint32_t GetCurrentThreadId();
        static uint32_t GetCurrentProcessId();

        static void GetLocalTime( tm*, const time_t& );

        static std::optional<std::string> GetEnvironmentVar(const char* pVariableName);
    };

    /***********************************************************************************\

    Class:
        EnumArray

    Description:
        Enumerated array can be indexed using enum values.

    \***********************************************************************************/
    template<typename EnumT, typename ValueT, size_t count>
    class EnumArray : public std::array<ValueT, count>
    {
    public:
        using std::array<ValueT, count>::operator[];

        inline ValueT& operator[]( EnumT e ) { return this->operator[]( static_cast<size_t>( e ) ); }
        inline const ValueT& operator[]( EnumT e ) const { return this->operator[]( static_cast<size_t>( e ) ); }
    };

    /***********************************************************************************\

    Class:
        BitsetArray

    Description:
        Enumerated array can be indexed using bit flags.

    \***********************************************************************************/
    template<typename EnumT, typename ValueT, size_t count>
    class BitsetArray : public std::array<ValueT, count>
    {
    public:
        using std::array<ValueT, count>::operator[];

        inline ValueT& operator[]( EnumT e ) { return this->operator[]( IndexOf( e ) ); }
        inline const ValueT& operator[]( EnumT e ) const { return this->operator[]( IndexOf( e ) ); }

    private:
        inline static constexpr size_t IndexOf( EnumT e )
        {
            for( size_t i = 0; i < 64; ++i )
                if (((e >> i) & 1U) == 1U)
                    return i;
            return SIZE_MAX;
        }
    };

    /***********************************************************************************\

    Function:
        CopyElements

    Description:
        Allocates an array of <count> elements and copies the data from pElements to
        the new location.

    \***********************************************************************************/
    template<typename T>
    PROFILER_FORCE_INLINE T* CopyElements(uint32_t count, const T* pElements)
    {
        T* pDuplicated = nullptr;
        if (count > 0)
        {
            pDuplicated = reinterpret_cast<T*>(malloc(count * sizeof(T)));
            if (pDuplicated)
            {
                memcpy(pDuplicated, pElements, count * sizeof(T));
            }
        }
        return pDuplicated;
    }
}
