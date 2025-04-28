cmake_minimum_required(VERSION 3.16)
project(SatelliteOrbitSim VERSION 1.0)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output executables to predictable locations
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add source files
set(SOURCES
    src/main.cpp
    src/application.cpp
    
    src/vulkan/renderer.cpp
    src/vulkan/instance.cpp
    src/vulkan/swapchain.cpp
    
    src/ui/imgui_manager.cpp
    
    src/orbit/orbital_mechanics.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Find Vulkan package
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)

# External dependencies using FetchContent
include(FetchContent)

# GLFW - Window creation and input handling
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

# GLM - Mathematics library
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)
target_include_directories(${PROJECT_NAME} PRIVATE ${glm_SOURCE_DIR})

# ImGui - User interface
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.89.9
)
FetchContent_MakeAvailable(imgui)

# Add ImGui source files
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
)
target_sources(${PROJECT_NAME} PRIVATE ${IMGUI_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)

# Ensure shader directory exists
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

# Find DirectX Shader Compiler (dxc) - used for HLSL shaders
find_program(DXC_EXECUTABLE dxc HINTS
    $ENV{VULKAN_SDK}/bin
    ${VULKAN_SDK}/bin
    $ENV{DXC_PATH}
    $ENV{DXC_PATH}/bin
)

# Try to find glslc compiler - used for fallback GLSL shaders
find_program(GLSLC_EXECUTABLE glslc HINTS
    ${Vulkan_GLSLC_EXECUTABLE}
    $ENV{VULKAN_SDK}/bin
    ${VULKAN_SDK}/bin
)

# Set shader compiler to use
if(DXC_EXECUTABLE)
    message(STATUS "Using DirectX Shader Compiler (dxc) for HLSL shaders: ${DXC_EXECUTABLE}")
    set(USE_HLSL TRUE)
    
    # Earth shader (vertex + pixel)
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/shaders/earth_vert.spv
        COMMAND ${DXC_EXECUTABLE} -spirv -T vs_6_0 -E VSMain ${CMAKE_CURRENT_SOURCE_DIR}/shaders/earth.hlsl -Fo ${CMAKE_BINARY_DIR}/shaders/earth_vert.spv
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/earth.hlsl
        COMMENT "Compiling Earth vertex shader (HLSL)"
    )
    
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/shaders/earth_frag.spv
        COMMAND ${DXC_EXECUTABLE} -spirv -T ps_6_0 -E PSMain ${CMAKE_CURRENT_SOURCE_DIR}/shaders/earth.hlsl -Fo ${CMAKE_BINARY_DIR}/shaders/earth_frag.spv
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/earth.hlsl
        COMMENT "Compiling Earth fragment shader (HLSL)"
    )
    
    # Satellite shader (vertex + pixel)
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/shaders/satellite_vert.spv
        COMMAND ${DXC_EXECUTABLE} -spirv -T vs_6_0 -E VSMain ${CMAKE_CURRENT_SOURCE_DIR}/shaders/satellite.hlsl -Fo ${CMAKE_BINARY_DIR}/shaders/satellite_vert.spv
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/satellite.hlsl
        COMMENT "Compiling Satellite vertex shader (HLSL)"
    )
    
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/shaders/satellite_frag.spv
        COMMAND ${DXC_EXECUTABLE} -spirv -T ps_6_0 -E PSMain ${CMAKE_CURRENT_SOURCE_DIR}/shaders/satellite.hlsl -Fo ${CMAKE_BINARY_DIR}/shaders/satellite_frag.spv
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/satellite.hlsl
        COMMENT "Compiling Satellite fragment shader (HLSL)"
    )
    
    # Create list of shader outputs
    set(SHADER_OUTPUTS
        ${CMAKE_BINARY_DIR}/shaders/earth_vert.spv
        ${CMAKE_BINARY_DIR}/shaders/earth_frag.spv
        ${CMAKE_BINARY_DIR}/shaders/satellite_vert.spv
        ${CMAKE_BINARY_DIR}/shaders/satellite_frag.spv
    )
    
elseif(GLSLC_EXECUTABLE)
    message(STATUS "Using glslc for GLSL shaders: ${GLSLC_EXECUTABLE}")
    set(USE_HLSL FALSE)
    
    # Create fallback GLSL shaders
    message(STATUS "Creating fallback GLSL shaders")
    
    # Earth vertex shader (GLSL)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/shaders/earth.vert
"#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    // Earth color (blue with some green)
    fragColor = vec3(0.0, 0.3, 0.8);
    
    // Transform position to world space
    vec4 worldPosition = ubo.model * vec4(inPosition, 1.0);
    
    // Pass world position to fragment shader
    fragPosition = worldPosition.xyz;
    
    // Transform normal to world space
    fragNormal = mat3(ubo.model) * inNormal;
    
    // Project to screen coordinates
    gl_Position = ubo.proj * ubo.view * worldPosition;
}
")

    # Earth fragment shader (GLSL)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/shaders/earth.frag
