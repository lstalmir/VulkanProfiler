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
#include <simdjson.h>
#include <string_view>

namespace Profiler
{
    /*************************************************************************\

    Class:
        DeviceProfilerJsonBuilder

    Description:
        Helper class to build JSON objects.
        It provides a higher-level interface to simdjson builder.

    \*************************************************************************/
    class DeviceProfilerJsonBuilder
    {
    public:
        DeviceProfilerJsonBuilder();

        void Reset();

        void AddValue( std::string_view value );
        void AddValue( uint64_t value );
        void AddValue( int64_t value );
        void AddValue( uint32_t value );
        void AddValue( int32_t value );
        void AddValue( double value );
        void AddValue( bool value );

        void AddKeyValue( std::string_view key, std::string_view value );
        void AddKeyValue( std::string_view key, uint64_t value );
        void AddKeyValue( std::string_view key, int64_t value );
        void AddKeyValue( std::string_view key, uint32_t value );
        void AddKeyValue( std::string_view key, int32_t value );
        void AddKeyValue( std::string_view key, double value );
        void AddKeyValue( std::string_view key, bool value );

        void BeginKeyValue( std::string_view key );
        void EndKeyValue();

        void BeginArray( std::string_view name = std::string_view() );
        bool BeginArray( std::string_view name, const void* pSourceArray );
        void EndArray();

        void BeginObject( std::string_view name = std::string_view() );
        bool BeginObject( std::string_view name, const void* pSourceObject );
        void EndObject();

        void EndScope();

        simdjson::builder::string_builder& GetStringBuilder();

    private:
        simdjson::builder::string_builder m_Builder;

        bool m_FirstElementInScope;
    };

    /*************************************************************************\

    Class:
        DeviceProfilerJsonSerializer

    Description:
        Serializes data into JSON objects.

    \*************************************************************************/
    class DeviceProfilerJsonSerializer
    {
    public:
        DeviceProfilerJsonSerializer( const class DeviceProfilerStringSerializer* );

        void WriteCommandArgs( DeviceProfilerJsonBuilder&, const struct DeviceProfilerDrawcall& ) const;
        void WritePipelineArgs( DeviceProfilerJsonBuilder&, const struct DeviceProfilerPipeline& ) const;

    private:
        const class DeviceProfilerStringSerializer* m_pStringSerializer;

        void WriteColorClearValue( DeviceProfilerJsonBuilder&, const VkClearColorValue& ) const;
        void WriteDepthStencilClearValue( DeviceProfilerJsonBuilder&, const VkClearDepthStencilValue& ) const;
        void WriteShaderStageArgs( DeviceProfilerJsonBuilder&, const struct ProfilerShader& ) const;
        void WriteGraphicsPipelineCreateInfoArgs( DeviceProfilerJsonBuilder&, const VkGraphicsPipelineCreateInfo& ) const;
        void WriteComputePipelineCreateInfoArgs( DeviceProfilerJsonBuilder&, const VkComputePipelineCreateInfo& ) const;
        void WriteRayTracingPipelineCreateInfoArgs( DeviceProfilerJsonBuilder&, const VkRayTracingPipelineCreateInfoKHR& ) const;
    };
}
