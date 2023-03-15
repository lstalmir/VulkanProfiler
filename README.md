# Vulkan profiler
This tool is a basic real-time GPU profiling layer for applications using [Vulkan API](https://www.khronos.org/vulkan/). It provides data for performance analysis on all levels, down to single draw calls.

If you have any questions, suggestions or problems, please [create an issue](https://github.com/lstalmir/VulkanProfiler/issues/new).

## Compiling
The layer is available for Windows and Linux. On both platforms CMake is used as the main build management tool.

### Windows
The solution compiles on Visual Studio 2017 or later. Using C++ CMake tools for Windows component is recommended, but generating VS project files using `cmake -G` works fine as well.

`VkLayer_profiler_layer.dll` and `VkLayer_profiler_layer.json` files are the products of the compilation. Both must be placed in the same directory. To use the layer register it in the Windows registry:
```
REG ADD HKCU\Software\Khronos\Vulkan\ExplicitLayers /F /T REG_DWORD /D 0 /V <path-to-json>
```
(Use HKLM instead of HKCU to install for all users)

### Linux
Following packages are required for building on Debian-based systems:
- g++-8.3 (or later)
- cmake-3.8 (or later)
- extra-cmake-modules
- libx11-dev, libxext-dev, libxrandr-dev (for Xlib support)
- libx11-xcb-dev, libxcb-shape0-dev (for XCB support)

To build the layer create "cmake_build" folder in the project root directory and run following command from it:
```
cmake .. && make all
```

To install the layer copy `VkLayer_profiler_layer.so` and `VkLayer_profiler_layer.json` files to `~/.local/share/vulkan/explicit_layer.d` directory.

### Notes
Installation of the layer can be avoided. To use the layer without installation set VK_LAYER_PATH environment variable to the directory containing layer files.

## Tests
A small set of unit tests is provided at VkLayer_profiler_layer/profiler_tests. To build and run the tests additional dependencies must be installed.

On Windows, install [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/).

On Linux (Debian-based systems), install libvulkan-dev and glslang-tools packages.

## Usage
To enable the layer add `"VK_LAYER_PROFILER"` to the layer list in VkInstanceCreateInfo or to the VK_INSTANCE_LAYERS environment variable. The later method allows to profile applications without the need to modify the source code and is recommended for manual analysis.

### Configuration
The layer can be configured to handle more specific use cases. The following table describes the options available to the user.

| Option | Default | Description |
| ------ | ------- | ----------- |
| enable_overlay | 1 | Displays an interactive overlay with the collected data on the application's window. The profiler will set color attachment bit in the swapchain's image usage flags. |
| enable_performance_query_ext | 1 | Available on Intel graphics cards. Enables VK_INTEL_performance_query device extension and collects more detailed metrics. |
| enable_render_pass_begin_end_profiling | 0 | Measures time of vkCmdBeginRenderPass and vkCmdEndRenderPass in per render pass sampling mode. |
| sampling_mode | 0 | Controls the frequency of inserting timestamp queries. More frequent queries may impact performance of the applicaiton (but not the peformance of the measured region). See table with available sampling modes for more details. |
| sync_mode | 0 | Controls the frequency of collecting data from the submitted command buffers. More frequect synchronization points may impact performance of the application. See table with available synchronization modes for more details. |

The profiler loads the configuration from 3 sources, in the following order (which implies the priority of each source):
- VK_LAYER_profiler_config.ini - Located in application's directory.  
  ```
  enable_overlay 1
  enable_performance_query_ext 0
  sampling_mode 2
  ```
- VkProfilerCreateInfoEXT - Passed as a pNext to VkDeviceCreateInfo.
- Environment variables - The variables use the same naming as the configuration file, but are prefixed with "VKPROF_", e.g.: VKPROF_enable_overlay.

#### Sampling modes
Supported sampling modes are described in the following table:
| Mode | VkProfilerModeEXT | Description |
| ---- | ----------------- | ----------- |
| 0    | VK_PROFILER_MODE_PER_DRAWCALL_EXT | The profiler will measure GPU time of each command. |
| 1    | VK_PROFILER_MODE_PER_PIPELINE_EXT | The profiler will measure time of each continuous pipeline usage. Note: the profiler defines a pipeline as a unique tuple of shaders and does not include fixed-function pipeline state settings, like depth test or blending. |
| 2    | VK_PROFILER_MODE_PER_RENDER_PASS_EXT | The profiler will measure time of each VkRenderPass. Consecutive compute dispatches will be measured as a 'compute pass', and consecutive copy operations (copy, clear, blit, resolve, fill and update) will be measured as a 'copy pass'. |
| 3    | VK_PROFILER_MODE_PER_COMMAND_BUFFER_EXT | The profiler will measure time of each submitted VkCommandBuffer. |

There are also 2 sampling modes which are defined, but not currently supported:
| Mode | VkProfilerModeEXT | Description |
| ---- | ----------------- | ----------- |
| 4    | VK_PROFILER_MODE_PER_SUBMIT_BIT | The profiler will measure GPU time of each vkQueueSubmit. |
| 5    | VK_PROFILER_MODE_PER_FRAME_BIT | The profiler will measure GPU time of each frame, separated by vkQueuePresentKHR. |

#### Sync mode
Supported synchronization modes are described in the table below:
| Mode | VkProfilerSyncModeEXT | Description |
| ---- | --------------------- | ----------- |
| 0    | VK_PROFILER_SYNC_MODE_PRESENT_EXT | Collects the data on vkQueuePresentKHR. Inserts a vkDeviceWaitIdle before the call is forwarded to the ICD. |
| 1    | VK_PROFILER_SYNC_MODE_SUBMIT_EXT | Collects the data on vkQueueSubmit. Inserts a fence after the submitted commands and waits until it is signaled. |

Synchronization mode can be changed in the runtime using either the `vkSetProfilerSyncModeEXT` function or by selecting it in the "Settings" tab of the overlay.

## License
The profiler is released under the MIT license, see [LICENSE.md](LICENSE.md) for full text.
