// Copyright (c) 2019-2021 Lukasz Stalmirski
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
#include <nlohmann/json.hpp>

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

        nlohmann::json GetCommandArgs( const struct DeviceProfilerDrawcall& ) const;

    private:
        const class DeviceProfilerStringSerializer* m_pStringSerializer;

        nlohmann::json GetColorClearValue( const VkClearColorValue& ) const;
        nlohmann::json GetDepthStencilClearValue( const VkClearDepthStencilValue& ) const;
    };
}
