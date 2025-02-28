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
- libxkbcommon-dev
- libx11-dev, libxext-dev (for Xlib support)
- libxcb1-dev, libxcb-shape0-dev (for XCB support)

To build the layer create "cmake_build" folder in the project root directory and run following command from it:
```
cmake .. && make all
```

To install the layer copy `libVkLayer_profiler_layer.so` and `VkLayer_profiler_layer.json` files to `~/.local/share/vulkan/explicit_layer.d` directory.

### Notes
Installation of the layer can be avoided. To use the layer without installation set VK_LAYER_PATH environment variable to the directory containing layer files.

## Tests
A small set of unit tests is provided at VkLayer_profiler_layer/profiler_tests. To build and run the tests additional dependencies must be installed.

On Windows, install [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/).

On Linux (Debian-based systems), install libvulkan-dev and glslang-tools packages.

## Usage
To enable the layer add `"VK_LAYER_PROFILER_unified"` to the layer list in VkInstanceCreateInfo. It can be also enabled without any modifications to the profiled application using [Vulkan Configurator](https://vulkan.lunarg.com/doc/view/latest/windows/vkconfig.html) (distributed with Vulkan SDK) or via environment variables: `VK_INSTANCE_LAYERS` or `VK_LOADER_LAYERS_ENABLE` (available in 1.3.243 loader or newer).

### Configuration
The layer can be configured to handle more specific use cases. The following table describes the options available to the user.

| Option | Default | Description |
| ------ | ------- | ----------- |
| *BOOL*<br>enable_overlay | true | Displays an interactive overlay with the collected data on the application's window. The profiler will set color attachment bit in the swapchain's image usage flags. |
| *BOOL*<br>enable_performance_query_ext | true | Available on Intel graphics cards. Enables VK_INTEL_performance_query device extension and collects more detailed metrics. |
| *BOOL*<br>enable_pipeline_executable_properties_ext | false | Capture VkPipeline's executable properties and internal shader representations for more detailed insights into the shaders executed on the GPU. Enables VK_KHR_pipeline_executable_properties extension. |
| *BOOL*<br>enable_render_pass_begin_end_profiling | false | Measures time of vkCmdBeginRenderPass and vkCmdEndRenderPass in per render pass sampling mode. |
| *BOOL*<br>set_stable_power_state | true | Use DirectX12 API to set stable power state for GPU time measurements. |
| *BOOL*<br>enable_threading | true | Create threads to collect and process the data in background. If disabled, the data collection frequency is defined by `sync_mode`. |
| *ENUM*<br>sampling_mode | drawcall | Controls the frequency of inserting timestamp queries. More frequent queries may impact performance of the applicaiton (but not the peformance of the measured region). See table with available sampling modes for more details. |
| *ENUM*<br>sync_mode | present | Controls the frequency of collecting data from the submitted command buffers. More frequect synchronization points may impact performance of the application. See table with available synchronization modes for more details. |

The profiler loads the configuration from the sources below, in the following order (which implies the priority of each source):
1. VkLayerSettingsCreateInfoEXT - Provided as a pNext to VkInstanceCreateInfo. See [Integration](#integration).

2. vk_layer_settings.txt - Located in the application's directory.
   ```
   profiler_unified.enable_overlay = true
   profiler_unified.enable_performance_query_ext = false
   profiler_unified.sampling_mode = pipeline
   ```

3. **\[Deprecated\]** VkLayer_profiler_config.ini - Located in the application's directory.
   ```
   enable_overlay 1
   enable_performance_query_ext 0
   sampling_mode 2
   ```

4. VkProfilerCreateInfoEXT - Passed as a pNext to VkDeviceCreateInfo. See [Integration](#integration).

5. Environment variables - The variables use the same naming as the configuration file, but are prefixed with "VKPROF_", e.g.: VKPROF_enable_overlay.
   ```cmd
   SET VKPROF_enable_overlay=true
   SET VKPROF_enable_performance_query_ext=false
   SET VKPROF_sampling_mode=pipeline
   ```

The easiest way to configure the layer is to use the [Vulkan Configurator](https://vulkan.lunarg.com/doc/view/latest/windows/vkconfig.html).

#### Sampling modes
Supported sampling modes are described in the following table:

| # | Setting | Description |
| ---- | ------- | ----------- |
| 0    | drawcall | `VK_PROFILER_MODE_PER_DRAWCALL_EXT`<br>The profiler will measure GPU time of each command. |
| 1    | pipeline | `VK_PROFILER_MODE_PER_PIPELINE_EXT`<br>The profiler will measure time of each continuous pipeline usage. Note: the profiler defines a pipeline as a unique tuple of shaders and does not include fixed-function pipeline state settings, like depth test or blending. |
| 2    | renderpass | `VK_PROFILER_MODE_PER_RENDER_PASS_EXT`<br>The profiler will measure time of each VkRenderPass. Consecutive compute dispatches will be measured as a 'compute pass', and consecutive copy operations (copy, clear, blit, resolve, fill and update) will be measured as a 'copy pass'. |
| 3    | commandbuffer | `VK_PROFILER_MODE_PER_COMMAND_BUFFER_EXT`<br>The profiler will measure time of each submitted VkCommandBuffer. |

There are also 2 sampling modes which are defined, but not currently supported:

| # | Setting | Description |
| ---- | ------- | ----------- |
| 4    | submit | `VK_PROFILER_MODE_PER_SUBMIT_BIT`<br>The profiler will measure GPU time of each vkQueueSubmit. |
| 5    | present | `VK_PROFILER_MODE_PER_FRAME_BIT`<br>The profiler will measure GPU time of each frame, separated by vkQueuePresentKHR. |

#### Sync mode
Supported synchronization modes are described in the table below:

| # | Setting | Description |
| ---- | ------- | ----------- |
| 0    | present | `VK_PROFILER_SYNC_MODE_PRESENT_EXT`<br>Checks for complete frames before each vkQueuePresentKHR. |
| 1    | submit | `VK_PROFILER_SYNC_MODE_SUBMIT_EXT`<br>Checks for complete frames before each vkQueueSubmit. |

Synchronization mode can be changed in the runtime using either the `vkSetProfilerSyncModeEXT` function or by selecting it in the "Settings" tab of the overlay.

### Integration
The layer can be integrated into the application and provide performance data e.g.: for runtime optimizations or as a base tool for more advanced profiling.
The package contains a header with an unregistered extension that can be used to control the profiler and collect the data programatically.

The layer can be configured using both VkLayerSettingsCreateInfoEXT (since Vulkan 1.3.275) and VkProfilerCreateInfoEXT.

```c++
const VkBool32 bFalse = VK_FALSE;
const char* pSamplingMode = "pipeline";

// Configure the layer using VkLayerSettingsCreateInfoEXT.
const VkLayerSettingEXT settings[] = {
    { "VK_LAYER_PROFILER_unified", "enable_overlay", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &bFalse },
    { "VK_LAYER_PROFILER_unified", "sampling_mode" VK_LAYER_SETTING_TYPE_STRING_EXT, 1, &pSamplingMode } };

const VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo = {
    VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
    nullptr,
    static_cast<uint32_t>(std::size(settings)), settings };

instanceCreateInfo.pNext = &layerSettingsCreateInfo;
```

```c++
// Configure the profiler using VkProfilerCreateInfoEXT.
VkProfilerCreateInfoEXT profilerCreateInfo = {};
profilerCreateInfo.sType = VK_STRUCTURE_TYPE_PROFILER_CREATE_INFO_EXT;
profilerCreateInfo.flags = VK_PROFILER_CREATE_NO_OVERLAY_BIT_EXT;
profilerCreateInfo.samplingMode = VK_PROFILER_MODE_PER_PIPELINE_EXT;

deviceCreateInfo.pNext = &profilerCreateInfo;
```

Both configuration methods can be used simultaneously. VkProfilerCreateInfoEXT will override global configuration provided in VkLayerSettingsCreateInfoEXT.

Once the profiler is enabled for the device, the results can be read using `vkGetProfilerFrameDataEXT`. The function will allocate a tree, starting with frame region at root, and ending with the drawcalls in the leaves. Performance data may not be available for some of the tree nodes, depending on the sampling mode. To free the allocated tree, call `vkFreeProfilerFrameDataEXT`.

## License
The profiler is released under the MIT license, see [LICENSE.md](LICENSE.md) for full text.
