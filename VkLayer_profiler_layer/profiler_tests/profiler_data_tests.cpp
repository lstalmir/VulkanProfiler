// Copyright (c) 2026 Lukasz Stalmirski
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

#include "profiler_testing_common.h"

#include "profiler/profiler_data.h"

template<typename T>
void ExpectStructureEqual( const T& expected, const T& actual )
{
    ASSERT_NE( &expected, &actual );
    EXPECT_EQ( 0, memcmp( &expected, &actual, sizeof( T ) ) );
}

template<typename T>
void ExpectPNextChainEqual(
    const T* pExpected,
    const T* pActual )
{
    for( const auto& structure : Profiler::PNextIterator( pExpected ) )
    {
        switch( structure.sType )
        {
        case VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_MICROMAP_DATA_KHR:
        {
            const auto* pExpectedStructure = reinterpret_cast<const VkAccelerationStructureGeometryMicromapDataKHR*>( &structure );
            const auto* pActualStructure =
                Profiler::PNextChain( pActual )
                    .Find<VkAccelerationStructureGeometryMicromapDataKHR>( VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_MICROMAP_DATA_KHR );

            ASSERT_NE( nullptr, pActualStructure );
            ExpectStructureEqual( *pExpectedStructure, *pActualStructure );
            break;
        }
        default:
        {
            break;
        }
        }
    }
}

template<>
void ExpectStructureEqual(
    const VkAccelerationStructureGeometryMicromapDataKHR& expected,
    const VkAccelerationStructureGeometryMicromapDataKHR& actual )
{
    ASSERT_NE( &expected, &actual );
    EXPECT_EQ( expected.sType, actual.sType );

    EXPECT_EQ( expected.usageCountsCount, actual.usageCountsCount );
    EXPECT_EQ( expected.data, actual.data );
    EXPECT_EQ( expected.triangleArray, actual.triangleArray );
    EXPECT_EQ( expected.triangleArrayStride, actual.triangleArrayStride );

    if( expected.pUsageCounts != nullptr )
    {
        ASSERT_NE( expected.pUsageCounts, actual.pUsageCounts );
        ASSERT_EQ( nullptr, actual.ppUsageCounts );

        for( uint32_t i = 0; i < expected.usageCountsCount; ++i )
        {
            ExpectStructureEqual( expected.pUsageCounts[i], actual.pUsageCounts[i] );
        }
    }

    if( expected.ppUsageCounts != nullptr )
    {
        ASSERT_EQ( nullptr, actual.pUsageCounts );
        ASSERT_NE( expected.ppUsageCounts, actual.ppUsageCounts );

        for( uint32_t i = 0; i < expected.usageCountsCount; ++i )
        {
            ExpectStructureEqual( *expected.ppUsageCounts[i], *actual.ppUsageCounts[i] );
        }
    }
}

template<>
void ExpectStructureEqual(
    const VkAccelerationStructureGeometryKHR& expected,
    const VkAccelerationStructureGeometryKHR& actual )
{
    ASSERT_NE( &expected, &actual );
    EXPECT_EQ( expected.sType, actual.sType );
    ExpectPNextChainEqual( expected.pNext, actual.pNext );

    EXPECT_EQ( expected.flags, actual.flags );
    EXPECT_EQ( expected.geometryType, actual.geometryType );

    switch( expected.geometryType )
    {
    case VK_GEOMETRY_TYPE_TRIANGLES_KHR:
        EXPECT_EQ( expected.geometry.triangles.sType, actual.geometry.triangles.sType );
        EXPECT_EQ( expected.geometry.triangles.vertexFormat, actual.geometry.triangles.vertexFormat );
        EXPECT_EQ( expected.geometry.triangles.vertexData.deviceAddress, actual.geometry.triangles.vertexData.deviceAddress );
        EXPECT_EQ( expected.geometry.triangles.vertexStride, actual.geometry.triangles.vertexStride );
        EXPECT_EQ( expected.geometry.triangles.maxVertex, actual.geometry.triangles.maxVertex );
        EXPECT_EQ( expected.geometry.triangles.indexType, actual.geometry.triangles.indexType );
        EXPECT_EQ( expected.geometry.triangles.indexData.deviceAddress, actual.geometry.triangles.indexData.deviceAddress );
        EXPECT_EQ( expected.geometry.triangles.transformData.deviceAddress, actual.geometry.triangles.transformData.deviceAddress );
        break;

    case VK_GEOMETRY_TYPE_AABBS_KHR:
        EXPECT_EQ( expected.geometry.aabbs.sType, actual.geometry.aabbs.sType );
        EXPECT_EQ( expected.geometry.aabbs.data.deviceAddress, actual.geometry.aabbs.data.deviceAddress );
        EXPECT_EQ( expected.geometry.aabbs.stride, actual.geometry.aabbs.stride );
        break;

    case VK_GEOMETRY_TYPE_INSTANCES_KHR:
        EXPECT_EQ( expected.geometry.instances.sType, actual.geometry.instances.sType );
        EXPECT_EQ( expected.geometry.instances.arrayOfPointers, actual.geometry.instances.arrayOfPointers );
        EXPECT_EQ( expected.geometry.instances.data.deviceAddress, actual.geometry.instances.data.deviceAddress );
        break;
    }
}

