# Satellite Orbit Simulator

A cross-platform (Windows, Linux, macOS) Vulkan-based simulator showing a satellite orbiting Earth. The application uses Kepler's equations for accurate orbital mechanics and provides ImGUI-based controls for camera movement and time manipulation.

![Satellite Orbit Simulator](https://raw.githubusercontent.com/MatejGomboc-Claude-MCP/satellite-orbit-sim/main/docs/screenshot.png)

## Features

- **3D Earth Visualization**: Rendered as a simple sphere with basic lighting and atmospheric effects
- **Satellite Simulation**: Represented as a bright point moving along an elliptical orbit
- **Orbital Mechanics**: Based on Kepler's equations for accurate elliptical orbits
- **Interactive Controls**:
  - Camera controls for viewing the simulation from different angles
  - Time controls to speed up, slow down, or pause the simulation
  - Adjustable orbital parameters (semi-major axis, eccentricity, inclination)
- **Cross-Platform Support**: Works on Windows, Linux, and macOS
- **Modern Shader Support**: Uses HLSL shaders compiled to SPIR-V via the DirectX Shader Compiler (DXC)

## Technical Details

- **Graphics API**: Vulkan 1.2+
- **Window Management**: GLFW3
- **Mathematics**: GLM
- **User Interface**: Dear ImGui
- **Build System**: CMake 3.16+
- **Language**: C++20
- **Shader Language**: HLSL (Direct3D style)

## Building

See [BUILDING.md](BUILDING.md) for detailed build instructions for all supported platforms.

### Quick Start

For a quick start on most systems:

```bash
# Clone the repository
git clone https://github.com/MatejGomboc-Claude-MCP/satellite-orbit-sim.git
cd satellite-orbit-sim

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
cmake --build . --config Release

# Run the application (Windows)
.\bin\Release\SatelliteOrbitSim.exe

# Run the application (Linux/macOS)
./bin/SatelliteOrbitSim
```

## Controls

### Mouse Controls
- **Left-click + drag**: Rotate the camera around Earth
- **Right-click + drag**: Pan the camera
- **Scroll wheel**: Zoom in/out

### GUI Controls
- **Time Controls**:
  - Time multiplier slider: Adjust simulation speed (0.1x to 100x)
  - Reset Time: Return to normal (1x) speed
- **Camera Controls**:
  - Distance slider: Adjust camera distance from Earth
  - Yaw slider: Rotate camera horizontally
  - Pitch slider: Rotate camera vertically
  - Reset Camera: Return to default camera position
- **Orbit Parameters**:
  - Semi-major Axis: Adjust orbit size
  - Eccentricity: Adjust orbit shape (0 = circular, approaching 1 = more elliptical)
  - Inclination: Adjust orbit's tilt relative to equator

## Orbital Mechanics

The simulation uses Kepler's laws of planetary motion:

1. **Orbits are elliptical** with the central body at one focus
2. **Equal areas in equal times** - a satellite moves faster when closer to Earth
3. **Orbital period is related to the semi-major axis** - larger orbits have longer periods

The mathematical model solves Kepler's equation to convert from time to position:
- Mean anomaly (time-based angle) is converted to eccentric anomaly
- Position is calculated in the orbital plane
- Position is transformed based on inclination to get the 3D coordinates

## Shader System

This project uses Direct3D-style HLSL (High-Level Shading Language) shaders instead of traditional GLSL shaders for Vulkan. The HLSL shaders are compiled to SPIR-V bytecode using the DirectX Shader Compiler (DXC), which is part of the Vulkan SDK and provides compatibility with Vulkan while keeping the shader code in the familiar Direct3D style.

Key shader components:
- `earth.hlsl` - Combined vertex/pixel shader for Earth visualization
- `satellite.hlsl` - Combined vertex/pixel shader for satellite rendering

The build system will automatically detect and use DXC if available, with a fallback to glslc for GLSL shaders if needed.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Acknowledgements

- [Vulkan SDK](https://vulkan.lunarg.com/) for the graphics API
- [GLFW](https://www.glfw.org/) for cross-platform window management
- [GLM](https://github.com/g-truc/glm) for mathematics
- [Dear ImGui](https://github.com/ocornut/imgui) for the user interface
- [DirectX Shader Compiler](https://github.com/microsoft/DirectXShaderCompiler) for HLSL to SPIR-V compilation
