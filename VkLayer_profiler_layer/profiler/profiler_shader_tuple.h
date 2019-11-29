#pragma once
#include <vector>

namespace Profiler
{
    struct ProfilerShaderTuple
    {
        uint64_t VertexShaderHash;
        uint64_t TesselationControlShaderHash;
        uint64_t TesselationEvaluationShaderHash;
        uint64_t GeometryShaderHash;
        uint64_t FragmentShaderHash;
        uint64_t ComputeShaderHash;

        std::vector<uint32_t> QueryIndices;
    };
}
