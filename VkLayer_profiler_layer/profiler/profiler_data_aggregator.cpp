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
    std::vector<ProfilerSubmitData> ProfilerDataAggregator::GetAggregatedData()
    {
        // Application may use multiple command buffers to perform the same tasks
        MergeCommandBuffers();

        auto aggregatedData = m_AggregatedData;

        // Prepare aggregator for next profiling run
        Reset();

        return { aggregatedData.begin(), aggregatedData.end() };
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
}
