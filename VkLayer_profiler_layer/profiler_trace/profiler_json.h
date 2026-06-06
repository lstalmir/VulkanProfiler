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

#include <stdio.h>

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

        void WriteCommandArgs( FILE*, const struct DeviceProfilerDrawcall& ) const;
        void WritePipelineArgs( FILE*, const struct DeviceProfilerPipeline& ) const;

    private:
        const class DeviceProfilerStringSerializer* m_pStringSerializer;

        void WriteColorClearValue( FILE*, const VkClearColorValue& ) const;
        void WriteDepthStencilClearValue( FILE*, const VkClearDepthStencilValue& ) const;
        void WriteShaderStageArgs( FILE*, const struct ProfilerShader& ) const;
        void WriteGraphicsPipelineCreateInfoArgs( FILE*, const VkGraphicsPipelineCreateInfo& ) const;
        void WriteComputePipelineCreateInfoArgs( FILE*, const VkComputePipelineCreateInfo& ) const;
        void WriteRayTracingPipelineCreateInfoArgs( FILE*, const VkRayTracingPipelineCreateInfoKHR& ) const;
    };
}
