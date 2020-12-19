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
#include <memory>
#include <vector>

namespace Profiler
{
    /***********************************************************************************\

    Class:
        FrameTreeNode

    Description:

    \***********************************************************************************/
    class FrameTreeNode
    {
    public:
        inline FrameTreeNode( std::shared_ptr<const Command> pCommand )
            : m_pCommand( pCommand )
            , m_pChildren()
        {
        }

        inline ~FrameTreeNode()
        {
            for( FrameTreeNode* pChild : m_pChildren )
            {
                delete pChild;
            }
        }

        inline void Add( FrameTreeNode* pNode ) { m_pChildren.push_back( pNode ); }
        inline bool IsEmpty() const { return m_pChildren.empty(); }
        inline size_t GetSize() const { return m_pChildren.size(); }
        inline std::shared_ptr<const Command> GetCommand() const { return m_pCommand; }
        inline const std::vector<FrameTreeNode*>& EnumerateChildren() { return m_pChildren; }

    private:
        std::shared_ptr<const Command> m_pCommand;
        std::vector<FrameTreeNode*> m_pChildren;
    };

    /***********************************************************************************\

    Class:
        FrameTreeConstructor

    Description:
        Creates logical frame tree from list of commands.

    \***********************************************************************************/
    class FrameTreeConstructor : public CommandVisitor
    {
        FrameTreeNode* m_pRoot;

        FrameTreeNode* m_pCurrentRenderPass;

        FrameTreeNode* m_pCurrentGraphicsPipelineNode;
        FrameTreeNode* m_pCurrentComputePipelineNode;

    public:
        inline void Visit( const BeginRenderPassCommand& command )
        {
            assert( m_pCurrentRenderPass == nullptr );

            // Begin next render pass
            m_pCurrentRenderPass = new FrameTreeNode( command );
        }

        inline void Visit( const EndRenderPassCommand& command )
        {
            assert( m_pCurrentRenderPass != nullptr );

            // Check if any commands have been recorded
            if( m_pCurrentGraphicsPipelineNode && !m_pCurrentGraphicsPipelineNode->IsEmpty() )
            {
                m_pCurrentRenderPass->Add( m_pCurrentGraphicsPipelineNode );
            }

            // Insert render pass node into the root node
            m_pRoot->Add( m_pCurrentRenderPass );

            // Invalidate render pass before next call to vkCmdBeginRenderPass
            m_pCurrentRenderPass = nullptr;
        }

        inline void Visit( const BindPipelineCommand& command )
        {
            FrameTreeNode** ppCurrentPipelineNode = nullptr;

            switch( command.GetPipelineBindPoint() )
            {
            case VK_PIPELINE_BIND_POINT_GRAPHICS:
                ppCurrentPipelineNode = &m_pCurrentGraphicsPipelineNode;
                break;

            case VK_PIPELINE_BIND_POINT_COMPUTE:
                ppCurrentPipelineNode = &m_pCurrentComputePipelineNode;
                break;

            case VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR:
                assert( !"Ray-tracing not supported" );
            }

            assert( ppCurrentPipelineNode );


        }
    };
}
