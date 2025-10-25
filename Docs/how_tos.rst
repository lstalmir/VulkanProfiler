How-tos
=======

This section contains useful information and recommendations on using the profiling layer.

Profiling offscreen applications
--------------------------------

The layer initially supported only graphics applications, where presentation to the screen was required to browse and read the results.
However, recent releases also support offscreen and compute-only workloads.
Such applications may run in console with no GUI implementation, so in order to read the data the profiler has to be configured to write it to a file.

Recommended configuration for collecting API-command-level data has been attached below.

.. tabs::
    .. code-tab:: batch Windows

        set VK_LOADER_LAYERS_ENABLE=*profiler_unified
        set VKPROF_output=trace
        set VKPROF_enable_threading=false
        set VKPROF_frame_delimiter=submit
        set VKPROF_frame_count=0

    .. code-tab:: bash Linux

        export VK_LOADER_LAYERS_ENABLE="*profiler_unified"
        export VKPROF_output="trace"
        export VKPROF_enable_threading="false"
        export VKPROF_frame_delimiter="submit"
        export VKPROF_frame_count=0

The first variable enables injection of the profiling layer into the application.
For more information on configuring the layer see :doc:`configuration </getting_started/configuration>`.

The two next variables change the data output to an Event Trace file.
Path to this file can be set with an optional :confval:`VKPROF_output_trace_file <output_trace_file>` variable.
By default the file will be named after the profiled application with a timestamp and PID to differentiate it between subsequent runs.

The configuration also disables background threading, which has the most impact on GUI applications and helps to reduce latency and GPU utilization gaps between frames.
In offscreen applications this feature is not required and keeping it enabled may lead to missing data if the application quits early.

The frame delimiter is changed to per-submit in order to keep grouping of operations - with the default per-present mode all commands would be packed into a single Frame #0.

The last option disables the frame count limit to record the entire application profile instead of the first frame only.
It can also be set to a different, greater than zero, value to limit the size of the collected data.

With all these options set, the layer should attach into the Vulkan application and produce a file that can be then opened either in Chromium-based browsers (chrome://tracing or edge://tracing) or in e.g., `ui.perfetto.dev <https://ui.perfetto.dev/>`_.
