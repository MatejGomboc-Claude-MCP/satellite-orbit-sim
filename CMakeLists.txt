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

if(NOT DXC_EXECUTABLE)
    message(WARNING "DirectX Shader Compiler (dxc) not found! Using glslc as fallback.")
    # Try to find glslc as fallback
    find_program(GLSLC_EXECUTABLE glslc HINTS
        ${Vulkan_GLSLC_EXECUTABLE}
        $ENV{VULKAN_SDK}/bin
        ${VULKAN_SDK}/bin
    )
    
    if(NOT GLSLC_EXECUTABLE)
        message(FATAL_ERROR "Neither dxc nor glslc shader compiler found! Please install the Vulkan SDK with shader compilers.")
    endif()
endif()

# Compile HLSL shaders with dxc
if(DXC_EXECUTABLE)
    message(STATUS "Using DirectX Shader Compiler (dxc) for HLSL shaders: ${DXC_EXECUTABLE}")
    
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
else()
    # Fallback to glslc for GLSL shaders (just in case)
    message(STATUS "Falling back to glslc for GLSL shaders: ${GLSLC_EXECUTABLE}")
    
    # Compile GLSL shaders if we still have them
    file(GLOB SHADER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag")
    foreach(SHADER ${SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SHADER_OUTPUT "${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv")
        
        add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Compiling shader ${SHADER_NAME} (GLSL fallback)"
        )
        list(APPEND SHADER_BINARIES ${SHADER_OUTPUT})
    endforeach()
endif()

# Create a custom target for all shaders
add_custom_target(shaders ALL 
    DEPENDS 
        ${CMAKE_BINARY_DIR}/shaders/earth_vert.spv
        ${CMAKE_BINARY_DIR}/shaders/earth_frag.spv
        ${CMAKE_BINARY_DIR}/shaders/satellite_vert.spv
        ${CMAKE_BINARY_DIR}/shaders/satellite_frag.spv
)
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

# Create directories structure
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/ui)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/orbit)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
