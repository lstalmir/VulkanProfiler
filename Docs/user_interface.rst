User Interface
==============

Once the layer has successfully attached, and :confval:`output` is set to ``overlay``, an overlay will be displayed on top of the image rendered by the application.
This section describes all features it offers.

Most of the windows and tabs in the overlay can be freely moved and resized to fit the user's needs.

.. figure:: /static/img/layer-ui.jpg
    :width: 90%
    :align: center

    Profiling layer's overlay window displaying rendering performance overview.

Performance graph
-----------------

    The graph presents the data in form of blocks that highlight the highest contributors to the frame's performance.
    The higher the block, the more contribution it has.
    This behavior can be disabled by selecting ``Constant`` height in *Height* combo box.

    Clicking on a column of the performance graph will jump to and highlight the selected region in the :ref:`Frame browser` for a short time.

    By default the graph presents ``Render passes`` (e.g., VkRenderPass objects).
    The setting can be changed with *Histogram Groups* combo box.

    Performance graph doesn't include any idling between presented regions.
    Such idling is presented in :ref:`Queue utilization graph`, but it can be enabled in this graph too by checking *Show idle* checkbox.

    When profiling multiple frames, the graph shows all frames collected by the overlay.
    It can become unreadable if the number of frames is high, even when ``Frames`` group is selected.
    *Show active frame* checkbox allows to limit the graph to the selected frame only.

Frames list
-----------

    When multiple frames are collected, this tab allows to quickly jump to a selected frame.

    If the layer is configured to delimit the data by command buffer submissions, this window will be titled **Submissions**, but its functionality will remain the same.

Frame browser
-------------

    The frame data is displayed in a form of a tree in the following hierarchy:

    - vkQueueSubmit - The API call that submitted the command buffers in batches of VkSubmitInfos.
    - VkSubmitInfo - A structure that contains a list of the submitted command buffers.
    - Command buffers - VkCommandBuffer objects that are recorded lists of commands.    
    - Render passes - VkRenderPass objects, dynamic rendering passes, compute or transfer operations passes.
    - Pipelines - VkPipeline objects defining state used for the subsequent commands.
    - Drawcalls - Commands that are executed on the GPU, e.g., vkCmdDraw, vkCmdDispatch.

    On the right side of each tree node there is a measured time of the entire node.
    The node's background also indicates its contribution to the total frame time.

    .. rubric:: Context menus

    Command buffers and pipelines also offer a context menu with additional options.

        *Command buffer context menu*

        - **Show performance counters**: Shows the counters collected for the selected command buffer in the :ref:`Performance counters` tab.

        *Pipeline context menu*

        - **Inspect**: Selects the pipeline for inspection in :ref:`Inspector` window.
        - **Copy name**: Copies the name of the pipeline to the system's clipboard.

Queue utilization graph
-----------------------

    This window shows an overview of queue utilization and synchronization semaphores.

    It is useful for profiling applications using multiple queues, where proper synchronization between them is crutial in fully leveraging GPU capabilities.

    Semaphores are marked with triangles above the timelines.
    Selecting a semaphore signal or wait event will also highlight other occurrences of the selected semaphores in the graph, helping in analysis of dependencies between the queues.
    Clicking again on a selected mark deselects it.

Top pipelines
-------------

    pass

Performance counters
--------------------

    pass

Memory
------

    pass

Inspector
---------

    Pipeline inspector allows to check the details of the profiled pipeline state.

    It provides contents of all core graphics pipeline create info structures used to create the pipeline, and the shaders used by the pipeline.

Shader inspector
^^^^^^^^^^^^^^^^

    The shader inspector is part of pipeline inspector tab.

    When a shader stage is selected in the combo box at the top of the window, the inspector will display the SPIR-V disassembly of the shader, as well as a high-level representation in GLSL or HLSL language obtained with spirv-cross.
    If the shader has the actual source code information embedded, it will be displayed instead of spirv-cross disassembly.

    SPIR-V disassembly viewer has built-in SPIR-V reference with descriptions of most of opcodes.
    The documentation is downloaded from https://registry.khronos.org/SPIR-V/specs/unified1/SPIRV.html (links to the document used are also present in the GUI below the description of an opcode).

    If pipeline executable properties extension is enabled, either by the layer or by the profiled application, the shader inspector will also display pipeline executable statistics and internal representations associated with the selected shader stage.

    All representations of the shader (SPIR-V disassembly, GLSL, HLSL and internal representations) can be saved to a file for further analysis.

    At the top right corner of the inspector, the shader module identifier is printed, and can be used to correlate the shader with other tools that also use those identifiers.

Statistics
----------

    pass

Settings
--------

    pass
