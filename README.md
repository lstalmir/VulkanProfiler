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

To install the layer copy `VkLayer_profiler_layer.so` and `VkLayer_profiler_layer.json` files to `~/.local/share/vulkan/explicit_layers.d` directory.

### Notes
Installation of the layer can be avoided. To use the layer without installation set VK_LAYER_PATH environment variable to the directory containing layer files.

## Usage
To enable the layer add `"VK_LAYER_PROFILER"` to the layer list in VkInstanceCreateInfo or to the VK_INSTANCE_LAYERS environment variable. The later method allows to profile applications without the need to modify the source code and is recommended for manual analysis.

## License
The profiler is released under the MIT license, see [LICENSE.md](LICENSE.md) for full text.
