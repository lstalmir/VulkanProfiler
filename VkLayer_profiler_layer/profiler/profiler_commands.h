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

#pragma once
#include "profiler_shader.h"
#include <vulkan/vulkan.h>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <vector>

namespace Profiler
{
    /***********************************************************************************\

    Enum:
        CommandId

    Description:
        Recorded command type.

    \***********************************************************************************/
    enum class CommandId
    {
        eCommandGroup,
        eRenderPassCommandGroup,
        eSubpassCommandGroup,
        ePipelineCommandGroup,

        #define ProfilerCommand( Cmd ) e##Cmd,
        #include "profiler_commands.inl"
    };

    /***********************************************************************************\

    Enum:
        RenderPassScope

    \***********************************************************************************/
    enum class RenderPassScope
    {
        eBoth = 0,
        eInside = 1,
        eOutside = 2
    };

    /***********************************************************************************\

    Enum:
        InternalPipelineId

    Description:
        Identifiers of internal pipelines.

    \***********************************************************************************/
    enum class InternalPipelineId : uint32_t
    {
        #define ProfilerPipeline( Pipeline ) e##Pipeline,
        #include "profiler_pipelines.inl"
    };

    /***********************************************************************************\

    Class:
        CommandVisitor

    Description:
        Executes action on command.

    \***********************************************************************************/
    class CommandVisitor
    {
    public:
        inline virtual void Visit( std::shared_ptr<class Command> pCommand ) {}

        // Command collections
        virtual void Visit( std::shared_ptr<class CommandGroup> pCommand );
        virtual void Visit( std::shared_ptr<class RenderPassCommandGroup> pCommand );
        virtual void Visit( std::shared_ptr<class SubpassCommandGroup> pCommand );
        virtual void Visit( std::shared_ptr<class PipelineCommandGroup> pCommand );

        // Meta-commands
        virtual void Visit( std::shared_ptr<class DebugCommand> pCommand );
        virtual void Visit( std::shared_ptr<class DebugLabelCommand> pCommand );
        virtual void Visit( std::shared_ptr<class InternalPipelineCommand> pCommand );
        virtual void Visit( std::shared_ptr<class PipelineCommand> pCommand );
        virtual void Visit( std::shared_ptr<class GraphicsCommand> pCommand );
        virtual void Visit( std::shared_ptr<class ComputeCommand> pCommand );
        virtual void Visit( std::shared_ptr<class TransferCommand> pCommand );
        virtual void Visit( std::shared_ptr<class ClearCommand> pCommand );

        #define ProfilerCommand( Cmd ) \
        virtual void Visit( std::shared_ptr<class Cmd##Command> pCommand );
        #include "profiler_commands.inl"
    };

    /***********************************************************************************\

    Class:
        Command

    Description:
        Wraps single command recorded in the command buffer.

    \***********************************************************************************/
    class Command : protected std::enable_shared_from_this<Command>
    {
    public:
        inline virtual void Accept( CommandVisitor& visitor )
        {
            visitor.Visit( shared_from_this() );
        }

        inline Command( CommandId id ) : m_Id( id ), m_pCommandData( nullptr ) {}
        virtual ~Command() { delete m_pCommandData; }

        inline CommandId GetId() const { return m_Id; }
        inline RenderPassScope GetRenderPassScope() const { return m_RenderPassScope; }
        inline VkPipelineBindPoint GetPipelineType() const { return m_PipelineType; }
        inline const char* GetName() const { return Command::Name( shared_from_this() ); }

        inline void SetParent( std::shared_ptr<Command> pParent ) { m_pParent = pParent; }
        inline auto GetParent() const { return m_pParent.lock(); }

        template<typename CommandType>
        inline std::shared_ptr<CommandType> As()
        {
            return std::dynamic_pointer_cast<CommandType>(shared_from_this());
        }

        template<typename CommandType>
        inline std::shared_ptr<const CommandType> As() const
        {
            return std::dynamic_pointer_cast<const CommandType>(shared_from_this());
        }

        static std::shared_ptr<Command> Copy( std::shared_ptr<const Command> pCommand );
        static const char* Name( std::shared_ptr<const Command> pCommand );

        inline virtual std::string ToString() const { return "Unknown command"; }

        inline void SetCommandData( void* pCommandData ) { m_pCommandData = pCommandData; }
        inline const void* GetCommandData() const { return m_pCommandData; }

        template<typename CommandDataType>
        inline const CommandDataType* GetCommandData() const
        {
            return reinterpret_cast<const CommandDataType*>(m_pCommandData);
        }

    private:
        std::weak_ptr<Command> m_pParent;

        const CommandId m_Id = {};
        const RenderPassScope m_RenderPassScope = {};
        const VkPipelineBindPoint m_PipelineType = {};

        void* m_pCommandData = {};
    };

    struct CommandTimestampData
    {
        uint64_t m_BeginTimestamp = {};
        uint64_t m_EndTimestamp = {};

        inline uint64_t GetTicks() const { return m_EndTimestamp - m_BeginTimestamp; }
    };

    /***********************************************************************************\

    Class:
        CommandGroup

    Description:
        Collection of commands.

    \***********************************************************************************/
    class CommandGroup : public Command
    {
    public:
        using Command::Command;
        using BaseCommandType = Command;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline CommandGroup()
            : CommandGroup( CommandId::eCommandGroup )
        {
        }

        inline CommandGroup( const CommandGroup& commandGroup )
            : CommandGroup( commandGroup.GetId() )
        {
            // Create deep copy of the command group
            for( std::shared_ptr<Command> pCommand : commandGroup.m_pCommands )
            {
                m_pCommands.push_back( Command::Copy( pCommand ) );
            }
        }

        inline CommandGroup( CommandGroup&& commandGroup )
            : CommandGroup( commandGroup.GetId() )
        {
            // Move commands to the new group
            std::swap( m_pCommands, commandGroup.m_pCommands );
        }

        inline void AddCommand( std::shared_ptr<Command> pCommand )
        {
            pCommand->SetParent( shared_from_this() );
            m_pCommands.push_back( pCommand );
        }

        inline bool IsEmpty() const { return m_pCommands.empty(); }
        inline uint32_t GetSize() const { return static_cast<uint32_t>(m_pCommands.size()); }

        inline auto begin() { return m_pCommands.begin(); }
        inline auto end() { return m_pCommands.end(); }
        inline auto begin() const { return m_pCommands.begin(); }
        inline auto end() const { return m_pCommands.end(); }

        inline auto GetCommand( uint32_t i ) const { return m_pCommands[ i ]; }

