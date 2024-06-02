// Copyright (c) 2024 Lukasz Stalmirski
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
#include "VkDevice_functions_base.h"

namespace Profiler
{
    struct VkPipelineExecutablePropertiesKhr_Functions : VkDevice_Functions_Base
    {
        /***********************************************************************************\

        Function:
            CapturePipelineExecutableProperties

        Description:

        \***********************************************************************************/
        template<typename VkPipelineCreateInfoT>
        static void CapturePipelineExecutableProperties(
            Dispatch& dd,
            uint32_t createInfoCount,
            const VkPipelineCreateInfoT** ppCreateInfos,
            VkPipelineCreateInfoT** ppCreateInfosWithExecutableProperties )
        {
            if( dd.Profiler.ShouldCapturePipelineExecutableProperties() )
            {
                const size_t createInfoSize = createInfoCount * sizeof( VkPipelineCreateInfoT );

                // To capture the properties and internal representations, flags must be passed to the ICD.
                // Create a copy of pCreateInfos list and add these flags.
                *ppCreateInfosWithExecutableProperties =
                    reinterpret_cast<VkPipelineCreateInfoT*>(malloc( createInfoSize ));

                if( *ppCreateInfosWithExecutableProperties != nullptr )
                {
                    memcpy( *ppCreateInfosWithExecutableProperties, *ppCreateInfos, createInfoSize );

                    for( uint32_t i = 0; i < createInfoCount; ++i )
                    {
                        (*ppCreateInfosWithExecutableProperties)[ i ].flags |=
                            VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR |
                            VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR;
                    }

                    *ppCreateInfos = *ppCreateInfosWithExecutableProperties;
                }
            }
        }
    };
}
