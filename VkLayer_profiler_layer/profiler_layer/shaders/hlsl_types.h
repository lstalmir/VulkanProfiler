#pragma once
#ifndef INCLUDED_FROM_HLSL
#include <stdint.h>

namespace Profiler
{
    using _hlsl_float = float;
    using _hlsl_double = double;
    using _hlsl_int = int32_t;
    using _hlsl_uint = uint32_t;
    using _hlsl_bool = int32_t;

    using uint = _hlsl_uint;

    template<typename ComponentType, int N>
    struct alignas(4) vector;

    template<typename ComponentType>
    struct alignas(4) vector<ComponentType, 1>
    {
        ComponentType x;
    };

    template<typename ComponentType>
    struct alignas(4) vector<ComponentType, 2>
        : vector<ComponentType, 1>
    {
        ComponentType y;
    };

    template<typename ComponentType>
    struct alignas(4) vector<ComponentType, 3>
        : vector<ComponentType, 2>
    {
        ComponentType z;
    };
    
    template<typename ComponentType>
    struct alignas(4) vector<ComponentType, 4>
        : vector<ComponentType, 3>
    {
        ComponentType w;
    };

    using float1 = vector<_hlsl_float, 1>;
    using float2 = vector<_hlsl_float, 2>;
    using float3 = vector<_hlsl_float, 3>;
    using float4 = vector<_hlsl_float, 4>;

    using double1 = vector<_hlsl_double, 1>;
    using double2 = vector<_hlsl_double, 2>;
    using double3 = vector<_hlsl_double, 3>;
    using double4 = vector<_hlsl_double, 4>;

    using int1 = vector<_hlsl_int, 1>;
    using int2 = vector<_hlsl_int, 2>;
    using int3 = vector<_hlsl_int, 3>;
    using int4 = vector<_hlsl_int, 4>;

    using uint1 = vector<_hlsl_uint, 1>;
    using uint2 = vector<_hlsl_uint, 2>;
    using uint3 = vector<_hlsl_uint, 3>;
    using uint4 = vector<_hlsl_uint, 4>;

    using bool1 = vector<_hlsl_bool, 1>;
    using bool2 = vector<_hlsl_bool, 2>;
    using bool3 = vector<_hlsl_bool, 3>;
    using bool4 = vector<_hlsl_bool, 4>;

    template<typename ComponentType, int N, int M>
    struct alignas(4) matrix
    {
        ComponentType r[M][N];
    };

    using float1x1 = matrix<_hlsl_float, 1, 1>;
    using float2x1 = matrix<_hlsl_float, 2, 1>;
    using float3x1 = matrix<_hlsl_float, 3, 1>;
    using float4x1 = matrix<_hlsl_float, 4, 1>;
    using float1x2 = matrix<_hlsl_float, 1, 2>;
    using float2x2 = matrix<_hlsl_float, 2, 2>;
    using float3x2 = matrix<_hlsl_float, 3, 2>;
    using float4x2 = matrix<_hlsl_float, 4, 2>;
    using float1x3 = matrix<_hlsl_float, 1, 3>;
    using float2x3 = matrix<_hlsl_float, 2, 3>;
    using float3x3 = matrix<_hlsl_float, 3, 3>;
    using float4x3 = matrix<_hlsl_float, 4, 3>;
    using float1x4 = matrix<_hlsl_float, 1, 4>;
    using float2x4 = matrix<_hlsl_float, 2, 4>;
    using float3x4 = matrix<_hlsl_float, 3, 4>;
    using float4x4 = matrix<_hlsl_float, 4, 4>;