        inline uint32_t GetIndexOf( std::shared_ptr<const Command> pCommand )
        {
            const uint32_t commandCount = GetSize();
            for( uint32_t i = 0; i < commandCount; ++i )
            {
                if( m_pCommands[ i ] == pCommand )
                {
                    return i;
                }
            }
            // Command not in group
            return std::numeric_limits<uint32_t>::max();
        }

    private:
        std::vector<std::shared_ptr<Command>> m_pCommands;
    };

    /***********************************************************************************\

    Class:
        DebugCommand

    Description:
        Meta-command.

    \***********************************************************************************/
    class DebugCommand : public Command
    {
    public:
        using Command::Command;
        using BaseCommandType = Command;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }
    };

    /***********************************************************************************\

    Class:
        DebugLabelCommand

    Description:
        Wraps:
        - vkCmdInsertDebugUtilsLabelEXT
        - vkCmdBeginDebugUtilsLabelEXT
        - vkCmdEndDebugUtilsLabelEXT

    \***********************************************************************************/
    class DebugLabelCommand : public DebugCommand
    {
        char* m_pLabel = {};
        float m_Color[ 4 ] = {};

    public:
        using BaseCommandType = DebugCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<DebugLabelCommand> Create( CommandId id )
        {
            // For begin/insert commands pLabel and pColor must be provided
            assert( id == CommandId::eEndDebugLabel );

            return std::make_shared<DebugLabelCommand>( id );
        }

        inline static std::shared_ptr<DebugLabelCommand> Create( CommandId id, const char* pLabel, const float* pColor )
        {
            // CommandId must be one of debug label commands
            assert( (id == CommandId::eBeginDebugLabel) ||
                (id == CommandId::eInsertDebugLabel) );

            auto pCommand = std::make_shared<DebugLabelCommand>( id );
            if( pCommand )
            {
                // Fill debug label command info
                pCommand->m_pLabel = strdup( pLabel );
                pCommand->m_Color[ 0 ] = pColor[ 0 ];
                pCommand->m_Color[ 1 ] = pColor[ 1 ];
                pCommand->m_Color[ 2 ] = pColor[ 2 ];
                pCommand->m_Color[ 3 ] = pColor[ 3 ];
            }
            return pCommand;
        }

        inline DebugLabelCommand( CommandId id )
            : DebugCommand( id )
        {
        }

        inline DebugLabelCommand( const DebugLabelCommand& command )
            : DebugCommand( command.GetId() )
            , m_pLabel( nullptr )
            , m_Color()
        { // Create copy of debug label command
            m_pLabel = strdup( command.m_pLabel );
            m_Color[ 0 ] = command.m_Color[ 0 ];
            m_Color[ 1 ] = command.m_Color[ 1 ];
            m_Color[ 2 ] = command.m_Color[ 2 ];
            m_Color[ 3 ] = command.m_Color[ 3 ];
        }

        inline DebugLabelCommand( DebugLabelCommand&& command )
            : DebugCommand( command.GetId() )
            , m_pLabel( nullptr )
            , m_Color()
        { // Move debug label command
            std::swap( m_pLabel, command.m_pLabel );
            std::swap( m_Color, command.m_Color );
        }

        inline ~DebugLabelCommand() { free( m_pLabel ); }

        inline const char* GetLabel() const { return m_pLabel; }
        inline const float* GetColor() const { return m_Color; }
    };

    class BeginDebugLabelCommand : public DebugLabelCommand {};
    class EndDebugLabelCommand : public DebugLabelCommand {};
    class InsertDebugLabelCommand : public DebugLabelCommand {};

    /***********************************************************************************\

    Class:
        RenderPassCommandGroup

    Description:
        Wraps vkCmdBegin-, vkCmdEnd- RenderPass and all commands inside the render pass.

    \***********************************************************************************/
    class RenderPassCommandGroup : public CommandGroup
    {
        VkRenderPass m_Handle;

    public:
        using CommandGroup::CommandGroup;
        using BaseCommandType = CommandGroup;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<RenderPassCommandGroup> Create( VkRenderPass handle )
        {
            auto pCommand = std::make_shared<RenderPassCommandGroup>( CommandId::eRenderPassCommandGroup );
            if( pCommand )
            {
                pCommand->m_Handle = handle;
            }
            return pCommand;
        }

        inline VkRenderPass GetRenderPassHandle() const { return m_Handle; }
        inline uint32_t GetSubpassCount() const { return GetSize() - 2; }
    };

    /***********************************************************************************\

    Class:
        SubpassCommandGroup

    Description:
        Wraps all commands inside the subpass.

    \***********************************************************************************/
    class SubpassCommandGroup : public CommandGroup
    {
        uint32_t m_Index;
        VkSubpassContents m_SubpassContents;

    public:
        using CommandGroup::CommandGroup;
        using BaseCommandType = CommandGroup;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<SubpassCommandGroup> Create( uint32_t index, VkSubpassContents contents )
        {
            auto pCommand = std::make_shared<SubpassCommandGroup>( CommandId::eSubpassCommandGroup );
            if( pCommand )
            {
                pCommand->m_Index = index;
                pCommand->m_SubpassContents = contents;
            }
            return pCommand;
        }

        inline uint32_t GetSubpassIndex() const { return m_Index; }
        inline VkSubpassContents GetSubpassContents() const { return m_SubpassContents; }
    };

    /***********************************************************************************\

    Class:
        InternalPipelineCommand

    Description:
        Wraps commands which use internal pipelines.

    \***********************************************************************************/
    class InternalPipelineCommand : public Command
    {
        const InternalPipelineId m_InternalPipelineId;

    public:
        using Command::Command;
        using BaseCommandType = Command;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline InternalPipelineCommand( CommandId id, InternalPipelineId pipelineId )
            : Command( id )
            , m_InternalPipelineId( pipelineId )
        {
        }

        inline InternalPipelineId GetInternalPipelineId() const { return m_InternalPipelineId; }
        inline uint32_t GetInternalPipelineHash() const { return static_cast<uint32_t>(m_InternalPipelineId); }
    };

    /***********************************************************************************\

    Class:
        BeginRenderPassCommand

    Description:
        Wraps:
        - vkCmdBeginRenderPass
        - vkCmdBeginRenderPass2
        - vkCmdBeginRenderPass2KHR

    \***********************************************************************************/
    class BeginRenderPassCommand : public InternalPipelineCommand
    {
        VkRenderPass m_Handle;
        VkSubpassContents m_Contents;

