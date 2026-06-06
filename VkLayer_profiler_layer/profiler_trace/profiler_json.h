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

namespace Profiler
{
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

        void WriteCommandArgs( simdjson::builder::string_builder&, const struct DeviceProfilerDrawcall& ) const;
        void WritePipelineArgs( simdjson::builder::string_builder&, const struct DeviceProfilerPipeline& ) const;

    private:
        const class DeviceProfilerStringSerializer* m_pStringSerializer;

        void WriteColorClearValue( simdjson::builder::string_builder&, const VkClearColorValue& ) const;
        void WriteDepthStencilClearValue( simdjson::builder::string_builder&, const VkClearDepthStencilValue& ) const;
        void WriteShaderStageArgs( simdjson::builder::string_builder&, const struct ProfilerShader& ) const;
        void WriteGraphicsPipelineCreateInfoArgs( simdjson::builder::string_builder&, const VkGraphicsPipelineCreateInfo&, bool ) const;
        void WriteComputePipelineCreateInfoArgs( simdjson::builder::string_builder&, const VkComputePipelineCreateInfo&, bool ) const;
        void WriteRayTracingPipelineCreateInfoArgs( simdjson::builder::string_builder&, const VkRayTracingPipelineCreateInfoKHR&, bool ) const;
    };
}
