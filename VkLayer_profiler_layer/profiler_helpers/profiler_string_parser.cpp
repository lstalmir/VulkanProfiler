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

#include "profiler_string_parser.h"
#include "profiler/profiler_data.h"
#include "profiler/profiler_frontend.h"
#include "profiler/profiler_helpers.h"
#include "profiler_layer_objects/VkDevice_object.h"
#include <scn/scan.h>
#include <sstream>

#include "profiler_string_mappings.h"

namespace Profiler
{
    /***********************************************************************************\

    Function:
        MapStringToValue

    Description:
        Return value of the provided key from the mapping.

    \***********************************************************************************/
    template<typename T>
    static typename T::KeyType MapStringToValue( const T& mapping, const std::string_view& str )
    {
        auto value = mapping[str];
        if( value != typename T::KeyType( -1 ) )
        {
            return value;
        }

        auto scanResult = scn::scan<uint64_t>( str,
            scn::scan_format_string<const std::string_view&, uint64_t>( mapping.GetDefaultFormat() ) );

        if( scanResult )
        {
            return typename T::KeyType( scanResult->value() );
        }

        return typename T::KeyType( -1 );
    }

    /***********************************************************************************\

    Function:
        MapStringToFlags

    Description:
        Return flags value of the provided string from the mapping.

    \***********************************************************************************/
    template<typename T>
    static typename T::KeyType MapStringToFlags( const T& mapping, const std::string_view& str, const std::string_view& separator )
    {
        uint64_t flags = 0;

        std::string_view remainingStr = str;
        while( !remainingStr.empty() )
        {
            auto separatorPos = remainingStr.find( separator );
            auto flagName = remainingStr.substr( 0, separatorPos );

            auto value = mapping[flagName];
            if( value != typename T::KeyType( -1 ) )
            {
                flags |= static_cast<uint64_t>( value );
            }
            else
            {
                auto scanResult = scn::scan<uint64_t>( flagName,
                    scn::scan_format_string<const std::string_view&, uint64_t>( mapping.GetDefaultFormat() ) );

                if( scanResult )
                {
                    flags |= scanResult->value();
                }
            }

            if( separatorPos == std::string_view::npos )
            {
                break;
            }

            remainingStr.remove_prefix( separatorPos + separator.size() );
        }

        return typename T::KeyType( flags );
    }

    /***********************************************************************************\

    Function:
        DeviceProfilerStringParser

    Description:
        Constructor.

    \***********************************************************************************/
    DeviceProfilerStringParser::DeviceProfilerStringParser( DeviceProfilerFrontend& frontend )
        : m_Frontend( frontend )
    {
    }

    /***********************************************************************************\

    Function:
        GetCommandType

    Description:
        Returns name of the Vulkan API function.

    \***********************************************************************************/
    DeviceProfilerDrawcallType DeviceProfilerStringParser::GetCommandType( const std::string_view& str ) const
    {
        return MapStringToValue( g_scProfilerDrawcallTypeNames, str );
    }

    /***********************************************************************************\

    Function:
        GetPointer

    \***********************************************************************************/
    const void* DeviceProfilerStringParser::GetPointer( const std::string_view& str ) const
    {
        if( str == "null" )
        {
            return nullptr;
        }

        uint64_t value = 0;
        if( std::from_chars( str.data() + 2, str.data() + str.size(), value, 16 ).ec == std::errc() )
        {
            return reinterpret_cast<const void*>( static_cast<uintptr_t>( value ) );
        }

        return nullptr;
    }

