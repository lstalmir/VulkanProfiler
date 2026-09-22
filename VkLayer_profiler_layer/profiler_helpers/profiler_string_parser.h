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
#include <vulkan/vulkan.h>

#include <array>
#include <string>
#include <string_view>

#include "profiler/profiler_data.h"
#include "profiler_ext/VkProfilerCustomFlagsEXT.h"

namespace Profiler
{
    class DeviceProfilerFrontend;

    /***********************************************************************************\

    Class:
        DeviceProfilerStringParser

    Description:
        Parses human-readable strings into structures and enum values.

    \***********************************************************************************/
    class DeviceProfilerStringParser
    {
    public:
        inline static const char* DefaultFlagsSeparator = " | ";

        DeviceProfilerStringParser( DeviceProfilerFrontend& frontend );

        DeviceProfilerDrawcallType GetCommandType( const std::string_view& str ) const;

        const void* GetPointer( const std::string_view& str ) const;
        VkBool32 GetBool( const std::string_view& str ) const;
        std::array<float, 4> GetVec4( const std::string_view& str ) const;
        uint32_t GetVersion( const std::string_view& str ) const;

        std::array<float, 3> GetColorHex( const std::string_view& str ) const;

        VkDeviceSize GetByteSize( const std::string_view& str ) const;

        VkPhysicalDeviceType GetDeviceType( const std::string_view& str ) const;

        VkQueueFlags GetQueueFlags( const std::string_view& str, const char* separator = DefaultFlagsSeparator ) const;

        VkShaderStageFlagBits GetShaderStage( const std::string_view& str ) const;
        VkShaderStageFlagBits GetShortShaderStage( const std::string_view& str ) const;
        VkRayTracingShaderGroupTypeKHR GetShaderGroupType( const std::string_view& str ) const;
        VkShaderStageFlagBits GetGeneralShaderGroupType( const std::string_view& str ) const;

        VkFormat GetFormat( const std::string_view& str ) const;
        VkIndexType GetIndexType( const std::string_view& str ) const;
        VkVertexInputRate GetVertexInputRate( const std::string_view& str ) const;
        VkPrimitiveTopology GetPrimitiveTopology( const std::string_view& str ) const;
        VkPolygonMode GetPolygonMode( const std::string_view& str ) const;
        VkCullModeFlags GetCullMode( const std::string_view& str ) const;
        VkFrontFace GetFrontFace( const std::string_view& str ) const;
        VkBlendFactor GetBlendFactor( const std::string_view& str ) const;
        VkBlendOp GetBlendOp( const std::string_view& str ) const;
        VkCompareOp GetCompareOp( const std::string_view& str ) const;
        VkLogicOp GetLogicOp( const std::string_view& str ) const;
        VkColorComponentFlags GetColorComponentFlags( const std::string_view& str ) const;
        VkDynamicState GetDynamicState( const std::string_view& str ) const;

        VkMemoryPropertyFlags GetMemoryPropertyFlags( const std::string_view& str, const char* separator = DefaultFlagsSeparator ) const;
        VkBufferUsageFlags GetBufferUsageFlags( const std::string_view& str, const char* separator = DefaultFlagsSeparator ) const;
        VkImageUsageFlags GetImageUsageFlags( const std::string_view& str, const char* separator = DefaultFlagsSeparator ) const;

        VkImageType GetImageType( const std::string_view& str ) const;
        VkImageTiling GetImageTiling( const std::string_view& str ) const;
        VkImageAspectFlags GetImageAspectFlags( const std::string_view& str, const char* separator = DefaultFlagsSeparator ) const;

        VkCopyAccelerationStructureModeKHR GetCopyAccelerationStructureMode( const std::string_view& str ) const;
        VkAccelerationStructureTypeKHR GetAccelerationStructureType( const std::string_view& str ) const;
        VkProfilerAccelerationStructureTypeFlagsEXT GetAccelerationStructureTypeFlags( const std::string_view& str, const char* separator = DefaultFlagsSeparator ) const;
        VkBuildAccelerationStructureFlagsKHR GetBuildAccelerationStructureFlags( const std::string_view& str, const char* separator = DefaultFlagsSeparator ) const;
        VkBuildAccelerationStructureModeKHR GetBuildAccelerationStructureMode( const std::string_view& str ) const;

        VkCopyMicromapModeEXT GetCopyMicromapMode( const std::string_view& str ) const;
        VkMicromapTypeEXT GetMicromapType( const std::string_view& str ) const;
        VkFlags GetMicromapTypeFlags( const std::string_view& str, const char* separator = DefaultFlagsSeparator ) const;
        VkBuildMicromapModeEXT GetBuildMicromapMode( const std::string_view& str ) const;
        VkBuildMicromapFlagsEXT GetBuildMicromapFlags( const std::string_view& str, const char* separator = DefaultFlagsSeparator ) const;

        VkGeometryTypeKHR GetGeometryType( const std::string_view& str ) const;
        VkGeometryFlagsKHR GetGeometryFlags( const std::string_view& str, const char* separator = DefaultFlagsSeparator ) const;

    private:
        DeviceProfilerFrontend& m_Frontend;
    };
}