    public:
        using InternalPipelineCommand::InternalPipelineCommand;
        using BaseCommandType = InternalPipelineCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<BeginRenderPassCommand> Create( const VkRenderPassBeginInfo* pBeginInfo, VkSubpassContents contents )
        {
            auto pCommand = std::make_shared<BeginRenderPassCommand>( CommandId::eBeginRenderPass, InternalPipelineId::eBeginRenderPass );
            if( pCommand )
            {
                pCommand->m_Handle = pBeginInfo->renderPass;
                pCommand->m_Contents = contents;
            }
            return pCommand;
        }

        inline VkRenderPass GetRenderPassHandle() const { return m_Handle; }
        inline VkSubpassContents GetSubpassContents() const { return m_Contents; }
    };

    /***********************************************************************************\

    Class:
        EndRenderPassCommand

    Description:
        Wraps:
        - vkCmdEndRenderPass
        - vkCmdEndRenderPass2
        - vkCmdEndRenderPass2KHR

    \***********************************************************************************/
    class EndRenderPassCommand : public InternalPipelineCommand
    {
    public:
        using InternalPipelineCommand::InternalPipelineCommand;
        using BaseCommandType = InternalPipelineCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<EndRenderPassCommand> Create()
        {
            return std::make_shared<EndRenderPassCommand>( CommandId::eEndRenderPass, InternalPipelineId::eEndRenderPass );
        }
    };

    /***********************************************************************************\

    Class:
        NextSubpassCommand

    Description:
        Wraps:
        - vkCmdNextSubpass
        - vkCmdNextSubpass2
        - vkCmdNextSubpass2KHR

    \***********************************************************************************/
    class NextSubpassCommand : public InternalPipelineCommand
    {
        VkSubpassContents m_Contents;

    public:
        using InternalPipelineCommand::InternalPipelineCommand;
        using BaseCommandType = InternalPipelineCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<NextSubpassCommand> Create( VkSubpassContents contents )
        {
            auto pCommand = std::make_shared<NextSubpassCommand>( CommandId::eNextSubpass, InternalPipelineId::eNextSubpass );
            if( pCommand )
            {
                pCommand->m_Contents = contents;
            }
            return pCommand;
        }

        inline VkSubpassContents GetSubpassContents() const { return m_Contents; }
    };

    /***********************************************************************************\

    Class:
        BindPipelineCommand

    Description:
        Wraps:
        - vkCmdBindPipeline

    \***********************************************************************************/
    class BindPipelineCommand : public Command
    {
        VkPipelineBindPoint m_BindPoint;
        VkPipeline m_Handle;

    public:
        using Command::Command;
        using BaseCommandType = Command;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<BindPipelineCommand> Create( VkPipelineBindPoint bindPoint, VkPipeline pipeline )
        {
            auto pCommand = std::make_shared<BindPipelineCommand>( CommandId::eBindPipeline );
            if( pCommand )
            {
                pCommand->m_BindPoint = bindPoint;
                pCommand->m_Handle = pipeline;
            }
            return pCommand;
        }

        inline VkPipelineBindPoint GetPipelineBindPoint() const { return m_BindPoint; }
        inline VkPipeline GetPipelineHandle() const { return m_Handle; }
    };

    /***********************************************************************************\

    Class:
        PipelineCommandGroup

    Description:
        Wraps all commands executed using one pipeline state.

    \***********************************************************************************/
    class PipelineCommandGroup : public CommandGroup
    {
        std::shared_ptr<BindPipelineCommand> m_pBindCommand;
        ProfilerShaderTuple m_ShaderTuple;

    public:
        using CommandGroup::CommandGroup;
        using BaseCommandType = CommandGroup;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<PipelineCommandGroup> Create( std::shared_ptr<BindPipelineCommand> pBindCommand, const ProfilerShaderTuple& shaderTuple )
        {
            auto pCommand = std::make_shared<PipelineCommandGroup>( CommandId::ePipelineCommandGroup );
            if( pCommand )
            {
                pCommand->m_pBindCommand = pBindCommand;
                pCommand->m_ShaderTuple = shaderTuple;
            }
            return pCommand;
        }

        inline std::shared_ptr<BindPipelineCommand> GetBindPipelineCommand() const { return m_pBindCommand; }

        inline uint32_t GetPipelineHash() const { return m_ShaderTuple.m_Hash; }

        // Classic 3D graphics pipeline stages
        inline uint32_t GetVertexShaderHash() const { return m_ShaderTuple.m_Vert; }
        inline uint32_t GetTessellationControlShaderHash() const { return m_ShaderTuple.m_Tesc; }
        inline uint32_t GetTessellationEvaluationShaderHash() const { return m_ShaderTuple.m_Tese; }
        inline uint32_t GetGeometryShaderHash() const { return m_ShaderTuple.m_Geom; }
        inline uint32_t GetFragmentShaderHash() const { return m_ShaderTuple.m_Frag; }

        // Compute pipeline stages
        inline uint32_t GetComputeShaderHash() const { return m_ShaderTuple.m_Comp; }
    };

    /***********************************************************************************\

    Class:
        PipelineBarrierCommand

    Description:
        Wraps:
        - vkCmdPipelineBarrier

    \***********************************************************************************/
    class PipelineBarrierCommand : public InternalPipelineCommand
    {
        VkPipelineStageFlags m_SrcStageFlags;
        VkPipelineStageFlags m_DstStageFlags;
        VkDependencyFlags m_DependencyFlags;
        std::vector<VkMemoryBarrier> m_MemoryBarriers;
        std::vector<VkBufferMemoryBarrier> m_BufferMemoryBarriers;
        std::vector<VkImageMemoryBarrier> m_ImageMemoryBarriers;

    public:
        using InternalPipelineCommand::InternalPipelineCommand;
        using BaseCommandType = InternalPipelineCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<PipelineBarrierCommand> Create(
            VkPipelineStageFlags srcStageFlags,
            VkPipelineStageFlags dstStageFlags,
            VkDependencyFlags dependencyFlags,
            uint32_t memoryBarrierCount,
            const VkMemoryBarrier* pMemoryBarriers,
            uint32_t bufferMemoryBarrierCount,
            const VkBufferMemoryBarrier* pBufferMemoryBarriers,
            uint32_t imageMemoryBarrierCount,
            const VkImageMemoryBarrier* pImageMemoryBarriers )
        {
            auto pCommand = std::make_shared<PipelineBarrierCommand>( CommandId::ePipelineBarrier, InternalPipelineId::ePipelineBarrier );
            if( pCommand )
            {
                pCommand->m_SrcStageFlags = srcStageFlags;
                pCommand->m_DstStageFlags = dstStageFlags;
                pCommand->m_DependencyFlags = dependencyFlags;

                pCommand->m_MemoryBarriers.resize( memoryBarrierCount );

                for( uint32_t i = 0; i < memoryBarrierCount; ++i )
                {
                    pCommand->m_MemoryBarriers.push_back( pMemoryBarriers[ i ] );
                }

                pCommand->m_BufferMemoryBarriers.reserve( bufferMemoryBarrierCount );

                for( uint32_t i = 0; i < bufferMemoryBarrierCount; ++i )
                {
                    pCommand->m_BufferMemoryBarriers.push_back( pBufferMemoryBarriers[ i ] );
                }

                pCommand->m_ImageMemoryBarriers.reserve( imageMemoryBarrierCount );

                for( uint32_t i = 0; i < imageMemoryBarrierCount; ++i )
                {
                    pCommand->m_ImageMemoryBarriers.push_back( pImageMemoryBarriers[ i ] );
                }
            }
            return pCommand;
        }

