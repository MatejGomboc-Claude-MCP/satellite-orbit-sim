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
   - Ensure DirectX Shader Compiler (DXC) is included in your Vulkan SDK installation for HLSL support

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

## Building HLSL Shaders

This project uses DirectX-style HLSL shaders that are compiled to SPIR-V bytecode for Vulkan. The build system is configured to use the DirectX Shader Compiler (DXC) from the Vulkan SDK.

The shaders will be compiled automatically during the build process if you have the Vulkan SDK with DXC installed correctly. If DXC isn't found, the build system will fall back to using glslc for GLSL shaders.

If you want to manually compile the HLSL shaders:

```
# Compile Earth vertex shader
dxc -spirv -T vs_6_0 -E VSMain shaders/earth.hlsl -Fo shaders/earth_vert.spv

# Compile Earth pixel shader
dxc -spirv -T ps_6_0 -E PSMain shaders/earth.hlsl -Fo shaders/earth_frag.spv

# Compile Satellite vertex shader
dxc -spirv -T vs_6_0 -E VSMain shaders/satellite.hlsl -Fo shaders/satellite_vert.spv

# Compile Satellite pixel shader
dxc -spirv -T ps_6_0 -E PSMain shaders/satellite.hlsl -Fo shaders/satellite_frag.spv
```

Note the following differences compared to GLSL compilation:
- We use `-T vs_6_0` or `-T ps_6_0` to specify shader type and profile
- We use `-E VSMain` or `-E PSMain` to specify the entry point function name
- Both vertex and pixel shaders can be in a single HLSL file with different entry points

## Troubleshooting

### Vulkan SDK or DirectX Shader Compiler not found

If CMake cannot find the Vulkan SDK or DXC, make sure:
1. The Vulkan SDK is properly installed
2. The `VULKAN_SDK` environment variable is set
3. DirectX Shader Compiler (dxc) is included in your Vulkan SDK
4. On Windows, you may need to restart your terminal or IDE after installing the SDK

You can manually set the DXC path using the environment variable `DXC_PATH` if it's installed separately.

### Missing GLFW, GLM, or ImGui

These dependencies are automatically downloaded and built by the CMake script using FetchContent. If there are issues:
1. Check your internet connection
2. Make sure you are not behind a proxy that blocks Git downloads
3. Try manually downloading these libraries and placing them in the `external` directory

### Shader Compilation Errors

If you encounter shader compilation errors:
1. Make sure the Vulkan SDK's DirectX Shader Compiler (`dxc`) is in your PATH
2. Look for syntax errors in the shader files
3. If DXC isn't available, the build will try to use `glslc` with the legacy GLSL shaders
4. Check that your HLSL shader code conforms to the features available in the shader model you're targeting (typically vs_6_0/ps_6_0)

### Runtime Errors

If the application fails to run:
1. Check that your GPU supports Vulkan
2. Update your graphics drivers
3. Make sure you're running from the correct directory so the application can find the shader files
4. Verify that the shader files were correctly compiled to SPIR-V

For any other issues, please open an issue on the GitHub repository.