    /***********************************************************************************\

    Function:
        GetBool

    \***********************************************************************************/
    VkBool32 DeviceProfilerStringParser::GetBool( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanBoolNames, str );
    }

    /***********************************************************************************\

    Function:
        GetVec4

    \***********************************************************************************/
    std::array<float, 4> DeviceProfilerStringParser::GetVec4( const std::string_view& str ) const
    {
        auto result = scn::scan<float, float, float, float>( str, "{}, {}, {}, {}" );
        if( result )
        {
            const auto& values = result.value().values();
            return {
                std::get<0>( values ),
                std::get<1>( values ),
                std::get<2>( values ),
                std::get<3>( values )
            };
        }

        return { 0.f, 0.f, 0.f, 0.f };
    }

    /***********************************************************************************\

    Function:
        GetVersion

    \***********************************************************************************/
    uint32_t DeviceProfilerStringParser::GetVersion( const std::string_view& str ) const
    {
        auto result = scn::scan<uint32_t, uint32_t, uint32_t>( str, "{}.{}.{}" );
        if( result )
        {
            const auto& values = result.value().values();
            return VK_MAKE_VERSION(
                std::get<0>( values ),
                std::get<1>( values ),
                std::get<2>( values ) );
        }

        return 0;
    }

    /***********************************************************************************\

    Function:
        GetColorHex

    Description:
        Parses hexadecimal 24-bit color representation (in #RRGGBB format).

    \***********************************************************************************/
    std::array<float, 3> DeviceProfilerStringParser::GetColorHex( const std::string_view& str ) const
    {
        uint32_t value = 0;
        if( std::from_chars( str.data() + 1, str.data() + str.size(), value, 16 ).ec == std::errc() )
        {
            const uint8_t R = static_cast<uint8_t>( ( value >> 16 ) & 0xFF );
            const uint8_t G = static_cast<uint8_t>( ( value >> 8 ) & 0xFF );
            const uint8_t B = static_cast<uint8_t>( value & 0xFF );
            return { R / 255.f, G / 255.f, B / 255.f };
        }

        return { 0.f, 0.f, 0.f };
    }

    /***********************************************************************************\

    Function:
        GetByteSize

    Description:
        Returns a human-readable string representation of the given byte size,
        using the appropriate unit (B, kB, MB, GB).

    \***********************************************************************************/
    VkDeviceSize DeviceProfilerStringParser::GetByteSize( const std::string_view& str ) const
    {
        typedef uint8_t Kilobyte[1024];
        typedef Kilobyte Megabyte[1024];
        typedef Megabyte Gigabyte[1024];

        float size = 0.f;
        std::string_view unit = "B";

        if( auto result = scn::scan<float, std::string_view>( str, "{} {}" ) )
        {
            const auto& values = result.value().values();
            size = std::get<0>( values );
            unit = std::get<1>( values );
        }
        else if( auto result = scn::scan<float>( str, "{}" ) )
        {
            size = result->value();
        }

        switch( FNV( unit ) )
        {
        default:
        case FNV( "B" ):
            return static_cast<VkDeviceSize>( size );
        case FNV( "kB" ):
            return static_cast<VkDeviceSize>( size * sizeof( Kilobyte ) );
        case FNV( "MB" ):
            return static_cast<VkDeviceSize>( size * sizeof( Megabyte ) );
        case FNV( "GB" ):
            return static_cast<VkDeviceSize>( size * sizeof( Gigabyte ) );
        }
    }

    /***********************************************************************************\

    Function:
        GetDeviceType

    \***********************************************************************************/
    VkPhysicalDeviceType DeviceProfilerStringParser::GetDeviceType( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanDeviceTypeNames, str );
    }

    /***********************************************************************************\

    Function:
        GetQueueFlags

    \***********************************************************************************/
    VkQueueFlags DeviceProfilerStringParser::GetQueueFlags( const std::string_view& str, const char* separator ) const
    {
        return MapStringToFlags( g_scVulkanQueueFlagNames, str, separator );
    }

    /***********************************************************************************\

    Function:
        GetShaderStage

    \***********************************************************************************/
    VkShaderStageFlagBits DeviceProfilerStringParser::GetShaderStage( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanShaderStageNames, str );
    }

    /***********************************************************************************\

    Function:
        GetShortShaderStage

    \***********************************************************************************/
    VkShaderStageFlagBits DeviceProfilerStringParser::GetShortShaderStage( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanShortShaderStageNames, str );
    }

    /***********************************************************************************\

    Function:
        GetShaderGroupType

    \***********************************************************************************/
    VkRayTracingShaderGroupTypeKHR DeviceProfilerStringParser::GetShaderGroupType( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanRayTracingShaderGroupTypeNames, str );
    }

    /***********************************************************************************\

    Function:
        GetGeneralShaderGroupType

    \***********************************************************************************/
    VkShaderStageFlagBits DeviceProfilerStringParser::GetGeneralShaderGroupType( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanRayTracingGeneralShaderGroupTypeNames, str );
    }

    /***********************************************************************************\

    Function:
        GetFormat

    \***********************************************************************************/
    VkFormat DeviceProfilerStringParser::GetFormat( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanFormatNames, str );
    }

    /***********************************************************************************\

    Function:
        GetIndexType

    \***********************************************************************************/
    VkIndexType DeviceProfilerStringParser::GetIndexType( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanIndexTypeNames, str );
    }

    /***********************************************************************************\

    Function:
        GetVertexInputRate

    \***********************************************************************************/
    VkVertexInputRate DeviceProfilerStringParser::GetVertexInputRate( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanVertexInputRateNames, str );
    }

    /***********************************************************************************\

    Function:
        GetPrimitiveTopology

    \***********************************************************************************/
    VkPrimitiveTopology DeviceProfilerStringParser::GetPrimitiveTopology( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanPrimitiveTopologyNames, str );
    }

    /***********************************************************************************\

    Function:
        GetPolygonMode

    \***********************************************************************************/
    VkPolygonMode DeviceProfilerStringParser::GetPolygonMode( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanPolygonModeNames, str );
    }

    /***********************************************************************************\

    Function:
        GetCullMode

    \***********************************************************************************/
    VkCullModeFlags DeviceProfilerStringParser::GetCullMode( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanCullModeNames, str );
    }

    /***********************************************************************************\

    Function:
        GetFrontFace

    \***********************************************************************************/
    VkFrontFace DeviceProfilerStringParser::GetFrontFace( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanFrontFaceNames, str );
    }

    /***********************************************************************************\

    Function:
        GetBlendFactor

    \***********************************************************************************/
    VkBlendFactor DeviceProfilerStringParser::GetBlendFactor( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanBlendFactorNames, str );
    }

    /***********************************************************************************\

    Function:
        GetBlendOp

    \***********************************************************************************/
    VkBlendOp DeviceProfilerStringParser::GetBlendOp( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanBlendOpNames, str );
    }

    /***********************************************************************************\

    Function:
        GetCompareOp

    \***********************************************************************************/
    VkCompareOp DeviceProfilerStringParser::GetCompareOp( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanCompareOpNames, str );
    }

    /***********************************************************************************\

    Function:
        GetLogicOp

    \***********************************************************************************/
    VkLogicOp DeviceProfilerStringParser::GetLogicOp( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanLogicOpNames, str );
    }

    /***********************************************************************************\

    Function:
        GetColorComponentFlags

    \***********************************************************************************/
    VkColorComponentFlags DeviceProfilerStringParser::GetColorComponentFlags( const std::string_view& str ) const
    {
        VkColorComponentFlags flags = 0;
        for( char c : str )
        {
            switch( c )
            {
            case 'R': flags |= VK_COLOR_COMPONENT_R_BIT; break;
            case 'G': flags |= VK_COLOR_COMPONENT_G_BIT; break;
            case 'B': flags |= VK_COLOR_COMPONENT_B_BIT; break;
            case 'A': flags |= VK_COLOR_COMPONENT_A_BIT; break;
            }
        }
        return flags;
    }

    /***********************************************************************************\

    Function:
        GetDynamicStateName

    \***********************************************************************************/
    VkDynamicState DeviceProfilerStringParser::GetDynamicState( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanDynamicStateNames, str );
    }

    /***********************************************************************************\

    Function:
        GetMemoryPropertyFlags

    \***********************************************************************************/
    VkMemoryPropertyFlags DeviceProfilerStringParser::GetMemoryPropertyFlags( const std::string_view& str, const char* separator ) const
    {
        return MapStringToFlags( g_scVulkanMemoryPropertyFlagNames, str, separator );
    }

    /***********************************************************************************\

    Function:
        GetBufferUsageFlags

    \***********************************************************************************/
    VkBufferUsageFlags DeviceProfilerStringParser::GetBufferUsageFlags( const std::string_view& str, const char* separator ) const
    {
        return MapStringToFlags( g_scVulkanBufferUsageFlagNames, str, separator );
    }

    /***********************************************************************************\

    Function:
        GetImageUsageFlags

    \***********************************************************************************/
    VkImageUsageFlags DeviceProfilerStringParser::GetImageUsageFlags( const std::string_view& str, const char* separator ) const
    {
        return MapStringToFlags( g_scVulkanImageUsageFlagNames, str, separator );
    }

    /***********************************************************************************\

    Function:
        GetImageType

    \***********************************************************************************/
    VkImageType DeviceProfilerStringParser::GetImageType( const std::string_view& str ) const
    {
        // Strip "Cube" and "Array" suffixes from the string to get the base image type name.
        std::string_view imageTypeName = str;

        if( imageTypeName.rfind( " Array" ) == imageTypeName.length() - 6 )
        {
            imageTypeName.remove_suffix( 6 );
        }

        if( imageTypeName.rfind( " Cube" ) == imageTypeName.length() - 5 )
        {
            imageTypeName.remove_suffix( 5 );
        }

        return MapStringToValue( g_scVulkanImageTypeNames, imageTypeName );
    }

    /***********************************************************************************\

    Function:
        GetImageTiling

    \***********************************************************************************/
    VkImageTiling DeviceProfilerStringParser::GetImageTiling( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanImageTilingNames, str );
    }

    /***********************************************************************************\

    Function:
        GetImageAspectFlags

    \***********************************************************************************/
    VkImageAspectFlags DeviceProfilerStringParser::GetImageAspectFlags( const std::string_view& str, const char* separator ) const
    {
        return MapStringToFlags( g_scVulkanImageAspectFlagNames, str, separator );
    }

    /***********************************************************************************\

    Function:
        GetCopyAccelerationStructureMode

    \***********************************************************************************/
    VkCopyAccelerationStructureModeKHR DeviceProfilerStringParser::GetCopyAccelerationStructureMode( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanCopyAccelerationStructureModeNames, str );
    }

    /***********************************************************************************\

    Function:
        GetAccelerationStructureType

    \***********************************************************************************/
    VkAccelerationStructureTypeKHR DeviceProfilerStringParser::GetAccelerationStructureType( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanAccelerationStructureTypeNames, str );
    }

    /***********************************************************************************\

    Function:
        GetAccelerationStructureTypeFlags

    \***********************************************************************************/
    VkProfilerAccelerationStructureTypeFlagsEXT DeviceProfilerStringParser::GetAccelerationStructureTypeFlags( const std::string_view& str, const char* separator ) const
    {
        return MapStringToFlags( g_scProfilerAccelerationStructureTypeFlagNames, str, separator );
    }

    /***********************************************************************************\

    Function:
        GetBuildAccelerationStructureFlagNames

    \***********************************************************************************/
    VkBuildAccelerationStructureFlagsKHR DeviceProfilerStringParser::GetBuildAccelerationStructureFlags( const std::string_view& str, const char* separator ) const
    {
        return MapStringToFlags( g_scVulkanBuildAccelerationStructureFlagNames, str, separator );
    }

    /***********************************************************************************\

    Function:
        GetBuildAccelerationStructureMode

    \***********************************************************************************/
    VkBuildAccelerationStructureModeKHR DeviceProfilerStringParser::GetBuildAccelerationStructureMode( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanBuildAccelerationStructureModeNames, str );
    }

    /***********************************************************************************\

    Function:
        GetCopyMicromapMode

    \***********************************************************************************/
    VkCopyMicromapModeEXT DeviceProfilerStringParser::GetCopyMicromapMode( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanCopyMicromapModeNames, str );
    }

    /***********************************************************************************\

    Function:
        GetMicromapType

    \***********************************************************************************/
    VkMicromapTypeEXT DeviceProfilerStringParser::GetMicromapType( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanMicromapTypeNames, str );
    }

    /***********************************************************************************\

    Function:
        GetMicromapTypeFlags

    \***********************************************************************************/
    VkFlags DeviceProfilerStringParser::GetMicromapTypeFlags( const std::string_view& str, const char* separator ) const
    {
        return MapStringToFlags( g_scProfilerMicromapTypeFlagNames, str, separator );
    }

    /***********************************************************************************\

    Function:
        GetBuildMicromapMode

    \***********************************************************************************/
    VkBuildMicromapModeEXT DeviceProfilerStringParser::GetBuildMicromapMode( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanBuildMicromapModeNames, str );
    }

    /***********************************************************************************\

    Function:
        GetBuildMicromapFlags

    \***********************************************************************************/
    VkBuildMicromapFlagsEXT DeviceProfilerStringParser::GetBuildMicromapFlags( const std::string_view& str, const char* separator ) const
    {
        return MapStringToFlags( g_scVulkanBuildMicromapFlagNames, str, separator );
    }

    /***********************************************************************************\

    Function:
        GetGeometryType

    \***********************************************************************************/
    VkGeometryTypeKHR DeviceProfilerStringParser::GetGeometryType( const std::string_view& str ) const
    {
        return MapStringToValue( g_scVulkanGeometryTypeNames, str );
    }

    /***********************************************************************************\

    Function:
        GetGeometryFlags

    \***********************************************************************************/
    VkGeometryFlagsKHR DeviceProfilerStringParser::GetGeometryFlags( const std::string_view& str, const char* separator ) const
    {
        return MapStringToFlags( g_scVulkanGeometryFlagNames, str, separator );
    }
}