        inline VkPipelineStageFlags GetSrcStageFlags() const { return m_SrcStageFlags; }
        inline VkPipelineStageFlags GetDstStageFlags() const { return m_DstStageFlags; }
        inline VkDependencyFlags GetDependencyFlags() const { return m_DependencyFlags; }
        inline uint32_t GetMemoryBarrierCount() const { return static_cast<uint32_t>(m_MemoryBarriers.size()); }
        inline const VkMemoryBarrier* GetPMemoryBarriers() const { return m_MemoryBarriers.data(); }
        inline uint32_t GetBufferMemoryBarrierCount() const { return static_cast<uint32_t>(m_BufferMemoryBarriers.size()); }
        inline const VkBufferMemoryBarrier* GetPBufferMemoryBarriers() const { return m_BufferMemoryBarriers.data(); }
        inline uint32_t GetImageMemoryBarrierCount() const { return static_cast<uint32_t>(m_ImageMemoryBarriers.size()); }
        inline const VkImageMemoryBarrier* GetPImageMemoryBarriers() const { return m_ImageMemoryBarriers.data(); }
    };

    /***********************************************************************************\

    Class:
        ExecuteCommandsCommand

    Description:
        Wraps:
        - vkCmdExecuteCommands

    \***********************************************************************************/
    class ExecuteCommandsCommand : public Command
    {
        std::vector<VkCommandBuffer> m_CommandBuffers;

    public:
        using Command::Command;
        using BaseCommandType = Command;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<ExecuteCommandsCommand> Create( uint32_t count, const VkCommandBuffer* pCommandBuffers )
        {
            auto pCommand = std::make_shared<ExecuteCommandsCommand>( CommandId::eExecuteCommands );
            if( pCommand )
            {
                pCommand->m_CommandBuffers.reserve( count );

                for( uint32_t i = 0; i < count; ++i )
                {
                    pCommand->m_CommandBuffers.push_back( pCommandBuffers[ i ] );
                }
            }
            return pCommand;
        }

        inline uint32_t GetCommandBufferCount() const { return static_cast<uint32_t>(m_CommandBuffers.size()); }
        inline const VkCommandBuffer* GetPCommandBuffers() const { return m_CommandBuffers.data(); }

        inline const struct ExecuteCommandsCommandData* GetCommandData() const
        {
            return BaseCommandType::template GetCommandData<struct ExecuteCommandsCommandData>();
        }
    };

    struct ExecuteCommandsCommandData
    {
        std::vector<std::shared_ptr<const struct CommandBufferData>> m_pCommandBuffers;
    };

