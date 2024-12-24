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

#include "profiler_test_command_buffer.h"

namespace Profiler::ICD
{
    CommandBuffer::CommandBuffer()
        : m_Commands( 0 )
    {
    }

    CommandBuffer::~CommandBuffer()
    {
    }

    VkResult CommandBuffer::vkBeginCommandBuffer( const VkCommandBufferBeginInfo* pBeginInfo )
    {
        return vkResetCommandBuffer( 0 );
    }

    VkResult CommandBuffer::vkResetCommandBuffer( VkCommandBufferResetFlags flags )
    {
        m_Commands.clear();
        return VK_SUCCESS;
    }

    void CommandBuffer::vkCmdDraw( uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance )
    {
        Command& command = m_Commands.emplace_back();
        command.m_Type = Command::eDraw;
        command.m_Draw.m_VertexCount = vertexCount;
        command.m_Draw.m_InstanceCount = instanceCount;
        command.m_Draw.m_FirstVertex = firstVertex;
        command.m_Draw.m_FirstInstance = firstInstance;
    }

    void CommandBuffer::vkCmdDispatch( uint32_t x, uint32_t y, uint32_t z )
    {
        Command& command = m_Commands.emplace_back();
        command.m_Type = Command::eDispatch;
        command.m_Dispatch.m_GroupCountX = x;
        command.m_Dispatch.m_GroupCountY = y;
        command.m_Dispatch.m_GroupCountZ = z;
    }

    void CommandBuffer::vkCmdWriteTimestamp( VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query )
    {
        Command& command = m_Commands.emplace_back();
        command.m_Type = Command::eWriteTimestamp;
        command.m_WriteTimestamp.m_QueryPool = queryPool;
        command.m_WriteTimestamp.m_Index = query;
    }

    void CommandBuffer::vkCmdCopyQueryPoolResults( VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags )
    {
        Command& command = m_Commands.emplace_back();
        command.m_Type = Command::eCopyQueryPoolResults;
        command.m_CopyQueryPoolResults.m_QueryPool = queryPool;
        command.m_CopyQueryPoolResults.m_FirstQuery = firstQuery;
        command.m_CopyQueryPoolResults.m_QueryCount = queryCount;
        command.m_CopyQueryPoolResults.m_DstBuffer = dstBuffer;
        command.m_CopyQueryPoolResults.m_DstOffset = dstOffset;
        command.m_CopyQueryPoolResults.m_Stride = stride;
        command.m_CopyQueryPoolResults.m_Flags = flags;
    }
}
