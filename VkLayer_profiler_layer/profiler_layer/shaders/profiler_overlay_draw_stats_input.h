#pragma once
#include "hlsl_defines.h"
#include "hlsl_types.h"
#ifndef INCLUDED_FROM_HLSL
#include <vulkan/vulkan.h>
#endif

#ifndef INCLUDED_FROM_HLSL
namespace Profiler
{
#endif

    /***********************************************************************************\

    Structure:
        VertexShaderInput

    Description:
        Set of vertex attributes passed to the VertexShader stage by the application.

    \***********************************************************************************/
    struct VertexShaderInput
    {
        float2 position SEMANTIC( POSITION );
        float2 texcoord SEMANTIC( TEXCOORD );

#ifndef INCLUDED_FROM_HLSL

        // Input vertex attributes description
        inline static const VkVertexInputAttributeDescription VertexInputAttributes[2] =
        {
            { 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 },   // position
            { 1, 0, VK_FORMAT_R32G32_SFLOAT, 8 }    // texcoord
        };

        // Input vertex binding description
        inline static const VkVertexInputBindingDescription VertexInputBindings[1] =
        {
            { 0, 16, VK_VERTEX_INPUT_RATE_VERTEX }
        };

        // Number of vertex input attributes
        inline static const uint32_t VertexInputAttributeCount =
            std::extent<decltype(VertexInputAttributes)>::value;

        // Number of vertex input bindings
        inline static const uint32_t VertexInputBindingCount =
            std::extent<decltype(VertexInputBindings)>::value;

#endif // INCLUDED_FROM_HLSL
    };

    /***********************************************************************************\

    Structure:
        VertexShaderOutput

    Description:
        Set of vertex attributes passed from VertexShader to the further stages.

    \***********************************************************************************/
    struct VertexShaderOutput
    {
        float4 position SEMANTIC( SV_Position );
        float2 texcoord SEMANTIC( TEXCOORD );
    };

    typedef VertexShaderOutput GeometryShaderInput;
    typedef VertexShaderOutput PixelShaderInput;

#ifndef INCLUDED_FROM_HLSL
} // namespace Profiler
#endif
