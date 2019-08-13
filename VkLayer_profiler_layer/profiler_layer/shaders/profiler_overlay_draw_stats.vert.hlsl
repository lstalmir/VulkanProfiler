#define INCLUDED_FROM_HLSL

// Import input/output structures
#include "profiler_overlay_draw_stats_input.h"

VertexShaderOutput main( VertexShaderInput input )
{
    VertexShaderOutput output;
    output.position = float4(input.position, 0, 1);
    output.texcoord = input.texcoord;
    return output;
}
