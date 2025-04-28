# Building the Satellite Orbit Simulator

This document provides detailed instructions for building the Satellite Orbit Simulator on different platforms.

## Prerequisites

Before building the project, you need to install:

1. **C++20 compatible compiler**:
   - Windows: Visual Studio 2019 or newer
   - macOS: Xcode 12+ with command line tools
   - Linux: GCC 10+ or Clang 10+

2. **CMake 3.16 or newer**: [Download from cmake.org](https://cmake.org/download/)

3. **Vulkan SDK 1.2 or newer**: [Download from LunarG](https://vulkan.lunarg.com/sdk/home)
   - Make sure the Vulkan SDK is properly installed and environment variables are set up
   - The `VULKAN_SDK` environment variable should point to your Vulkan SDK installation

## Building on Windows

1. Clone the repository:
   ```
   git clone https://github.com/MatejGomboc-Claude-MCP/satellite-orbit-sim.git
   cd satellite-orbit-sim
   ```

2. Create a build directory:
   ```
   mkdir build
   cd build
   ```

3. Configure with CMake:
   ```
   cmake ..
   ```

4. Build the project:
   ```
   cmake --build . --config Release
   ```

5. Run the application:
   ```
   .\bin\Release\SatelliteOrbitSim.exe
   ```

## Building on Linux

1. Clone the repository:
   ```
   git clone https://github.com/MatejGomboc-Claude-MCP/satellite-orbit-sim.git
   cd satellite-orbit-sim
   ```

2. Install dependencies (Ubuntu/Debian example):
   ```
   sudo apt update
   sudo apt install build-essential libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxext-dev
   ```

3. Create a build directory:
   ```
   mkdir build
   cd build
   ```

4. Configure with CMake:
   ```
   cmake ..
   ```

5. Build the project:
   ```
   cmake --build . -j$(nproc)
   ```

6. Run the application:
   ```
   ./bin/SatelliteOrbitSim
   ```

## Building on macOS

1. Clone the repository:
   ```
   git clone https://github.com/MatejGomboc-Claude-MCP/satellite-orbit-sim.git
   cd satellite-orbit-sim
   ```

2. Install dependencies via Homebrew (recommended):
   ```
   brew install cmake
   ```

3. Create a build directory:
   ```
   mkdir build
   cd build
   ```

4. Configure with CMake:
   ```
   cmake ..
   ```

5. Build the project:
   ```
   cmake --build . -j$(sysctl -n hw.ncpu)
   ```

6. Run the application:
   ```
   ./bin/SatelliteOrbitSim
   ```

## Building Shaders

The shaders will be compiled automatically during the build process if you have the Vulkan SDK installed correctly. The `glslc` compiler from the Vulkan SDK is used to compile the GLSL shaders to SPIR-V bytecode.

If you need to manually compile the shaders:

```
glslc shaders/earth.vert -o shaders/earth.vert.spv
glslc shaders/earth.frag -o shaders/earth.frag.spv
glslc shaders/satellite.vert -o shaders/satellite.vert.spv
glslc shaders/satellite.frag -o shaders/satellite.frag.spv
```

## Troubleshooting

### Vulkan SDK not found

If CMake cannot find the Vulkan SDK, make sure:
1. The Vulkan SDK is properly installed
2. The `VULKAN_SDK` environment variable is set
3. On Windows, you may need to restart your terminal or IDE after installing the SDK

### Missing GLFW, GLM, or ImGui

These dependencies are automatically downloaded and built by the CMake script using FetchContent. If there are issues:
1. Check your internet connection
2. Make sure you are not behind a proxy that blocks Git downloads
3. Try manually downloading these libraries and placing them in the `external` directory

### Shader Compilation Errors

If you encounter shader compilation errors:
1. Make sure the Vulkan SDK's `glslc` compiler is in your PATH
2. Look for syntax errors in the shader files
3. Check that the shader version (#version directive) is supported by your GPU

### Runtime Errors

If the application fails to run:
1. Check that your GPU supports Vulkan
2. Update your graphics drivers
3. Make sure you're running from the correct directory so the application can find the shader files

For any other issues, please open an issue on the GitHub repository.