    using double1x1 = matrix<_hlsl_double, 1, 1>;
    using double2x1 = matrix<_hlsl_double, 2, 1>;
    using double3x1 = matrix<_hlsl_double, 3, 1>;
    using double4x1 = matrix<_hlsl_double, 4, 1>;
    using double1x2 = matrix<_hlsl_double, 1, 2>;
    using double2x2 = matrix<_hlsl_double, 2, 2>;
    using double3x2 = matrix<_hlsl_double, 3, 2>;
    using double4x2 = matrix<_hlsl_double, 4, 2>;
    using double1x3 = matrix<_hlsl_double, 1, 3>;
    using double2x3 = matrix<_hlsl_double, 2, 3>;
    using double3x3 = matrix<_hlsl_double, 3, 3>;
    using double4x3 = matrix<_hlsl_double, 4, 3>;
    using double1x4 = matrix<_hlsl_double, 1, 4>;
    using double2x4 = matrix<_hlsl_double, 2, 4>;
    using double3x4 = matrix<_hlsl_double, 3, 4>;
    using double4x4 = matrix<_hlsl_double, 4, 4>;

    using int1x1 = matrix<_hlsl_int, 1, 1>;
    using int2x1 = matrix<_hlsl_int, 2, 1>;
    using int3x1 = matrix<_hlsl_int, 3, 1>;
    using int4x1 = matrix<_hlsl_int, 4, 1>;
    using int1x2 = matrix<_hlsl_int, 1, 2>;
    using int2x2 = matrix<_hlsl_int, 2, 2>;
    using int3x2 = matrix<_hlsl_int, 3, 2>;
    using int4x2 = matrix<_hlsl_int, 4, 2>;
    using int1x3 = matrix<_hlsl_int, 1, 3>;
    using int2x3 = matrix<_hlsl_int, 2, 3>;
    using int3x3 = matrix<_hlsl_int, 3, 3>;
    using int4x3 = matrix<_hlsl_int, 4, 3>;
    using int1x4 = matrix<_hlsl_int, 1, 4>;
    using int2x4 = matrix<_hlsl_int, 2, 4>;
    using int3x4 = matrix<_hlsl_int, 3, 4>;
    using int4x4 = matrix<_hlsl_int, 4, 4>;

    using uint1x1 = matrix<_hlsl_uint, 1, 1>;
    using uint2x1 = matrix<_hlsl_uint, 2, 1>;
    using uint3x1 = matrix<_hlsl_uint, 3, 1>;
    using uint4x1 = matrix<_hlsl_uint, 4, 1>;
    using uint1x2 = matrix<_hlsl_uint, 1, 2>;
    using uint2x2 = matrix<_hlsl_uint, 2, 2>;
    using uint3x2 = matrix<_hlsl_uint, 3, 2>;
    using uint4x2 = matrix<_hlsl_uint, 4, 2>;
    using uint1x3 = matrix<_hlsl_uint, 1, 3>;
    using uint2x3 = matrix<_hlsl_uint, 2, 3>;
    using uint3x3 = matrix<_hlsl_uint, 3, 3>;
    using uint4x3 = matrix<_hlsl_uint, 4, 3>;
    using uint1x4 = matrix<_hlsl_uint, 1, 4>;
    using uint2x4 = matrix<_hlsl_uint, 2, 4>;
    using uint3x4 = matrix<_hlsl_uint, 3, 4>;
    using uint4x4 = matrix<_hlsl_uint, 4, 4>;

    using bool1x1 = matrix<_hlsl_bool, 1, 1>;
    using bool2x1 = matrix<_hlsl_bool, 2, 1>;
    using bool3x1 = matrix<_hlsl_bool, 3, 1>;
    using bool4x1 = matrix<_hlsl_bool, 4, 1>;
    using bool1x2 = matrix<_hlsl_bool, 1, 2>;
    using bool2x2 = matrix<_hlsl_bool, 2, 2>;
    using bool3x2 = matrix<_hlsl_bool, 3, 2>;
    using bool4x2 = matrix<_hlsl_bool, 4, 2>;
    using bool1x3 = matrix<_hlsl_bool, 1, 3>;
    using bool2x3 = matrix<_hlsl_bool, 2, 3>;
    using bool3x3 = matrix<_hlsl_bool, 3, 3>;
    using bool4x3 = matrix<_hlsl_bool, 4, 3>;
    using bool1x4 = matrix<_hlsl_bool, 1, 4>;
    using bool2x4 = matrix<_hlsl_bool, 2, 4>;
    using bool3x4 = matrix<_hlsl_bool, 3, 4>;
    using bool4x4 = matrix<_hlsl_bool, 4, 4>;
}

#endif
