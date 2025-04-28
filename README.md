# Satellite Orbit Simulator

A cross-platform (Windows, Linux, macOS) Vulkan-based simulator showing a satellite orbiting Earth. The application uses Kepler's equations for orbit calculations and provides ImGUI-based controls for camera movement and time manipulation.

## Features

- 3D visualization of Earth as a sphere
- Satellite represented as a dot moving along an elliptical orbit
- Orbital mechanics based on Kepler's equations
- Camera controls for viewing the simulation from different angles
- Time controls to speed up, slow down, or pause the simulation

## Requirements

- C++20 compatible compiler
- CMake 3.16 or higher
- Vulkan SDK 1.2 or higher

The project automatically fetches and builds the following dependencies:
- GLFW3 (window creation and input)
- GLM (mathematics library)
- ImGui (user interface)

## Building

### Prerequisites

1. Install a C++20 compatible compiler:
   - Windows: Visual Studio 2019 or newer
   - macOS: Xcode 12 or newer with command line tools
   - Linux: GCC 10+ or Clang 10+

2. Install [CMake](https://cmake.org/download/) 3.16 or newer

3. Install the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)

### Clone the repository

```bash
git clone https://github.com/MatejGomboc-Claude-MCP/satellite-orbit-sim.git
cd satellite-orbit-sim
```

### Build with CMake

#### Windows
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

#### Linux/macOS
```bash
mkdir build
cd build
cmake ..
cmake --build . -j$(nproc)
```

### Running

After building, you can find the executable in the `build` directory (or `build/Release` on Windows):

```bash
./SatelliteOrbitSim
```

## Controls

- Left mouse button + drag: Rotate camera
- Right mouse button + drag: Pan camera
- Mouse wheel: Zoom in/out
- GUI controls for:
  - Adjusting time speed (simulation speed)
  - Modifying orbital parameters (semi-major axis, eccentricity, inclination)
  - Camera positioning

## Implementation Details

- Earth is represented as a simple sphere with basic lighting
- The satellite is a bright point following an elliptical orbit
- Orbital mechanics are calculated using Kepler's equations
- Cross-platform compatibility through CMake, GLFW, and Vulkan
- Time-based simulation with adjustable time scale
- ImGUI integration for user-friendly controls

## License

This project is licensed under the MIT License - see the LICENSE file for details.
