Installation
============

Prebuilt binaries can be downloaded from `GitHub releases page <https://github.com/lstalmir/VulkanProfiler/releases>`_.

Once downloaded or built, the dynamic-link library with the layer must be installed so that the Vulkan loader knows where to look for it. The process is different for Windows and Linux-based systems, and it's described in detail in `Vulkan Layer Interface to the Loader documentation <https://github.com/KhronosGroup/Vulkan-Loader/blob/main/docs/LoaderLayerInterface.md>`_.

.. note::

    Installation of the layer can be avoided. To use the layer without installation set ``VK_LAYER_PATH`` environment variable to the directory containing layer files.

Windows
-------

On Windows systems the layers are installed in the registry. The profiling layer can be registered in one of the following locations:

- HKLM\\Software\\Khronos\\Vulkan\\ExplicitLayers
- HKCU\\Software\\Khronos\\Vulkan\\ExplicitLayers

The first location includes paths to layer manifest files available for all users of the computer (HKLM - HKEY_LOCAL_MACHINE). The second one lists layers installed for the currently logged user (HKCU - HKEY_CURRENT_USER).

Once the location has been selected, a new DWORD value must be added, with its name being an absolute path to the layer's JSON manifest (e.g., "C:\\layers\\VkLayer_profiler_layer.json") and its value set to 0.

Linux
-----

The following instructions have been prepared for Debian-based distributions. Locations of Vulkan layers may be different on other distros.

Vulkan layers on Linux are stored in predefined directories. To install the layer, simply copy the .so and .json files from the package to one of those directories.

- $HOME/.local/share/vulkan/explicit_layer.d
- /usr/local/share/vulkan/explicit_layer.d
- /usr/share/vulkan/explicit_layer.d
- /etc/vulkan/explicit_layer.d

Apart from the libVkLayer_profiler_layer.so that is the Vulkan layer library, there layer also requires libigdmd to support Intel performance counters on Linux. This library can be found either in the package, next to libVkLayer_profiler_layer.so, or in "<REPOSITORY_DIR>/External/metrics-discovery/dump/linux64/<CONFIG>/metrics_discovery" directory when building the layer from source.
