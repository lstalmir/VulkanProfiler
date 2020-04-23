#include "profiler_data_aggregator.h"
#include "profiler_helpers.h"
#include <unordered_set>

namespace Profiler
{
    /***********************************************************************************\

    Function:
        AppendData

    Description:
        Add submit data to the aggregator. The data must be ready for reading.

    \***********************************************************************************/
    void ProfilerDataAggregator::AppendData( const ProfilerSubmitData& submit )
    {
        #if 0
        const auto submitTuples = CollectShaderTuples( submit );

        ProfilerSubmitData* pBestMatch = nullptr;
        float bestMatchPercentage = std::numeric_limits<float>::max();

        // Find command buffer which uses the same pipelines
        for( auto& existingSubmit : m_AggregatedData )
        {
            const auto existingSubmitTuples = CollectShaderTuples( existingSubmit );

            // Find number of differences between sets
            const float diffPercentage =
                static_cast<float>((submitTuples - existingSubmitTuples).size()) / 
                static_cast<float>(existingSubmitTuples.size());

            if( (diffPercentage / existingSubmitTuples.size()) < bestMatchPercentage )
            {
                // Update candidate
                pBestMatch = &existingSubmit;
                bestMatchPercentage = diffPercentage;
            }
        }

        // Allow up to 10% difference between similar submits
        if( pBestMatch && bestMatchPercentage < 0.1f )
        {
            for( const auto& commandBuffer : submit.m_CommandBuffers )
            {
                // Get tuples in current command buffer
                const auto commandBufferTuples = CollectShaderTuples( commandBuffer );

                // Find existing command buffer with the same tuples

            }

        }
        #endif

        m_AggregatedData.push_back( submit );
    }

    /***********************************************************************************\

    Function:
        Reset

    Description:
        Clear results map.

    \***********************************************************************************/
    void ProfilerDataAggregator::Reset()
    {
        m_AggregatedData.clear();
    }

    /***********************************************************************************\

    Function:
        GetAggregatedData

    Description:
        Merge similar command buffers and return aggregated results.
        Prepare aggregator for the next profiling run.

    \***********************************************************************************/
    ProfilerAggregatedData ProfilerDataAggregator::GetAggregatedData()
    {
        // Application may use multiple command buffers to perform the same tasks
        MergeCommandBuffers();

        auto aggregatedSubmits = m_AggregatedData;
        auto aggregatedPipelines = CollectTopPipelines();

        ProfilerAggregatedData aggregatedData;
        aggregatedData.m_Stats.Clear();
        aggregatedData.m_Submits = { aggregatedSubmits.begin(), aggregatedSubmits.end() };
        aggregatedData.m_TopPipelines = { aggregatedPipelines.begin(), aggregatedPipelines.end() };

        for( const auto& submit : aggregatedData.m_Submits )
        {
            for( const auto& commandBuffer : submit.m_CommandBuffers )
            {
                if( commandBuffer.m_Stats.m_BeginTimestamp != 0 )
                {
                    aggregatedData.m_Stats.m_BeginTimestamp = commandBuffer.m_Stats.m_BeginTimestamp;
                    break;
                }
            }
        }

        // Collect per-frame stats
        for( const auto& pipeline : aggregatedPipelines )
        {
            aggregatedData.m_Stats.Add( pipeline.m_Stats );
        }

        return aggregatedData;
    }

    /***********************************************************************************\

    Function:
        MergeCommandBuffers

    Description:
        Find similar command buffers and merge results.

    \***********************************************************************************/
    void ProfilerDataAggregator::MergeCommandBuffers()
    {
    }

    #if 0
    /***********************************************************************************\

    Function:
        CollectShaderTuples

    Description:
        Get all shader tuples referenced in the submit.

    \***********************************************************************************/
    std::unordered_set<ProfilerShaderTuple> ProfilerDataAggregator::CollectShaderTuples( const ProfilerSubmitData& submit ) const
    {
        std::unordered_set<ProfilerShaderTuple> tuples;

        // Assuming relatively low number of command buffers in single submit, we're O(n) here
        for( const auto& commandBuffer : submit.m_CommandBuffers )
        {
            for( const auto& pipelineDrawCount : commandBuffer.m_PipelineDrawCount )
            {
                // tuples is a set so all duplicates will be removed here
                tuples.insert( pipelineDrawCount.first.m_ShaderTuple );
            }
        }

        return tuples;
    }
    #endif


    std::list<ProfilerPipeline> ProfilerDataAggregator::CollectTopPipelines()
    {
        std::unordered_set<ProfilerPipeline> aggregatedPipelines;

        for( const auto& submit : m_AggregatedData )
        {
            for( const auto& commandBuffer : submit.m_CommandBuffers )
            {
                for( const auto& renderPass : commandBuffer.m_Subregions )
                {
                    for( const auto& subpass : renderPass.m_Subregions )
                    {
                        for( const auto& pipeline : subpass.m_Subregions )
                        {
                            ProfilerPipeline aggregatedPipeline = pipeline;

                            auto it = aggregatedPipelines.find( pipeline );
                            if( it != aggregatedPipelines.end() )
                            {
                                aggregatedPipeline = *it;
                                aggregatedPipeline.m_Stats.Add( pipeline.m_Stats );

                                aggregatedPipelines.erase( it );
                            }

                            // Clear values which don't make sense after aggregation
                            aggregatedPipeline.m_Handle = pipeline.m_Handle;
                            aggregatedPipeline.m_Subregions.clear();

                            aggregatedPipelines.insert( aggregatedPipeline );
                        }
                    }
                }
            }
        }
        
        // Sort by time
        std::list<ProfilerPipeline> pipelines = { aggregatedPipelines.begin(), aggregatedPipelines.end() };

        pipelines.sort( []( const ProfilerPipeline& a, const ProfilerPipeline& b )
            {
                return a.m_Stats.m_TotalTicks > b.m_Stats.m_TotalTicks;
            } );

        return pipelines;
    }
}