"#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    // Use a simple lighting model
    
    // Define light direction (from the center outward)
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    
    // Normalize the fragment normal
    vec3 normal = normalize(fragNormal);
    
    // Calculate diffuse component
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Add ambient lighting
    float ambient = 0.2;
    
    // Calculate the final lighting
    float lighting = ambient + diff * 0.8;
    
    // Apply lighting to the color
    outColor = vec4(fragColor * lighting, 1.0);
    
    // Add a slight atmospheric glow at the edges
    float rim = 1.0 - max(dot(normalize(-fragPosition), normal), 0.0);
    rim = pow(rim, 4.0);
    
    // Mix in the rim light (subtle blue glow)
    outColor.rgb = mix(outColor.rgb, vec3(0.3, 0.7, 1.0), rim * 0.3);
}
")

    # Satellite vertex shader (GLSL)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/shaders/satellite.vert
"#version 450

layout(location = 0) in vec3 inPosition;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    // Transform the satellite position to clip space
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    
    // Set the point size (satellite will be rendered as a point)
    gl_PointSize = 10.0;
}
")

    # Satellite fragment shader (GLSL)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/shaders/satellite.frag
"#version 450

layout(location = 0) out vec4 outColor;

void main() {
    // Calculate distance from center of point
    vec2 center = gl_PointCoord - vec2(0.5);
    float dist = length(center);
    
    // Create a circular point with soft edges
    float alpha = 1.0 - smoothstep(0.4, 0.5, dist);
    
    // Set the satellite color (bright white/yellow)
    outColor = vec4(1.0, 0.9, 0.5, alpha);
    
    // Add a glow effect
    if (dist > 0.2) {
        // Outer glow (yellow/orange)
        vec3 glowColor = vec3(1.0, 0.6, 0.2);
        float glowIntensity = smoothstep(0.5, 0.2, dist);
        
        // Mix the glow color with the base color
        outColor.rgb = mix(glowColor, outColor.rgb, glowIntensity);
    }
}
")

    # Compile GLSL shaders with glslc
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/shaders/earth_vert.spv
        COMMAND ${GLSLC_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/shaders/earth.vert -o ${CMAKE_BINARY_DIR}/shaders/earth_vert.spv
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/earth.vert
        COMMENT "Compiling Earth vertex shader (GLSL)"
    )
    
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/shaders/earth_frag.spv
        COMMAND ${GLSLC_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/shaders/earth.frag -o ${CMAKE_BINARY_DIR}/shaders/earth_frag.spv
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/earth.frag
        COMMENT "Compiling Earth fragment shader (GLSL)"
    )
    
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/shaders/satellite_vert.spv
        COMMAND ${GLSLC_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/shaders/satellite.vert -o ${CMAKE_BINARY_DIR}/shaders/satellite_vert.spv
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/satellite.vert
        COMMENT "Compiling Satellite vertex shader (GLSL)"
    )
    
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/shaders/satellite_frag.spv
        COMMAND ${GLSLC_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/shaders/satellite.frag -o ${CMAKE_BINARY_DIR}/shaders/satellite_frag.spv
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/satellite.frag
        COMMENT "Compiling Satellite fragment shader (GLSL)"
    )
    
    # Create list of shader outputs
    set(SHADER_OUTPUTS
        ${CMAKE_BINARY_DIR}/shaders/earth_vert.spv
        ${CMAKE_BINARY_DIR}/shaders/earth_frag.spv
        ${CMAKE_BINARY_DIR}/shaders/satellite_vert.spv
        ${CMAKE_BINARY_DIR}/shaders/satellite_frag.spv
    )
else()
    message(FATAL_ERROR "Neither dxc nor glslc shader compiler found! Please install the Vulkan SDK with shader compilers.")
endif()

# Add a target for the shaders
add_custom_target(shaders ALL DEPENDS ${SHADER_OUTPUTS})
add_dependencies(${PROJECT_NAME} shaders)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
elseif(APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE VK_USE_PLATFORM_MACOS_MVK)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(METAL_LIBRARY Metal REQUIRED)
    find_library(QUARTZCORE_LIBRARY QuartzCore REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${COCOA_LIBRARY} ${METAL_LIBRARY} ${QUARTZCORE_LIBRARY})
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE VK_USE_PLATFORM_XCB_KHR)
    find_package(X11 REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE X11::X11)
endif()

# Pass shader type to C++ code
if(USE_HLSL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_HLSL)
endif()

# Create directories structure
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/ui)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/orbit)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
