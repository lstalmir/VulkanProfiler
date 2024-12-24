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

#include "profiler_test_queue.h"
#include "profiler_test_command_buffer.h"
#include "profiler_test_query_pool.h"
#include <chrono>
#include <thread>

namespace Profiler::ICD
{
    Queue::Queue( Device& device, const VkDeviceQueueCreateInfo& createInfo )
    {
    }

    Queue::~Queue()
    {
    }

    VkResult Queue::vkQueueSubmit( uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence )
    {
        for( uint32_t i = 0; i < submitCount; ++i )
        {
            for( uint32_t j = 0; j < pSubmits[ i ].commandBufferCount; ++j )
            {
                Exec_CommandBuffer( *static_cast<CommandBuffer*>(
                    pSubmits[ i ].pCommandBuffers[ j ]->m_pImpl ) );
            }
        }

        return VK_SUCCESS;
    }

    VkResult Queue::vkQueueSubmit2( uint32_t submitCount, const VkSubmitInfo2* pSubmits, VkFence fence )
    {
        for( uint32_t i = 0; i < submitCount; ++i )
        {
            for( uint32_t j = 0; j < pSubmits[ i ].commandBufferInfoCount; ++j )
            {
                Exec_CommandBuffer( *static_cast<CommandBuffer*>(
                    pSubmits[ i ].pCommandBufferInfos[ j ].commandBuffer->m_pImpl ) );
            }
        }

        return VK_SUCCESS;
    }

    void Queue::Exec_CommandBuffer( CommandBuffer& commandBuffer )
    {
        for( const Command& command : commandBuffer.m_Commands )
        {
            switch( command.m_Type )
            {
            case Command::eDraw:
                Exec_Draw( command.m_Draw.m_VertexCount, command.m_Draw.m_InstanceCount, command.m_Draw.m_FirstVertex, command.m_Draw.m_FirstInstance );
                break;

            case Command::eDispatch:
                Exec_Dispatch( command.m_Dispatch.m_GroupCountX, command.m_Dispatch.m_GroupCountY, command.m_Dispatch.m_GroupCountZ );
                break;

            case Command::eWriteTimestamp:
                Exec_WriteTimestamp( *reinterpret_cast<QueryPool*>( command.m_WriteTimestamp.m_QueryPool ), command.m_WriteTimestamp.m_Index );
                break;
            }
        }
    }

    void Queue::Exec_Draw( uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance )
    {
        std::this_thread::sleep_for(
            std::chrono::nanoseconds( vertexCount * instanceCount ) );
    }

    void Queue::Exec_Dispatch( uint32_t x, uint32_t y, uint32_t z )
    {
        std::this_thread::sleep_for(
            std::chrono::nanoseconds( 100 * x * y * z ) );
    }

    void Queue::Exec_WriteTimestamp( QueryPool& queryPool, uint32_t query )
    {
        auto nanosecondsSinceEpoch = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch() );

        queryPool.m_Timestamps.at( query ) = nanosecondsSinceEpoch.count();
    }
}