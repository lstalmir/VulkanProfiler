#pragma once
#include "profiler_shader.h"
#include <vulkan.h>

namespace Profiler
{
    struct ProfilerPipeline
    {
        VkPipeline m_Pipeline;
        ProfilerShaderTuple m_ShaderTuple;
    };
}