template<>
void ExpectStructureEqual(
    const VkAccelerationStructureBuildGeometryInfoKHR& expected,
    const VkAccelerationStructureBuildGeometryInfoKHR& actual )
{
    ASSERT_NE( &expected, &actual );
    EXPECT_EQ( expected.sType, actual.sType );
    ExpectPNextChainEqual( expected.pNext, actual.pNext );

    EXPECT_EQ( expected.type, actual.type );
    EXPECT_EQ( expected.flags, actual.flags );
    EXPECT_EQ( expected.mode, actual.mode );
    EXPECT_EQ( expected.srcAccelerationStructure, actual.srcAccelerationStructure );
    EXPECT_EQ( expected.dstAccelerationStructure, actual.dstAccelerationStructure );
    EXPECT_EQ( expected.geometryCount, actual.geometryCount );
    EXPECT_EQ( expected.scratchData.deviceAddress, actual.scratchData.deviceAddress );

    if( expected.pGeometries != nullptr )
    {
        ASSERT_NE( expected.pGeometries, actual.pGeometries );
        ASSERT_EQ( nullptr, actual.ppGeometries );

        for( uint32_t i = 0; i < expected.geometryCount; ++i )
        {
            ExpectStructureEqual( expected.pGeometries[i], actual.pGeometries[i] );
        }
    }

    if( expected.ppGeometries != nullptr )
    {
        ASSERT_EQ( nullptr, actual.pGeometries );
        ASSERT_NE( expected.ppGeometries, actual.ppGeometries );

        for( uint32_t i = 0; i < expected.geometryCount; ++i )
        {
            ExpectStructureEqual( *expected.ppGeometries[i], *actual.ppGeometries[i] );
        }
    }
}

namespace Profiler
{
    TEST( ProfilerDataULT, CopyAccelerationStructureBuildInfos )
    {
        VkAccelerationStructureGeometryKHR geometry = {};
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        geometry.geometry.instances.data.deviceAddress = 0xDEADBEEF;

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {};
        buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.dstAccelerationStructure = VkObjectTraits<VkAccelerationStructureKHR>::GetObjectHandleAsVulkanHandle( 0x12345678 );
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geometry;

        VkAccelerationStructureBuildRangeInfoKHR rangeInfo = {};
        rangeInfo.primitiveCount = 1;
        rangeInfo.primitiveOffset = 256;
        rangeInfo.transformOffset = 1024;

        VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;

        DeviceProfilerDrawcallBuildAccelerationStructuresPayload payload = {};
        payload.m_InfoCount = 1;
        payload.m_pInfos = &buildInfo;
        payload.m_ppRanges = &pRangeInfo;

        DeviceProfilerDrawcallBuildAccelerationStructuresPayload copiedPayload = {};
        copiedPayload.CopyDynamicAllocations( payload );

        ASSERT_NE( nullptr, copiedPayload.m_pInfos );
        ExpectStructureEqual( *payload.m_pInfos, *copiedPayload.m_pInfos );

        ASSERT_NE( nullptr, copiedPayload.m_ppRanges );
        for( uint32_t i = 0; i < payload.m_InfoCount; ++i )
        {
            const uint32_t geometryCount = payload.m_pInfos[i].geometryCount;
            for( uint32_t j = 0; j < geometryCount; ++j )
            {
                ExpectStructureEqual( payload.m_ppRanges[i][j], copiedPayload.m_ppRanges[i][j] );
            }
        }

        copiedPayload.FreeDynamicAllocations();
    }

    TEST( ProfilerDataULT, CopyAccelerationStructureBuildInfosWithMicromapData )
    {
        VkMicromapUsageKHR usageCount = {};
        usageCount.count = 1;
        usageCount.format = VK_OPACITY_MICROMAP_FORMAT_2_STATE_KHR;
        usageCount.subdivisionLevel = 2;

        VkAccelerationStructureGeometryMicromapDataKHR micromapData = {};
        micromapData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_MICROMAP_DATA_KHR;
        micromapData.usageCountsCount = 1;
        micromapData.pUsageCounts = &usageCount;
        micromapData.data = 0x98765432;
        micromapData.triangleArray = 0x87654321;
        micromapData.triangleArrayStride = 64;

        VkAccelerationStructureGeometryKHR geometry = {};
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.pNext = &micromapData;
        geometry.geometryType = VK_GEOMETRY_TYPE_MICROMAP_KHR;

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {};
        buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_OPACITY_MICROMAP_KHR;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.dstAccelerationStructure = VkObjectTraits<VkAccelerationStructureKHR>::GetObjectHandleAsVulkanHandle( 0x12345678 );
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geometry;

        VkAccelerationStructureBuildRangeInfoKHR rangeInfo = {};
        rangeInfo.primitiveCount = 1;
        rangeInfo.primitiveOffset = 256;
        rangeInfo.transformOffset = 1024;

        VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;

        DeviceProfilerDrawcallBuildAccelerationStructuresPayload payload = {};
        payload.m_InfoCount = 1;
        payload.m_pInfos = &buildInfo;
        payload.m_ppRanges = &pRangeInfo;

        DeviceProfilerDrawcallBuildAccelerationStructuresPayload copiedPayload = {};
        copiedPayload.CopyDynamicAllocations( payload );

        ASSERT_NE( nullptr, copiedPayload.m_pInfos );
        ExpectStructureEqual( *payload.m_pInfos, *copiedPayload.m_pInfos );

        ASSERT_NE( nullptr, copiedPayload.m_ppRanges );
        for( uint32_t i = 0; i < payload.m_InfoCount; ++i )
        {
            const uint32_t geometryCount = payload.m_pInfos[i].geometryCount;
            for( uint32_t j = 0; j < geometryCount; ++j )
            {
                ExpectStructureEqual( payload.m_ppRanges[i][j], copiedPayload.m_ppRanges[i][j] );
            }
        }

        copiedPayload.FreeDynamicAllocations();
    }
}
