#ifndef ProfilerPipeline
#error ProfilerPipeline macro must be defined before including profiler_pipelines.inl
#endif

// Internal pipelines
ProfilerPipeline( BeginRenderPass )
ProfilerPipeline( EndRenderPass )
ProfilerPipeline( NextSubpass )
ProfilerPipeline( PipelineBarrier )
ProfilerPipeline( CopyBuffer )
ProfilerPipeline( CopyBufferToImage )
ProfilerPipeline( CopyImage )
ProfilerPipeline( CopyImageToBuffer )
ProfilerPipeline( ResolveImage )
ProfilerPipeline( BlitImage )
ProfilerPipeline( UpdateBuffer )
ProfilerPipeline( FillBuffer )
ProfilerPipeline( ClearAttachments )
ProfilerPipeline( ClearColorImage )
ProfilerPipeline( ClearDepthStencilImage )

// Undefine the macro before including this file next time
#undef ProfilerPipeline
