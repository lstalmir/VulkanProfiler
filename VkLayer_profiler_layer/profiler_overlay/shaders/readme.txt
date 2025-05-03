# Compiling GLSL shaders

GLSL shaders in this directory are not automatically compiled to their corresponding C header files to avoid dependency on Vulkan SDK in CI environment. For this reason, shaders have to be recompiled manually using the command line below after any modifications.

```
glslangValidator -V -e main --vn "spv_<file>_<type>" -o "spv_<file>_<type>.h" "glsl_<file>.<type>"
```
