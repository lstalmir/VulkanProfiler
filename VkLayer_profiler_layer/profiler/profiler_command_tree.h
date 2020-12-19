// Copyright (c) 2020 Lukasz Stalmirski
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

#include "profiler_commands.h"

namespace Profiler
{
    class CommandTreeBuilder : public CommandVisitor
    {
        bool m_IsInRenderPassScope;
        bool m_IsInSecondaryCommandBuffer;

        std::shared_ptr<BindPipelineCommand> m_pLastBindGraphicsPipelineCommand;
        std::shared_ptr<BindPipelineCommand> m_pLastBindComputePipelineCommand;

        std::shared_ptr<RenderPassCommandGroup> m_pCurrentRenderPassCommandGroup;
        std::shared_ptr<SubpassCommandGroup> m_pCurrentSubpassCommandGroup;
        std::shared_ptr<PipelineCommandGroup> m_pCurrentPipelineCommandGroup;

        std::shared_ptr<CommandGroup> m_pCommands;

    public:
        inline CommandTreeBuilder( VkCommandBufferUsageFlags commandBufferUsage, VkCommandBufferLevel commandBufferLevel )
            : m_IsInRenderPassScope( commandBufferUsage & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT )
            , m_IsInSecondaryCommandBuffer( commandBufferLevel == VK_COMMAND_BUFFER_LEVEL_SECONDARY )
            , m_pLastBindGraphicsPipelineCommand( nullptr )
            , m_pLastBindComputePipelineCommand( nullptr )
            , m_pCurrentRenderPassCommandGroup( nullptr )
            , m_pCurrentSubpassCommandGroup( nullptr )
            , m_pCurrentPipelineCommandGroup( nullptr )
            , m_pCommands( nullptr )
        {
            // Create root command group
            m_pCommands = std::make_shared<CommandGroup>();

            if( m_IsInRenderPassScope )
            {
                // Record directly to the subpass
                m_pCurrentSubpassCommandGroup = m_pCommands;
            }
        }

        inline std::shared_ptr<CommandGroup> GetCommands() const { return m_pCommands; }

        inline void Visit( std::shared_ptr<BeginRenderPassCommand> pCommand ) override
        {
            assert( !m_IsInRenderPassScope );
            assert( !m_pCurrentRenderPassCommandGroup && !m_pCurrentSubpassCommandGroup );

            // Construct render pass command group
            m_pCurrentRenderPassCommandGroup = RenderPassCommandGroup::Create( pCommand->GetRenderPassHandle() );
            m_pCurrentSubpassCommandGroup = SubpassCommandGroup::Create( 0, pCommand->GetSubpassContents() );

            m_pCurrentRenderPassCommandGroup->AddCommand( pCommand );
            m_pCurrentRenderPassCommandGroup->AddCommand( m_pCurrentSubpassCommandGroup );

            m_IsInRenderPassScope = true;
        }

        inline void Visit( std::shared_ptr<EndRenderPassCommand> pCommand ) override
        {
            assert( m_IsInRenderPassScope );
            assert( m_pCurrentRenderPassCommandGroup );

            // Insert ending command into render pass group
            m_pCurrentRenderPassCommandGroup->AddCommand( pCommand );

            m_pCurrentPipelineCommandGroup = nullptr;
            m_pCurrentSubpassCommandGroup = nullptr;
            m_pCurrentRenderPassCommandGroup = nullptr;

            m_IsInRenderPassScope = false;
        }

        inline void Visit( std::shared_ptr<NextSubpassCommand> pCommand ) override
        {
            assert( m_IsInRenderPassScope );
            assert( m_pCurrentRenderPassCommandGroup && m_pCurrentSubpassCommandGroup );

            // Get index of the current subpass
            const uint32_t currentSubpassIndex = m_pCurrentSubpassCommandGroup->GetSubpassIndex();

            // Construct new subpass command group
            m_pCurrentSubpassCommandGroup = SubpassCommandGroup::Create( currentSubpassIndex + 1, pCommand->GetSubpassContents() );

            m_pCurrentRenderPassCommandGroup->AddCommand( m_pCurrentSubpassCommandGroup );
        }

        inline void Visit( std::shared_ptr<BindPipelineCommand> pCommand ) override
        {
            switch( pCommand->GetPipelineBindPoint() )
            {
            case VK_PIPELINE_BIND_POINT_GRAPHICS:
                m_pLastBindGraphicsPipelineCommand = pCommand;
                break;

            case VK_PIPELINE_BIND_POINT_COMPUTE:
                m_pLastBindComputePipelineCommand = pCommand;
                break;

            default:
                // Pipeline bind point not supported
                assert( false );
                break;
            }
        }

        inline void Visit( std::shared_ptr<PipelineCommand> pCommand ) override
        {
            std::shared_ptr<BindPipelineCommand> pBindPipelineCommand = nullptr;

            switch( pCommand->GetPipelineType() )
            {
            case VK_PIPELINE_BIND_POINT_GRAPHICS:
                pBindPipelineCommand = m_pLastBindGraphicsPipelineCommand;
                break;

            case VK_PIPELINE_BIND_POINT_COMPUTE:
                pBindPipelineCommand = m_pLastBindComputePipelineCommand;
                break;

            default:
                // Pipeline bind point not supported
                assert( false );
                break;
            }

            // Pipeline must be defined
            assert( pBindPipelineCommand );

            if( !m_pCurrentPipelineCommandGroup ||
                m_pCurrentPipelineCommandGroup->GetBindPipelineCommand() != pBindPipelineCommand )
            {
                // Pipeline changed
                m_pCurrentPipelineCommandGroup = PipelineCommandGroup::Create( pBindPipelineCommand );

                if( m_IsInRenderPassScope )
                {
                    assert( m_pCurrentSubpassCommandGroup );

                    // Commands are recorded into the render pass
                    m_pCurrentSubpassCommandGroup->AddCommand( m_pCurrentPipelineCommandGroup );
                }
            }

            // Insert command into the pipeline
            m_pCurrentPipelineCommandGroup->AddCommand( pCommand );
        }

        inline void Visit( std::shared_ptr<InternalPipelineCommand> pCommand ) override
        {
            // Invalidate current pipeline group
            m_pCurrentPipelineCommandGroup = nullptr;

            // Insert command directly into the command buffer
            if( m_IsInRenderPassScope )
            {
                m_pCurrentSubpassCommandGroup->AddCommand( pCommand );
            }
            else
            {
                m_pCommands->AddCommand( pCommand );
            }
        }
    };
}
