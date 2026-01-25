Options
=======

This section describes in detail all available configuration options exposed by the profiling layer.

.. confval:: output
    :type: enum
    :default: overlay

    Type of the output used for presenting the profiling data. By default the layer creates an overlay in the application's window and intercepts incoming input events when the mouse cursor is on top of the overlay. This is the most convenient output that also supports all features of the layer.

    The following outputs are available:

    .. glossary::

        overlay
            An overlay is displayed in the application's window. It intercepts incoming input events to allow the user to interact with it and browse through the profiling data. The overlay displays live performance data, which can be paused and resumed at any time. It also provides useful widgets showing post-processed data, such as command queue utilization graphs, top pipelines list, drawcall statistics, shader code disassembly, and more.

        trace
            The profiling data is written directly to a trace file in the JSON format. It is useful when profiling applications that don't present the rendered image in a window, such as command line applications and compute-only workloads. The data is limited to timestamp query results only.

.. confval:: output_trace_file
    :type: path
    :default: empty

    When :confval:`output` is set to **trace**, this option allows to override the default file name and location of the output trace file.

.. confval:: enable_memory_profiling
    :type: bool
    :default: true

    Enables tracking of allocations and resources created by the application. The data can be used to investigate potential memory-related issues, like resource placement on a heap or frequent reallocations. It can be disabled to reduce CPU overhead.

.. confval:: enable_performance_query_ext
    :type: enum
    :default: intel

    Enables the selected performance extension and collects detailed performance metrics. The metrics are collected at VkCommandBuffer level and then aggregated into the entire frame. The scope of available metrics depends on the driver and the GPU used for measurements.

    The following options are available:

    .. glossary::

        none
            Disables collection of performance counters.

        intel
            Enables VK_INTEL_performance_query extension and uses Metrics-Discovery library to process the results. The extension provides predefined metrics sets exposed by the Intel graphics driver and does not support custom sets.

        khr
            Enables VK_KHR_performance_query extension. It does not come with any built-in sets, but it provides a list of available counters that user can select from to build custom sets. The layer has a limitation on the selected counters that all of them must be collected in a single query pass.

            .. NOTE::
                The extension allows to collect more in more passes, but that requires resubmission of the command buffers, which could result in unexpected behavior when done from the layer without application's knowledge. Because of that, the layer does not support multi-pass performance queries.

.. confval:: performance_query_mode
    :type: enum
    :default: query

    Controls the type of instrumentation used to collect performance counters.

    Currently the option is available only for Intel performance counters backend, the KHR backend always uses **query** mode.

    The following options are available:

    .. glossary::

        query
            Uses Vulkan queries to instrument VkCommandBuffers. The resulting data is collected at the command buffer level, and then aggregated to the entire frame range.

        stream
            Uses Intel Metrics-Discovery API to sample the performance counters in regular intervals, allowing to analyze the GPU metrics at much finer scale. Depending on the selected sampling period, the data may present the value changes even within the execution of a single command.

            This mode requires the profiler to spawn an internal thread, regardless of :confval:`enable_threading` value, to ensure that the data is collected before the metrics buffer overflows.

.. confval:: performance_stream_timer_period
    :type: int
    :default: 25000

    When :confval:`performance_query_mode` is set to **stream**, this option controls the period of the timer triggering metrics samples.

    The value is expressed in nanoseconds, and is recommended to be kept in range of 5000ns to 1000000ns (1ms). Smaller values may result in buffer overflows, and consequently the data loss, and higher values may expand over multiple frames, which may result in data presentation issues.

    Decreasing the period causes more samples to be generated and processed, so increase in CPU utilization is expected. If the application is CPU-bound or heavily multithreaded, this may impact its overall performance.

.. confval:: default_metrics_set
    :type: string
    :default: RenderBasic

    Name of the default metrics set selected immediately after initialization of the performance query extension. Available only if :confval:`enable_performance_query_ext` is set to **intel**.

.. confval:: enable_pipeline_executable_properties_ext
    :type: bool
    :default: false

    Enables VK_KHR_pipeline_executable_properties extension and collects detailed information about the pipeline shader stages, including shader statistics and internal representations, if available.

    Enabling this option may slightly impact shader compilation time and memory usage.

.. confval:: enable_render_pass_begin_end_profiling
    :type: bool
    :default: false

    When :confval:`sampling_mode` is set to **render_pass** or higher, setting this option will enable profiling of vkCmdBeginRenderPass and vkCmdEndRenderPass commands.

.. confval:: capture_indirect_args
    :type: bool
    :default: false

    When enabled, the layer will capture indirect draw and dispatch arguments from the command buffer. This allows to analyze the actual parameters used for indirect draws and dispatches, which can be useful for debugging and performance analysis.

    The option has significant performance and memory overhead due to additional copying of the indirect argument buffers to the host memory.

.. confval:: set_stable_power_state
    :type: bool
    :default: true

    Uses DirectX12 API to set the GPU to a stable power state before profiling. This can help to reduce variability in performance measurements caused by power state changes during the profiling session. The option is only applicable for Windows platforms only.

.. confval:: enable_threading
    :type: bool
    :default: true

    Enables multithreading support in the profiling layer.

    When enabled, the layer will use multiple threads to process profiling data, which can significantly improve performance and reduce overhead. However, it may cause frequent context switches when the application is heavily multithreaded, which can lead to performance degradation in some cases.

    It is recommended to leave this option enabled, but it can be disabled for specific use cases where multithreading is not beneficial.

.. confval:: sampling_mode
    :type: enum
    :default: drawcall

    Defines the granularity of the profiling data collected by the layer.

    The following sampling modes are available:

    .. glossary::

        drawcall

            The layer will collect profiling data for each draw call and dispatch command. This is the most detailed mode and provides the best insight into the performance of individual rendering commands. However, it may have higher overhead, especially for applications with a large number of draw calls or dispatches.

        pipeline

            The layer will collect profiling data for each pipeline, inserting timestamp queries when a new pipeline state is used. This mode provides a good balance between detail and overhead, allowing to analyze performance of individual pipelines without the overhead of collecting data for each draw call or dispatch command.

        render_pass

            The layer will collect profiling data for each render pass, dynamic rendering pass, compute or transfer commands pass, inserting timestamp queries at boundaries of those passes.

        command_buffer

            The layer will collect profiling data for each command buffer, placing timestamp queries at the beginning and end of each command buffer. This is the most coarse-grained mode supported by the layer.

.. confval:: frame_delimiter
    :type: enum
    :default: present

    Defines the granularity of the frame boundaries used for profiling data.

    The following frame delimiters are available:

    .. glossary::

        present

            The layer will delimit frames at swapchain present operations. This is the default mode and is recommended for most applications that use swapchains for rendering.

        submit

            The layer will delimit frames at command buffer submission operations. This mode is useful for applications that do not use swapchains or each submission should be considered as a separate frame.

.. confval:: frame_count
    :type: int
    :default: 1

    The number of frames to profile. When :confval:`output` is set to **overlay**, this option controls how many frames of profiling data are displayed in the overlay. When :confval:`output` is set to **trace**, this option controls how many frames of profiling data are written to the trace file.

    When this option is set to 0, the layer will profile all frames until the profiling session is stopped manually.

.. confval:: ref_pipelines
    :type: path
    :default: empty

.. confval:: ref_metrics
    :type: path
    :default: empty
