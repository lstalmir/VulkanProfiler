Configuration
=============

The layer can be configured to handle more specific use cases.

For list of all available configuration options, see :doc:`/options` section.

Vulkan Configurator
-------------------

The easiest way to configure the layer is to use `VkConfig <https://vulkan.lunarg.com/doc/view/1.3.268.0/windows/vkconfig.html>`_ tool, which is installed as a part of the Vulkan SDK. It is a GUI application that lists all available layers and their options, allowing to quickly set up the layer chain.

.. image:: /static/img/vulkan-configurator.jpg

.. topic:: Creating a new configuration

    On the first launch the tool contains only built-in layer presets. Follow the steps below to add a new preset with the profiling layer.

    1. Create a new configuration in the top left window (the option is located in the context menu).

        .. image:: /static/img/new-configuration.jpg
            :scale: 75%
            :align: center

        |

    2. Select the new configuration and enable the layer in the list below.

        .. image:: /static/img/enable-layer.jpg
            :scale: 75%
            :align: center

        |

    If the layer doesn't appear in the list, make sure it is correctly :doc:`installed <installation>`.

Environment variables
---------------------

The layer can also be enabled by setting environment variables before launching an application. Depending on the version of the Vulkan loader installed on the system, there are 2 variables available.

New loaders provide a variable that allows to use a glob expression instead of the full name of the layer, so the command can be simplified a bit.

.. tabs::
    .. code-tab:: batch Windows

        set VK_LOADER_LAYERS_ENABLE=*profiler_unified

    .. code-tab:: bash Linux

        export VK_LOADER_LAYERS_ENABLE="*profiler_unified"

The old way of enabling the layers requires providing the full name of the layer.

.. tabs::
    .. code-tab:: batch Windows

        set VK_INSTANCE_LAYERS=VK_LAYER_profiler_unified

    .. code-tab:: bash Linux

        export VK_INSTANCE_LAYERS="VK_LAYER_profiler_unified"

.. deprecated:: 1.3.234
    The variable is still supported by the newer Vulkan loaders, but it has been deprecated and will be removed in the future. Prefer using ``VK_LOADER_LAYERS_ENABLE`` instead.

Once enabled, the layer can be configured using the environment variables as well. In such case, all options have to be prefixed with ``VKPROF_``, for example:

.. tabs::
    .. code-tab:: batch Windows

        set VKPROF_sampling_mode=pipeline
        set VKPROF_frame_delimiter=present
        set VKPROF_frame_count=3

    .. code-tab:: bash Linux

        export VKPROF_sampling_mode="pipeline"
        export VKPROF_frame_delimiter="present"
        export VKPROF_frame_count=3

Configuration file
------------------

.. admonition:: Warning

    This method does not enable the layer, so one of the approaches described above has to be taken before using the file.

The profiling layer also supports a standarized vk_layer_settings.txt file.

The file has to be placed in the profiled application's directory. It can be used to configure other layers as well, and has higher priority than options set by the Vulkan Configurator, so application specific settings can be applied this way.

.. code-block::
    :caption: vk_layer_settings.txt

    profiler_unified.sampling_mode = pipeline
    profiler_unified.frame_delimiter = present
    profiler_unified.frame_count = 3
