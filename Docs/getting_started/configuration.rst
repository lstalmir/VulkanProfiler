Configuration
=============

The layer can be configured to handle more specific use cases.

For list of all available configuration options, see :doc:`options` section.

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

.. code::

    VK_LOADER_LAYERS_ENABLE = *profiler_unified

The old way of enabling the layers requires providing the full name of the layer.

.. code::

    VK_INSTANCE_LAYERS = VK_LAYER_profiler_unified

The variable is still supported, but it has been deprecated and will be removed in the future.

Once enabled, the layer can be configured using the environment variables as well. In such case, all options have to be prefixed with ``VKPROF_``, for example:

.. code::

    VKPROF_sampling_mode = pipeline
    VKPROF_frame_delimiter = present
    VKPROF_frame_count = 3

Configuration files
-------------------

Another way to set the layer options is to use configuration files.

This method does not enable the layer, so one of the approaches described above has to be taken before using the files.

The layer supports both the standarized vk_layer_settings.txt file, and a custom VkLayer_profiler_layer.ini, which is deprecated.
