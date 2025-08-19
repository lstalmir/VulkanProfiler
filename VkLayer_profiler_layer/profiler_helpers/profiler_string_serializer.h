// Copyright (c) 2019-2025 Lukasz Stalmirski
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
#include <string>

namespace Profiler
{
    class DeviceProfilerFrontend;

    /***********************************************************************************\

    Class:
        DeviceProfilerStringSerializer

    Description:
        Serializes structures into human-readable strings.

    \***********************************************************************************/
    class DeviceProfilerStringSerializer
    {
    public:
        inline static const char* DefaultFlagsSeparator = " | ";

        DeviceProfilerStringSerializer( DeviceProfilerFrontend& );

        std::string GetName( const struct DeviceProfilerDrawcall& ) const;
        std::string GetName( const struct DeviceProfilerPipelineData& ) const;
        std::string GetName( const struct DeviceProfilerSubpassData& ) const;
        std::string GetName( const struct DeviceProfilerRenderPassData& ) const;
        std::string GetName( const struct DeviceProfilerRenderPassBeginData&, bool dynamic ) const;
        std::string GetName( const struct DeviceProfilerRenderPassEndData&, bool dynamic ) const;
        std::string GetName( const struct DeviceProfilerCommandBufferData& ) const;

        std::string GetName( const struct VkObject& object ) const;
        std::string GetObjectID( const struct VkObject& object ) const;
        std::string GetObjectTypeName( const VkObjectType objectType ) const;
        std::string GetShortObjectTypeName( const VkObjectType objectType ) const;

        std::string GetCommandName( const struct DeviceProfilerDrawcall& ) const;

        std::string GetPointer( const void* ) const;
        std::string GetBool( VkBool32 ) const;
        std::string GetVec4( const float* ) const;

        std::string GetColorHex( const float* ) const;

        std::string GetByteSize( VkDeviceSize ) const;

        std::string GetQueueTypeName( VkQueueFlags ) const;
        std::string GetQueueFlagNames( VkQueueFlags ) const;

        std::string GetShaderName( const struct ProfilerShader& ) const;
        std::string GetShortShaderName( const struct ProfilerShader& ) const;
        std::string GetShaderStageName( VkShaderStageFlagBits ) const;
        std::string GetShortShaderStageName( VkShaderStageFlagBits ) const;
        std::string GetShaderGroupTypeName( VkRayTracingShaderGroupTypeKHR ) const;
        std::string GetGeneralShaderGroupTypeName( VkShaderStageFlagBits ) const;

        std::string GetFormatName( VkFormat ) const;
        std::string GetIndexTypeName( VkIndexType ) const;
        std::string GetVertexInputRateName( VkVertexInputRate ) const;
        std::string GetPrimitiveTopologyName( VkPrimitiveTopology ) const;
        std::string GetPolygonModeName( VkPolygonMode ) const;
        std::string GetCullModeName( VkCullModeFlags ) const;
        std::string GetFrontFaceName( VkFrontFace ) const;
        std::string GetBlendFactorName( VkBlendFactor ) const;
        std::string GetBlendOpName( VkBlendOp ) const;
        std::string GetCompareOpName( VkCompareOp ) const;
        std::string GetLogicOpName( VkLogicOp ) const;
        std::string GetColorComponentFlagNames( VkColorComponentFlags ) const;

        std::string GetMemoryPropertyFlagNames( VkMemoryPropertyFlags, const char* separator = DefaultFlagsSeparator ) const;
        std::string GetBufferUsageFlagNames( VkBufferUsageFlags, const char* separator = DefaultFlagsSeparator ) const;
        std::string GetImageUsageFlagNames( VkImageUsageFlags, const char* separator = DefaultFlagsSeparator ) const;

        std::string GetImageTypeName( VkImageType, VkImageCreateFlags = 0, uint32_t = 1 ) const;
        std::string GetImageTilingName( VkImageTiling ) const;
        std::string GetImageAspectFlagNames( VkImageAspectFlags, const char* separator = DefaultFlagsSeparator ) const;

        std::string GetCopyAccelerationStructureModeName( VkCopyAccelerationStructureModeKHR mode ) const;
        std::string GetAccelerationStructureTypeName( VkAccelerationStructureTypeKHR type ) const;
        std::string GetAccelerationStructureTypeFlagNames( VkFlags flags, const char* separator = DefaultFlagsSeparator ) const;
        std::string GetBuildAccelerationStructureFlagNames( VkBuildAccelerationStructureFlagsKHR flags ) const;
        std::string GetBuildAccelerationStructureModeName( VkBuildAccelerationStructureModeKHR mode ) const;

        std::string GetCopyMicromapModeName( VkCopyMicromapModeEXT mode ) const;
        std::string GetMicromapTypeName( VkMicromapTypeEXT type ) const;
        std::string GetBuildMicromapModeName( VkBuildMicromapModeEXT mode ) const;
        std::string GetBuildMicromapFlagNames( VkBuildMicromapFlagsEXT flags ) const;

        std::string GetGeometryTypeName( VkGeometryTypeKHR ) const;
        std::string GetGeometryFlagNames( VkGeometryFlagsKHR ) const;

    private:
        DeviceProfilerFrontend& m_Frontend;
    };
}