    /***********************************************************************************\

    Class:
        PipelineCommand

    Description:
        Meta-command.

    \***********************************************************************************/
    class PipelineCommand : public Command
    {
    public:
        using Command::Command;
        using BaseCommandType = Command;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }
    };

    /***********************************************************************************\

    Class:
        GraphicsCommand

    Description:
        Meta-command.

    \***********************************************************************************/
    class GraphicsCommand : public PipelineCommand
    {
    public:
        using PipelineCommand::PipelineCommand;
        using BaseCommandType = PipelineCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }
    };

    /***********************************************************************************\

    Class:
        DrawCommand

    Description:
        Wraps:
        - vkCmdDraw

    \***********************************************************************************/
    class DrawCommand : public GraphicsCommand
    {
        uint32_t m_VertexCount;
        uint32_t m_InstanceCount;
        uint32_t m_FirstVertex;
        uint32_t m_FirstInstance;

    public:
        using GraphicsCommand::GraphicsCommand;
        using BaseCommandType = GraphicsCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<DrawCommand> Create( uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance )
        {
            auto pCommand = std::make_shared<DrawCommand>( CommandId::eDraw );
            if( pCommand )
            {
                pCommand->m_VertexCount = vertexCount;
                pCommand->m_InstanceCount = instanceCount;
                pCommand->m_FirstVertex = firstVertex;
                pCommand->m_FirstInstance = firstInstance;
            }
            return pCommand;
        }

        inline uint32_t GetVertexCount() const { return m_VertexCount; }
        inline uint32_t GetInstanceCount() const { return m_InstanceCount; }
        inline uint32_t GetFirstVertex() const { return m_FirstVertex; }
        inline uint32_t GetFirstInstance() const { return m_FirstInstance; }
    };

    /***********************************************************************************\

    Class:
        DrawIndexedCommand

    Description:
        Wraps:
        - vkCmdDrawIndexed

    \***********************************************************************************/
    class DrawIndexedCommand : public GraphicsCommand
    {
        uint32_t m_IndexCount;
        uint32_t m_InstanceCount;
        uint32_t m_FirstIndex;
        int32_t m_VertexOffset;
        uint32_t m_FirstInstance;

    public:
        using GraphicsCommand::GraphicsCommand;
        using BaseCommandType = GraphicsCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<DrawIndexedCommand> Create( uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance )
        {
            auto pCommand = std::make_shared<DrawIndexedCommand>( CommandId::eDrawIndexed );
            if( pCommand )
            {
                pCommand->m_IndexCount = indexCount;
                pCommand->m_InstanceCount = instanceCount;
                pCommand->m_FirstIndex = firstIndex;
                pCommand->m_VertexOffset = vertexOffset;
                pCommand->m_FirstInstance = firstInstance;
            }
            return pCommand;
        }

        inline uint32_t GetIndexCount() const { return m_IndexCount; }
        inline uint32_t GetInstanceCount() const { return m_InstanceCount; }
        inline uint32_t GetFirstIndex() const { return m_FirstIndex; }
        inline int32_t GetVertexOffset() const { return m_VertexOffset; }
        inline uint32_t GetFirstInstance() const { return m_FirstInstance; }
    };

    /***********************************************************************************\

    Class:
        DrawIndirectCommand

    Description:
        Wraps:
        - vkCmdDrawIndirect

    \***********************************************************************************/
    class DrawIndirectCommand : public GraphicsCommand
    {
        VkBuffer m_Buffer;
        VkDeviceSize m_Offset;
        uint32_t m_DrawCount;
        uint32_t m_Stride;

    public:
        using GraphicsCommand::GraphicsCommand;
        using BaseCommandType = GraphicsCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<DrawIndirectCommand> Create( VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride )
        {
            auto pCommand = std::make_shared<DrawIndirectCommand>( CommandId::eDrawIndirect );
            if( pCommand )
            {
                pCommand->m_Buffer = buffer;
                pCommand->m_Offset = offset;
                pCommand->m_DrawCount = drawCount;
                pCommand->m_Stride = stride;
            }
            return pCommand;
        }

        inline VkBuffer GetBuffer() const { return m_Buffer; }
        inline VkDeviceSize GetOffset() const { return m_Offset; }
        inline uint32_t GetDrawCount() const { return m_DrawCount; }
        inline uint32_t GetStride() const { return m_Stride; }
    };

    /***********************************************************************************\

    Class:
        DrawIndexedIndirectCommand

    Description:
        Wraps:
        - vkCmdDrawIndexedIndirect

    \***********************************************************************************/
    class DrawIndexedIndirectCommand : public GraphicsCommand
    {
        VkBuffer m_Buffer;
        VkDeviceSize m_Offset;
        uint32_t m_DrawCount;
        uint32_t m_Stride;

    public:
        using GraphicsCommand::GraphicsCommand;
        using BaseCommandType = GraphicsCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<DrawIndexedIndirectCommand> Create( VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride )
        {
            auto pCommand = std::make_shared<DrawIndexedIndirectCommand>( CommandId::eDrawIndexedIndirect );
            if( pCommand )
            {
                pCommand->m_Buffer = buffer;
                pCommand->m_Offset = offset;
                pCommand->m_DrawCount = drawCount;
                pCommand->m_Stride = stride;
            }
            return pCommand;
        }

        inline VkBuffer GetBuffer() const { return m_Buffer; }
        inline VkDeviceSize GetOffset() const { return m_Offset; }
        inline uint32_t GetDrawCount() const { return m_DrawCount; }
        inline uint32_t GetStride() const { return m_Stride; }
    };

    /***********************************************************************************\

    Class:
        DrawIndirectCountCommand

    Description:
        Wraps:
        - vkCmdDrawIndirectCount
        - vkCmdDrawIndirectCountKHR
        - vkCmdDrawIndirectCountAMD

    \***********************************************************************************/
    class DrawIndirectCountCommand : public GraphicsCommand
    {
        VkBuffer m_Buffer;
        VkDeviceSize m_Offset;
        VkBuffer m_CountBuffer;
        VkDeviceSize m_CountOffset;
        uint32_t m_MaxDrawCount;
        uint32_t m_Stride;

    public:
        using GraphicsCommand::GraphicsCommand;
        using BaseCommandType = GraphicsCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<DrawIndirectCountCommand> Create( VkBuffer buffer, uint32_t offset, VkBuffer countBuffer, uint32_t countOffset, uint32_t maxDrawCount, uint32_t stride )
        {
            auto pCommand = std::make_shared<DrawIndirectCountCommand>( CommandId::eDrawIndirectCount );
            if( pCommand )
            {
                pCommand->m_Buffer = buffer;
                pCommand->m_Offset = offset;
                pCommand->m_CountBuffer = countBuffer;
                pCommand->m_CountOffset = countOffset;
                pCommand->m_MaxDrawCount = maxDrawCount;
                pCommand->m_Stride = stride;
            }
            return pCommand;
        }

        inline VkBuffer GetBuffer() const { return m_Buffer; }
        inline VkDeviceSize GetOffset() const { return m_Offset; }
        inline VkBuffer GetCountBuffer() const { return m_CountBuffer; }
        inline VkDeviceSize GetCountOffset() const { return m_CountOffset; }
        inline uint32_t GetMaxDrawCount() const { return m_MaxDrawCount; }
        inline uint32_t GetStride() const { return m_Stride; }
    };

    /***********************************************************************************\

    Class:
        DrawIndexedIndirectCountCommand

    Description:
        Wraps:
        - vkCmdDrawIndexedIndirectCount
        - vkCmdDrawIndexedIndirectCountKHR
        - vkCmdDrawIndexedIndirectCountAMD

    \***********************************************************************************/
    class DrawIndexedIndirectCountCommand : public GraphicsCommand
    {
        VkBuffer m_Buffer;
        VkDeviceSize m_Offset;
        VkBuffer m_CountBuffer;
        VkDeviceSize m_CountOffset;
        uint32_t m_MaxDrawCount;
        uint32_t m_Stride;

    public:
        using GraphicsCommand::GraphicsCommand;
        using BaseCommandType = GraphicsCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<DrawIndexedIndirectCountCommand> Create( VkBuffer buffer, uint32_t offset, VkBuffer countBuffer, uint32_t countOffset, uint32_t maxDrawCount, uint32_t stride )
        {
            auto pCommand = std::make_shared<DrawIndexedIndirectCountCommand>( CommandId::eDrawIndexedIndirectCount );
            if( pCommand )
            {
                pCommand->m_Buffer = buffer;
                pCommand->m_Offset = offset;
                pCommand->m_CountBuffer = countBuffer;
                pCommand->m_CountOffset = countOffset;
                pCommand->m_MaxDrawCount = maxDrawCount;
                pCommand->m_Stride = stride;
            }
            return pCommand;
        }

        inline VkBuffer GetBuffer() const { return m_Buffer; }
        inline VkDeviceSize GetOffset() const { return m_Offset; }
        inline VkBuffer GetCountBuffer() const { return m_CountBuffer; }
        inline VkDeviceSize GetCountOffset() const { return m_CountOffset; }
        inline uint32_t GetMaxDrawCount() const { return m_MaxDrawCount; }
        inline uint32_t GetStride() const { return m_Stride; }
    };

    /***********************************************************************************\

    Class:
        ComputeCommand

    Description:
        Meta-command.

    \***********************************************************************************/
    class ComputeCommand : public PipelineCommand
    {
    public:
        using PipelineCommand::PipelineCommand;
        using BaseCommandType = PipelineCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }
    };

    /***********************************************************************************\

    Class:
        DispatchCommand

    Description:
        Wraps:
        - vkCmdDispatch

    \***********************************************************************************/
    class DispatchCommand : public ComputeCommand
    {
        uint32_t m_X;
        uint32_t m_Y;
        uint32_t m_Z;

    public:
        using ComputeCommand::ComputeCommand;
        using BaseCommandType = ComputeCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<DispatchCommand> Create( uint32_t x, uint32_t y, uint32_t z )
        {
            auto pCommand = std::make_shared<DispatchCommand>( CommandId::eDispatch );
            if( pCommand )
            {
                pCommand->m_X = x;
                pCommand->m_Y = y;
                pCommand->m_Z = z;
            }
            return pCommand;
        }

        inline uint32_t GetX() const { return m_X; }
        inline uint32_t GetY() const { return m_Y; }
        inline uint32_t GetZ() const { return m_Z; }
    };

    /***********************************************************************************\

    Class:
        DispatchIndirectCommand

    Description:
        Wraps:
        - vkCmdDispatchIndirect

    \***********************************************************************************/
    class DispatchIndirectCommand : public ComputeCommand
    {
        VkBuffer m_Buffer;
        VkDeviceSize m_Offset;

    public:
        using ComputeCommand::ComputeCommand;
        using BaseCommandType = ComputeCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<DispatchIndirectCommand> Create( VkBuffer buffer, VkDeviceSize offset )
        {
            auto pCommand = std::make_shared<DispatchIndirectCommand>( CommandId::eDispatchIndirect );
            if( pCommand )
            {
                pCommand->m_Buffer = buffer;
                pCommand->m_Offset = offset;
            }
            return pCommand;
        }

        inline VkBuffer GetBuffer() const { return m_Buffer; }
        inline VkDeviceSize GetOffset() const { return m_Offset; }
    };

    /***********************************************************************************\

    Class:
        TransferCommand

    Description:
        Meta-command.

    \***********************************************************************************/
    class TransferCommand : public InternalPipelineCommand
    {
    public:
        using InternalPipelineCommand::InternalPipelineCommand;
        using BaseCommandType = InternalPipelineCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }
    };

    /***********************************************************************************\

    Class:
        CopyBufferCommand

    Description:
        Wraps:
        - vkCmdCopyBuffer

    \***********************************************************************************/
    class CopyBufferCommand : public TransferCommand
    {
        VkBuffer m_SrcBuffer;
        VkBuffer m_DstBuffer;

    public:
        using TransferCommand::TransferCommand;
        using BaseCommandType = TransferCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<CopyBufferCommand> Create( VkBuffer srcBuffer, VkBuffer dstBuffer )
        {
            auto pCommand = std::make_shared<CopyBufferCommand>( CommandId::eCopyBuffer, InternalPipelineId::eCopyBuffer );
            if( pCommand )
            {
                pCommand->m_SrcBuffer = srcBuffer;
                pCommand->m_DstBuffer = dstBuffer;
            }
            return pCommand;
        }

        inline VkBuffer GetSrcBuffer() const { return m_SrcBuffer; }
        inline VkBuffer GetDstBuffer() const { return m_DstBuffer; }
    };

    /***********************************************************************************\

    Class:
        CopyBufferToImageCommand

    Description:
        Wraps:
        - vkCmdCopyBufferToImage

    \***********************************************************************************/
    class CopyBufferToImageCommand : public TransferCommand
    {
        VkBuffer m_SrcBuffer;
        VkImage m_DstImage;

    public:
        using TransferCommand::TransferCommand;
        using BaseCommandType = TransferCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<CopyBufferToImageCommand> Create( VkBuffer srcBuffer, VkImage dstImage )
        {
            auto pCommand = std::make_shared<CopyBufferToImageCommand>( CommandId::eCopyBufferToImage, InternalPipelineId::eCopyBufferToImage );
            if( pCommand )
            {
                pCommand->m_SrcBuffer = srcBuffer;
                pCommand->m_DstImage = dstImage;
            }
            return pCommand;
        }

        inline VkBuffer GetSrcBuffer() const { return m_SrcBuffer; }
        inline VkImage GetDstImage() const { return m_DstImage; }
    };

    /***********************************************************************************\

    Class:
        CopyImageCommand

    Description:
        Wraps:
        - vkCmdCopyImage

    \***********************************************************************************/
    class CopyImageCommand : public TransferCommand
    {
        VkImage m_SrcImage;
        VkImage m_DstImage;

    public:
        using TransferCommand::TransferCommand;
        using BaseCommandType = TransferCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<CopyImageCommand> Create( VkImage srcImage, VkImage dstImage )
        {
            auto pCommand = std::make_shared<CopyImageCommand>( CommandId::eCopyImage, InternalPipelineId::eCopyImage );
            if( pCommand )
            {
                pCommand->m_SrcImage = srcImage;
                pCommand->m_DstImage = dstImage;
            }
            return pCommand;
        }

        inline VkImage GetSrcImage() const { return m_SrcImage; }
        inline VkImage GetDstImage() const { return m_DstImage; }
    };

    /***********************************************************************************\

    Class:
        CopyImageToBufferCommand

    Description:
        Wraps:
        - vkCmdCopyImageToBuffer

    \***********************************************************************************/
    class CopyImageToBufferCommand : public TransferCommand
    {
        VkImage m_SrcImage;
        VkBuffer m_DstBuffer;

    public:
        using TransferCommand::TransferCommand;
        using BaseCommandType = TransferCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<CopyImageToBufferCommand> Create( VkImage srcImage, VkBuffer dstBuffer )
        {
            auto pCommand = std::make_shared<CopyImageToBufferCommand>( CommandId::eCopyImageToBuffer, InternalPipelineId::eCopyImageToBuffer );
            if( pCommand )
            {
                pCommand->m_SrcImage = srcImage;
                pCommand->m_DstBuffer = dstBuffer;
            }
            return pCommand;
        }

        inline VkImage GetSrcImage() const { return m_SrcImage; }
        inline VkBuffer GetDstBuffer() const { return m_DstBuffer; }
    };

    /***********************************************************************************\

    Class:
        ResolveImageCommand

    Description:
        Wraps:
        - vkCmdResolveImage

    \***********************************************************************************/
    class ResolveImageCommand : public TransferCommand
    {
        VkImage m_SrcImage;
        VkImage m_DstImage;

    public:
        using TransferCommand::TransferCommand;
        using BaseCommandType = TransferCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<ResolveImageCommand> Create( VkImage srcImage, VkImage dstImage )
        {
            auto pCommand = std::make_shared<ResolveImageCommand>( CommandId::eResolveImage, InternalPipelineId::eResolveImage );
            if( pCommand )
            {
                pCommand->m_SrcImage = srcImage;
                pCommand->m_DstImage = dstImage;
            }
            return pCommand;
        }

        inline VkImage GetSrcImage() const { return m_SrcImage; }
        inline VkImage GetDstImage() const { return m_DstImage; }
    };

    /***********************************************************************************\

    Class:
        BlitImageCommand

    Description:
        Wraps:
        - vkCmdBlitImage

    \***********************************************************************************/
    class BlitImageCommand : public TransferCommand
    {
        VkImage m_SrcImage;
        VkImage m_DstImage;

    public:
        using TransferCommand::TransferCommand;
        using BaseCommandType = TransferCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<BlitImageCommand> Create( VkImage srcImage, VkImage dstImage )
        {
            auto pCommand = std::make_shared<BlitImageCommand>( CommandId::eResolveImage, InternalPipelineId::eResolveImage );
            if( pCommand )
            {
                pCommand->m_SrcImage = srcImage;
                pCommand->m_DstImage = dstImage;
            }
            return pCommand;
        }

        inline VkImage GetSrcImage() const { return m_SrcImage; }
        inline VkImage GetDstImage() const { return m_DstImage; }
    };

    /***********************************************************************************\

    Class:
        UpdateBufferCommand

    Description:
        Wraps:
        - vkCmdUpdateBuffer

    \***********************************************************************************/
    class UpdateBufferCommand : public TransferCommand
    {
        VkBuffer m_DstBuffer;
        VkDeviceSize m_DstOffset;
        VkDeviceSize m_DataSize;
        const void* m_pData;

    public:
        using TransferCommand::TransferCommand;
        using BaseCommandType = TransferCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<UpdateBufferCommand> Create( VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData )
        {
            auto pCommand = std::make_shared<UpdateBufferCommand>( CommandId::eUpdateBuffer, InternalPipelineId::eUpdateBuffer );
            if( pCommand )
            {
                pCommand->m_DstBuffer = dstBuffer;
                pCommand->m_DstOffset = dstOffset;
                pCommand->m_DataSize = dataSize;
                pCommand->m_pData = pData;
            }
            return pCommand;
        }

        inline VkBuffer GetDstBuffer() const { return m_DstBuffer; }
        inline VkDeviceSize GetDstOffset() const { return m_DstOffset; }
        inline VkDeviceSize GetDataSize() const { return m_DataSize; }
        inline const void* GetPData() const { return m_pData; }
    };

    /***********************************************************************************\

    Class:
        FillBufferCommand

    Description:
        Wraps:
        - vkCmdFillBuffer

    \***********************************************************************************/
    class FillBufferCommand : public TransferCommand
    {
        VkBuffer m_DstBuffer;
        VkDeviceSize m_DstOffset;
        VkDeviceSize m_Size;
        uint32_t m_Data;

    public:
        using TransferCommand::TransferCommand;
        using BaseCommandType = TransferCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<FillBufferCommand> Create( VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data )
        {
            auto pCommand = std::make_shared<FillBufferCommand>( CommandId::eFillBuffer, InternalPipelineId::eFillBuffer );
            if( pCommand )
            {
                pCommand->m_DstBuffer = dstBuffer;
                pCommand->m_DstOffset = dstOffset;
                pCommand->m_Size = size;
                pCommand->m_Data = data;
            }
            return pCommand;
        }

        inline VkBuffer GetDstBuffer() const { return m_DstBuffer; }
        inline VkDeviceSize GetDstOffset() const { return m_DstOffset; }
        inline VkDeviceSize GetDataSize() const { return m_Size; }
        inline uint32_t GetData() const { return m_Data; }
    };

    /***********************************************************************************\

    Class:
        ClearCommand

    Description:
        Meta-command.

    \***********************************************************************************/
    class ClearCommand : public TransferCommand
    {
    public:
        using TransferCommand::TransferCommand;
        using BaseCommandType = TransferCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }
    };

    /***********************************************************************************\

    Class:
        ClearAttachmentsCommand

    Description:
        Wraps:
        - vkCmdClearAttachments

    \***********************************************************************************/
    class ClearAttachmentsCommand : public ClearCommand
    {
        std::vector<VkClearAttachment> m_Attachments;
        std::vector<VkClearRect> m_Rects;

    public:
        using ClearCommand::ClearCommand;
        using BaseCommandType = ClearCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<ClearAttachmentsCommand> Create( uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects )
        {
            auto pCommand = std::make_shared<ClearAttachmentsCommand>( CommandId::eClearAttachments, InternalPipelineId::eClearAttachments );
            if( pCommand )
            {
                pCommand->m_Attachments.reserve( attachmentCount );

                for( uint32_t i = 0; i < attachmentCount; ++i )
                {
                    pCommand->m_Attachments.push_back( pAttachments[ i ] );
                }

                pCommand->m_Rects.reserve( rectCount );

                for( uint32_t i = 0; i < rectCount; ++i )
                {
                    pCommand->m_Rects.push_back( pRects[ i ] );
                }
            }
            return pCommand;
        }

        inline uint32_t GetAttachmentCount() const { return static_cast<uint32_t>(m_Attachments.size()); }
        inline const VkClearAttachment* GetPAttachments() const { return m_Attachments.data(); }
        inline uint32_t GetRectCount() const { return static_cast<uint32_t>(m_Rects.size()); }
        inline const VkClearRect* GetPRects() const { return m_Rects.data(); }
    };

    /***********************************************************************************\

    Class:
        ClearColorImageCommand

    Description:
        Wraps:
        - vkCmdClearColorImage

    \***********************************************************************************/
    class ClearColorImageCommand : public ClearCommand
    {
        VkImage m_Image;
        VkImageLayout m_ImageLayout;
        VkClearColorValue m_Color;
        std::vector<VkImageSubresourceRange> m_Ranges;

    public:
        using ClearCommand::ClearCommand;
        using BaseCommandType = ClearCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<ClearColorImageCommand> Create( VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges )
        {
            auto pCommand = std::make_shared<ClearColorImageCommand>( CommandId::eClearColorImage, InternalPipelineId::eClearColorImage );
            if( pCommand )
            {
                pCommand->m_Image = image;
                pCommand->m_ImageLayout = imageLayout;
                pCommand->m_Color = *pColor;

                pCommand->m_Ranges.reserve( rangeCount );

                for( uint32_t i = 0; i < rangeCount; ++i )
                {
                    pCommand->m_Ranges.push_back( pRanges[ i ] );
                }
            }
            return pCommand;
        }

        inline VkImage GetImage() const { return m_Image; }
        inline VkImageLayout GetImageLayout() const { return m_ImageLayout; }
        inline VkClearColorValue GetColor() const { return m_Color; }
        inline uint32_t GetRangeCount() const { return static_cast<uint32_t>(m_Ranges.size()); }
        inline const VkImageSubresourceRange* GetPRanges() const { return m_Ranges.data(); }
    };

    /***********************************************************************************\

    Class:
        ClearDepthStencilImageCommand

    Description:
        Wraps:
        - vkCmdClearDepthStencilImage

    \***********************************************************************************/
    class ClearDepthStencilImageCommand : public ClearCommand
    {
        VkImage m_Image;
        VkImageLayout m_ImageLayout;
        VkClearDepthStencilValue m_DepthStencil;
        std::vector<VkImageSubresourceRange> m_Ranges;

    public:
        using ClearCommand::ClearCommand;
        using BaseCommandType = ClearCommand;

        inline void Accept( CommandVisitor& visitor ) override
        {
            visitor.Visit( std::static_pointer_cast<std::remove_reference_t<decltype(*this)>>(shared_from_this()) );
        }

        inline static std::shared_ptr<ClearDepthStencilImageCommand> Create( VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges )
        {
            auto pCommand = std::make_shared<ClearDepthStencilImageCommand>( CommandId::eClearDepthStencilImage, InternalPipelineId::eClearDepthStencilImage );
            if( pCommand )
            {
                pCommand->m_Image = image;
                pCommand->m_ImageLayout = imageLayout;
                pCommand->m_DepthStencil = *pDepthStencil;

                pCommand->m_Ranges.reserve( rangeCount );

                for( uint32_t i = 0; i < rangeCount; ++i )
                {
                    pCommand->m_Ranges.push_back( pRanges[ i ] );
                }
            }
            return pCommand;
        }

        inline VkImage GetImage() const { return m_Image; }
        inline VkImageLayout GetImageLayout() const { return m_ImageLayout; }
        inline VkClearDepthStencilValue GetDepthStencil() const { return m_DepthStencil; }
        inline uint32_t GetRangeCount() const { return static_cast<uint32_t>(m_Ranges.size()); }
        inline const VkImageSubresourceRange* GetPRanges() const { return m_Ranges.data(); }
    };

    /***********************************************************************************\

    Function:
        Command::Copy

    Description:
        Creates copy of the command.

    \***********************************************************************************/
    inline std::shared_ptr<Command> Command::Copy( std::shared_ptr<const Command> pCommand )
    {
        switch( pCommand->GetId() )
        {
        case CommandId::eCommandGroup:
            return std::make_shared<CommandGroup>(
                *std::static_pointer_cast<const CommandGroup>(pCommand) );

        case CommandId::eRenderPassCommandGroup:
            return std::make_shared<RenderPassCommandGroup>(
                *std::static_pointer_cast<const RenderPassCommandGroup>(pCommand) );

        case CommandId::eSubpassCommandGroup:
            return std::make_shared<SubpassCommandGroup>(
                *std::static_pointer_cast<const SubpassCommandGroup>(pCommand) );

        case CommandId::ePipelineCommandGroup:
            return std::make_shared<PipelineCommandGroup>(
                *std::static_pointer_cast<const PipelineCommandGroup>(pCommand) );

        #define ProfilerCommand( Cmd ) \
        case CommandId::e##Cmd: \
            return std::make_shared<Cmd##Command>( \
                *std::static_pointer_cast<const Cmd##Command>(pCommand) );

        #include "profiler_commands.inl"
        }
    }

    /***********************************************************************************\

    Function:
        Command::Name

    Description:
        Returns name of the command.

    \***********************************************************************************/
    inline const char* Command::Name( std::shared_ptr<const Command> pCommand )
    {
        const char* commandNames[] = {
            #define ProfilerCommand( Cmd ) "vkCmd"#Cmd,
            #include "profiler_commands.inl"
        };
        return commandNames[ static_cast<size_t>(pCommand->GetId()) ];
    }

    /***********************************************************************************\

    Function:
        CommandVisitor::Visit

    Description:
        Performs operation on the command.

    \***********************************************************************************/
    inline void CommandVisitor::Visit( std::shared_ptr<CommandGroup> pCommand )
    {
        Visit( std::static_pointer_cast<CommandGroup::BaseCommandType>(pCommand) );
    }

    inline void CommandVisitor::Visit( std::shared_ptr<RenderPassCommandGroup> pCommand )
    {
        Visit( std::static_pointer_cast<RenderPassCommandGroup::BaseCommandType>(pCommand) );
    }

    inline void CommandVisitor::Visit( std::shared_ptr<SubpassCommandGroup> pCommand )
    {
        Visit( std::static_pointer_cast<SubpassCommandGroup::BaseCommandType>(pCommand) );
    }

    inline void CommandVisitor::Visit( std::shared_ptr<PipelineCommandGroup> pCommand )
    {
        Visit( std::static_pointer_cast<PipelineCommandGroup::BaseCommandType>(pCommand) );
    }

    inline void CommandVisitor::Visit( std::shared_ptr<DebugCommand> pCommand )
    {
        Visit( std::static_pointer_cast<DebugCommand::BaseCommandType>(pCommand) );
    }

    inline void CommandVisitor::Visit( std::shared_ptr<InternalPipelineCommand> pCommand )
    {
        Visit( std::static_pointer_cast<InternalPipelineCommand::BaseCommandType>(pCommand) );
    }

    inline void CommandVisitor::Visit( std::shared_ptr<PipelineCommand> pCommand )
    {
        Visit( std::static_pointer_cast<PipelineCommand::BaseCommandType>(pCommand) );
    }

    inline void CommandVisitor::Visit( std::shared_ptr<GraphicsCommand> pCommand )
    {
        Visit( std::static_pointer_cast<GraphicsCommand::BaseCommandType>(pCommand) );
    }

    inline void CommandVisitor::Visit( std::shared_ptr<ComputeCommand> pCommand )
    {
        Visit( std::static_pointer_cast<ComputeCommand::BaseCommandType>(pCommand) );
    }

    inline void CommandVisitor::Visit( std::shared_ptr<TransferCommand> pCommand )
    {
        Visit( std::static_pointer_cast<TransferCommand::BaseCommandType>(pCommand) );
    }

    inline void CommandVisitor::Visit( std::shared_ptr<ClearCommand> pCommand )
    {
        Visit( std::static_pointer_cast<ClearCommand::BaseCommandType>(pCommand) );
    }

    #define ProfilerCommand( Cmd ) \
    inline void CommandVisitor::Visit( std::shared_ptr<Cmd##Command> pCommand ) \
    { \
        Visit( std::static_pointer_cast<Cmd##Command::BaseCommandType>( pCommand ) ); \
    }
    #include "profiler_commands.inl"
}
