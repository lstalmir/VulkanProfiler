#ifndef ProfilerCommand
#error ProfilerCommand macro must be defined before including profiler_commands.inl
#endif

// All supported vulkan commands
ProfilerCommand( BeginDebugLabel )
ProfilerCommand( EndDebugLabel )
ProfilerCommand( InsertDebugLabel )
ProfilerCommand( BeginRenderPass )
ProfilerCommand( EndRenderPass )
ProfilerCommand( NextSubpass )
ProfilerCommand( BindPipeline )
ProfilerCommand( PipelineBarrier )
ProfilerCommand( ExecuteCommands )
ProfilerCommand( Draw )
ProfilerCommand( DrawIndexed )
ProfilerCommand( DrawIndirect )
ProfilerCommand( DrawIndexedIndirect )
ProfilerCommand( DrawIndirectCount )
ProfilerCommand( DrawIndexedIndirectCount )
ProfilerCommand( Dispatch )
ProfilerCommand( DispatchIndirect )
ProfilerCommand( CopyBuffer )
ProfilerCommand( CopyBufferToImage )
ProfilerCommand( CopyImage )
ProfilerCommand( CopyImageToBuffer )
ProfilerCommand( ResolveImage )
ProfilerCommand( BlitImage )
ProfilerCommand( UpdateBuffer )
ProfilerCommand( FillBuffer )
ProfilerCommand( ClearAttachments )
ProfilerCommand( ClearColorImage )
ProfilerCommand( ClearDepthStencilImage )

// Undefine the macro before including this file next time
#undef ProfilerCommand
